#include "media.h"
#include "utils.h"
#include "statusbar.h"
#include "nav.h"
#include "window.h"
#include "settings.h"
#include "i18n.h"
#include <cmath>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <evr.h>
#include <shlwapi.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "strmiids.lib")

// === 対応拡張子 ===
static const wchar_t* kVideoExts[] = {
    L".mp4", L".mkv", L".avi", L".mov", L".wmv", L".webm", L".flv", L".m4v", L".ts", L".mpg"
};
static const wchar_t* kAudioExts[] = {
    L".mp3", L".wav", L".ogg", L".flac", L".m4a", L".aac", L".wma", L".opus"
};

bool IsVideoFile(const std::wstring& path) { return HasExtension(path, kVideoExts); }
bool IsAudioFile(const std::wstring& path) { return HasExtension(path, kAudioExts); }
bool IsMediaFile(const std::wstring& path) { return IsVideoFile(path) || IsAudioFile(path); }

// ========================================
// libmpv 動的ロード
// ========================================

// mpv API 型定義
typedef struct mpv_handle mpv_handle;
typedef enum { MPV_FORMAT_NONE=0, MPV_FORMAT_STRING=1, MPV_FORMAT_INT64=4, MPV_FORMAT_DOUBLE=5, MPV_FORMAT_FLAG=3 } mpv_format;

// mpv 関数ポインタ
typedef mpv_handle* (*pfn_mpv_create)();
typedef int (*pfn_mpv_initialize)(mpv_handle*);
typedef int (*pfn_mpv_command)(mpv_handle*, const char**);
typedef int (*pfn_mpv_command_string)(mpv_handle*, const char*);
typedef int (*pfn_mpv_set_option)(mpv_handle*, const char*, mpv_format, void*);
typedef int (*pfn_mpv_set_option_string)(mpv_handle*, const char*, const char*);
typedef int (*pfn_mpv_set_property)(mpv_handle*, const char*, mpv_format, void*);
typedef int (*pfn_mpv_set_property_string)(mpv_handle*, const char*, const char*);
typedef int (*pfn_mpv_get_property)(mpv_handle*, const char*, mpv_format, void*);
typedef void (*pfn_mpv_terminate_destroy)(mpv_handle*);

static HMODULE g_mpvDll = nullptr;
static pfn_mpv_create fn_create = nullptr;
static pfn_mpv_initialize fn_initialize = nullptr;
static pfn_mpv_command fn_command = nullptr;
static pfn_mpv_command_string fn_command_string = nullptr;
static pfn_mpv_set_option fn_set_option = nullptr;
static pfn_mpv_set_option_string fn_set_option_string = nullptr;
static pfn_mpv_set_property fn_set_property = nullptr;
static pfn_mpv_set_property_string fn_set_property_string = nullptr;
static pfn_mpv_get_property fn_get_property = nullptr;
static pfn_mpv_terminate_destroy fn_terminate_destroy = nullptr;

static bool LoadMpvDll()
{
    if (g_mpvDll) return true;

    // exe と同じディレクトリ
    wchar_t dllPath[MAX_PATH];
    GetModuleFileNameW(nullptr, dllPath, MAX_PATH);
    PathRemoveFileSpecW(dllPath);
    // libmpv-2.dll (shinchiro build) または mpv-2.dll を探す
    PathAppendW(dllPath, L"libmpv-2.dll");
    g_mpvDll = LoadLibraryW(dllPath);

    if (!g_mpvDll) {
        PathRemoveFileSpecW(dllPath);
        PathAppendW(dllPath, L"mpv-2.dll");
        g_mpvDll = LoadLibraryW(dllPath);
    }

    // フォールバック: カレントディレクトリ検索は DLL インジェクションの危険があるため廃止
    // exe 同ディレクトリになければ libmpv は使用しない

    if (!g_mpvDll) return false;

    fn_create = (pfn_mpv_create)GetProcAddress(g_mpvDll, "mpv_create");
    fn_initialize = (pfn_mpv_initialize)GetProcAddress(g_mpvDll, "mpv_initialize");
    fn_command = (pfn_mpv_command)GetProcAddress(g_mpvDll, "mpv_command");
    fn_command_string = (pfn_mpv_command_string)GetProcAddress(g_mpvDll, "mpv_command_string");
    fn_set_option = (pfn_mpv_set_option)GetProcAddress(g_mpvDll, "mpv_set_option");
    fn_set_option_string = (pfn_mpv_set_option_string)GetProcAddress(g_mpvDll, "mpv_set_option_string");
    fn_set_property = (pfn_mpv_set_property)GetProcAddress(g_mpvDll, "mpv_set_property");
    fn_set_property_string = (pfn_mpv_set_property_string)GetProcAddress(g_mpvDll, "mpv_set_property_string");
    fn_get_property = (pfn_mpv_get_property)GetProcAddress(g_mpvDll, "mpv_get_property");
    fn_terminate_destroy = (pfn_mpv_terminate_destroy)GetProcAddress(g_mpvDll, "mpv_terminate_destroy");

    return fn_create && fn_initialize && fn_command;
}

// === 状態 ===
static mpv_handle* g_mpv = nullptr;
static HWND g_mediaHwnd = nullptr;
static HWND g_renderHwnd = nullptr;
static HWND g_controlBar = nullptr;
static bool g_isLoop = false;
static bool g_isPlaying = false;
static bool g_mpvAvailable = false;
static bool g_mfInitialized = false;
static UINT_PTR g_updateTimer = 0;
static double g_volume = 1.0;
static bool g_muted = false;
static double g_volumeBeforeMute = 1.0;
static bool g_seekDragging = false;
static int g_hoverBtn = 0; // 0=none, 1=loop, 2=autoplay, 3=vol, 4=speed
static HWND g_tooltip = nullptr;
static double g_speed = 1.0;
static const double kSpeeds[] = { 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0 };
static void GetLoopBtnRect(HWND h, RECT& o);
static void GetAutoPlayBtnRect(HWND h, RECT& o);
static void GetSpeedBtnRect(HWND h, RECT& o);

