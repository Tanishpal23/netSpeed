#pragma once

#include "Common.h"

struct AppSettings {
    // General
    int updateIntervalMs = 1000;
    bool startWithWindows = false;
    bool minimizeToTray = true;
    WindowMode windowMode = WindowMode::TaskbarDocked;
    bool isPaused = false;

    // Network
    std::wstring selectedAdapterId = L"auto";

    // Display & Layout
    SpeedUnit speedUnit = SpeedUnit::Auto;
    DisplayLayout displayLayout = DisplayLayout::SingleLine;
    int decimalPlaces = 1;
    bool showArrows = true;

    // Appearance & Colors
    ColorTheme colorTheme = ColorTheme::Dark;
    COLORREF textColor = RGB(245, 245, 245);
    COLORREF backgroundColor = RGB(20, 20, 20);
    COLORREF downloadColor = RGB(0, 210, 255);    // Vibrant Cyan
    COLORREF uploadColor = RGB(255, 153, 0);       // Warm Amber
    std::wstring fontFamily = L"Segoe UI";
    int fontSizePt = 9;
    int opacityPercent = 100;

    // Floating window coordinates
    int floatingX = -1;
    int floatingY = -1;
};

class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager() = default;

    // Load settings from config file (%APPDATA%\NetSpeed\config.ini)
    bool Load();

    // Save settings to config file
    bool Save();

    // Access current settings (thread-safe copy)
    AppSettings GetSettings() const;

    // Update settings (thread-safe)
    void UpdateSettings(const AppSettings& newSettings);

    // Windows startup registry helper
    static bool IsStartWithWindowsEnabled();
    static bool SetStartWithWindows(bool enable);

    // Get configuration file path
    std::wstring GetConfigFilePath() const;

private:
    std::wstring EnsureConfigDirectory();

    mutable std::mutex m_mutex;
    AppSettings m_settings;
    std::wstring m_configPath;
};
