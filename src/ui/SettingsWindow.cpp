#include "ui/SettingsWindow.h"
#include "ui/TaskbarWidget.h"
#include <commdlg.h>

constexpr UINT IDC_COMBO_INTERVAL   = 4001;
constexpr UINT IDC_COMBO_UNIT       = 4002;
constexpr UINT IDC_COMBO_LAYOUT     = 4003;
constexpr UINT IDC_COMBO_DECIMALS   = 4004;
constexpr UINT IDC_COMBO_THEME      = 4005;
constexpr UINT IDC_COMBO_ADAPTER    = 4006;
constexpr UINT IDC_COMBO_MODE       = 4007;
constexpr UINT IDC_CHECK_STARTUP    = 4008;
constexpr UINT IDC_CHECK_ARROWS     = 4009;
constexpr UINT IDC_CHECK_MINTRAY    = 4010;
constexpr UINT IDC_BTN_COLOR_DL     = 4011;
constexpr UINT IDC_BTN_COLOR_UL     = 4012;
constexpr UINT IDC_BTN_SAVE         = 4013;
constexpr UINT IDC_BTN_CANCEL       = 4014;

SettingsWindow::SettingsWindow(SettingsManager& settingsMgr, NetworkMonitor& monitor)
    : m_settingsMgr(settingsMgr), m_monitor(monitor) {
}

SettingsWindow::~SettingsWindow() {
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hBoldFont) DeleteObject(m_hBoldFont);
    if (m_hWnd && IsWindow(m_hWnd)) {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
}

