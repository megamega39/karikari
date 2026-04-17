#include "addressbar.h"
#include "nav.h"
#include "i18n.h"

static HFONT g_addrFont = nullptr;
static HFONT g_addrFontBold = nullptr;
static HWND g_breadcrumb = nullptr;
static WNDPROC g_origEditProc = nullptr;
static std::wstring g_currentAddrPath;
static int g_hoverSegment = -1; // マウスホバー中のセグメント番号

// パスをセグメントに分割
static std::vector<std::wstring> SplitPath(const std::wstring& path)
{
    std::vector<std::wstring> segs;
    size_t pos = 0;
    while (pos < path.size())
    {
        size_t next = path.find(L'\\', pos);
        if (next == std::wstring::npos) next = path.size();
        std::wstring seg = path.substr(pos, next - pos);
        if (!seg.empty()) segs.push_back(seg);
        pos = next + 1;
    }
    return segs;
}

// セグメントの描画位置を計算
struct SegmentRect {
    int x, w; // 左端と幅
};

static std::vector<SegmentRect> CalcSegmentRects(HDC hdc, const std::vector<std::wstring>& segs)
{
    std::vector<SegmentRect> rects;
    int x = 4; // 左パディング
    for (size_t i = 0; i < segs.size(); i++)
    {
        if (i > 0)
        {
            SIZE sepSize;
            GetTextExtentPoint32W(hdc, L" \u203A ", 3, &sepSize);
            x += sepSize.cx;
        }
        SIZE segSize;
        GetTextExtentPoint32W(hdc, segs[i].c_str(), (int)segs[i].size(), &segSize);
        rects.push_back({ x, segSize.cx });
        x += segSize.cx;
    }
    return rects;
}

// クリック/ホバー位置からセグメントを特定
static int HitTestSegment(HWND hwnd, int clickX)
{
    auto segs = SplitPath(g_currentAddrPath);
    if (segs.empty()) return -1;

    HDC hdc = GetDC(hwnd);
    HFONT hOld = (HFONT)SelectObject(hdc, g_addrFont);
    auto rects = CalcSegmentRects(hdc, segs);
    SelectObject(hdc, hOld);
    ReleaseDC(hwnd, hdc);

    for (size_t i = 0; i < rects.size(); i++)
    {
        if (clickX >= rects[i].x && clickX < rects[i].x + rects[i].w)
            return (int)i;
    }
    return -1;
}

// ブレッドクラム描画
static void PaintBreadcrumb(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));

    auto segs = SplitPath(g_currentAddrPath);
    if (segs.empty()) { EndPaint(hwnd, &ps); return; }

    SetBkMode(hdc, TRANSPARENT);
    HFONT hOld = (HFONT)SelectObject(hdc, g_addrFont);
    auto rects = CalcSegmentRects(hdc, segs);

    int y = rc.top;
    int h = rc.bottom - rc.top;

    for (size_t i = 0; i < segs.size(); i++)
    {
        // セパレータ描画
        if (i > 0)
        {
            int prevEnd = rects[i - 1].x + rects[i - 1].w;
            SetTextColor(hdc, RGB(160, 160, 160)); // グレー
            RECT sepRc = { prevEnd, y, rects[i].x, y + h };
            DrawTextW(hdc, L" \u203A ", 3, &sepRc, DT_VCENTER | DT_SINGLELINE | DT_CENTER);
        }

        // セグメント描画
        bool isLast = (i == segs.size() - 1);
        bool isHover = ((int)i == g_hoverSegment);

        SelectObject(hdc, g_addrFont);

        // ホバー: 青色、通常: 黒
        SetTextColor(hdc, isHover ? RGB(0, 120, 212) : GetSysColor(COLOR_WINDOWTEXT));

        RECT segRc = { rects[i].x, y, rects[i].x + rects[i].w, y + h };
        DrawTextW(hdc, segs[i].c_str(), (int)segs[i].size(), &segRc, DT_VCENTER | DT_SINGLELINE);

        // ホバー下線
        if (isHover)
        {
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 120, 212));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            int baseline = y + h - 4;
            MoveToEx(hdc, rects[i].x, baseline, nullptr);
            LineTo(hdc, rects[i].x + rects[i].w, baseline);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        }
    }

    SelectObject(hdc, hOld);
    EndPaint(hwnd, &ps);
}

