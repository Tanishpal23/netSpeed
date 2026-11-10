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
    if (m_hSubValueFont) DeleteObject(m_hSubValueFont);
    if (m_hWnd && IsWindow(m_hWnd)) {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
}

void DashboardWindow::CreateFonts() {
    if (m_hTitleFont) DeleteObject(m_hTitleFont);
    if (m_hNormalFont) DeleteObject(m_hNormalFont);
    if (m_hBoldFont) DeleteObject(m_hBoldFont);
    if (m_hValueFont) DeleteObject(m_hValueFont);
    if (m_hSubValueFont) DeleteObject(m_hSubValueFont);

    m_hTitleFont = CreateFontW(-MulDiv(15, m_currentDpi, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hNormalFont = CreateFontW(-MulDiv(9, m_currentDpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hBoldFont = CreateFontW(-MulDiv(9, m_currentDpi, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hValueFont = CreateFontW(-MulDiv(13, m_currentDpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hSubValueFont = CreateFontW(-MulDiv(11, m_currentDpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
}

void DashboardWindow::RepositionControls() {
    if (!m_hWnd) return;

    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);
    int clientW = rcClient.right - rcClient.left;
    int clientH = rcClient.bottom - rcClient.top;

    float scale = static_cast<float>(m_currentDpi) / 96.0f;
    int margin = static_cast<int>(24 * scale);
    int btnW = static_cast<int>(130 * scale);
    int btnH = static_cast<int>(32 * scale);
    int btnY = clientH - btnH - static_cast<int>(16 * scale);

    if (m_hBtnReset && IsWindow(m_hBtnReset)) {
        SetWindowPos(m_hBtnReset, NULL, margin, btnY, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
        SendMessageW(m_hBtnReset, WM_SETFONT, reinterpret_cast<WPARAM>(m_hNormalFont), TRUE);
    }

    if (m_hBtnSettings && IsWindow(m_hBtnSettings)) {
        int gap = static_cast<int>(12 * scale);
        SetWindowPos(m_hBtnSettings, NULL, margin + btnW + gap, btnY, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
        SendMessageW(m_hBtnSettings, WM_SETFONT, reinterpret_cast<WPARAM>(m_hNormalFont), TRUE);
    }

    if (m_hBtnClose && IsWindow(m_hBtnClose)) {
        SetWindowPos(m_hBtnClose, NULL, clientW - margin - btnW, btnY, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
        SendMessageW(m_hBtnClose, WM_SETFONT, reinterpret_cast<WPARAM>(m_hNormalFont), TRUE);
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

    CreateFonts();

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        DASHBOARD_CLASS_NAME,
        L"NetSpeed — Real-Time Network Monitor",
        style,
        x, y, w, h,
        NULL, NULL, hInstance, this
    );

    if (!m_hWnd) return false;

    // Create action buttons at bottom
    m_hBtnReset = CreateWindowExW(0, L"BUTTON", L"Reset Session", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 100, 30, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_RESET), hInstance, NULL);

    m_hBtnSettings = CreateWindowExW(0, L"BUTTON", L"Settings...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 100, 30, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_SETTINGS), hInstance, NULL);

    m_hBtnClose = CreateWindowExW(0, L"BUTTON", L"Close to Tray", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 100, 30, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_CLOSE), hInstance, NULL);

    RepositionControls();
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
    float scale = static_cast<float>(m_currentDpi) / 96.0f;

    // Card background
    HBRUSH bgBrush = CreateSolidBrush(RGB(26, 28, 36));
    FillRect(hdc, &cardRect, bgBrush);
    DeleteObject(bgBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(44, 48, 62));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, borderPen));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    RoundRect(hdc, cardRect.left, cardRect.top, cardRect.right, cardRect.bottom,
        static_cast<int>(10 * scale), static_cast<int>(10 * scale));
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);

    SetBkMode(hdc, TRANSPARENT);

    int padX = static_cast<int>(14 * scale);

    // Title (e.g. "CURRENT SPEED")
    int titleTop = cardRect.top + static_cast<int>(10 * scale);
    int titleH = static_cast<int>(18 * scale);
    RECT rTitle = { cardRect.left + padX, titleTop, cardRect.right - padX, titleTop + titleH };
    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, m_hBoldFont));
    SetTextColor(hdc, RGB(150, 155, 172));
    DrawTextW(hdc, title.c_str(), -1, &rTitle, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    // Line 1 (Download / Primary Stat)
    int line1Top = titleTop + titleH + static_cast<int>(6 * scale);
    int line1H = static_cast<int>(26 * scale);
    RECT rVal1 = { cardRect.left + padX, line1Top, cardRect.right - padX, line1Top + line1H };
    SelectObject(hdc, m_hValueFont);
    SetTextColor(hdc, color1);
    DrawTextW(hdc, line1.c_str(), -1, &rVal1, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    // Line 2 (Upload / Secondary Stat)
    int line2Top = line1Top + line1H + static_cast<int>(4 * scale);
    int line2H = static_cast<int>(24 * scale);
    RECT rVal2 = { cardRect.left + padX, line2Top, cardRect.right - padX, line2Top + line2H };
    SelectObject(hdc, m_hSubValueFont);
    SetTextColor(hdc, color2);
    DrawTextW(hdc, line2.c_str(), -1, &rVal2, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, oldFont);
}

void DashboardWindow::DrawGraph(HDC hdc, const RECT& graphRect) {
    float scale = static_cast<float>(m_currentDpi) / 96.0f;

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

    int plotLeft = graphRect.left + static_cast<int>(90 * scale);
    int plotRight = graphRect.right - static_cast<int>(16 * scale);
    int plotTop = graphRect.top + static_cast<int>(32 * scale);
    int plotBottom = graphRect.bottom - static_cast<int>(26 * scale);

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
        RECT rLbl = { graphRect.left + 6, y - static_cast<int>(9 * scale), plotLeft - 8, y + static_cast<int>(9 * scale) };
        DrawTextW(hdc, lbl.c_str(), -1, &rLbl, DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);

    // Draw timeline markers at bottom
    RECT rT1 = { plotLeft - 10, plotBottom + 4, plotLeft + static_cast<int>(40 * scale), plotBottom + static_cast<int>(22 * scale) };
    RECT rT2 = { plotLeft + plotW / 2 - 25, plotBottom + 4, plotLeft + plotW / 2 + 25, plotBottom + static_cast<int>(22 * scale) };
    RECT rT3 = { plotRight - static_cast<int>(40 * scale), plotBottom + 4, plotRight + 5, plotBottom + static_cast<int>(22 * scale) };
    DrawTextW(hdc, L"-60s", -1, &rT1, DT_LEFT | DT_SINGLELINE);
    DrawTextW(hdc, L"-30s", -1, &rT2, DT_CENTER | DT_SINGLELINE);
    DrawTextW(hdc, L"Now", -1, &rT3, DT_RIGHT | DT_SINGLELINE);

    // Draw Legend
    int legY = graphRect.top + static_cast<int>(8 * scale);
    int legH = static_cast<int>(18 * scale);
    RECT rLeg1 = { plotLeft, legY, plotLeft + static_cast<int>(130 * scale), legY + legH };
    RECT rLeg2 = { plotLeft + static_cast<int>(140 * scale), legY, plotLeft + static_cast<int>(270 * scale), legY + legH };
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

    float scale = static_cast<float>(m_currentDpi) / 96.0f;

    // Double buffer
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

    // Dark background
    HBRUSH bgBrush = CreateSolidBrush(RGB(18, 20, 26));
    FillRect(memDC, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    SetBkMode(memDC, TRANSPARENT);

    int margin = static_cast<int>(24 * scale);

    // Header Title
    int titleY = static_cast<int>(18 * scale);
    int titleH = static_cast<int>(26 * scale);
    RECT rTitle = { margin, titleY, width - margin, titleY + titleH };
    HFONT oldFont = static_cast<HFONT>(SelectObject(memDC, m_hTitleFont));
    SetTextColor(memDC, RGB(245, 245, 250));
    DrawTextW(memDC, L"NetSpeed — Real-Time Network Monitor", -1, &rTitle, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    // Active Adapter & Status
    std::wstring adapterName = m_monitor.GetActiveAdapterName();
    bool isConnected = m_monitor.IsConnected();
    std::wstring statusText = L"Adapter: " + (adapterName.empty() ? L"Detecting..." : adapterName);
    statusText += isConnected ? L"  •  Connected" : L"  •  Disconnected";

    int subY = titleY + titleH + static_cast<int>(4 * scale);
    int subH = static_cast<int>(20 * scale);
    RECT rSub = { margin, subY, width - margin, subY + subH };
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

    int cardGap = static_cast<int>(14 * scale);
    int cardW = (width - 2 * margin - cardGap) / 2;
    int cardH = static_cast<int>(94 * scale);
    int cardY1 = subY + subH + static_cast<int>(10 * scale);
    int cardY2 = cardY1 + cardH + cardGap;

    RECT rc1 = { margin, cardY1, margin + cardW, cardY1 + cardH };
    RECT rc2 = { margin + cardW + cardGap, cardY1, width - margin, cardY1 + cardH };
    RECT rc3 = { margin, cardY2, margin + cardW, cardY2 + cardH };
    RECT rc4 = { margin + cardW + cardGap, cardY2, width - margin, cardY2 + cardH };

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
        L"Update Interval: " + std::to_wstring(s.updateIntervalMs) + L" ms", RGB(205, 210, 225),
        L"Mode: " + std::wstring(s.windowMode == WindowMode::TaskbarDocked ? L"Taskbar Docked" : L"Desktop Floating"), RGB(205, 210, 225));

    // Rolling 60-Second Speed Graph
    int graphY = cardY2 + cardH + static_cast<int>(14 * scale);
    int btnH = static_cast<int>(32 * scale);
    int btnBottomPad = static_cast<int>(16 * scale);
    int graphBottom = height - btnH - btnBottomPad - static_cast<int>(14 * scale);
    int graphH = graphBottom - graphY;

    if (graphH > 50) {
        RECT rGraph = { margin, graphY, width - margin, graphBottom };
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

    case WM_SIZE:
        RepositionControls();
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;

    case WM_DPICHANGED: {
        m_currentDpi = HIWORD(wParam);
        CreateFonts();
        RECT* prcNew = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hWnd, NULL, prcNew->left, prcNew->top,
            prcNew->right - prcNew->left, prcNew->bottom - prcNew->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        RepositionControls();
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }

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
        Hide();
        return 0;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