static void CreateControlBarTooltip(HWND hwndParent, HINSTANCE hInst)
{
    g_tooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwndParent, nullptr, hInst, nullptr);
    if (!g_tooltip) return;

    // IDベースのツール（RECT不要、TTN_GETDISPINFOで動的に返す）
    TOOLINFOW ti = {}; ti.cbSize = sizeof(ti);
    ti.uFlags = TTF_IDISHWND;
    ti.hwnd = hwndParent;
    ti.uId = (UINT_PTR)hwndParent;
    ti.lpszText = LPSTR_TEXTCALLBACKW;
    SendMessageW(g_tooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    SendMessageW(g_tooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 300);
}
static std::wstring g_currentMediaPath;
std::wstring g_tempMediaFile; // 一時ファイルパス（空なら一時ファイルでない、nav.cppから参照）
static constexpr int kControlBarH = 40;

// MF フォールバック用
static ComPtr<IMFMediaSession> g_mfSession;
static ComPtr<IMFVideoDisplayControl> g_mfVideoDisplay;
static ComPtr<IMFSimpleAudioVolume> g_mfAudioVolume;
static double g_mfDuration = 0;
static bool g_usingMF = false; // true: MF使用中, false: mpv使用中

// === MF コールバック ===
class MFCallback : public IMFAsyncCallback {
    LONG refCount_ = 1;
public:
    STDMETHOD(QueryInterface)(REFIID iid, void** ppv) override {
        if (iid == IID_IUnknown || iid == __uuidof(IMFAsyncCallback)) { *ppv = this; AddRef(); return S_OK; }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return InterlockedIncrement(&refCount_); }
    STDMETHOD_(ULONG, Release)() override { LONG r = InterlockedDecrement(&refCount_); if (r == 0) delete this; return r; }
    STDMETHOD(GetParameters)(DWORD*, DWORD*) override { return E_NOTIMPL; }
    STDMETHOD(Invoke)(IMFAsyncResult* pResult) override {
        ComPtr<IMFMediaSession> session = g_mfSession; // ローカルコピー（スレッド安全）
        if (!session) return S_OK;
        ComPtr<IMFMediaEvent> event;
        if (FAILED(session->EndGetEvent(pResult, event.GetAddressOf()))) return S_OK;
        MediaEventType type; event->GetType(&type);

        if (type == MESessionTopologyStatus) {
            UINT32 status = 0; event->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status);
            if (status == MF_TOPOSTATUS_READY) {
                g_mfVideoDisplay.Reset(); g_mfAudioVolume.Reset();
                ComPtr<IMFGetService> svc;
                if (SUCCEEDED(session->QueryInterface(svc.GetAddressOf()))) {
                    svc->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(g_mfVideoDisplay.ReleaseAndGetAddressOf()));
                    svc->GetService(MR_POLICY_VOLUME_SERVICE, IID_PPV_ARGS(g_mfAudioVolume.ReleaseAndGetAddressOf()));
                }
                if (g_mfVideoDisplay && g_renderHwnd) {
                    g_mfVideoDisplay->SetVideoWindow(g_renderHwnd);
                    RECT rc; GetClientRect(g_renderHwnd, &rc);
                    g_mfVideoDisplay->SetVideoPosition(nullptr, &rc);
                }
                if (g_mfAudioVolume) g_mfAudioVolume->SetMasterVolume((float)g_volume);
            }
        } else if (type == MESessionEnded) {
            if (g_isLoop) { PROPVARIANT v; PropVariantInit(&v); v.vt = VT_I8; v.hVal.QuadPart = 0; session->Start(&GUID_NULL, &v); PropVariantClear(&v); }
            else { g_isPlaying = false; }
        }
        session->BeginGetEvent(this, nullptr);
        return S_OK;
    }
};
static MFCallback* g_mfCallback = nullptr;

static HRESULT CreateMFTopology(const std::wstring& path, IMFTopology** ppTopo) {
    ComPtr<IMFTopology> topo;
    HRESULT hr = MFCreateTopology(topo.GetAddressOf());
    if (FAILED(hr)) return hr;

    ComPtr<IMFSourceResolver> res;
    hr = MFCreateSourceResolver(res.GetAddressOf());
    if (FAILED(hr)) return hr;

    MF_OBJECT_TYPE ot; ComPtr<IUnknown> srcU;
    hr = res->CreateObjectFromURL(path.c_str(), MF_RESOLUTION_MEDIASOURCE, nullptr, &ot, srcU.GetAddressOf());
    if (FAILED(hr)) return hr;

    ComPtr<IMFMediaSource> src;
    hr = srcU.As(&src);
    if (FAILED(hr)) return hr;

    ComPtr<IMFPresentationDescriptor> pd;
    hr = src->CreatePresentationDescriptor(pd.GetAddressOf());
    if (FAILED(hr)) return hr;
    UINT64 d = 0; pd->GetUINT64(MF_PD_DURATION, &d); g_mfDuration = d / 10000000.0;
    DWORD cnt; pd->GetStreamDescriptorCount(&cnt);
    for (DWORD i = 0; i < cnt; i++) {
        BOOL sel; ComPtr<IMFStreamDescriptor> sd; pd->GetStreamDescriptorByIndex(i, &sel, sd.GetAddressOf());
        if (!sel) continue;
        ComPtr<IMFMediaTypeHandler> h; sd->GetMediaTypeHandler(h.GetAddressOf());
        GUID mt; h->GetMajorType(&mt);
        ComPtr<IMFTopologyNode> sn, on;
        MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, sn.GetAddressOf());
        sn->SetUnknown(MF_TOPONODE_SOURCE, src.Get()); sn->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pd.Get()); sn->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, sd.Get());
        topo->AddNode(sn.Get());
        MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, on.GetAddressOf());
        ComPtr<IMFActivate> act;
        if (mt == MFMediaType_Video) MFCreateVideoRendererActivate(g_renderHwnd, act.GetAddressOf());
        else if (mt == MFMediaType_Audio) MFCreateAudioRendererActivate(act.GetAddressOf());
        else continue;
        on->SetObject(act.Get()); topo->AddNode(on.Get()); sn->ConnectOutput(0, on.Get(), 0);
    }
    *ppTopo = topo.Detach(); return S_OK;
}

