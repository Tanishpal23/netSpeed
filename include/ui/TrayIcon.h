#pragma once

#include "Common.h"
#include "settings/SettingsManager.h"
#include "network/SpeedCalculator.h"
#include "network/NetworkMonitor.h"

// Menu command IDs
constexpr UINT IDM_TRAY_DASHBOARD     = 2001;
constexpr UINT IDM_TRAY_SETTINGS      = 2002;
constexpr UINT IDM_TRAY_MODE_DOCK     = 2003;
constexpr UINT IDM_TRAY_MODE_FLOAT    = 2004;
constexpr UINT IDM_TRAY_LAYOUT_SINGLE = 2005;
constexpr UINT IDM_TRAY_LAYOUT_DOUBLE = 2006;
constexpr UINT IDM_TRAY_PAUSE         = 2007;
constexpr UINT IDM_TRAY_AUTORUN       = 2008;
constexpr UINT IDM_TRAY_ABOUT         = 2009;
constexpr UINT IDM_TRAY_EXIT          = 2010;

class TaskbarWidget;
class DashboardWindow;
class SettingsWindow;

class TrayIcon {
public:
    TrayIcon(HWND hOwner, SettingsManager& settingsMgr, SpeedCalculator& calculator, NetworkMonitor& monitor);
    ~TrayIcon();

    bool Create();
    void Remove();
    void UpdateTooltip(const std::wstring& dlStr, const std::wstring& ulStr, const std::wstring& adapterName);
    void ShowContextMenu();
    bool HandleTrayMessage(LPARAM lParam);

    void SetWidget(TaskbarWidget* widget) { m_pWidget = widget; }
    void SetDashboard(DashboardWindow* dash) { m_pDashboard = dash; }
    void SetSettingsWindow(SettingsWindow* settings) { m_pSettings = settings; }

private:
    HICON CreateAppIcon();

    HWND m_hOwner = NULL;
    SettingsManager& m_settingsMgr;
    SpeedCalculator& m_calculator;
    NetworkMonitor& m_monitor;

    TaskbarWidget* m_pWidget = nullptr;
    DashboardWindow* m_pDashboard = nullptr;
    SettingsWindow* m_pSettings = nullptr;

    NOTIFYICONDATAW m_nid = { sizeof(NOTIFYICONDATAW) };
    HICON m_hIcon = NULL;
    bool m_isCreated = false;
};
