#pragma once

#include "Common.h"
#include "settings/SettingsManager.h"
#include "network/SpeedCalculator.h"
#include "network/NetworkMonitor.h"

class TrayIcon;
class DashboardWindow;
class SettingsWindow;

class TaskbarWidget {
public:
    TaskbarWidget(SettingsManager& settingsMgr, SpeedCalculator& calculator, NetworkMonitor& monitor);
    ~TaskbarWidget();

    bool Create();
    void Show();
    void Hide();
    void UpdateDisplay();
    void Reposition();
    void UpdateDimensions();
    void UpdateThemeColors();

    HWND GetHwnd() const { return m_hWnd; }
    void SetTrayIcon(TrayIcon* tray) { m_pTrayIcon = tray; }
    void SetDashboard(DashboardWindow* dash) { m_pDashboard = dash; }
    void SetSettingsWindow(SettingsWindow* settings) { m_pSettings = settings; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnPaint(HWND hWnd);
    bool IsTaskbarAutoHide() const;
    bool IsFullscreenAppActive() const;

    SettingsManager& m_settingsMgr;
    SpeedCalculator& m_calculator;
    NetworkMonitor& m_monitor;

    TrayIcon* m_pTrayIcon = nullptr;
    DashboardWindow* m_pDashboard = nullptr;
    SettingsWindow* m_pSettings = nullptr;

    HWND m_hWnd = NULL;
    HFONT m_hFont = NULL;
    UINT m_wmTaskbarCreated = 0;

    int m_width = 135;
    int m_height = 36;
    UINT m_currentDpi = 96;

    COLORREF m_curBgColor = RGB(20, 20, 20);
    COLORREF m_curTextColor = RGB(245, 245, 245);
    COLORREF m_curDlColor = RGB(0, 210, 255);
    COLORREF m_curUlColor = RGB(255, 153, 0);

    bool m_isDragging = false;
    POINT m_dragStartPos = { 0, 0 };
};
