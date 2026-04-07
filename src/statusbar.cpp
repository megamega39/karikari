#include "statusbar.h"

HWND CreateStatusBar(HWND parent, HINSTANCE hInst)
{
    HWND hwnd = CreateWindowExW(
        0, STATUSCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        parent, (HMENU)(UINT_PTR)IDC_STATUSBAR, hInst, nullptr);

    if (!hwnd) return nullptr;

    // フォント設定
    static HFONT hStatusFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)hStatusFont, TRUE);

    // 4パート: パス | 解像度 | ファイルサイズ | メッセージ
    int parts[] = { -1, 0, 0, 0 }; // LayoutChildrenで再計算
    SendMessageW(hwnd, SB_SETPARTS, 4, (LPARAM)parts);

    g_app.wnd.hwndStatusBar = hwnd;
    return hwnd;
}

void UpdateStatusBar(const std::wstring& path, int imgW, int imgH, const std::wstring& msg,
                     ULONGLONG fileSize)
{
    if (!g_app.wnd.hwndStatusBar) return;

    // パート幅を再計算（幅変更時のみ SB_SETPARTS 送信）
    RECT rc;
    GetClientRect(g_app.wnd.hwndStatusBar, &rc);
    int totalW = rc.right;
    static int lastTotalW = -1;
    if (totalW != lastTotalW)
    {
        lastTotalW = totalW;
        int parts[] = { totalW - 400, totalW - 280, totalW - 180, -1 };
        SendMessageW(g_app.wnd.hwndStatusBar, SB_SETPARTS, 4, (LPARAM)parts);
    }

    SendMessageW(g_app.wnd.hwndStatusBar, SB_SETTEXTW, 0, (LPARAM)path.c_str());

    if (imgW > 0 && imgH > 0)
    {
        wchar_t dim[64];
        swprintf_s(dim, _countof(dim), L"%d \u00D7 %d", imgW, imgH);
        SendMessageW(g_app.wnd.hwndStatusBar, SB_SETTEXTW, 1, (LPARAM)dim);
    }
    else
    {
        SendMessageW(g_app.wnd.hwndStatusBar, SB_SETTEXTW, 1, (LPARAM)L"");
    }

    // ファイルサイズ
    if (fileSize > 0)
    {
        wchar_t sizeBuf[64];
        if (fileSize >= 1024ULL * 1024 * 1024)
            swprintf_s(sizeBuf, _countof(sizeBuf), L"%.1f GB", fileSize / (1024.0 * 1024 * 1024));
        else if (fileSize >= 1024ULL * 1024)
            swprintf_s(sizeBuf, _countof(sizeBuf), L"%.1f MB", fileSize / (1024.0 * 1024));
        else if (fileSize >= 1024ULL)
            swprintf_s(sizeBuf, _countof(sizeBuf), L"%.1f KB", fileSize / 1024.0);
        else
            swprintf_s(sizeBuf, _countof(sizeBuf), L"%llu B", fileSize);
        SendMessageW(g_app.wnd.hwndStatusBar, SB_SETTEXTW, 2, (LPARAM)sizeBuf);
    }
    else
    {
        SendMessageW(g_app.wnd.hwndStatusBar, SB_SETTEXTW, 2, (LPARAM)L"");
    }

    SendMessageW(g_app.wnd.hwndStatusBar, SB_SETTEXTW, 3, (LPARAM)msg.c_str());
}