// === コントロールバー ===
static std::wstring FormatTime(double s) {
    if (s < 0 || s != s) s = 0;
    int h = (int)(s/3600), m = (int)(s/60)%60, sec = (int)s%60;
    wchar_t b[32];
    if (h > 0) swprintf_s(b, 32, L"%d:%02d:%02d", h, m, sec);
    else swprintf_s(b, 32, L"%d:%02d", m, sec);
    return b;
}

// 右端から: [音量バー90px][gap2][🔊20px][gap8][🔁32px][gap8][A32px][gap8][1.0x 42px][gap4][時間110px][シークバー]
static void GetSeekBarRect(HWND h, RECT& o) { RECT r; GetClientRect(h,&r); o={44,14,r.right-376,26}; }
static void GetVolumeBarRect(HWND h, RECT& o) { RECT r; GetClientRect(h,&r); o={r.right-98,16,r.right-8,24}; }
static void GetVolIconRect(HWND h, RECT& o) { RECT r; GetClientRect(h,&r); o={r.right-120,4,r.right-100,36}; }
static void GetLoopBtnRect(HWND h, RECT& o) { RECT r; GetClientRect(h,&r); o={r.right-160,4,r.right-128,36}; }
static void GetAutoPlayBtnRect(HWND h, RECT& o) { RECT r; GetClientRect(h,&r); o={r.right-200,4,r.right-168,36}; }
static void GetSpeedBtnRect(HWND h, RECT& o) { RECT r; GetClientRect(h,&r); o={r.right-250,4,r.right-208,36}; }

// GDI ブラシキャッシュ（MediaInit で作成）
static HBRUSH g_brDark   = nullptr;
static HBRUSH g_brGray   = nullptr;
static HBRUSH g_brBlue   = nullptr;
static HBRUSH g_brWhite  = nullptr;
static HBRUSH g_brGreen  = nullptr;

