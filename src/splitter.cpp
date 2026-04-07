#include "splitter.h"

static constexpr int kSplitterWidth = 4;

struct SplitterData {
    bool vertical;  // true=左右分割, false=上下分割
    bool dragging;
    int dragOffset;
};

static LRESULT CALLBACK SplitterWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SplitterData* data = (SplitterData*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        auto cs = (LPCREATESTRUCT)lParam;
        data = new SplitterData{};
        data->vertical = (cs->lpCreateParams != nullptr);
        data->dragging = false;
        data->dragOffset = 0;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)data);
        return 0;
    }

    case WM_SETCURSOR:
        SetCursor(LoadCursorW(nullptr, data && data->vertical ? IDC_SIZEWE : IDC_SIZENS));
        return TRUE;

    case WM_LBUTTONDOWN:
    {
        SetCapture(hwnd);
        data->dragging = true;
        if (data->vertical)
            data->dragOffset = (short)LOWORD(lParam);
        else
            data->dragOffset = (short)HIWORD(lParam);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (!data->dragging) return 0;

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(GetParent(hwnd), &pt);

        // 親ウィンドウにスプリッター移動を通知
        // wParam = コントロールID, lParam = 新しい位置
        UINT id = (UINT)GetWindowLongPtrW(hwnd, GWL_ID);
        int newPos = data->vertical ? pt.x : pt.y;
        SendMessageW(GetParent(hwnd), WM_SPLITTER_MOVED, (WPARAM)id, (LPARAM)newPos);
        return 0;
    }

    case WM_LBUTTONUP:
        if (data->dragging)
        {
            data->dragging = false;
            ReleaseCapture();
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE + 1));
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        delete data;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool RegisterSplitterClass(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SplitterWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"KarikariSplitter";
    return RegisterClassExW(&wc) != 0;
}

HWND CreateSplitter(HWND parent, HINSTANCE hInst, bool vertical, UINT id)
{
    return CreateWindowExW(
        0, L"KarikariSplitter", nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0, kSplitterWidth, kSplitterWidth,
        parent, (HMENU)(UINT_PTR)id, hInst,
        vertical ? (LPVOID)1 : nullptr);  // lpParam: non-null = vertical
}
