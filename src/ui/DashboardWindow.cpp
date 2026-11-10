#include "ui/DashboardWindow.h"
#include "ui/SettingsWindow.h"

constexpr UINT IDC_BTN_RESET = 3001;
constexpr UINT IDC_BTN_SETTINGS = 3002;
constexpr UINT IDC_BTN_CLOSE = 3003;

DashboardWindow::DashboardWindow(SettingsManager& settingsMgr, SpeedCalculator& calculator, NetworkMonitor& monitor)
    : m_settingsMgr(settingsMgr), m_calculator(calculator), m_monitor(monitor) {
}

DashboardWindow::~DashboardWindow() {
    if (m_hTitleFont) DeleteObject(m_hTitleFont);
    if (m_hNormalFont) DeleteObject(m_hNormalFont);
    if (m_hBoldFont) DeleteObject(m_hBoldFont);
    if (m_hValueFont) DeleteObject(m_hValueFont);
    if (m_hWnd && IsWindow(m_hWnd)) {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
}

bool DashboardWindow::Create() {
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = DASHBOARD_CLASS_NAME;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = NULL;

    RegisterClassExW(&wc);

    HDC hdc = GetDC(NULL);
    m_currentDpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
    if (m_currentDpi == 0) m_currentDpi = 96;

    float scale = static_cast<float>(m_currentDpi) / 96.0f;
    int w = static_cast<int>(m_width * scale);
    int h = static_cast<int>(m_height * scale);

    // Center on screen
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - w) / 2;
    int y = (screenH - h) / 2;

    m_hTitleFont = CreateFontW(-MulDiv(14, m_currentDpi, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hNormalFont = CreateFontW(-MulDiv(9, m_currentDpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hBoldFont = CreateFontW(-MulDiv(10, m_currentDpi, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hValueFont = CreateFontW(-MulDiv(12, m_currentDpi, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        DASHBOARD_CLASS_NAME,
        L"NetSpeed - Real-Time Network Dashboard",
        style,
        x, y, w, h,
        NULL, NULL, hInstance, this
    );

    if (!m_hWnd) return false;

    // Create action buttons at bottom
    int btnW = static_cast<int>(130 * scale);
    int btnH = static_cast<int>(30 * scale);
    int btnY = h - static_cast<int>(75 * scale);

    m_hBtnReset = CreateWindowExW(0, L"BUTTON", L"Reset Session", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        static_cast<int>(25 * scale), btnY, btnW, btnH, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_RESET), hInstance, NULL);

    m_hBtnSettings = CreateWindowExW(0, L"BUTTON", L"Settings...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        static_cast<int>(170 * scale), btnY, btnW, btnH, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_SETTINGS), hInstance, NULL);

    m_hBtnClose = CreateWindowExW(0, L"BUTTON", L"Close to Tray", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        w - btnW - static_cast<int>(40 * scale), btnY, btnW, btnH, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_CLOSE), hInstance, NULL);

    SendMessageW(m_hBtnReset, WM_SETFONT, reinterpret_cast<WPARAM>(m_hNormalFont), TRUE);
    SendMessageW(m_hBtnSettings, WM_SETFONT, reinterpret_cast<WPARAM>(m_hNormalFont), TRUE);
    SendMessageW(m_hBtnClose, WM_SETFONT, reinterpret_cast<WPARAM>(m_hNormalFont), TRUE);

    return true;
}

void DashboardWindow::Show() {
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_SHOW);
        SetForegroundWindow(m_hWnd);
        UpdateWindow(m_hWnd);
    }
}

void DashboardWindow::Hide() {
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_HIDE);
    }
}

void DashboardWindow::Toggle() {
    if (!m_hWnd) return;
    if (IsWindowVisible(m_hWnd)) {
        Hide();
    } else {
        Show();
    }
}

void DashboardWindow::UpdateData() {
    if (m_hWnd && IsWindowVisible(m_hWnd)) {
        InvalidateRect(m_hWnd, NULL, FALSE);
        UpdateWindow(m_hWnd);
    }
}

void DashboardWindow::DrawStatCard(HDC hdc, const RECT& cardRect, const std::wstring& title,
                                   const std::wstring& line1, COLORREF color1,
                                   const std::wstring& line2, COLORREF color2) {
    // Card background
    HBRUSH bgBrush = CreateSolidBrush(RGB(28, 30, 38));
    FillRect(hdc, &cardRect, bgBrush);
    DeleteObject(bgBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(45, 48, 60));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, borderPen));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    RoundRect(hdc, cardRect.left, cardRect.top, cardRect.right, cardRect.bottom, 8, 8);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);

    SetBkMode(hdc, TRANSPARENT);

    // Title
    RECT rTitle = { cardRect.left + 10, cardRect.top + 8, cardRect.right - 10, cardRect.top + 26 };
    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, m_hBoldFont));
    SetTextColor(hdc, RGB(160, 165, 180));
    DrawTextW(hdc, title.c_str(), -1, &rTitle, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    // Value 1
    RECT rVal1 = { cardRect.left + 10, cardRect.top + 30, cardRect.right - 10, cardRect.top + 50 };
    SelectObject(hdc, m_hValueFont);
    SetTextColor(hdc, color1);
    DrawTextW(hdc, line1.c_str(), -1, &rVal1, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    // Value 2
    RECT rVal2 = { cardRect.left + 10, cardRect.top + 52, cardRect.right - 10, cardRect.top + 72 };
    SetTextColor(hdc, color2);
    DrawTextW(hdc, line2.c_str(), -1, &rVal2, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, oldFont);
}

void DashboardWindow::DrawGraph(HDC hdc, const RECT& graphRect) {
    // Fill graph canvas
    HBRUSH bgBrush = CreateSolidBrush(RGB(14, 16, 22));
    FillRect(hdc, &graphRect, bgBrush);
    DeleteObject(bgBrush);

    // Graph border
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(40, 44, 56));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, borderPen));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    Rectangle(hdc, graphRect.left, graphRect.top, graphRect.right, graphRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);

    // Retrieve history samples (last 60 seconds)
    auto history = m_calculator.GetHistory(60);
    AppSettings s = m_settingsMgr.GetSettings();

    // Determine max speed for Y-axis scaling
    double maxSpeed = 1000000.0 / 8.0; // Minimum 1 Mbps floor
    for (const auto& smp : history) {
        if (smp.downloadSpeedBps > maxSpeed) maxSpeed = smp.downloadSpeedBps;
        if (smp.uploadSpeedBps > maxSpeed) maxSpeed = smp.uploadSpeedBps;
    }
    maxSpeed *= 1.15; // 15% headroom

    int plotLeft = graphRect.left + 65;
    int plotRight = graphRect.right - 15;
    int plotTop = graphRect.top + 25;
    int plotBottom = graphRect.bottom - 25;

    int plotW = plotRight - plotLeft;
    int plotH = plotBottom - plotTop;
    if (plotW <= 0 || plotH <= 0) return;

    // Draw horizontal grid lines & labels
    HPEN gridPen = CreatePen(PS_DOT, 1, RGB(35, 38, 50));
    oldPen = static_cast<HPEN>(SelectObject(hdc, gridPen));
    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, m_hNormalFont));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(130, 135, 150));

    int gridDivs = 4;
    for (int i = 0; i <= gridDivs; ++i) {
        int y = plotBottom - (i * plotH) / gridDivs;
        MoveToEx(hdc, plotLeft, y, NULL);
        LineTo(hdc, plotRight, y);

        double gridSpeed = (maxSpeed * i) / gridDivs;
        std::wstring lbl = SpeedCalculator::FormatSpeed(gridSpeed, s.speedUnit, 1, true);
        RECT rLbl = { graphRect.left + 5, y - 8, plotLeft - 5, y + 8 };
        DrawTextW(hdc, lbl.c_str(), -1, &rLbl, DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);

    // Draw timeline markers at bottom
    RECT rT1 = { plotLeft - 10, plotBottom + 4, plotLeft + 30, plotBottom + 20 };
    RECT rT2 = { plotLeft + plotW / 2 - 20, plotBottom + 4, plotLeft + plotW / 2 + 20, plotBottom + 20 };
    RECT rT3 = { plotRight - 35, plotBottom + 4, plotRight + 5, plotBottom + 20 };
    DrawTextW(hdc, L"-60s", -1, &rT1, DT_LEFT | DT_SINGLELINE);
    DrawTextW(hdc, L"-30s", -1, &rT2, DT_CENTER | DT_SINGLELINE);
    DrawTextW(hdc, L"Now", -1, &rT3, DT_RIGHT | DT_SINGLELINE);

    // Draw Legend
    RECT rLeg1 = { plotLeft, graphRect.top + 5, plotLeft + 120, graphRect.top + 22 };
    RECT rLeg2 = { plotLeft + 130, graphRect.top + 5, plotLeft + 250, graphRect.top + 22 };
    SetTextColor(hdc, RGB(0, 210, 255));
    DrawTextW(hdc, L"■ Download (↓)", -1, &rLeg1, DT_LEFT | DT_SINGLELINE);
    SetTextColor(hdc, RGB(255, 153, 0));
    DrawTextW(hdc, L"■ Upload (↑)", -1, &rLeg2, DT_LEFT | DT_SINGLELINE);

    if (history.size() < 2) {
        SelectObject(hdc, oldFont);
        return;
    }

    // Plot Download Line (Cyan)
    HPEN dlPen = CreatePen(PS_SOLID, 2, RGB(0, 210, 255));
    oldPen = static_cast<HPEN>(SelectObject(hdc, dlPen));

    size_t n = history.size();
    for (size_t i = 0; i < n; ++i) {
        int x = plotLeft + static_cast<int>((static_cast<double>(i) / (n - 1)) * plotW);
        double ratio = history[i].downloadSpeedBps / maxSpeed;
        if (ratio > 1.0) ratio = 1.0;
        int y = plotBottom - static_cast<int>(ratio * plotH);

        if (i == 0) {
            MoveToEx(hdc, x, y, NULL);
        } else {
            LineTo(hdc, x, y);
        }
    }
    SelectObject(hdc, oldPen);
    DeleteObject(dlPen);

    // Plot Upload Line (Orange)
    HPEN ulPen = CreatePen(PS_SOLID, 2, RGB(255, 153, 0));
    oldPen = static_cast<HPEN>(SelectObject(hdc, ulPen));

    for (size_t i = 0; i < n; ++i) {
        int x = plotLeft + static_cast<int>((static_cast<double>(i) / (n - 1)) * plotW);
        double ratio = history[i].uploadSpeedBps / maxSpeed;
        if (ratio > 1.0) ratio = 1.0;
        int y = plotBottom - static_cast<int>(ratio * plotH);

        if (i == 0) {
            MoveToEx(hdc, x, y, NULL);
        } else {
            LineTo(hdc, x, y);
        }
    }
    SelectObject(hdc, oldPen);
    DeleteObject(ulPen);

    SelectObject(hdc, oldFont);
}