static void PaintControlBar(HWND hwnd, HDC hdc) {
    RECT rc; GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, g_brDark);
    SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, RGB(220,220,220));
    static HFONT hFont = CreateFontW(-13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    HFONT hOld = (HFONT)SelectObject(hdc, hFont);

    double pos = MediaGetPosition(), dur = MediaGetDuration();
    RECT btn = {8,4,38,36};
    {
        static HFONT hBtnFont = CreateFontW(-18, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe Fluent Icons");
        HFONT hPrev = hBtnFont ? (HFONT)SelectObject(hdc, hBtnFont) : nullptr;
        // E769=Pause, E768=Play (Segoe Fluent Icons、枠なし)
        DrawTextW(hdc, g_isPlaying ? L"\uE769" : L"\uE768", -1, &btn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (hPrev) SelectObject(hdc, hPrev);
    }

    RECT sr; GetSeekBarRect(hwnd, sr);
    if (sr.right > sr.left+10) {
        FillRect(hdc, &sr, g_brGray);
        if (dur > 0) {
            int fw = (int)((pos/dur)*(sr.right-sr.left));
            RECT sf={sr.left,sr.top,sr.left+fw,sr.bottom}; FillRect(hdc,&sf,g_brBlue);
            int kx=sr.left+fw; RECT kn={kx-5,sr.top-3,kx+5,sr.bottom+3}; FillRect(hdc,&kn,g_brWhite);
        }
    }
    std::wstring ts = FormatTime(pos)+L" / "+FormatTime(dur);
    RECT tr={sr.right+8,4,sr.right+118,36}; DrawTextW(hdc,ts.c_str(),-1,&tr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    // ループボタン（Segoe Fluent Icons、大きめ表示 + オン時は背景色付き）
    RECT lr; GetLoopBtnRect(hwnd, lr);
    {
        RECT bgRect = { lr.left - 2, lr.top + 4, lr.right + 2, lr.bottom - 4 };
        if (g_isLoop)
        {
            HBRUSH hBg = CreateSolidBrush(RGB(66, 133, 244));
            FillRect(hdc, &bgRect, hBg);
            DeleteObject(hBg);
            SetTextColor(hdc, RGB(255, 255, 255));
        }
        else if (g_hoverBtn == 1)
        {
            HBRUSH hBg = CreateSolidBrush(RGB(70, 70, 70));
            FillRect(hdc, &bgRect, hBg);
            DeleteObject(hBg);
            SetTextColor(hdc, RGB(220, 220, 220));
        }
        else
        {
            SetTextColor(hdc, RGB(160, 160, 160));
        }
    }
    {
        static HFONT hLoopFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe Fluent Icons");
        HFONT hPrev = hLoopFont ? (HFONT)SelectObject(hdc, hLoopFont) : nullptr;
        // U+E8EE = RepeatAll アイコン
        DrawTextW(hdc, L"\uE8EE", -1, &lr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (hPrev) SelectObject(hdc, hPrev);
    }
    SetTextColor(hdc, RGB(220, 220, 220));

    // 自動再生ボタン
    RECT ar; GetAutoPlayBtnRect(hwnd, ar);
    {
        bool autoPlay = GetCachedSettings().autoPlayMedia;
        RECT bgRect = { ar.left - 2, ar.top + 4, ar.right + 2, ar.bottom - 4 };
        if (autoPlay)
        {
            HBRUSH hBg = CreateSolidBrush(RGB(66, 133, 244));
            FillRect(hdc, &bgRect, hBg);
            DeleteObject(hBg);
            SetTextColor(hdc, RGB(255, 255, 255));
        }
        else if (g_hoverBtn == 2)
        {
            HBRUSH hBg = CreateSolidBrush(RGB(70, 70, 70));
            FillRect(hdc, &bgRect, hBg);
            DeleteObject(hBg);
            SetTextColor(hdc, RGB(220, 220, 220));
        }
        else
        {
            SetTextColor(hdc, RGB(160, 160, 160));
        }
        static HFONT hAFont = CreateFontW(-14, 0, 0, 0, FW_BOLD, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HFONT hPrev = hAFont ? (HFONT)SelectObject(hdc, hAFont) : nullptr;
        DrawTextW(hdc, L"A", -1, &ar, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (hPrev) SelectObject(hdc, hPrev);
    }
    SetTextColor(hdc, RGB(220, 220, 220));

    // 速度ボタン
    RECT spr; GetSpeedBtnRect(hwnd, spr);
    {
        bool isDefault = (abs(g_speed - 1.0) < 0.01);
        RECT bgRect = { spr.left - 2, spr.top + 4, spr.right + 2, spr.bottom - 4 };
        if (!isDefault)
        {
            HBRUSH hBg = CreateSolidBrush(RGB(66, 133, 244));
            FillRect(hdc, &bgRect, hBg);
            DeleteObject(hBg);
            SetTextColor(hdc, RGB(255, 255, 255));
        }
        else if (g_hoverBtn == 4)
        {
            HBRUSH hBg = CreateSolidBrush(RGB(70, 70, 70));
            FillRect(hdc, &bgRect, hBg);
            DeleteObject(hBg);
            SetTextColor(hdc, RGB(220, 220, 220));
        }
        else
        {
            SetTextColor(hdc, RGB(160, 160, 160));
        }
        static HFONT hSpeedFont = CreateFontW(-12, 0, 0, 0, FW_BOLD, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HFONT hPrev = hSpeedFont ? (HFONT)SelectObject(hdc, hSpeedFont) : nullptr;
        wchar_t speedBuf[16];
        swprintf_s(speedBuf, L"%.1fx", g_speed);
        DrawTextW(hdc, speedBuf, -1, &spr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (hPrev) SelectObject(hdc, hPrev);
    }
    SetTextColor(hdc, RGB(220, 220, 220));

    // 音量アイコン（Segoe Fluent Icons）
    RECT vi; GetVolIconRect(hwnd, vi);
    {
        static HFONT hVolFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe Fluent Icons");
        HFONT hPrev = hVolFont ? (HFONT)SelectObject(hdc, hVolFont) : nullptr;
        // U+E767 = Volume2 (中音量), U+E74F = Mute
        DrawTextW(hdc, g_muted ? L"\uE74F" : L"\uE767", -1, &vi, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (hPrev) SelectObject(hdc, hPrev);
    }

    RECT vr; GetVolumeBarRect(hwnd, vr);
    FillRect(hdc, &vr, g_brGray);
    int vfw=(int)(g_volume*(vr.right-vr.left));
    RECT vf={vr.left,vr.top,vr.left+vfw,vr.bottom}; FillRect(hdc,&vf,g_brGreen);
    SelectObject(hdc, hOld);
}

static void OnControlBarClick(HWND hwnd, int x, int y, bool drag) {
    if (!drag && x < 44) { MediaTogglePlayPause(); InvalidateRect(hwnd,0,TRUE); return; }
    RECT sr; GetSeekBarRect(hwnd, sr);
    if (!drag) {
        RECT lr; GetLoopBtnRect(hwnd, lr);
        if (x>=lr.left&&x<lr.right) { MediaToggleLoop(); InvalidateRect(hwnd,0,TRUE); return; }
        // 自動再生トグル
        RECT ar; GetAutoPlayBtnRect(hwnd, ar);
        if (x>=ar.left&&x<ar.right) {
            AppSettings s; LoadSettings(s);
            s.autoPlayMedia = !s.autoPlayMedia;
            SaveSettings(s);
            InvalidateSettingsCache();
            InvalidateRect(hwnd,0,TRUE); return;
        }
        // 速度ボタン: ポップアップメニューで選択
        RECT spr; GetSpeedBtnRect(hwnd, spr);
        if (x>=spr.left&&x<spr.right) {
            HMENU hMenu = CreatePopupMenu();
            for (int i = 0; i < _countof(kSpeeds); i++) {
                wchar_t buf[16];
                swprintf_s(buf, L"%.1fx", kSpeeds[i]);
                UINT flags = MF_STRING;
                if (abs(g_speed - kSpeeds[i]) < 0.01) flags |= MF_CHECKED;
                AppendMenuW(hMenu, flags, 100 + i, buf);
            }
            POINT pt = { spr.left, spr.top };
            ClientToScreen(hwnd, &pt);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(hMenu);
            if (cmd >= 100 && cmd < 100 + (int)_countof(kSpeeds)) {
                g_speed = kSpeeds[cmd - 100];
                MediaSetSpeed(g_speed);
            }
            InvalidateRect(hwnd,0,TRUE); return;
        }
        // 音量アイコンクリック: ミュートトグル
        RECT vi; GetVolIconRect(hwnd, vi);
        if (x>=vi.left&&x<vi.right) {
            g_muted = !g_muted;
            if (g_muted) { g_volumeBeforeMute = g_volume; MediaSetVolume(0.0); }
            else { MediaSetVolume(g_volumeBeforeMute); }
            InvalidateRect(hwnd,0,TRUE); return;
        }
    }
    if (x>=sr.left && x<=sr.right && sr.right>sr.left) {
        double r = std::max(0.0, std::min(1.0, (double)(x-sr.left)/(sr.right-sr.left)));
        double d = MediaGetDuration(); if (d>0) MediaSeek(r*d); InvalidateRect(hwnd,0,TRUE); return;
    }
    RECT vr; GetVolumeBarRect(hwnd, vr);
    if (x>=vr.left-5 && x<=vr.right+5) {
        double r = std::max(0.0, std::min(1.0, (double)(x-vr.left)/(vr.right-vr.left)));
        g_muted = false; // 音量バー操作時はミュート解除
        MediaSetVolume(r); InvalidateRect(hwnd,0,TRUE); return;
    }
}

static LRESULT CALLBACK ControlBarWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch(m) {
    case WM_PAINT: { PAINTSTRUCT ps; HDC dc=BeginPaint(h,&ps); PaintControlBar(h,dc); EndPaint(h,&ps); return 0; }
    case WM_LBUTTONDOWN: SetCapture(h); g_seekDragging=true; OnControlBarClick(h,(short)LOWORD(l),(short)HIWORD(l),false); return 0;
    case WM_MOUSEMOVE: {
        if(g_seekDragging) { OnControlBarClick(h,(short)LOWORD(l),(short)HIWORD(l),true); return 0; }
        // ツールチップにリレー
        if (g_tooltip) {
            MSG msg = { h, m, w, l };
            SendMessageW(g_tooltip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
        }
        // ホバー追跡
        int mx = (short)LOWORD(l);
        int newHover = 0;
        RECT lr; GetLoopBtnRect(h, lr);
        RECT ar; GetAutoPlayBtnRect(h, ar);
        RECT vi; GetVolIconRect(h, vi);
        RECT spr; GetSpeedBtnRect(h, spr);
        if (mx>=lr.left&&mx<lr.right) newHover = 1;
        else if (mx>=ar.left&&mx<ar.right) newHover = 2;
        else if (mx>=vi.left&&mx<vi.right) newHover = 3;
        else if (mx>=spr.left&&mx<spr.right) newHover = 4;
        if (newHover != g_hoverBtn) {
            g_hoverBtn = newHover;
            InvalidateRect(h,0,TRUE);
            // ツールチップのテキスト更新
            if (g_tooltip) {
                TOOLINFOW ti = {}; ti.cbSize = sizeof(ti);
                ti.uFlags = TTF_IDISHWND;
                ti.hwnd = h;
                ti.uId = (UINT_PTR)h;
                if (newHover == 1) ti.lpszText = (LPWSTR)I18nGet(L"media.loop").c_str();
                else if (newHover == 2) ti.lpszText = (LPWSTR)I18nGet(L"media.autoplay").c_str();
                else if (newHover == 4) ti.lpszText = (LPWSTR)I18nGet(L"media.speed").c_str();
                else ti.lpszText = (LPWSTR)L"";
                SendMessageW(g_tooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
                if (!newHover) SendMessageW(g_tooltip, TTM_POP, 0, 0);
            }
        }
        // WM_MOUSELEAVE登録
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, h, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_MOUSELEAVE:
        if (g_hoverBtn) { g_hoverBtn = 0; InvalidateRect(h,0,TRUE); }
        if (g_tooltip) SendMessageW(g_tooltip, TTM_POP, 0, 0);
        return 0;
    case WM_LBUTTONUP: g_seekDragging=false; ReleaseCapture(); return 0;
    case WM_TIMER: {
        double pos = MediaGetPosition();
        double dur = MediaGetDuration();
        // 常に再描画（位置・時間表示を更新）
        InvalidateRect(h, nullptr, TRUE);
        if(g_mfVideoDisplay) g_mfVideoDisplay->RepaintVideo();
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    }
    return DefWindowProcW(h,m,w,l);
}

static bool g_renderClickPending = false;
static bool g_dblClickConsumed = false;
static constexpr UINT_PTR kClickTimer = 50;
static constexpr UINT_PTR kAutoHideTimer = 51;
static constexpr int kAutoHideCheckMs = 500; // 500ms間隔でチェック
static constexpr int kAutoHideIdleMs = 3000; // 3秒無操作で非表示
static bool g_controlBarVisible = true;
static bool g_cursorHidden = false;
static POINT g_lastCursorPos = {-1, -1};
static ULONGLONG g_lastCursorMoveTime = 0;

static void ShowControlBar(bool show)
{
    if (!g_controlBar) return;
    if (show == g_controlBarVisible) return;
    g_controlBarVisible = show;
    ShowWindow(g_controlBar, show ? SW_SHOW : SW_HIDE);
    // 全画面時: バーと一緒にカーソルも表示/非表示
    if (g_app.isFullscreen) {
        if (!show && !g_cursorHidden) {
            SetCursor(nullptr);
            g_cursorHidden = true;
        } else if (show && g_cursorHidden) {
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            g_cursorHidden = false;
        }
    }
    if (g_app.isFullscreen && g_app.wnd.hwndMediaPlayer)
    {
        RECT r; GetClientRect(g_app.wnd.hwndMediaPlayer, &r);
        int h = r.bottom;
        int barH = show ? kControlBarH : 0;
        if (g_renderHwnd) MoveWindow(g_renderHwnd, 0, 0, r.right, h - barH, TRUE);
        if (g_controlBar && show) MoveWindow(g_controlBar, 0, h - kControlBarH, r.right, kControlBarH, TRUE);
        if (g_mfVideoDisplay) { RECT vr = {0, 0, r.right, h - barH}; g_mfVideoDisplay->SetVideoPosition(nullptr, &vr); }
    }
}

static bool g_autoHideActive = false;

static void StartAutoHidePolling(HWND hwnd)
{
    if (!g_app.isFullscreen) return;
    if (!g_autoHideActive) {
        // 初回のみタイムスタンプ更新（WM_SIZEによる再起動でリセットしない）
        GetCursorPos(&g_lastCursorPos);
        g_lastCursorMoveTime = GetTickCount64();
    }
    g_autoHideActive = true;
    SetTimer(hwnd, kAutoHideTimer, kAutoHideCheckMs, nullptr);
}

static void StopAutoHidePolling(HWND hwnd)
{
    KillTimer(hwnd, kAutoHideTimer);
    g_autoHideActive = false;
    ShowControlBar(true);
}

static LRESULT CALLBACK RenderWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch(m) {
    case WM_SETCURSOR:
        if (g_cursorHidden) { SetCursor(nullptr); return TRUE; }
        break; // デフォルト処理に任せる
    case WM_LBUTTONUP:
        if (g_dblClickConsumed) { g_dblClickConsumed = false; return 0; } // ダブルクリック後の2回目UPを無視
        g_renderClickPending = true;
        SetTimer(h, kClickTimer, 250, nullptr);
        return 0;
    case WM_LBUTTONDBLCLK:
        g_renderClickPending = false;
        g_dblClickConsumed = true;
        KillTimer(h, kClickTimer);
        PostMessageW(g_app.wnd.hwndMain, WM_TOGGLE_FULLSCREEN, 0, 0);
        return 0;
    case WM_MOUSEMOVE:
        // mpvが描画中にWM_MOUSEMOVEを連発するため、ここでは何もしない
        // カーソル移動の検出はポーリングタイマー（GetCursorPos比較）で行う
        return 0;
    case WM_TIMER:
        if (w == kClickTimer) {
            KillTimer(h, kClickTimer);
            if (g_renderClickPending) {
                g_renderClickPending = false;
                MediaTogglePlayPause();
                if (g_controlBar) InvalidateRect(g_controlBar, 0, TRUE);
                // 一時停止したらバーを表示し続ける
                if (!g_isPlaying && g_app.isFullscreen) {
                    KillTimer(h, kAutoHideTimer);
                    ShowControlBar(true);
                }
            }
            return 0;
        }
        if (w == kAutoHideTimer) {
            // ポーリング: カーソル位置を比較して動いたか判定
            POINT cur; GetCursorPos(&cur);
            if (cur.x != g_lastCursorPos.x || cur.y != g_lastCursorPos.y) {
                g_lastCursorPos = cur;
                g_lastCursorMoveTime = GetTickCount64();
                if (!g_controlBarVisible) ShowControlBar(true);
            } else {
                // カーソル静止中: アイドル時間超過なら非表示
                if (g_app.isFullscreen && g_isPlaying &&
                    GetTickCount64() - g_lastCursorMoveTime >= (ULONGLONG)kAutoHideIdleMs)
                    ShowControlBar(false);
            }
            return 0;
        }
        break;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(h,&ps); if(g_mfVideoDisplay) g_mfVideoDisplay->RepaintVideo(); else { RECT r; GetClientRect(h,&r); FillRect(ps.hdc,&r,(HBRUSH)GetStockObject(BLACK_BRUSH)); } EndPaint(h,&ps); return 0; }
    case WM_SIZE: if(g_mfVideoDisplay){ RECT r; GetClientRect(h,&r); g_mfVideoDisplay->SetVideoPosition(nullptr,&r); } return 0;
    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(w);
        int step = delta > 0 ? -1 : 1;
        GoToFile(g_app.nav.currentFileIndex + step);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    }
    return DefWindowProcW(h,m,w,l);
}

static void LayoutMediaPlayer(HWND h) {
    RECT r; GetClientRect(h,&r); int w=r.right, ht=r.bottom;
    // 全画面解除時はバーを強制表示
    if (!g_app.isFullscreen && !g_controlBarVisible)
        ShowControlBar(true);
    int barH = g_controlBarVisible ? kControlBarH : 0;
    if(g_renderHwnd) MoveWindow(g_renderHwnd,0,0,w,ht-barH,TRUE);
    if(g_controlBar && g_controlBarVisible) MoveWindow(g_controlBar,0,ht-kControlBarH,w,kControlBarH,TRUE);
    if(g_mfVideoDisplay){ RECT vr={0,0,w,ht-barH}; g_mfVideoDisplay->SetVideoPosition(nullptr,&vr); }
    // 全画面に入った時にポーリング開始
    if (g_app.isFullscreen && g_isPlaying && g_renderHwnd)
        StartAutoHidePolling(g_renderHwnd);
}

static LRESULT CALLBACK MediaPlayerWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if(m==WM_SIZE){ LayoutMediaPlayer(h); return 0; }
    return DefWindowProcW(h,m,w,l);
}

// === 公開関数 ===

bool MediaInit(HINSTANCE hInst) {
    g_mpvAvailable = LoadMpvDll();
    HRESULT hr = MFStartup(MF_VERSION);
    g_mfInitialized = SUCCEEDED(hr);
    g_isLoop = GetCachedSettings().loopMedia;

    // GDI ブラシ初期化
    g_brDark  = CreateSolidBrush(RGB(30, 30, 30));
    g_brGray  = CreateSolidBrush(RGB(80, 80, 80));
    g_brBlue  = CreateSolidBrush(RGB(66, 133, 244));
    g_brWhite = CreateSolidBrush(RGB(255, 255, 255));
    g_brGreen = CreateSolidBrush(RGB(76, 175, 80));

    return g_mpvAvailable || g_mfInitialized;
}

void MediaShutdown() {
    MediaStop();
    if (g_mpv && fn_terminate_destroy) { fn_terminate_destroy(g_mpv); g_mpv = nullptr; }
    if (g_mpvDll) { FreeLibrary(g_mpvDll); g_mpvDll = nullptr; }
    if (g_mfCallback) { g_mfCallback->Release(); g_mfCallback = nullptr; }
    if (g_mfInitialized) { MFShutdown(); g_mfInitialized = false; }

    // GDI ブラシ解放
    if (g_brDark)  { DeleteObject(g_brDark);  g_brDark = nullptr; }
    if (g_brGray)  { DeleteObject(g_brGray);  g_brGray = nullptr; }
    if (g_brBlue)  { DeleteObject(g_brBlue);  g_brBlue = nullptr; }
    if (g_brWhite) { DeleteObject(g_brWhite); g_brWhite = nullptr; }
    if (g_brGreen) { DeleteObject(g_brGreen); g_brGreen = nullptr; }
}

bool RegisterMediaPlayerClass(HINSTANCE hInst) {
    WNDCLASSEXW wc={}; wc.cbSize=sizeof(wc);
    wc.hCursor=LoadCursorW(nullptr, IDC_ARROW);
    wc.lpfnWndProc=MediaPlayerWndProc; wc.hInstance=hInst; wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH); wc.lpszClassName=L"KarikariMedia";
    if(!RegisterClassExW(&wc)) return false;
    wc.lpfnWndProc=RenderWndProc; wc.lpszClassName=L"KarikariMediaRender"; wc.style=CS_DBLCLKS;
    if(!RegisterClassExW(&wc)) return false;
    wc.style=0;
    wc.lpfnWndProc=ControlBarWndProc; wc.lpszClassName=L"KarikariMediaControl"; wc.hbrBackground=nullptr;
    return RegisterClassExW(&wc)!=0;
}

HWND CreateMediaPlayer(HWND parent, HINSTANCE hInst) {
    g_mediaHwnd = CreateWindowExW(0,L"KarikariMedia",0,WS_CHILD|WS_CLIPCHILDREN,0,0,100,100,parent,0,hInst,0);
    g_renderHwnd = CreateWindowExW(0,L"KarikariMediaRender",0,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,0,0,100,60,g_mediaHwnd,0,hInst,0);
    g_controlBar = CreateWindowExW(0,L"KarikariMediaControl",0,WS_CHILD|WS_VISIBLE,0,60,100,kControlBarH,g_mediaHwnd,0,hInst,0);
    CreateControlBarTooltip(g_controlBar, hInst);
    g_app.wnd.hwndMediaPlayer = g_mediaHwnd;
    return g_mediaHwnd;
}

// --- mpv でパスをUTF-8に変換 ---
static std::string WideToUtf8(const std::wstring& w) {
    int len = WideCharToMultiByte(CP_UTF8,0,w.c_str(),(int)w.size(),0,0,0,0);
    std::string s(len,0); WideCharToMultiByte(CP_UTF8,0,w.c_str(),(int)w.size(),&s[0],len,0,0);
    return s;
}

static bool PlayWithMpv(const std::wstring& path) {
    if (!g_mpvAvailable) return false;

    // mpv インスタンスを毎回作り直し（ウィンドウ埋め込みのため）
    if (g_mpv) { fn_terminate_destroy(g_mpv); g_mpv = nullptr; }

    g_mpv = fn_create();
    if (!g_mpv) return false;

    // レンダリング先ウィンドウ
    int64_t wid = (int64_t)(intptr_t)g_renderHwnd;
    fn_set_option(g_mpv, "wid", MPV_FORMAT_INT64, &wid);
    fn_set_option_string(g_mpv, "keep-open", "yes");
    fn_set_option_string(g_mpv, "osc", "no");        // OSD コントローラ無効（自前UI）
    fn_set_option_string(g_mpv, "input-default-bindings", "no");
    fn_set_option_string(g_mpv, "input-vo-keyboard", "no");

    if (fn_initialize(g_mpv) < 0) { fn_terminate_destroy(g_mpv); g_mpv = nullptr; return false; }

    // 音量設定
    int64_t vol = (int64_t)(g_volume * 100);
    fn_set_property(g_mpv, "volume", MPV_FORMAT_INT64, &vol);

    // ループ
    fn_set_property_string(g_mpv, "loop-file", g_isLoop ? "inf" : "no");

    // 再生
    std::string u8path = WideToUtf8(path);
    const char* cmd[] = {"loadfile", u8path.c_str(), NULL};
    fn_command(g_mpv, cmd);

    g_usingMF = false;
    return true;
}

static bool PlayWithMF(const std::wstring& path) {
    if (!g_mfInitialized) return false;

    if (g_mfSession) { g_mfSession->Stop(); g_mfSession->Close(); g_mfSession.Reset(); }
    g_mfVideoDisplay.Reset(); g_mfAudioVolume.Reset(); g_mfDuration = 0;

    HRESULT hr = MFCreateMediaSession(nullptr, g_mfSession.ReleaseAndGetAddressOf());
    if (FAILED(hr)) return false;

    if (!g_mfCallback) g_mfCallback = new MFCallback();
    g_mfSession->BeginGetEvent(g_mfCallback, nullptr);

    ComPtr<IMFTopology> topo;
    hr = CreateMFTopology(path, topo.GetAddressOf());
    if (FAILED(hr)) { g_mfSession.Reset(); return false; }

    hr = g_mfSession->SetTopology(0, topo.Get());
    if (FAILED(hr)) { g_mfSession.Reset(); return false; }

    PROPVARIANT v; PropVariantInit(&v);
    g_mfSession->Start(&GUID_NULL, &v);
    PropVariantClear(&v);

    g_usingMF = true;
    return true;
}

void MediaPlay(const std::wstring& path) {
    // 遅延初期化（初回呼び出し時のみ）
    static bool initialized = false;
    if (!initialized)
    {
        MediaInit(g_app.hInstance);
        RegisterMediaPlayerClass(g_app.hInstance);
        if (!g_app.wnd.hwndMediaPlayer)
            CreateMediaPlayer(g_app.wnd.hwndMain, g_app.hInstance);
        initialized = true;
        // 初回作成後にレイアウト更新（プレーヤーの位置/サイズ設定）
        LayoutChildren(g_app.wnd.hwndMain);
    }

    MediaStop();

    // mpv を優先、失敗したら MF にフォールバック
    bool ok = PlayWithMpv(path);
    if (!ok) ok = PlayWithMF(path);

    if (!ok) {
        UpdateStatusBar(path, 0, 0, L"再生できません（mpv-2.dll を配置してください）");
        return;
    }

    g_isPlaying = true;
    g_currentMediaPath = path;
    if (g_controlBar) g_updateTimer = SetTimer(g_controlBar, 1, 250, nullptr);

    // 全画面なら自動非表示ポーリング開始
    if (g_app.isFullscreen && g_renderHwnd)
        StartAutoHidePolling(g_renderHwnd);

    // 自動再生OFF: 読み込み直後に一時停止
    if (!GetCachedSettings().autoPlayMedia)
        MediaTogglePlayPause();
}

void MediaStop() {
    if (g_updateTimer && g_controlBar) { KillTimer(g_controlBar, g_updateTimer); g_updateTimer = 0; }
    if (g_renderHwnd) KillTimer(g_renderHwnd, kAutoHideTimer);
    g_autoHideActive = false;
    g_controlBarVisible = true; // 次回表示時は表示状態から開始
    if (g_controlBar) ShowWindow(g_controlBar, SW_SHOW);
    if (g_cursorHidden) { SetCursor(LoadCursorW(nullptr, IDC_ARROW)); g_cursorHidden = false; }

    if (g_mpv && !g_usingMF) {
        const char* cmd[] = {"stop", NULL};
        fn_command(g_mpv, cmd);
    }
    if (g_mfSession && g_usingMF) {
        g_mfSession->Stop(); g_mfSession->Close(); g_mfSession.Reset();
    }
    g_mfVideoDisplay.Reset(); g_mfAudioVolume.Reset();
    g_isPlaying = false; g_mfDuration = 0; g_currentMediaPath.clear(); g_speed = 1.0;
    // 一時メディアファイル削除
    if (!g_tempMediaFile.empty()) {
        DeleteFileW(g_tempMediaFile.c_str());
        g_tempMediaFile.clear();
    }
    if (g_controlBar) InvalidateRect(g_controlBar, 0, TRUE);
    if (g_renderHwnd) InvalidateRect(g_renderHwnd, 0, TRUE);
}

void MediaTogglePlayPause() {
    if (g_mpv && !g_usingMF) {
        int pause = g_isPlaying ? 1 : 0;
        fn_set_property(g_mpv, "pause", MPV_FORMAT_FLAG, &pause);
        g_isPlaying = !g_isPlaying;
    } else if (g_mfSession && g_usingMF) {
        if (g_isPlaying) { g_mfSession->Pause(); g_isPlaying = false; }
        else { PROPVARIANT v; PropVariantInit(&v); g_mfSession->Start(&GUID_NULL,&v); PropVariantClear(&v); g_isPlaying = true; }
    }
    if (g_controlBar) InvalidateRect(g_controlBar, 0, TRUE);
    // 再生再開時にポーリング開始、一時停止時にポーリング停止してバー表示
    if (g_app.isFullscreen && g_renderHwnd) {
        if (g_isPlaying) StartAutoHidePolling(g_renderHwnd);
        else StopAutoHidePolling(g_renderHwnd);
    }
}

void MediaSeek(double seconds) {
    if (g_mpv && !g_usingMF) {
        char buf[64]; snprintf(buf, 64, "%.3f", seconds);
        const char* cmd[] = {"seek", buf, "absolute", NULL};
        fn_command(g_mpv, cmd);
    } else if (g_mfSession && g_usingMF) {
        PROPVARIANT v; PropVariantInit(&v); v.vt=VT_I8; v.hVal.QuadPart=(LONGLONG)(seconds*10000000.0);
        g_mfSession->Start(&GUID_NULL,&v); PropVariantClear(&v);
        if (!g_isPlaying) g_isPlaying = true;
    }
}

void MediaSetVolume(double vol) {
    g_volume = std::max(0.0, std::min(1.0, vol));
    if (g_mpv && !g_usingMF) {
        int64_t v = (int64_t)(g_volume * 100);
        fn_set_property(g_mpv, "volume", MPV_FORMAT_INT64, &v);
    }
    if (g_mfAudioVolume && g_usingMF)
        g_mfAudioVolume->SetMasterVolume((float)g_volume);
}

void MediaSetSpeed(double speed) {
    if (g_mpv && !g_usingMF) {
        fn_set_property(g_mpv, "speed", MPV_FORMAT_DOUBLE, &speed);
    } else if (g_mfSession && g_usingMF) {
        ComPtr<IMFGetService> svc;
        if (SUCCEEDED(g_mfSession->QueryInterface(svc.GetAddressOf()))) {
            ComPtr<IMFRateControl> rate;
            if (SUCCEEDED(svc->GetService(MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(rate.GetAddressOf()))))
                rate->SetRate(FALSE, (float)speed);
        }
    }
}

void MediaToggleLoop() {
    g_isLoop = !g_isLoop;
    if (g_mpv && !g_usingMF)
        fn_set_property_string(g_mpv, "loop-file", g_isLoop ? "inf" : "no");
    // 設定に永続化
    AppSettings s;
    if (LoadSettings(s)) {
        s.loopMedia = g_isLoop;
        SaveSettings(s);
        InvalidateSettingsCache();
    }
}

double MediaGetPosition() {
    if (g_mpv && !g_usingMF) {
        double pos = 0;
        if (fn_get_property(g_mpv, "time-pos", MPV_FORMAT_DOUBLE, &pos) >= 0) return pos;
        return 0;
    }
    if (g_mfSession && g_usingMF) {
        ComPtr<IMFClock> clk; if (FAILED(g_mfSession->GetClock(clk.GetAddressOf()))) return 0;
        ComPtr<IMFPresentationClock> pc; if (FAILED(clk.As(&pc))) return 0;
        MFTIME t=0; pc->GetTime(&t); return t/10000000.0;
    }
    return 0;
}

double MediaGetDuration() {
    if (g_mpv && !g_usingMF) {
        double dur = 0;
        if (fn_get_property(g_mpv, "duration", MPV_FORMAT_DOUBLE, &dur) >= 0) return dur;
        return 0;
    }
    return g_mfDuration;
}

bool MediaIsPlaying() { return g_isPlaying; }
