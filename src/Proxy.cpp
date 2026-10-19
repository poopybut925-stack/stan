#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <string>

// Native Geometry Dash 2.2081 loader experiment (Win64).
// No Geode, no third-party injectors or game offsets.
// This revision OBSERVES Windows input; it does not buffer/modify GD clicks.

namespace {
using GetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);
using SetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_VIBRATION*);
using GetCapabilitiesFn = DWORD (WINAPI*)(DWORD, DWORD, XINPUT_CAPABILITIES*);

HMODULE g_real = nullptr;
GetStateFn g_getState = nullptr;
SetStateFn g_setState = nullptr;
GetCapabilitiesFn g_getCapabilities = nullptr;
INIT_ONCE g_loadOnce = INIT_ONCE_STATIC_INIT;
std::atomic<bool> g_capture{false};
std::atomic<HWND> g_window{nullptr};
WNDPROC g_oldWndProc = nullptr;
SRWLOCK g_logLock = SRWLOCK_INIT;

void logLine(const char* line) {
    AcquireSRWLockExclusive(&g_logLock);
    wchar_t path[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (n && n < MAX_PATH) {
        wchar_t* slash = wcsrchr(path, L'\\');
        if (slash) wcscpy_s(slash + 1, MAX_PATH - static_cast<size_t>(slash + 1 - path), L"gd_native_safeclick.log");
        FILE* file = nullptr;
        if (_wfopen_s(&file, path, L"a") == 0 && file) {
            SYSTEMTIME st = {};
            GetLocalTime(&st);
            std::fprintf(file, "%02u:%02u:%02u.%03u %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, line);
            std::fclose(file);
        }
    }
    ReleaseSRWLockExclusive(&g_logLock);
}

BOOL CALLBACK loadOriginal(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t dir[MAX_PATH] = {};
    UINT n = GetSystemDirectoryW(dir, MAX_PATH);
    if (!n || n > MAX_PATH - 18) return TRUE;
    wcscat_s(dir, L"\\XINPUT9_1_0.dll");
    g_real = LoadLibraryW(dir);
    if (g_real) {
        g_getState = reinterpret_cast<GetStateFn>(GetProcAddress(g_real, "XInputGetState"));
        g_setState = reinterpret_cast<SetStateFn>(GetProcAddress(g_real, "XInputSetState"));
        g_getCapabilities = reinterpret_cast<GetCapabilitiesFn>(GetProcAddress(g_real, "XInputGetCapabilities"));
    }
    return TRUE;
}
void ensureRealLoaded() { InitOnceExecuteOnce(&g_loadOnce, loadOriginal, nullptr, nullptr); }

LRESULT CALLBACK interceptWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // F6 toggles instrumentation; never injects gameplay inputs.
    if (msg == WM_KEYDOWN && wParam == VK_F6 && !(lParam & (1LL << 30))) {
        const bool next = !g_capture.load();
        g_capture.store(next);
        logLine(next ? "F6: input observation ON (no gameplay manipulation)" : "F6: input observation OFF");
    }
    if (g_capture.load()) {
        if (msg == WM_LBUTTONDOWN) logLine("observed: mouse left DOWN");
        if (msg == WM_LBUTTONUP) logLine("observed: mouse left UP");
        if (msg == WM_KEYDOWN && !(lParam & (1LL << 30))) {
            if (wParam == VK_SPACE) logLine("observed: SPACE DOWN");
            if (wParam == VK_UP) logLine("observed: UP DOWN");
        }
        if (msg == WM_KEYUP) {
            if (wParam == VK_SPACE) logLine("observed: SPACE UP");
            if (wParam == VK_UP) logLine("observed: UP UP");
        }
    }
    return CallWindowProcW(g_oldWndProc, hwnd, msg, wParam, lParam);
}

struct WindowSearch { DWORD pid; HWND found; };
BOOL CALLBACK chooseWindow(HWND window, LPARAM data) {
    auto* search = reinterpret_cast<WindowSearch*>(data);
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid != search->pid || !IsWindowVisible(window) || GetWindow(window, GW_OWNER)) return TRUE;
    search->found = window;
    return FALSE;
}

DWORD WINAPI startup(LPVOID) {
    ensureRealLoaded();
    logLine("Standalone SafeClick v0.1 started; loader-only, predictor NOT connected");
    if (!g_real) logLine("WARNING: failed to load original Windows XINPUT9_1_0.dll");
    for (int i = 0; i < 300; ++i) {
        WindowSearch search{GetCurrentProcessId(), nullptr};
        EnumWindows(chooseWindow, reinterpret_cast<LPARAM>(&search));
        if (search.found) {
            SetLastError(ERROR_SUCCESS);
            auto old = SetWindowLongPtrW(search.found, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(interceptWndProc));
            if (old || GetLastError() == ERROR_SUCCESS) {
                g_oldWndProc = reinterpret_cast<WNDPROC>(old);
                g_window.store(search.found);
                logLine("Window input observer attached; press F6 to toggle logs");
                return 0;
            }
            logLine("Window detected, but WndProc observer could not attach");
            return 1;
        }
        Sleep(100);
    }
    logLine("No game window found within 30 seconds");
    return 1;
}
}

extern "C" DWORD WINAPI XInputGetState(DWORD player, XINPUT_STATE* state) {
    ensureRealLoaded();
    return g_getState ? g_getState(player, state) : ERROR_DEVICE_NOT_CONNECTED;
}
extern "C" DWORD WINAPI XInputSetState(DWORD player, XINPUT_VIBRATION* vibration) {
    ensureRealLoaded();
    return g_setState ? g_setState(player, vibration) : ERROR_DEVICE_NOT_CONNECTED;
}
extern "C" DWORD WINAPI XInputGetCapabilities(DWORD player, DWORD flags, XINPUT_CAPABILITIES* caps) {
    ensureRealLoaded();
    return g_getCapabilities ? g_getCapabilities(player, flags, caps) : ERROR_DEVICE_NOT_CONNECTED;
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        HANDLE worker = CreateThread(nullptr, 0, startup, nullptr, 0, nullptr);
        if (worker) CloseHandle(worker);
    }
    return TRUE;
}
