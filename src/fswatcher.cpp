#include "fswatcher.h"
#include <thread>
#include <atomic>

static std::atomic<bool> g_watchStop{false};
static std::thread g_watchThread;
static HANDLE g_watchDir = INVALID_HANDLE_VALUE;

static void WatcherThread(std::wstring folderPath)
{
    HANDLE hDir = CreateFileW(folderPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

    if (hDir == INVALID_HANDLE_VALUE) return;
    g_watchDir = hDir;

    BYTE buf[16384];
    OVERLAPPED ol = {};
    ol.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    while (!g_watchStop.load())
    {
        DWORD bytesReturned = 0;
        ResetEvent(ol.hEvent);

        if (!ReadDirectoryChangesW(hDir, buf, sizeof(buf), FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned, &ol, nullptr))
            break;

        // 500ms タイムアウトで待機（定期的にキャンセルチェック）
        DWORD wait = WaitForSingleObject(ol.hEvent, 500);
        if (g_watchStop.load()) break;

        if (wait == WAIT_OBJECT_0)
        {
            GetOverlappedResult(hDir, &ol, &bytesReturned, FALSE);
            if (bytesReturned > 0 && g_app.wnd.hwndMain)
                PostMessageW(g_app.wnd.hwndMain, WM_FOLDER_CHANGED, 0, 0);

            // デバウンス: 連続変更を 300ms にまとめる
            Sleep(300);
        }
    }

    CloseHandle(ol.hEvent);
    CloseHandle(hDir);
    g_watchDir = INVALID_HANDLE_VALUE;
}

void FsWatcherStart(const std::wstring& folderPath)
{
    FsWatcherStop();
    if (folderPath.empty()) return;

    g_watchStop.store(false);
    g_watchThread = std::thread(WatcherThread, folderPath);
}

void FsWatcherStop()
{
    g_watchStop.store(true);
    if (g_watchDir != INVALID_HANDLE_VALUE)
        CancelIoEx(g_watchDir, nullptr); // 待機中のI/Oをキャンセル
    if (g_watchThread.joinable())
        g_watchThread.join();
}
