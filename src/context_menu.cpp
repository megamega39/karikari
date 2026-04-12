#include "context_menu.h"
#include "bookshelf.h"
#include "favorites.h"
#include "tree.h"
#include "filelist.h"
#include "archive.h"
#include "i18n.h"
#include <shlwapi.h>
#include <shellapi.h>
#include <wincodec.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// 前方宣言
static void ToggleFullscreen(HWND hwnd);

// 簡易テキスト入力ダイアログ
static INT_PTR CALLBACK InputDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM)
{
    if (msg == WM_COMMAND)
    {
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t* buf = (wchar_t*)GetWindowLongPtrW(hDlg, GWLP_USERDATA);
            if (buf) GetDlgItemTextW(hDlg, 101, buf, 256);
            DestroyWindow(hDlg);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            wchar_t* buf = (wchar_t*)GetWindowLongPtrW(hDlg, GWLP_USERDATA);
            if (buf) buf[0] = L'\0';
            DestroyWindow(hDlg);
            return TRUE;
        }
    }
    return FALSE;
}

bool ShowInputDialog(HWND hwndParent, const wchar_t* title, const wchar_t* prompt,
                     wchar_t* buf, int bufSize, const wchar_t* defaultText)
{
    wcscpy_s(buf, bufSize, defaultText);

    struct { DLGTEMPLATE dt; WORD menu, cls, title; wchar_t titleText[64]; } tmpl = {};
    tmpl.dt.style = DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    tmpl.dt.cx = 1; tmpl.dt.cy = 1;
    wcsncpy_s(tmpl.titleText, title, _TRUNCATE);

    HWND hDlg = CreateDialogIndirectW(g_app.hInstance, &tmpl.dt, hwndParent, InputDlgProc);
    if (!hDlg) return false;

    RECT rc = { 0, 0, 320, 120 };
    AdjustWindowRectEx(&rc, GetWindowLong(hDlg, GWL_STYLE), FALSE, GetWindowLong(hDlg, GWL_EXSTYLE));
    RECT parentRc; GetWindowRect(hwndParent, &parentRc);
    int px = parentRc.left + (parentRc.right - parentRc.left - (rc.right - rc.left)) / 2;
    int py = parentRc.top + (parentRc.bottom - parentRc.top - (rc.bottom - rc.top)) / 2;
    SetWindowPos(hDlg, nullptr, px, py, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)buf);

    static HFONT hFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HWND hLabel = CreateWindowExW(0, L"STATIC", prompt, WS_CHILD | WS_VISIBLE, 10, 10, 290, 20, hDlg, nullptr, g_app.hInstance, nullptr);
    SendMessageW(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", defaultText, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 10, 34, 290, 24, hDlg, (HMENU)101, g_app.hInstance, nullptr);
    SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(hEdit, EM_SETSEL, 0, -1);
    HWND hOk = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 150, 70, 70, 28, hDlg, (HMENU)IDOK, g_app.hInstance, nullptr);
    HWND hCancel = CreateWindowExW(0, L"BUTTON", L"キャンセル", WS_CHILD | WS_VISIBLE, 230, 70, 70, 28, hDlg, (HMENU)IDCANCEL, g_app.hInstance, nullptr);
    SendMessageW(hOk, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(hCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

    ShowWindow(hDlg, SW_SHOW);
    SetFocus(hEdit);
    EnableWindow(hwndParent, FALSE);

    MSG msg;
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0))
    {
        if (IsDialogMessageW(hDlg, &msg)) continue;
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);

    return buf[0] != L'\0';
}

void CopyToClipboard(HWND hwnd, const std::wstring& text)
{
    if (text.empty()) return;
    if (!OpenClipboard(hwnd)) return;
    EmptyClipboard();
    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem)
    {
        memcpy(GlobalLock(hMem), text.c_str(), bytes);
        GlobalUnlock(hMem);
        SetClipboardData(CF_UNICODETEXT, hMem);
    }
    CloseClipboard();
}

void ShowInExplorer(const std::wstring& path)
{
    std::wstring cmd = L"/select,\"" + path + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", cmd.c_str(), nullptr, SW_SHOW);
}

