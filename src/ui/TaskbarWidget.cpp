#include "ui/TaskbarWidget.h"
#include "ui/TrayIcon.h"
#include "ui/DashboardWindow.h"
#include "ui/SettingsWindow.h"

TaskbarWidget::TaskbarWidget(SettingsManager& settingsMgr, SpeedCalculator& calculator, NetworkMonitor& monitor)
    : m_settingsMgr(settingsMgr), m_calculator(calculator), m_monitor(monitor) {
    m_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
}

TaskbarWidget::~TaskbarWidget() {
    if (m_hFont) {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
    if (m_hWnd && IsWindow(m_hWnd)) {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
}

bool TaskbarWidget::Create() {
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WIDGET_CLASS_NAME;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.hbrBackground = NULL; // We do our own double-buffered painting

    RegisterClassExW(&wc);

    // Initial DPI
    HDC hdc = GetDC(NULL);
    m_currentDpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
    if (m_currentDpi == 0) m_currentDpi = 96;

    UpdateDimensions();
    UpdateThemeColors();

    DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST;
    DWORD style = WS_POPUP | WS_CLIPCHILDREN;

    m_hWnd = CreateWindowExW(
        exStyle,
        WIDGET_CLASS_NAME,
        L"NetSpeed Widget",
        style,
        100, 100, m_width, m_height,
        NULL, NULL, hInstance, this
    );

    if (!m_hWnd) return false;

    // Set window opacity if layered
    AppSettings s = m_settingsMgr.GetSettings();
    if (s.opacityPercent < 100) {
        SetWindowLongPtrW(m_hWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
        BYTE alpha = static_cast<BYTE>((s.opacityPercent * 255) / 100);
        SetLayeredWindowAttributes(m_hWnd, 0, alpha, LWA_ALPHA);
    }

    Reposition();
    return true;
}

void TaskbarWidget::Show() {
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
        UpdateWindow(m_hWnd);
    }
}

void TaskbarWidget::Hide() {
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_HIDE);
    }
}

void TaskbarWidget::UpdateDimensions() {
    AppSettings s = m_settingsMgr.GetSettings();
    float scale = static_cast<float>(m_currentDpi) / 96.0f;

    if (s.displayLayout == DisplayLayout::SingleLine) {
        m_width = static_cast<int>(168.0f * scale);
        m_height = static_cast<int>(32.0f * scale);
    } else {
        m_width = static_cast<int>(115.0f * scale);
        m_height = static_cast<int>(38.0f * scale);
    }

    if (m_hFont) {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }

    int fontHeight = -MulDiv(s.fontSizePt, m_currentDpi, 72);
    m_hFont = CreateFontW(
        fontHeight, 0, 0, 0,
        FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        s.fontFamily.c_str()
    );
}

void TaskbarWidget::UpdateThemeColors() {
    AppSettings s = m_settingsMgr.GetSettings();
    switch (s.colorTheme) {
    case ColorTheme::Dark:
        m_curBgColor = RGB(22, 22, 26);
        m_curTextColor = RGB(235, 235, 240);
        m_curDlColor = RGB(0, 210, 255);    // Neon Cyan
        m_curUlColor = RGB(255, 160, 20);   // Vibrant Orange
        break;
    case ColorTheme::Light:
        m_curBgColor = RGB(242, 244, 248);
        m_curTextColor = RGB(30, 30, 35);
        m_curDlColor = RGB(0, 120, 215);    // Deep Blue
        m_curUlColor = RGB(216, 80, 0);     // Burnt Orange
        break;
    case ColorTheme::Custom:
        m_curBgColor = s.backgroundColor;
        m_curTextColor = s.textColor;
        m_curDlColor = s.downloadColor;
        m_curUlColor = s.uploadColor;
        break;
    }
}

bool TaskbarWidget::IsTaskbarAutoHide() const {
    APPBARDATA abd = { sizeof(APPBARDATA) };
    UINT state = static_cast<UINT>(SHAppBarMessage(ABM_GETSTATE, &abd));
    return (state & ABS_AUTOHIDE) != 0;
}

