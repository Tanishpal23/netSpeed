#include "Common.h"
#include "settings/SettingsManager.h"
#include "network/SpeedCalculator.h"
#include "network/NetworkMonitor.h"
#include "ui/TaskbarWidget.h"
#include "ui/TrayIcon.h"
#include "ui/DashboardWindow.h"
#include "ui/SettingsWindow.h"

#include <atomic>
#include <thread>

// Global State
static std::atomic<bool> g_running{ true };
static HWND g_hMsgWnd = NULL;
static SettingsManager* g_pSettingsMgr = nullptr;
static SpeedCalculator* g_pCalculator = nullptr;
static NetworkMonitor* g_pMonitor = nullptr;
static TaskbarWidget* g_pWidget = nullptr;
static TrayIcon* g_pTrayIcon = nullptr;
static DashboardWindow* g_pDashboard = nullptr;
static SettingsWindow* g_pSettingsWindow = nullptr;

// Background Network Sampling Thread
static void NetworkWorkerThread() {
    while (g_running) {
        AppSettings s = g_pSettingsMgr->GetSettings();

        if (!s.isPaused) {
            // Poll network interface statistics
            NetworkStats stats = g_pMonitor->PollStats(s.selectedAdapterId);

            if (stats.isConnected) {
                g_pCalculator->Update(stats.inOctets, stats.outOctets);
            } else {
                g_pCalculator->Update(0, 0);
            }

            // Notify UI thread
            if (g_hMsgWnd && IsWindow(g_hMsgWnd)) {
                PostMessageW(g_hMsgWnd, WM_APP_UPDATE_SPEED, 0, 0);
            }
        }

        // Sleep for the configured interval
        int sleepMs = s.updateIntervalMs;
        if (sleepMs < 100) sleepMs = 100;

        // Break sleep into small chunks for responsive termination
        for (int i = 0; i < sleepMs && g_running; i += 50) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

// Hidden Message-only Window Procedure
static LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_APP_TRAYICON:
        if (g_pTrayIcon) {
            g_pTrayIcon->HandleTrayMessage(lParam);
        }
        return 0;

    case WM_APP_UPDATE_SPEED: {
        if (g_pWidget) {
            g_pWidget->UpdateDisplay();
        }
        if (g_pDashboard) {
            g_pDashboard->UpdateData();
        }
        if (g_pTrayIcon) {
            AppSettings s = g_pSettingsMgr->GetSettings();
            double dl = g_pCalculator->GetCurrentDownloadBps();
            double ul = g_pCalculator->GetCurrentUploadBps();
            std::wstring dlStr = SpeedCalculator::FormatSpeed(dl, s.speedUnit, s.decimalPlaces, true);
            std::wstring ulStr = SpeedCalculator::FormatSpeed(ul, s.speedUnit, s.decimalPlaces, true);
            g_pTrayIcon->UpdateTooltip(dlStr, ulStr, g_pMonitor->GetActiveAdapterName());
        }
        return 0;
    }

    case WM_COMMAND: {
        UINT id = LOWORD(wParam);
        switch (id) {
        case IDM_TRAY_DASHBOARD:
            if (g_pDashboard) g_pDashboard->Show();
            break;

        case IDM_TRAY_SETTINGS:
            if (g_pSettingsWindow) g_pSettingsWindow->Show();
            break;

        case IDM_TRAY_MODE_DOCK: {
            AppSettings s = g_pSettingsMgr->GetSettings();
            s.windowMode = WindowMode::TaskbarDocked;
            g_pSettingsMgr->UpdateSettings(s);
            if (g_pWidget) {
                g_pWidget->Reposition();
                g_pWidget->UpdateDisplay();
            }
            break;
        }

        case IDM_TRAY_MODE_FLOAT: {
            AppSettings s = g_pSettingsMgr->GetSettings();
            s.windowMode = WindowMode::Floating;
            g_pSettingsMgr->UpdateSettings(s);
            if (g_pWidget) {
                g_pWidget->Reposition();
                g_pWidget->UpdateDisplay();
            }
            break;
        }

        case IDM_TRAY_LAYOUT_SINGLE: {
            AppSettings s = g_pSettingsMgr->GetSettings();
            s.displayLayout = DisplayLayout::SingleLine;
            g_pSettingsMgr->UpdateSettings(s);
            if (g_pWidget) {
                g_pWidget->UpdateDimensions();
                g_pWidget->Reposition();
                g_pWidget->UpdateDisplay();
            }
            break;
        }

        case IDM_TRAY_LAYOUT_DOUBLE: {
            AppSettings s = g_pSettingsMgr->GetSettings();
            s.displayLayout = DisplayLayout::TwoLine;
            g_pSettingsMgr->UpdateSettings(s);
            if (g_pWidget) {
                g_pWidget->UpdateDimensions();
                g_pWidget->Reposition();
                g_pWidget->UpdateDisplay();
            }
            break;
        }

        case IDM_TRAY_PAUSE: {
            AppSettings s = g_pSettingsMgr->GetSettings();
            s.isPaused = !s.isPaused;
            g_pSettingsMgr->UpdateSettings(s);
            if (g_pWidget) g_pWidget->UpdateDisplay();
            break;
        }

        case IDM_TRAY_AUTORUN: {
            bool current = SettingsManager::IsStartWithWindowsEnabled();
            SettingsManager::SetStartWithWindows(!current);
            AppSettings s = g_pSettingsMgr->GetSettings();
            s.startWithWindows = !current;
            g_pSettingsMgr->UpdateSettings(s);
            break;
        }

        case IDM_TRAY_ABOUT: {
            std::wstring aboutMsg = L"NetSpeed — Windows Real-Time Internet Speed Monitor\n"
                                    L"Version: 1.0.0 (x64)\n\n"
                                    L"• Genuine Windows 10 & 11 Taskbar Integration\n"
                                    L"• Native OS Network Interface Counters\n"
                                    L"• Rolling 60-second throughput graph\n"
                                    L"• Ultra-low CPU & RAM consumption\n\n"
                                    L"Open Source & Free Software";
            MessageBoxW(hWnd, aboutMsg.c_str(), L"About NetSpeed", MB_OK | MB_ICONINFORMATION);
            break;
        }

        case IDM_TRAY_EXIT:
            PostQuitMessage(0);
            break;
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Enable modern DPI awareness
static void EnableDpiAwareness() {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        auto pSetDpi = reinterpret_cast<SetProcessDpiAwarenessContextProc>(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
        if (pSetDpi) {
            pSetDpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }
    SetProcessDPIAware();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // Single instance protection
    HANDLE hMutex = CreateMutexW(NULL, TRUE, APP_MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // App already running! Find existing window and exit cleanly
        HWND hExistingWidget = FindWindowW(WIDGET_CLASS_NAME, NULL);
        if (hExistingWidget) {
            SetForegroundWindow(hExistingWidget);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    EnableDpiAwareness();

    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX) };
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // Initialize core components
    SettingsManager settingsMgr;
    SpeedCalculator calculator;
    NetworkMonitor monitor;

    g_pSettingsMgr = &settingsMgr;
    g_pCalculator = &calculator;
    g_pMonitor = &monitor;

    // Create hidden message window for Tray & Thread notifications
    WNDCLASSEXW mc = { sizeof(WNDCLASSEXW) };
    mc.lpfnWndProc = MsgWndProc;
    mc.hInstance = hInstance;
    mc.lpszClassName = L"NetSpeedMessageWindowClass";
    RegisterClassExW(&mc);

    g_hMsgWnd = CreateWindowExW(0, mc.lpszClassName, L"NetSpeedMsgWnd", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    // Create UI components
    TaskbarWidget widget(settingsMgr, calculator, monitor);
    DashboardWindow dashboard(settingsMgr, calculator, monitor);
    SettingsWindow settingsWindow(settingsMgr, monitor);
    TrayIcon trayIcon(g_hMsgWnd, settingsMgr, calculator, monitor);

    g_pWidget = &widget;
    g_pDashboard = &dashboard;
    g_pSettingsWindow = &settingsWindow;
    g_pTrayIcon = &trayIcon;

    // Cross-wire component dependencies
    widget.SetTrayIcon(&trayIcon);
    widget.SetDashboard(&dashboard);
    widget.SetSettingsWindow(&settingsWindow);

    trayIcon.SetWidget(&widget);
    trayIcon.SetDashboard(&dashboard);
    trayIcon.SetSettingsWindow(&settingsWindow);

    dashboard.SetSettingsWindow(&settingsWindow);
    settingsWindow.SetWidget(&widget);

    // Initialize UI
    if (!widget.Create()) {
        MessageBoxW(NULL, L"Failed to initialize NetSpeed taskbar widget.", L"NetSpeed Error", MB_ICONERROR);
        return 1;
    }
    dashboard.Create();
    settingsWindow.Create();
    trayIcon.Create();

    widget.Show();

    // Check if launched with --dashboard or --settings
    std::wstring cmdLine = lpCmdLine ? lpCmdLine : L"";
    if (cmdLine.find(L"--dashboard") != std::wstring::npos) {
        dashboard.Show();
    } else if (cmdLine.find(L"--settings") != std::wstring::npos) {
        settingsWindow.Show();
    }

    // Start background sampling thread
    std::thread worker(NetworkWorkerThread);

    // Win32 Message Loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Graceful Shutdown
    g_running = false;
    if (worker.joinable()) {
        worker.join();
    }

    trayIcon.Remove();

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return 0;
}