void CopyImageToClipboard(HWND hwnd)
{
    if (!g_app.viewer.bitmap || g_app.nav.currentPath.empty()) return;

    // WIC でデコード（書庫内画像にも対応）
    ComPtr<IWICFormatConverter> converter;
    std::wstring arcPath, entryPath;

    if (SplitArchivePath(g_app.nav.currentPath, arcPath, entryPath))
    {
        // 書庫内画像: メモリ展開 → WIC デコード
        std::vector<BYTE> buf;
        if (!ExtractToMemory(arcPath, entryPath, buf)) return;

        ComPtr<IWICStream> stream;
        if (FAILED(g_app.viewer.wicFactory->CreateStream(stream.GetAddressOf()))) return;
        if (FAILED(stream->InitializeFromMemory(buf.data(), (DWORD)buf.size()))) return;

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(g_app.viewer.wicFactory->CreateDecoderFromStream(
            stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf()))) return;

        ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(decoder->GetFrame(0, frame.GetAddressOf()))) return;

        if (FAILED(g_app.viewer.wicFactory->CreateFormatConverter(converter.GetAddressOf()))) return;
        converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    }
    else
    {
        // 通常ファイル
        ComPtr<IWICBitmapDecoder> decoder;
        HRESULT hr = g_app.viewer.wicFactory->CreateDecoderFromFilename(
            g_app.nav.currentPath.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
        if (FAILED(hr)) return;

        ComPtr<IWICBitmapFrameDecode> frame;
        decoder->GetFrame(0, frame.GetAddressOf());
        if (!frame) return;

        g_app.viewer.wicFactory->CreateFormatConverter(converter.GetAddressOf());
        converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    }

    if (!converter) return;

    UINT imgW, imgH;
    converter->GetSize(&imgW, &imgH);

    if (!OpenClipboard(hwnd)) return;
    EmptyClipboard();

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = imgW;
    bmi.bmiHeader.biHeight = -(int)imgH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;

    void* dibBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &dibBits, nullptr, 0);
    if (hBmp && dibBits)
    {
        converter->CopyPixels(nullptr, imgW * 4, imgW * imgH * 4, (BYTE*)dibBits);
        if (!SetClipboardData(CF_BITMAP, hBmp))
            DeleteObject(hBmp); // 失敗時は自分で解放
    }
    CloseClipboard();
}