bool TaskbarWidget::IsFullscreenAppActive() const {
    HWND fgWnd = GetForegroundWindow();
    if (!fgWnd || fgWnd == m_hWnd || fgWnd == GetDesktopWindow()) return false;

    // Check if foreground window is desktop or taskbar
    wchar_t clsName[128];
    GetClassNameW(fgWnd, clsName, 128);
    if (wcscmp(clsName, L"Shell_TrayWnd") == 0 ||
        wcscmp(clsName, L"Progman") == 0 ||
        wcscmp(clsName, L"WorkerW") == 0) {
        return false;
    }

    RECT rcWnd;
    if (GetWindowRect(fgWnd, &rcWnd)) {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        if (rcWnd.left <= 0 && rcWnd.top <= 0 && rcWnd.right >= screenW && rcWnd.bottom >= screenH) {
            return true;
        }
    }
    return false;
}

void TaskbarWidget::Reposition() {
    if (!m_hWnd) return;

    AppSettings s = m_settingsMgr.GetSettings();

    if (IsFullscreenAppActive()) {
        Hide();
        return;
    }

    if (s.windowMode == WindowMode::Floating) {
        // Floating mode: position at user-saved coordinates or default center-bottom
        int x = s.floatingX;
        int y = s.floatingY;
        if (x < 0 || y < 0) {
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            x = screenW - m_width - 20;
            y = screenH - m_height - 60;
        }
        SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, m_width, m_height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        return;
    }

    // Taskbar Docked mode: find Shell_TrayWnd
    HWND hTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
    if (!hTrayWnd || !IsWindowVisible(hTrayWnd)) {
        Hide();
        return;
    }

    RECT rcTray;
    GetWindowRect(hTrayWnd, &rcTray);

    // If auto-hide taskbar is hidden offscreen, hide the widget
    if (IsTaskbarAutoHide()) {
        RECT rcScreen;
        rcScreen.left = 0; rcScreen.top = 0;
        rcScreen.right = GetSystemMetrics(SM_CXSCREEN);
        rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);

        // Check if taskbar is mostly off-screen
        if (rcTray.top >= rcScreen.bottom - 4 || rcTray.bottom <= 4 ||
            rcTray.left >= rcScreen.right - 4 || rcTray.right <= 4) {
            Hide();
            return;
        }
    }

    HWND hNotify = FindWindowExW(hTrayWnd, NULL, L"TrayNotifyWnd", NULL);
    RECT rcNotify;
    if (hNotify && IsWindowVisible(hNotify)) {
        GetWindowRect(hNotify, &rcNotify);
    } else {
        // Fallback: use right side of taskbar
        rcNotify.left = rcTray.right - 100;
        rcNotify.top = rcTray.top;
        rcNotify.right = rcTray.right;
        rcNotify.bottom = rcTray.bottom;
    }

    int posX = 0;
    int posY = 0;

    int taskbarH = rcTray.bottom - rcTray.top;
    int taskbarW = rcTray.right - rcTray.left;

    if (taskbarW >= taskbarH) {
        // Horizontal taskbar (bottom or top)
        // Position directly to the left of the notification area / system tray
        posX = rcNotify.left - m_width - 8;
        posY = rcTray.top + (taskbarH - m_height) / 2;
    } else {
        // Vertical taskbar (left or right)
        posX = rcTray.left + (taskbarW - m_width) / 2;
        posY = rcNotify.top - m_height - 8;
    }

    SetWindowPos(m_hWnd, HWND_TOPMOST, posX, posY, m_width, m_height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void TaskbarWidget::UpdateDisplay() {
    if (m_hWnd) {
        Reposition();
        InvalidateRect(m_hWnd, NULL, FALSE);
        UpdateWindow(m_hWnd);
    }
}

void TaskbarWidget::OnPaint(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // Double buffering
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

    // Fill background with subtle rounded border feel
    HBRUSH bgBrush = CreateSolidBrush(m_curBgColor);
    FillRect(memDC, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    // Draw subtle border
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(45, 45, 55));
    HPEN oldPen = static_cast<HPEN>(SelectObject(memDC, borderPen));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(memDC, GetStockObject(NULL_BRUSH)));
    RoundRect(memDC, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, 6, 6);
    SelectObject(memDC, oldPen);
    SelectObject(memDC, oldBrush);
    DeleteObject(borderPen);

    // Font setup
    HFONT oldFont = static_cast<HFONT>(SelectObject(memDC, m_hFont));
    SetBkMode(memDC, TRANSPARENT);

    AppSettings s = m_settingsMgr.GetSettings();
    double dlSpeed = m_calculator.GetCurrentDownloadBps();
    double ulSpeed = m_calculator.GetCurrentUploadBps();

    std::wstring dlStr = SpeedCalculator::FormatSpeed(dlSpeed, s.speedUnit, s.decimalPlaces, true);
    std::wstring ulStr = SpeedCalculator::FormatSpeed(ulSpeed, s.speedUnit, s.decimalPlaces, true);

    if (s.displayLayout == DisplayLayout::SingleLine) {
        // Single-line layout: "↓ 12.4 Mbps  ↑ 2.1 Mbps"
        int middleX = width / 2;

        RECT rDl = { 6, 0, middleX - 2, height };
        RECT rUl = { middleX + 2, 0, width - 6, height };

        std::wstring dlText = (s.showArrows ? L"↓ " : L"D: ") + dlStr;
        std::wstring ulText = (s.showArrows ? L"↑ " : L"U: ") + ulStr;

        SetTextColor(memDC, m_curDlColor);
        DrawTextW(memDC, dlText.c_str(), -1, &rDl, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SetTextColor(memDC, m_curUlColor);
        DrawTextW(memDC, ulText.c_str(), -1, &rUl, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    } else {
        // Two-line layout:
        // Line 1: "↓ 12.4 Mbps"
        // Line 2: "↑ 2.1 Mbps"
        int halfH = height / 2;

        RECT rDl = { 8, 1, width - 6, halfH };
        RECT rUl = { 8, halfH - 1, width - 6, height };

        std::wstring dlText = (s.showArrows ? L"↓ " : L"D: ") + dlStr;
        std::wstring ulText = (s.showArrows ? L"↑ " : L"U: ") + ulStr;

        SetTextColor(memDC, m_curDlColor);
        DrawTextW(memDC, dlText.c_str(), -1, &rDl, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SetTextColor(memDC, m_curUlColor);
        DrawTextW(memDC, ulText.c_str(), -1, &rUl, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    SelectObject(memDC, oldFont);

    // Blit to screen
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    EndPaint(hWnd, &ps);
}

LRESULT CALLBACK TaskbarWidget::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TaskbarWidget* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<TaskbarWidget*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<TaskbarWidget*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT TaskbarWidget::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == m_wmTaskbarCreated && m_wmTaskbarCreated != 0) {
        // Explorer restarted! Snap right back
        Reposition();
        return 0;
    }

    switch (msg) {
    case WM_PAINT:
        OnPaint(hWnd);
        return 0;

    case WM_ERASEBKGND:
        return 1; // Prevent flicker

    case WM_LBUTTONDOWN: {
        AppSettings s = m_settingsMgr.GetSettings();
        if (s.windowMode == WindowMode::Floating) {
            m_isDragging = true;
            GetCursorPos(&m_dragStartPos);
            SetCapture(hWnd);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (m_isDragging) {
            POINT pt;
            GetCursorPos(&pt);
            RECT rc;
            GetWindowRect(hWnd, &rc);
            int newX = rc.left + (pt.x - m_dragStartPos.x);
            int newY = rc.top + (pt.y - m_dragStartPos.y);
            m_dragStartPos = pt;
            SetWindowPos(hWnd, HWND_TOPMOST, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (m_isDragging) {
            m_isDragging = false;
            ReleaseCapture();

            // Save new floating coordinates
            RECT rc;
            GetWindowRect(hWnd, &rc);
            AppSettings s = m_settingsMgr.GetSettings();
            s.floatingX = rc.left;
            s.floatingY = rc.top;
            m_settingsMgr.UpdateSettings(s);
        }
        return 0;
    }

    case WM_LBUTTONDBLCLK:
        // Open Dashboard on double click
        if (m_pDashboard) {
            m_pDashboard->Show();
        }
        return 0;

    case WM_RBUTTONUP:
        // Show Tray Context Menu on right click
        if (m_pTrayIcon) {
            m_pTrayIcon->ShowContextMenu();
        }
        return 0;

    case WM_DPICHANGED: {
        m_currentDpi = HIWORD(wParam);
        UpdateDimensions();
        RECT* prcNew = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hWnd, NULL, prcNew->left, prcNew->top, m_width, m_height, SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }

    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
        UpdateDimensions();
        UpdateThemeColors();
        Reposition();
        return 0;

    case WM_POWERBROADCAST:
        if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND) {
            // Reset network counter baseline on wake from sleep
            m_calculator.Reset();
            Reposition();
        }
        return TRUE;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
