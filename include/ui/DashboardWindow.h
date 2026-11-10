#pragma once

#include "Common.h"
#include "settings/SettingsManager.h"
#include "network/SpeedCalculator.h"
#include "network/NetworkMonitor.h"

class SettingsWindow;

class DashboardWindow {
public:
    DashboardWindow(SettingsManager& settingsMgr, SpeedCalculator& calculator, NetworkMonitor& monitor);
    ~DashboardWindow();

    bool Create();
    void Show();
    void Hide();
    void Toggle();
    void UpdateData();

    HWND GetHwnd() const { return m_hWnd; }
    void SetSettingsWindow(SettingsWindow* settings) { m_pSettings = settings; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnPaint(HWND hWnd);
    void DrawGraph(HDC hdc, const RECT& graphRect);
    void DrawStatCard(HDC hdc, const RECT& cardRect, const std::wstring& title,
                      const std::wstring& line1, COLORREF color1,
                      const std::wstring& line2, COLORREF color2);
    void CreateFonts();
    void RepositionControls();

    SettingsManager& m_settingsMgr;
    SpeedCalculator& m_calculator;
    NetworkMonitor& m_monitor;
    SettingsWindow* m_pSettings = nullptr;

    HWND m_hWnd = NULL;
    HWND m_hBtnReset = NULL;
    HWND m_hBtnSettings = NULL;
    HWND m_hBtnClose = NULL;

    HFONT m_hTitleFont = NULL;
    HFONT m_hNormalFont = NULL;
    HFONT m_hBoldFont = NULL;
    HFONT m_hValueFont = NULL;
    HFONT m_hSubValueFont = NULL;

    int m_width = 720;
    int m_height = 600;
    UINT m_currentDpi = 96;
};