void DashboardWindow::OnPaint(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // Double buffer
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

    // Dark background
    HBRUSH bgBrush = CreateSolidBrush(RGB(18, 20, 26));
    FillRect(memDC, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    SetBkMode(memDC, TRANSPARENT);

    // Header Title
    RECT rTitle = { 25, 18, width - 25, 45 };
    HFONT oldFont = static_cast<HFONT>(SelectObject(memDC, m_hTitleFont));
    SetTextColor(memDC, RGB(245, 245, 250));
    DrawTextW(memDC, L"NetSpeed — Real-Time Network Monitor", -1, &rTitle, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    // Active Adapter & Status
    std::wstring adapterName = m_monitor.GetActiveAdapterName();
    bool isConnected = m_monitor.IsConnected();
    std::wstring statusText = L"Adapter: " + (adapterName.empty() ? L"Detecting..." : adapterName);
    statusText += isConnected ? L"  •  Connected" : L"  •  Disconnected";

    RECT rSub = { 25, 45, width - 25, 68 };
    SelectObject(memDC, m_hNormalFont);
    SetTextColor(memDC, isConnected ? RGB(70, 210, 130) : RGB(240, 70, 70));
    DrawTextW(memDC, statusText.c_str(), -1, &rSub, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    // Stat Cards (4 cards in a 2x2 grid)
    AppSettings s = m_settingsMgr.GetSettings();
    double curDl = m_calculator.GetCurrentDownloadBps();
    double curUl = m_calculator.GetCurrentUploadBps();
    double peakDl = m_calculator.GetPeakDownloadBps();
    double peakUl = m_calculator.GetPeakUploadBps();
    uint64_t totalDl = m_calculator.GetTotalDownloadedBytes();
    uint64_t totalUl = m_calculator.GetTotalUploadedBytes();

    int cardW = (width - 65) / 2;
    int cardH = 80;
    int cardY1 = 75;
    int cardY2 = cardY1 + cardH + 10;

    RECT rc1 = { 25, cardY1, 25 + cardW, cardY1 + cardH };
    RECT rc2 = { 35 + cardW, cardY1, width - 25, cardY1 + cardH };
    RECT rc3 = { 25, cardY2, 25 + cardW, cardY2 + cardH };
    RECT rc4 = { 35 + cardW, cardY2, width - 25, cardY2 + cardH };

    DrawStatCard(memDC, rc1, L"CURRENT SPEED",
        L"↓ " + SpeedCalculator::FormatSpeed(curDl, s.speedUnit, s.decimalPlaces), RGB(0, 210, 255),
        L"↑ " + SpeedCalculator::FormatSpeed(curUl, s.speedUnit, s.decimalPlaces), RGB(255, 153, 0));

    DrawStatCard(memDC, rc2, L"PEAK SPEED (SESSION)",
        L"Peak ↓ " + SpeedCalculator::FormatSpeed(peakDl, s.speedUnit, s.decimalPlaces), RGB(0, 210, 255),
        L"Peak ↑ " + SpeedCalculator::FormatSpeed(peakUl, s.speedUnit, s.decimalPlaces), RGB(255, 153, 0));

    DrawStatCard(memDC, rc3, L"DATA USAGE (SESSION)",
        L"Downloaded: " + SpeedCalculator::FormatBytes(totalDl), RGB(180, 220, 255),
        L"Uploaded: " + SpeedCalculator::FormatBytes(totalUl), RGB(255, 210, 160));

    DrawStatCard(memDC, rc4, L"STATUS & REFRESH",
        L"Update Interval: " + std::to_wstring(s.updateIntervalMs) + L" ms", RGB(200, 205, 220),
        L"Mode: " + std::wstring(s.windowMode == WindowMode::TaskbarDocked ? L"Taskbar Docked" : L"Desktop Floating"), RGB(200, 205, 220));

    // Rolling 60-Second Speed Graph
    int graphY = cardY2 + cardH + 15;
    int graphH = height - graphY - 60;
    if (graphH > 60) {
        RECT rGraph = { 25, graphY, width - 25, graphY + graphH };
        DrawGraph(memDC, rGraph);
    }

    SelectObject(memDC, oldFont);

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    EndPaint(hWnd, &ps);
}

LRESULT CALLBACK DashboardWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DashboardWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<DashboardWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<DashboardWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT DashboardWindow::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        OnPaint(hWnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_COMMAND: {
        UINT id = LOWORD(wParam);
        if (id == IDC_BTN_RESET) {
            m_calculator.ResetSessionStats();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (id == IDC_BTN_SETTINGS) {
            if (m_pSettings) {
                m_pSettings->Show();
            }
        } else if (id == IDC_BTN_CLOSE) {
            Hide();
        }
        return 0;
    }

    case WM_CLOSE:
        // Minimize to tray instead of quitting
        Hide();
        return 0;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