// ファイル/フォルダ用コンテキストメニュー共通ヘルパー
void ShowFileContextMenu(HWND hwnd, const std::wstring& path, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, CTX_OPEN_EXPLORER, I18nGet(L"ctx.explorer").c_str());
    AppendMenuW(hMenu, MF_STRING, CTX_COPY_PATH, I18nGet(L"ctx.copypath").c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // お気に入り
    if (FavoritesContains(path))
        AppendMenuW(hMenu, MF_STRING, CTX_REMOVE_FAV, I18nGet(L"ctx.removefav").c_str());
    else
        AppendMenuW(hMenu, MF_STRING, CTX_ADD_FAV, I18nGet(L"ctx.addfav").c_str());

    // 本棚に追加（サブメニュー）
    HMENU hShelfMenu = CreatePopupMenu();
    auto& cats = BookshelfGetCategories();
    for (int i = 0; i < (int)cats.size() && i < 99; i++)
        AppendMenuW(hShelfMenu, MF_STRING, CTX_SHELF_BASE + i, cats[i].name.c_str());
    if (!cats.empty())
        AppendMenuW(hShelfMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hShelfMenu, MF_STRING, CTX_SHELF_NEW, L"新しい本棚を作成...");
    // フォルダの場合:「フォルダを本棚として追加」（フォルダ名で本棚作成→配下の書庫も追加）
    DWORD pathAttr = GetFileAttributesW(path.c_str());
    if (pathAttr != INVALID_FILE_ATTRIBUTES && (pathAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        AppendMenuW(hShelfMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hShelfMenu, MF_STRING, CTX_SHELF_AS_CAT, L"フォルダを本棚として追加");
    }
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hShelfMenu, L"本棚に追加");

    // 本棚から解除（本棚に含まれている場合）
    if (BookshelfContains(path))
        AppendMenuW(hMenu, MF_STRING, CTX_REMOVE_SHELF, L"本棚から解除");

    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);

    std::wstring fileName = path;
    auto slashPos = fileName.find_last_of(L'\\');
    if (slashPos != std::wstring::npos) fileName = fileName.substr(slashPos + 1);

    if (cmd == CTX_OPEN_EXPLORER && !path.empty()) ShowInExplorer(path);
    else if (cmd == CTX_COPY_PATH && !path.empty()) CopyToClipboard(hwnd, path);
    else if (cmd == CTX_ADD_FAV) { FavoritesAdd(path); RefreshFavoritesInTree(); }
    else if (cmd == CTX_REMOVE_FAV) { FavoritesRemove(path); RefreshFavoritesInTree(); }
    else if (cmd == CTX_REMOVE_SHELF) { BookshelfRemoveItem(path); if (GetTreeMode() == 1) ShowBookshelfTree(); }
    else if (cmd == CTX_SHELF_NEW)
    {
        // 新しい本棚を作成して追加
        wchar_t buf[256] = {};
        if (ShowInputDialog(hwnd, L"新しい本棚を作成", L"本棚名を入力してください:", buf, 256, L""))
        {
            auto& cat = BookshelfAddCategory(buf);
            BookshelfAddItem(cat.id, fileName, path);
            if (GetTreeMode() == 1) ShowBookshelfTree();
        }
    }
    else if (cmd == CTX_SHELF_AS_CAT)
    {
        // フォルダを本棚として追加（フォルダ名で本棚作成→配下の書庫も追加）
        auto& cat = BookshelfAddCategory(fileName);
        std::wstring searchPath = path;
        if (!searchPath.empty() && searchPath.back() != L'\\') searchPath += L'\\';
        searchPath += L'*';
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileExW(searchPath.c_str(), FindExInfoBasic, &fd,
                                         FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) continue;
                bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                bool isArc = !isDir && IsArchiveFile(fd.cFileName);
                bool isImg = !isDir && IsImageFile(fd.cFileName);
                if (isDir || isArc || isImg)
                {
                    std::wstring childPath = path;
                    if (childPath.back() != L'\\') childPath += L'\\';
                    childPath += fd.cFileName;
                    BookshelfAddItem(cat.id, fd.cFileName, childPath);
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
        if (GetTreeMode() == 1) ShowBookshelfTree();
    }
    else if (cmd >= CTX_SHELF_BASE && cmd < CTX_SHELF_BASE + (int)cats.size())
    {
        int idx = cmd - CTX_SHELF_BASE;
        BookshelfAddItem(cats[idx].id, fileName, path);
        if (GetTreeMode() == 1) ShowBookshelfTree();
    }
}

static void ToggleFullscreen(HWND hwnd)
{
    SendMessageW(hwnd, WM_TOGGLE_FULLSCREEN, 0, 0);
}

void ShowContextMenu(HWND hwnd, int x, int y, bool isViewer)
{
    HMENU hMenu = CreatePopupMenu();

    if (isViewer)
    {
        AppendMenuW(hMenu, MF_STRING, CTX_FULLSCREEN, I18nGet(L"ctx.fullscreen").c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, CTX_COPY_IMAGE, I18nGet(L"ctx.copyimage").c_str());
        AppendMenuW(hMenu, MF_STRING, CTX_COPY_PATH, I18nGet(L"ctx.copypath").c_str());
        AppendMenuW(hMenu, MF_STRING, CTX_COPY_PARENT_PATH, I18nGet(L"ctx.copyparentpath").c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, CTX_OPEN_EXPLORER, I18nGet(L"ctx.explorer").c_str());
    }
    else
    {
        AppendMenuW(hMenu, MF_STRING, CTX_OPEN_EXPLORER, I18nGet(L"ctx.explorer").c_str());
        AppendMenuW(hMenu, MF_STRING, CTX_COPY_PATH, I18nGet(L"ctx.copypath").c_str());
    }

    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, x, y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);

    switch (cmd)
    {
    case CTX_FULLSCREEN: ToggleFullscreen(hwnd); break;
    case CTX_COPY_IMAGE: CopyImageToClipboard(hwnd); break;
    case CTX_COPY_PATH:  CopyToClipboard(hwnd, g_app.nav.currentPath); break;
    case CTX_COPY_PARENT_PATH:
    {
        wchar_t parent[MAX_PATH];
        wcscpy_s(parent, g_app.nav.currentPath.c_str());
        PathRemoveFileSpecW(parent);
        CopyToClipboard(hwnd, parent);
        break;
    }
    case CTX_OPEN_EXPLORER:
        if (!g_app.nav.currentPath.empty())
            ShowInExplorer(g_app.nav.currentPath);
        break;
    }
}