// ブレッドクラムのサブクラス
static LRESULT CALLBACK BreadcrumbSubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
        PaintBreadcrumb(hwnd);
        return 0;

    case WM_LBUTTONDOWN:
    {
        int seg = HitTestSegment(hwnd, (short)LOWORD(lParam));
        if (seg < 0)
        {
            // セグメント外クリック → Edit モードに切替
            ShowWindow(hwnd, SW_HIDE);
            ShowWindow(g_app.wnd.hwndAddressEdit, SW_SHOW);
            SetFocus(g_app.wnd.hwndAddressEdit);
            SendMessageW(g_app.wnd.hwndAddressEdit, EM_SETSEL, 0, -1);
        }
        else
        {
            auto segs = SplitPath(g_currentAddrPath);
            std::wstring navPath;
            for (int i = 0; i <= seg; i++)
            {
                if (i > 0) navPath += L"\\";
                navPath += segs[i];
            }
            if (navPath.size() == 2 && navPath[1] == L':') navPath += L"\\";
            NavigateTo(navPath);
        }
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        int seg = HitTestSegment(hwnd, (short)LOWORD(lParam));
        if (seg != g_hoverSegment)
        {
            g_hoverSegment = seg;
            InvalidateRect(hwnd, nullptr, FALSE);

            // マウスがウィンドウから出た時にWM_MOUSELEAVEを受け取る
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        if (g_hoverSegment != -1)
        {
            g_hoverSegment = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_SETCURSOR:
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        int seg = HitTestSegment(hwnd, pt.x);
        SetCursor(LoadCursorW(nullptr, seg >= 0 ? IDC_HAND : IDC_IBEAM));
        return TRUE;
    }

    case WM_ERASEBKGND:
        return 1;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Edit のサブクラスプロシージャ
static LRESULT CALLBACK EditSubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN)
    {
        if (wParam == VK_RETURN)
        {
            wchar_t buf[MAX_PATH];
            GetWindowTextW(hwnd, buf, MAX_PATH);
            ShowWindow(hwnd, SW_HIDE);
            ShowWindow(g_breadcrumb, SW_SHOW);
            SetFocus(g_app.wnd.hwndMain);
            NavigateTo(buf);
            return 0;
        }
        else if (wParam == VK_ESCAPE)
        {
            ShowWindow(hwnd, SW_HIDE);
            ShowWindow(g_breadcrumb, SW_SHOW);
            SetFocus(g_app.wnd.hwndMain);
            return 0;
        }
    }
    else if (msg == WM_KILLFOCUS)
    {
        ShowWindow(hwnd, SW_HIDE);
        ShowWindow(g_breadcrumb, SW_SHOW);
    }
    return CallWindowProcW(g_origEditProc, hwnd, msg, wParam, lParam);
}

void CreateAddressBar(HWND parent, HINSTANCE hInst)
{
    g_addrFont = CreateFontW(
        -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH,
        L"Segoe UI");

    g_addrFontBold = CreateFontW(
        -14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH,
        L"Segoe UI");

    // ラベル
    g_app.wnd.hwndAddressLabel = CreateWindowExW(
        0, L"STATIC", I18nGet(L"ui.address").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
        0, 0, 80, 28,
        parent, (HMENU)(UINT_PTR)IDC_ADDRESSBAR_LABEL, hInst, nullptr);

    // ブレッドクラム（オーナードロー、枠線付き）
    g_breadcrumb = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_NOTIFY,
        28, 0, 500, 28,
        parent, nullptr, hInst, nullptr);

    // パス編集用Edit（通常非表示）
    g_app.wnd.hwndAddressEdit = CreateWindowExW(
        0, L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        28, 0, 500, 24,
        parent, (HMENU)(UINT_PTR)IDC_ADDRESSBAR_EDIT, hInst, nullptr);

    SendMessageW(g_app.wnd.hwndAddressLabel, WM_SETFONT, (WPARAM)g_addrFont, TRUE);
    SendMessageW(g_app.wnd.hwndAddressEdit, WM_SETFONT, (WPARAM)g_addrFont, TRUE);

    // サブクラス化
    g_origEditProc = (WNDPROC)SetWindowLongPtrW(g_app.wnd.hwndAddressEdit, GWLP_WNDPROC, (LONG_PTR)EditSubProc);
    SetWindowLongPtrW(g_breadcrumb, GWLP_WNDPROC, (LONG_PTR)BreadcrumbSubProc);
}

void LayoutAddressBar(int x, int y, int w, int h)
{
    int labelW = 80;
    if (g_app.wnd.hwndAddressLabel)
        MoveWindow(g_app.wnd.hwndAddressLabel, x, y, labelW, h, TRUE);
    if (g_breadcrumb)
        MoveWindow(g_breadcrumb, x + labelW, y, w - labelW, h, TRUE);
    if (g_app.wnd.hwndAddressEdit)
        MoveWindow(g_app.wnd.hwndAddressEdit, x + labelW, y + 2, w - labelW - 4, h - 4, TRUE);
}

void SetAddressBarVisible(bool visible)
{
    int cmd = visible ? SW_SHOW : SW_HIDE;
    if (g_app.wnd.hwndAddressLabel) ShowWindow(g_app.wnd.hwndAddressLabel, cmd);
    if (g_breadcrumb)               ShowWindow(g_breadcrumb, cmd);
    // hwndAddressEdit は通常非表示で、breadcrumb クリック時のみ表示される。
    // トグル時は編集モードを必ず閉じて breadcrumb モードに戻す。
    if (g_app.wnd.hwndAddressEdit)
        ShowWindow(g_app.wnd.hwndAddressEdit, SW_HIDE);
}

void UpdateAddressBar(const std::wstring& path)
{
    g_currentAddrPath = path;

    if (g_app.wnd.hwndAddressEdit)
        SetWindowTextW(g_app.wnd.hwndAddressEdit, path.c_str());

    if (g_breadcrumb)
        InvalidateRect(g_breadcrumb, nullptr, FALSE);
}
