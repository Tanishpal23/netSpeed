#pragma once

#include "Common.h"
#include "settings/SettingsManager.h"
#include "network/NetworkMonitor.h"

class TaskbarWidget;

class SettingsWindow {
public:
    SettingsWindow(SettingsManager& settingsMgr, NetworkMonitor& monitor);
    ~SettingsWindow();

    bool Create();
    void Show();
    void Hide();

    HWND GetHwnd() const { return m_hWnd; }
    void SetWidget(TaskbarWidget* widget) { m_pWidget = widget; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void PopulateControls();
    void SaveSettingsFromControls();
    void ChooseCustomColor(COLORREF& targetColor);

    SettingsManager& m_settingsMgr;
    NetworkMonitor& m_monitor;
    TaskbarWidget* m_pWidget = nullptr;

    HWND m_hWnd = NULL;
    HFONT m_hFont = NULL;
    HFONT m_hBoldFont = NULL;

    // Controls
    HWND m_hComboInterval = NULL;
    HWND m_hComboUnit = NULL;
    HWND m_hComboLayout = NULL;
    HWND m_hComboDecimals = NULL;
    HWND m_hComboTheme = NULL;
    HWND m_hComboAdapter = NULL;
    HWND m_hCheckStartup = NULL;
    HWND m_hCheckArrows = NULL;
    HWND m_hCheckMinimizeTray = NULL;
    HWND m_hComboMode = NULL;

    HWND m_hBtnColorDl = NULL;
    HWND m_hBtnColorUl = NULL;
    HWND m_hBtnSave = NULL;
    HWND m_hBtnCancel = NULL;

    COLORREF m_tempDlColor = RGB(0, 210, 255);
    COLORREF m_tempUlColor = RGB(255, 153, 0);

    std::vector<AdapterInfo> m_adapterList;
    UINT m_currentDpi = 96;
};
