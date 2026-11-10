#include "ui/TrayIcon.h"
#include "ui/TaskbarWidget.h"
#include "ui/DashboardWindow.h"
#include "ui/SettingsWindow.h"

TrayIcon::TrayIcon(HWND hOwner, SettingsManager& settingsMgr, SpeedCalculator& calculator, NetworkMonitor& monitor)
    : m_hOwner(hOwner), m_settingsMgr(settingsMgr), m_calculator(calculator), m_monitor(monitor) {
}

TrayIcon::~TrayIcon() {
    Remove();
    if (m_hIcon) {
        DestroyIcon(m_hIcon);
        m_hIcon = NULL;
    }
}

HICON TrayIcon::CreateAppIcon() {
    // Generate a sleek 16x16 icon in memory: dark background with bright cyan/orange arrows
    int size = 16;
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, size, size);
    HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hBmp));

    // Dark rounded background
    HBRUSH bgBrush = CreateSolidBrush(RGB(18, 20, 24));
    RECT rc = { 0, 0, size, size };
    FillRect(hdcMem, &rc, bgBrush);
    DeleteObject(bgBrush);

    // Draw stylized network speed indicator (Download arrow in cyan, Upload arrow in orange)
    HPEN dlPen = CreatePen(PS_SOLID, 2, RGB(0, 220, 255));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdcMem, dlPen));
    // Download arrow (left side)
    MoveToEx(hdcMem, 4, 3, NULL);
    LineTo(hdcMem, 4, 12);
    LineTo(hdcMem, 2, 9);
    MoveToEx(hdcMem, 4, 12, NULL);
    LineTo(hdcMem, 6, 9);

    // Upload arrow (right side)
    HPEN ulPen = CreatePen(PS_SOLID, 2, RGB(255, 160, 20));
    SelectObject(hdcMem, ulPen);
    MoveToEx(hdcMem, 11, 12, NULL);
    LineTo(hdcMem, 11, 3);
    LineTo(hdcMem, 9, 6);
    MoveToEx(hdcMem, 11, 3, NULL);
    LineTo(hdcMem, 13, 6);

    SelectObject(hdcMem, oldPen);
    DeleteObject(dlPen);
    DeleteObject(ulPen);

    SelectObject(hdcMem, hOldBmp);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    // Create mask bitmap
    HBITMAP hMask = CreateBitmap(size, size, 1, 1, NULL);

    ICONINFO ii = { 0 };
    ii.fIcon = TRUE;
    ii.hbmMask = hMask;
    ii.hbmColor = hBmp;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hBmp);
    DeleteObject(hMask);

    return hIcon;
}

bool TrayIcon::Create() {
    if (m_isCreated) return true;

    m_hIcon = CreateAppIcon();

    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hOwner;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = WM_APP_TRAYICON;
    m_nid.hIcon = m_hIcon;
    wcscpy_s(m_nid.szTip, L"NetSpeed - Real-Time Network Monitor");

    m_isCreated = Shell_NotifyIconW(NIM_ADD, &m_nid);
    return m_isCreated;
}

void TrayIcon::Remove() {
    if (m_isCreated) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_isCreated = false;
    }
}

void TrayIcon::UpdateTooltip(const std::wstring& dlStr, const std::wstring& ulStr, const std::wstring& adapterName) {
    if (!m_isCreated) return;

    std::wstring tip = L"NetSpeed\n↓ " + dlStr + L"   ↑ " + ulStr;
    if (!adapterName.empty()) {
        tip += L"\n" + adapterName;
    }

    if (tip.length() >= 127) {
        tip = tip.substr(0, 124) + L"...";
    }

    wcscpy_s(m_nid.szTip, tip.c_str());
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void TrayIcon::ShowContextMenu() {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppSettings s = m_settingsMgr.GetSettings();

    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_DASHBOARD, L"&Dashboard && Graph\t(Double-Click)");
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_SETTINGS, L"&Settings...");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    // Display Mode submenu
    HMENU hModeMenu = CreatePopupMenu();
    AppendMenuW(hModeMenu, MF_STRING | (s.windowMode == WindowMode::TaskbarDocked ? MF_CHECKED : MF_UNCHECKED),
        IDM_TRAY_MODE_DOCK, L"Taskbar Docked");
    AppendMenuW(hModeMenu, MF_STRING | (s.windowMode == WindowMode::Floating ? MF_CHECKED : MF_UNCHECKED),
        IDM_TRAY_MODE_FLOAT, L"Desktop Floating");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hModeMenu), L"Display &Mode");

    // Layout submenu
    HMENU hLayoutMenu = CreatePopupMenu();
    AppendMenuW(hLayoutMenu, MF_STRING | (s.displayLayout == DisplayLayout::SingleLine ? MF_CHECKED : MF_UNCHECKED),
        IDM_TRAY_LAYOUT_SINGLE, L"Single-Line (↓ 12.4M ↑ 2.1M)");
    AppendMenuW(hLayoutMenu, MF_STRING | (s.displayLayout == DisplayLayout::TwoLine ? MF_CHECKED : MF_UNCHECKED),
        IDM_TRAY_LAYOUT_DOUBLE, L"Two-Line (Vertical Stack)");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hLayoutMenu), L"Widget &Layout");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    // Pause / Resume
    if (s.isPaused) {
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_PAUSE, L"▶ &Resume Monitoring");
    } else {
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_PAUSE, L"⏸ &Pause Monitoring");
    }

    // Start with Windows
    bool autoRun = SettingsManager::IsStartWithWindowsEnabled();
    AppendMenuW(hMenu, MF_STRING | (autoRun ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_AUTORUN, L"Start with &Windows");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_ABOUT, L"&About NetSpeed");
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"E&xit");

    SetMenuDefaultItem(hMenu, IDM_TRAY_DASHBOARD, FALSE);

    // Display at cursor position
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(m_hOwner);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN, pt.x, pt.y, 0, m_hOwner, NULL);
    DestroyMenu(hMenu);
}

bool TrayIcon::HandleTrayMessage(LPARAM lParam) {
    switch (lParam) {
    case WM_LBUTTONUP:
        // Left-click toggles dashboard
        if (m_pDashboard) {
            m_pDashboard->Toggle();
        }
        return true;

    case WM_LBUTTONDBLCLK:
        if (m_pDashboard) {
            m_pDashboard->Show();
        }
        return true;

    case WM_RBUTTONUP:
    case WM_CONTEXTMENU:
        ShowContextMenu();
        return true;
    }
    return false;
}
