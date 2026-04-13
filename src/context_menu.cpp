#include "context_menu.h"
#include "bookshelf.h"
#include "favorites.h"
#include "tree.h"
#include "filelist.h"
#include "archive.h"
#include "i18n.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include "media.h"
#include "viewer.h"
#include "window.h"
#include <algorithm>
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

// パスからダブルクォートを除去（ファイルシステムで使えない文字だが安全のため）
static std::wstring SanitizePath(const std::wstring& p)
{
    std::wstring s = p;
    s.erase(std::remove(s.begin(), s.end(), L'"'), s.end());
    return s;
}

void ShowInExplorer(const std::wstring& path)
{
    // 書庫内パスの場合は書庫ファイル自体を対象にする
    std::wstring realPath = path;
    std::wstring arcPath, entryPath;
    if (SplitArchivePath(path, arcPath, entryPath))
        realPath = arcPath;
    realPath = SanitizePath(realPath);

    DWORD attr = GetFileAttributesW(realPath.c_str());
    std::wstring cmdLine;
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
        cmdLine = L"explorer.exe \"" + realPath + L"\"";
    else
        cmdLine = L"explorer.exe /select,\"" + realPath + L"\"";

    STARTUPINFOW si = {}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    if (CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
        0, nullptr, nullptr, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
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
    // 書庫内ファイルではメニューを出さない
    {
        std::wstring arcP, entP;
        if (SplitArchivePath(path, arcP, entP)) return;
    }

    HMENU hMenu = CreatePopupMenu();

    // 関連付けで開く（ファイルのみ、フォルダは非表示）
    {
        DWORD attr = GetFileAttributesW(path.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
            AppendMenuW(hMenu, MF_STRING, CTX_OPEN_ASSOC, I18nGet(L"ctx.openassoc").c_str());
    }

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
    AppendMenuW(hShelfMenu, MF_STRING, CTX_SHELF_NEW, I18nGet(L"ctx.newshelf").c_str());
    // フォルダの場合:「フォルダを本棚として追加」
    DWORD pathAttr = GetFileAttributesW(path.c_str());
    if (pathAttr != INVALID_FILE_ATTRIBUTES && (pathAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        AppendMenuW(hShelfMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hShelfMenu, MF_STRING, CTX_SHELF_AS_CAT, I18nGet(L"ctx.shelfascat").c_str());
    }
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hShelfMenu, I18nGet(L"ctx.addshelf").c_str());

    // 本棚から解除（本棚に含まれている場合）
    if (BookshelfContains(path))
        AppendMenuW(hMenu, MF_STRING, CTX_REMOVE_SHELF, I18nGet(L"ctx.removeshelf").c_str());

    // 削除（通常モード・本棚モードのみ、特殊フォルダ・ドライブ・書庫内は非表示）
    {
        bool canDelete = true;
        int treeMode = GetTreeMode();
        if (treeMode == 2) canDelete = false; // 履歴モード

        std::wstring arcPath, entryPath;
        if (canDelete && SplitArchivePath(path, arcPath, entryPath)) canDelete = false;

        if (canDelete && PathIsRootW(path.c_str())) canDelete = false;

        if (canDelete)
        {
            static const KNOWNFOLDERID specialFolders[] = {
                FOLDERID_Desktop, FOLDERID_Downloads, FOLDERID_Documents,
                FOLDERID_Pictures, FOLDERID_Videos, FOLDERID_Music
            };
            for (auto& fid : specialFolders)
            {
                wchar_t* knownPath = nullptr;
                if (SUCCEEDED(SHGetKnownFolderPath(fid, 0, nullptr, &knownPath)))
                {
                    bool match = (_wcsicmp(path.c_str(), knownPath) == 0);
                    CoTaskMemFree(knownPath);
                    if (match) { canDelete = false; break; }
                }
            }
        }

        if (canDelete)
        {
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, CTX_RENAME, I18nGet(L"ctx.rename").c_str());
            AppendMenuW(hMenu, MF_STRING, CTX_DELETE, I18nGet(L"ctx.delete").c_str());
        }
    }

    // プロパティ（実ファイル/フォルダのみ）
    {
        std::wstring arcP, entP;
        std::wstring realP = SplitArchivePath(path, arcP, entP) ? arcP : path;
        if (GetFileAttributesW(realP.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, CTX_PROPERTIES, I18nGet(L"ctx.properties").c_str());
        }
    }

    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);

    std::wstring fileName = path;
    auto slashPos = fileName.find_last_of(L'\\');
    if (slashPos != std::wstring::npos) fileName = fileName.substr(slashPos + 1);

    if (cmd == CTX_PROPERTIES && !path.empty())
    {
        std::wstring arcP, entP;
        std::wstring propPath = SplitArchivePath(path, arcP, entP) ? arcP : path;
        // wscript経由でプロパティダイアログ表示
        // VBSの InvokeVerb("Properties") が最も確実
        wchar_t tmpPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tmpPath);
        std::wstring vbsPath = std::wstring(tmpPath) + L"karikari_prop.vbs";
        // 親フォルダとファイル名に分割
        std::wstring parent = propPath, name;
        auto pos = propPath.find_last_of(L'\\');
        if (pos != std::wstring::npos) { parent = propPath.substr(0, pos); name = propPath.substr(pos + 1); }
        else { parent = L"."; name = propPath; }
        // パスのダブルクォートをVBSエスケープ（"" に置換）
        for (auto* s : {&parent, &name}) {
            size_t pos = 0;
            while ((pos = s->find(L'"', pos)) != std::wstring::npos) {
                s->replace(pos, 1, L"\"\"");
                pos += 2;
            }
        }
        // VBSスクリプト生成
        std::wstring vbs = L"Set s=CreateObject(\"Shell.Application\")\r\n"
            L"Set f=s.NameSpace(\"" + parent + L"\")\r\n"
            L"If Not f Is Nothing Then\r\n"
            L"  Set i=f.ParseName(\"" + name + L"\")\r\n"
            L"  If Not i Is Nothing Then i.InvokeVerb \"properties\"\r\n"
            L"End If\r\n"
            L"WScript.Sleep 30000\r\n";
        HANDLE hFile = CreateFileW(vbsPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // BOM + UTF-16LE
            BYTE bom[] = {0xFF, 0xFE};
            DWORD written;
            WriteFile(hFile, bom, 2, &written, nullptr);
            WriteFile(hFile, vbs.c_str(), (DWORD)(vbs.size() * sizeof(wchar_t)), &written, nullptr);
            CloseHandle(hFile);
            std::wstring cmdLine = L"wscript.exe \"" + vbsPath + L"\"";
            STARTUPINFOW si = {}; si.cb = sizeof(si);
            PROCESS_INFORMATION pi = {};
            if (CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
    }
    else if (cmd == CTX_OPEN_ASSOC && !path.empty())
    {
        std::wstring arcP, entP;
        std::wstring openPath = SanitizePath(SplitArchivePath(path, arcP, entP) ? arcP : path);
        // CreateProcessW + cmd /c start で関連付けアプリを起動
        // パスからダブルクォートを除去済みなのでインジェクション防止
        std::wstring cmdLine = L"cmd /c start \"\" \"" + openPath + L"\"";
        STARTUPINFOW si = {}; si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};
        if (CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    else if (cmd == CTX_OPEN_EXPLORER && !path.empty()) ShowInExplorer(path);
    else if (cmd == CTX_COPY_PATH && !path.empty()) CopyToClipboard(hwnd, path);
    else if (cmd == CTX_ADD_FAV) { FavoritesAdd(path); RefreshFavoritesInTree(); }
    else if (cmd == CTX_REMOVE_FAV) { FavoritesRemove(path); RefreshFavoritesInTree(); }
    else if (cmd == CTX_REMOVE_SHELF) { BookshelfRemoveItem(path); if (GetTreeMode() == 1) ShowBookshelfTree(); }
    else if (cmd == CTX_SHELF_NEW)
    {
        // 新しい本棚を作成して追加
        wchar_t buf[256] = {};
        if (ShowInputDialog(hwnd, I18nGet(L"ctx.shelfname").c_str(), I18nGet(L"ctx.shelfprompt").c_str(), buf, 256, L""))
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
    else if (cmd == CTX_RENAME)
    {
        // どこから呼ばれたか判定: ファイルリスト or ツリービュー
        int sel = (int)SendMessageW(g_app.wnd.hwndList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
        bool fromList = (sel >= 0 && sel < (int)g_app.nav.fileItems.size()
            && _wcsicmp(g_app.nav.fileItems[sel].fullPath.c_str(), path.c_str()) == 0);

        if (fromList && !g_app.nav.inArchiveMode)
        {
            StartInlineRename(sel);
        }
        else
        {
            // ツリービューのインラインラベル編集（右クリックしたノードを直接編集）
            HTREEITEM hTarget = g_treeRClickItem;
            if (!hTarget)
                hTarget = (HTREEITEM)SendMessageW(g_app.wnd.hwndTree, TVM_GETNEXTITEM, TVGN_CARET, 0);
            if (hTarget)
                SendMessageW(g_app.wnd.hwndTree, TVM_EDITLABEL, 0, (LPARAM)hTarget);
        }
    }
    else if (cmd == CTX_DELETE)
    {
        std::wstring msg = L"\"" + fileName + L"\" " + I18nGet(L"dlg.delete");
        if (MessageBoxW(hwnd, msg.c_str(), I18nGet(L"dlg.confirm").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            // 削除対象のファイルロックを解放
            // 現在表示中の画像/アニメーション
            if (_wcsicmp(path.c_str(), g_app.nav.currentPath.c_str()) == 0)
            {
                ViewerStopAnimation();
                g_app.viewer.bitmap.Reset();
                g_app.viewer.bitmap2.Reset();
                MediaStop();
            }
            // 書庫キャッシュ（削除対象が書庫ファイルの場合）
            if (IsArchiveFile(path))
                CloseCurrentArchive();

            // ごみ箱へ移動
            std::wstring delPath = path;
            delPath.push_back(L'\0'); // SHFileOperation用のダブルNULL
            SHFILEOPSTRUCTW op = {};
            op.hwnd = hwnd;
            op.wFunc = FO_DELETE;
            op.pFrom = delPath.c_str();
            op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
            if (SHFileOperationW(&op) == 0)
            {
                RemoveFileItemByPath(path);
                RemoveTreeItemByPath(path);
            }
        }
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