bool SettingsWindow::Create() {
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = SETTINGS_CLASS_NAME;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);

    RegisterClassExW(&wc);

    HDC hdc = GetDC(NULL);
    m_currentDpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
    if (m_currentDpi == 0) m_currentDpi = 96;

    float scale = static_cast<float>(m_currentDpi) / 96.0f;
    int w = static_cast<int>(460 * scale);
    int h = static_cast<int>(560 * scale);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - w) / 2;
    int y = (screenH - h) / 2;

    m_hFont = CreateFontW(-MulDiv(9, m_currentDpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hBoldFont = CreateFontW(-MulDiv(9, m_currentDpi, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    m_hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        SETTINGS_CLASS_NAME,
        L"NetSpeed — Settings",
        style,
        x, y, w, h,
        NULL, NULL, hInstance, this
    );

    if (!m_hWnd) return false;

    // Helper to create controls
    auto makeLabel = [&](const wchar_t* text, int lx, int ly, int lw, int lh) {
        HWND hLbl = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
            static_cast<int>(lx * scale), static_cast<int>(ly * scale),
            static_cast<int>(lw * scale), static_cast<int>(lh * scale),
            m_hWnd, NULL, hInstance, NULL);
        SendMessageW(hLbl, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);
        return hLbl;
    };

    auto makeCombo = [&](int cx, int cy, int cw, int ch, UINT id) {
        HWND hCmb = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            static_cast<int>(cx * scale), static_cast<int>(cy * scale),
            static_cast<int>(cw * scale), static_cast<int>(ch * scale),
            m_hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)), hInstance, NULL);
        SendMessageW(hCmb, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);
        return hCmb;
    };

    int curY = 18;

    // 1. Refresh Interval
    makeLabel(L"Refresh Interval:", 20, curY + 3, 140, 20);
    m_hComboInterval = makeCombo(170, curY, 240, 150, IDC_COMBO_INTERVAL);
    SendMessageW(m_hComboInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"250 ms (Ultra High)"));
    SendMessageW(m_hComboInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"500 ms (Fast)"));
    SendMessageW(m_hComboInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1000 ms (Normal / Recommended)"));
    SendMessageW(m_hComboInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2000 ms (Low CPU)"));
    SendMessageW(m_hComboInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"5000 ms (Eco)"));

    curY += 36;
    // 2. Speed Unit
    makeLabel(L"Speed Unit:", 20, curY + 3, 140, 20);
    m_hComboUnit = makeCombo(170, curY, 240, 150, IDC_COMBO_UNIT);
    SendMessageW(m_hComboUnit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Auto (Kbps, Mbps, Gbps)"));
    SendMessageW(m_hComboUnit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mbps (Megabits / sec)"));
    SendMessageW(m_hComboUnit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"MB/s (Megabytes / sec)"));
    SendMessageW(m_hComboUnit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"KB/s (Kilobytes / sec)"));
    SendMessageW(m_hComboUnit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Kbps (Kilobits / sec)"));

    curY += 36;
    // 3. Display Layout
    makeLabel(L"Widget Layout:", 20, curY + 3, 140, 20);
    m_hComboLayout = makeCombo(170, curY, 240, 100, IDC_COMBO_LAYOUT);
    SendMessageW(m_hComboLayout, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Single-Line (↓ 12.4M  ↑ 2.1M)"));
    SendMessageW(m_hComboLayout, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Two-Line (Stacked Vertical)"));

    curY += 36;
    // 4. Decimal Places
    makeLabel(L"Decimal Precision:", 20, curY + 3, 140, 20);
    m_hComboDecimals = makeCombo(170, curY, 240, 100, IDC_COMBO_DECIMALS);
    SendMessageW(m_hComboDecimals, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"0 Decimals (12 Mbps)"));
    SendMessageW(m_hComboDecimals, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1 Decimal (12.4 Mbps)"));
    SendMessageW(m_hComboDecimals, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2 Decimals (12.45 Mbps)"));

    curY += 36;
    // 5. Window Mode
    makeLabel(L"Display Mode:", 20, curY + 3, 140, 20);
    m_hComboMode = makeCombo(170, curY, 240, 100, IDC_COMBO_MODE);
    SendMessageW(m_hComboMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Taskbar Docked (Windows 10/11)"));
    SendMessageW(m_hComboMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Desktop Floating (Draggable)"));

    curY += 36;
    // 6. Network Adapter
    makeLabel(L"Network Adapter:", 20, curY + 3, 140, 20);
    m_hComboAdapter = makeCombo(170, curY, 240, 150, IDC_COMBO_ADAPTER);

    curY += 36;
    // 7. Color Theme
    makeLabel(L"Color Theme:", 20, curY + 3, 140, 20);
    m_hComboTheme = makeCombo(170, curY, 240, 100, IDC_COMBO_THEME);
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Dark Theme (Default)"));
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Light Theme"));
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Custom Colors"));

    curY += 36;
    // Custom Color Buttons
    makeLabel(L"Custom Colors:", 20, curY + 3, 140, 20);
    m_hBtnColorDl = CreateWindowExW(0, L"BUTTON", L"Download Color...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        static_cast<int>(170 * scale), static_cast<int>(curY * scale),
        static_cast<int>(115 * scale), static_cast<int>(26 * scale),
        m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_COLOR_DL), hInstance, NULL);
    SendMessageW(m_hBtnColorDl, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);

    m_hBtnColorUl = CreateWindowExW(0, L"BUTTON", L"Upload Color...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        static_cast<int>(295 * scale), static_cast<int>(curY * scale),
        static_cast<int>(115 * scale), static_cast<int>(26 * scale),
        m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_COLOR_UL), hInstance, NULL);
    SendMessageW(m_hBtnColorUl, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);

    curY += 38;
    // Checkboxes
    m_hCheckArrows = CreateWindowExW(0, L"BUTTON", L"Show Arrow Indicators (↓ and ↑)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        static_cast<int>(20 * scale), static_cast<int>(curY * scale),
        static_cast<int>(390 * scale), static_cast<int>(22 * scale),
        m_hWnd, reinterpret_cast<HMENU>(IDC_CHECK_ARROWS), hInstance, NULL);
    SendMessageW(m_hCheckArrows, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);

    curY += 26;
    m_hCheckStartup = CreateWindowExW(0, L"BUTTON", L"Start automatically with Windows",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        static_cast<int>(20 * scale), static_cast<int>(curY * scale),
        static_cast<int>(390 * scale), static_cast<int>(22 * scale),
        m_hWnd, reinterpret_cast<HMENU>(IDC_CHECK_STARTUP), hInstance, NULL);
    SendMessageW(m_hCheckStartup, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);

    curY += 26;
    m_hCheckMinimizeTray = CreateWindowExW(0, L"BUTTON", L"Minimize to System Tray on close",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        static_cast<int>(20 * scale), static_cast<int>(curY * scale),
        static_cast<int>(390 * scale), static_cast<int>(22 * scale),
        m_hWnd, reinterpret_cast<HMENU>(IDC_CHECK_MINTRAY), hInstance, NULL);
    SendMessageW(m_hCheckMinimizeTray, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);

    // Save & Cancel Buttons at Bottom
    curY += 36;
    int btnW = static_cast<int>(100 * scale);
    int btnH = static_cast<int>(28 * scale);

    m_hBtnSave = CreateWindowExW(0, L"BUTTON", L"Save",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        static_cast<int>(210 * scale), static_cast<int>(curY * scale),
        btnW, btnH, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_SAVE), hInstance, NULL);
    SendMessageW(m_hBtnSave, WM_SETFONT, reinterpret_cast<WPARAM>(m_hBoldFont), TRUE);

    m_hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        static_cast<int>(320 * scale), static_cast<int>(curY * scale),
        btnW, btnH, m_hWnd, reinterpret_cast<HMENU>(IDC_BTN_CANCEL), hInstance, NULL);
    SendMessageW(m_hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);

    return true;
}

void SettingsWindow::PopulateControls() {
    AppSettings s = m_settingsMgr.GetSettings();

    // Refresh interval
    int selInterval = 2; // 1000 ms default
    if (s.updateIntervalMs <= 250) selInterval = 0;
    else if (s.updateIntervalMs <= 500) selInterval = 1;
    else if (s.updateIntervalMs <= 1000) selInterval = 2;
    else if (s.updateIntervalMs <= 2000) selInterval = 3;
    else selInterval = 4;
    SendMessageW(m_hComboInterval, CB_SETCURSEL, selInterval, 0);

    // Speed unit
    SendMessageW(m_hComboUnit, CB_SETCURSEL, static_cast<int>(s.speedUnit), 0);

    // Layout
    SendMessageW(m_hComboLayout, CB_SETCURSEL, static_cast<int>(s.displayLayout), 0);

    // Decimals
    SendMessageW(m_hComboDecimals, CB_SETCURSEL, s.decimalPlaces, 0);

    // Mode
    SendMessageW(m_hComboMode, CB_SETCURSEL, static_cast<int>(s.windowMode), 0);

    // Theme
    SendMessageW(m_hComboTheme, CB_SETCURSEL, static_cast<int>(s.colorTheme), 0);

    // Populate Adapter list
    SendMessageW(m_hComboAdapter, CB_RESETCONTENT, 0, 0);
    SendMessageW(m_hComboAdapter, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Auto-Detect Active Internet Adapter"));

    m_adapterList = m_monitor.EnumerateAdapters();
    int selAdapter = 0;
    for (size_t i = 0; i < m_adapterList.size(); ++i) {
        std::wstring item = m_adapterList[i].friendlyName;
        if (!m_adapterList[i].description.empty()) {
            item += L" (" + m_adapterList[i].description + L")";
        }
        SendMessageW(m_hComboAdapter, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
        if (m_adapterList[i].id == s.selectedAdapterId) {
            selAdapter = static_cast<int>(i + 1);
        }
    }
    SendMessageW(m_hComboAdapter, CB_SETCURSEL, selAdapter, 0);

    // Checkboxes
    Button_SetCheck(m_hCheckArrows, s.showArrows ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_hCheckStartup, s.startWithWindows ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_hCheckMinimizeTray, s.minimizeToTray ? BST_CHECKED : BST_UNCHECKED);

    m_tempDlColor = s.downloadColor;
    m_tempUlColor = s.uploadColor;
}

void SettingsWindow::ChooseCustomColor(COLORREF& targetColor) {
    CHOOSECOLORW cc = { sizeof(CHOOSECOLORW) };
    static COLORREF customColors[16] = { 0 };
    cc.hwndOwner = m_hWnd;
    cc.lpCustColors = customColors;
    cc.rgbResult = targetColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColorW(&cc)) {
        targetColor = cc.rgbResult;
        SendMessageW(m_hComboTheme, CB_SETCURSEL, static_cast<int>(ColorTheme::Custom), 0);
    }
}

void SettingsWindow::SaveSettingsFromControls() {
    AppSettings s = m_settingsMgr.GetSettings();

    // Interval
    int selInterval = static_cast<int>(SendMessageW(m_hComboInterval, CB_GETCURSEL, 0, 0));
    switch (selInterval) {
    case 0: s.updateIntervalMs = 250; break;
    case 1: s.updateIntervalMs = 500; break;
    case 2: s.updateIntervalMs = 1000; break;
    case 3: s.updateIntervalMs = 2000; break;
    case 4: s.updateIntervalMs = 5000; break;
    default: s.updateIntervalMs = 1000; break;
    }

    // Speed unit
    s.speedUnit = static_cast<SpeedUnit>(SendMessageW(m_hComboUnit, CB_GETCURSEL, 0, 0));

    // Layout
    s.displayLayout = static_cast<DisplayLayout>(SendMessageW(m_hComboLayout, CB_GETCURSEL, 0, 0));

    // Decimals
    s.decimalPlaces = static_cast<int>(SendMessageW(m_hComboDecimals, CB_GETCURSEL, 0, 0));

    // Mode
    s.windowMode = static_cast<WindowMode>(SendMessageW(m_hComboMode, CB_GETCURSEL, 0, 0));

    // Theme
    s.colorTheme = static_cast<ColorTheme>(SendMessageW(m_hComboTheme, CB_GETCURSEL, 0, 0));
    s.downloadColor = m_tempDlColor;
    s.uploadColor = m_tempUlColor;

    // Adapter
    int selAdapter = static_cast<int>(SendMessageW(m_hComboAdapter, CB_GETCURSEL, 0, 0));
    if (selAdapter <= 0 || selAdapter > static_cast<int>(m_adapterList.size())) {
        s.selectedAdapterId = L"auto";
    } else {
        s.selectedAdapterId = m_adapterList[selAdapter - 1].id;
    }

    // Checkboxes
    s.showArrows = (Button_GetCheck(m_hCheckArrows) == BST_CHECKED);
    s.startWithWindows = (Button_GetCheck(m_hCheckStartup) == BST_CHECKED);
    s.minimizeToTray = (Button_GetCheck(m_hCheckMinimizeTray) == BST_CHECKED);

    m_settingsMgr.UpdateSettings(s);

    if (m_pWidget) {
        m_pWidget->UpdateDimensions();
        m_pWidget->UpdateThemeColors();
        m_pWidget->Reposition();
        m_pWidget->UpdateDisplay();
    }
}

void SettingsWindow::Show() {
    if (m_hWnd) {
        PopulateControls();
        ShowWindow(m_hWnd, SW_SHOW);
        SetForegroundWindow(m_hWnd);
    }
}

void SettingsWindow::Hide() {
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_HIDE);
    }
}

LRESULT CALLBACK SettingsWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<SettingsWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT SettingsWindow::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        UINT id = LOWORD(wParam);
        if (id == IDC_BTN_COLOR_DL) {
            ChooseCustomColor(m_tempDlColor);
        } else if (id == IDC_BTN_COLOR_UL) {
            ChooseCustomColor(m_tempUlColor);
        } else if (id == IDC_BTN_SAVE) {
            SaveSettingsFromControls();
            Hide();
        } else if (id == IDC_BTN_CANCEL) {
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
