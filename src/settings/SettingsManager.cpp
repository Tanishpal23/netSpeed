#include "settings/SettingsManager.h"
#include <shlobj.h>

constexpr const wchar_t* REG_RUN_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr const wchar_t* REG_APP_VAL = L"NetSpeed";

SettingsManager::SettingsManager() {
    m_configPath = EnsureConfigDirectory() + L"\\config.ini";
    Load();
}

std::wstring SettingsManager::EnsureConfigDirectory() {
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring dir = std::wstring(appDataPath) + L"\\NetSpeed";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir;
    }
    return L".";
}

std::wstring SettingsManager::GetConfigFilePath() const {
    return m_configPath;
}

AppSettings SettingsManager::GetSettings() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings;
}

void SettingsManager::UpdateSettings(const AppSettings& newSettings) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_settings = newSettings;
    }
    Save();
}

bool SettingsManager::Load() {
    std::lock_guard<std::mutex> lock(m_mutex);
    const wchar_t* file = m_configPath.c_str();

    // Check if file exists
    DWORD attr = GetFileAttributesW(file);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        // Apply system startup state
        m_settings.startWithWindows = IsStartWithWindowsEnabled();
        return false;
    }

    // [General]
    m_settings.updateIntervalMs = GetPrivateProfileIntW(L"General", L"UpdateIntervalMs", 1000, file);
    if (m_settings.updateIntervalMs < 200) m_settings.updateIntervalMs = 200;
    if (m_settings.updateIntervalMs > 10000) m_settings.updateIntervalMs = 10000;

    m_settings.startWithWindows = (GetPrivateProfileIntW(L"General", L"StartWithWindows", 0, file) != 0);
    m_settings.minimizeToTray = (GetPrivateProfileIntW(L"General", L"MinimizeToTray", 1, file) != 0);
    m_settings.windowMode = static_cast<WindowMode>(GetPrivateProfileIntW(L"General", L"WindowMode", 0, file));
    m_settings.isPaused = (GetPrivateProfileIntW(L"General", L"IsPaused", 0, file) != 0);

    // [Network]
    wchar_t adapterBuf[256];
    GetPrivateProfileStringW(L"Network", L"SelectedAdapterId", L"auto", adapterBuf, 256, file);
    m_settings.selectedAdapterId = adapterBuf;

    // [Display]
    m_settings.speedUnit = static_cast<SpeedUnit>(GetPrivateProfileIntW(L"Display", L"SpeedUnit", 0, file));
    m_settings.displayLayout = static_cast<DisplayLayout>(GetPrivateProfileIntW(L"Display", L"DisplayLayout", 0, file));
    m_settings.decimalPlaces = GetPrivateProfileIntW(L"Display", L"DecimalPlaces", 1, file);
    m_settings.showArrows = (GetPrivateProfileIntW(L"Display", L"ShowArrows", 1, file) != 0);

    // [Appearance]
    m_settings.colorTheme = static_cast<ColorTheme>(GetPrivateProfileIntW(L"Appearance", L"ColorTheme", 0, file));
    m_settings.textColor = static_cast<COLORREF>(GetPrivateProfileIntW(L"Appearance", L"TextColor", RGB(245, 245, 245), file));
    m_settings.backgroundColor = static_cast<COLORREF>(GetPrivateProfileIntW(L"Appearance", L"BackgroundColor", RGB(20, 20, 20), file));
    m_settings.downloadColor = static_cast<COLORREF>(GetPrivateProfileIntW(L"Appearance", L"DownloadColor", RGB(0, 210, 255), file));
    m_settings.uploadColor = static_cast<COLORREF>(GetPrivateProfileIntW(L"Appearance", L"UploadColor", RGB(255, 153, 0), file));

    wchar_t fontBuf[64];
    GetPrivateProfileStringW(L"Appearance", L"FontFamily", L"Segoe UI", fontBuf, 64, file);
    m_settings.fontFamily = fontBuf;

    m_settings.fontSizePt = GetPrivateProfileIntW(L"Appearance", L"FontSizePt", 9, file);
    m_settings.opacityPercent = GetPrivateProfileIntW(L"Appearance", L"OpacityPercent", 100, file);
    if (m_settings.opacityPercent < 30) m_settings.opacityPercent = 30;
    if (m_settings.opacityPercent > 100) m_settings.opacityPercent = 100;

    // [Position]
    m_settings.floatingX = GetPrivateProfileIntW(L"Position", L"FloatingX", -1, file);
    m_settings.floatingY = GetPrivateProfileIntW(L"Position", L"FloatingY", -1, file);

    return true;
}

bool SettingsManager::Save() {
    std::lock_guard<std::mutex> lock(m_mutex);
    const wchar_t* file = m_configPath.c_str();

    // [General]
    WritePrivateProfileStringW(L"General", L"UpdateIntervalMs", std::to_wstring(m_settings.updateIntervalMs).c_str(), file);
    WritePrivateProfileStringW(L"General", L"StartWithWindows", m_settings.startWithWindows ? L"1" : L"0", file);
    WritePrivateProfileStringW(L"General", L"MinimizeToTray", m_settings.minimizeToTray ? L"1" : L"0", file);
    WritePrivateProfileStringW(L"General", L"WindowMode", std::to_wstring(static_cast<int>(m_settings.windowMode)).c_str(), file);
    WritePrivateProfileStringW(L"General", L"IsPaused", m_settings.isPaused ? L"1" : L"0", file);

    // [Network]
    WritePrivateProfileStringW(L"Network", L"SelectedAdapterId", m_settings.selectedAdapterId.c_str(), file);

    // [Display]
    WritePrivateProfileStringW(L"Display", L"SpeedUnit", std::to_wstring(static_cast<int>(m_settings.speedUnit)).c_str(), file);
    WritePrivateProfileStringW(L"Display", L"DisplayLayout", std::to_wstring(static_cast<int>(m_settings.displayLayout)).c_str(), file);
    WritePrivateProfileStringW(L"Display", L"DecimalPlaces", std::to_wstring(m_settings.decimalPlaces).c_str(), file);
    WritePrivateProfileStringW(L"Display", L"ShowArrows", m_settings.showArrows ? L"1" : L"0", file);

    // [Appearance]
    WritePrivateProfileStringW(L"Appearance", L"ColorTheme", std::to_wstring(static_cast<int>(m_settings.colorTheme)).c_str(), file);
    WritePrivateProfileStringW(L"Appearance", L"TextColor", std::to_wstring(m_settings.textColor).c_str(), file);
    WritePrivateProfileStringW(L"Appearance", L"BackgroundColor", std::to_wstring(m_settings.backgroundColor).c_str(), file);
    WritePrivateProfileStringW(L"Appearance", L"DownloadColor", std::to_wstring(m_settings.downloadColor).c_str(), file);
    WritePrivateProfileStringW(L"Appearance", L"UploadColor", std::to_wstring(m_settings.uploadColor).c_str(), file);
    WritePrivateProfileStringW(L"Appearance", L"FontFamily", m_settings.fontFamily.c_str(), file);
    WritePrivateProfileStringW(L"Appearance", L"FontSizePt", std::to_wstring(m_settings.fontSizePt).c_str(), file);
    WritePrivateProfileStringW(L"Appearance", L"OpacityPercent", std::to_wstring(m_settings.opacityPercent).c_str(), file);

    // [Position]
    WritePrivateProfileStringW(L"Position", L"FloatingX", std::to_wstring(m_settings.floatingX).c_str(), file);
    WritePrivateProfileStringW(L"Position", L"FloatingY", std::to_wstring(m_settings.floatingY).c_str(), file);

    // Synchronize Windows registry startup setting
    SetStartWithWindows(m_settings.startWithWindows);

    return true;
}

bool SettingsManager::IsStartWithWindowsEnabled() {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t path[MAX_PATH];
        DWORD pathLen = sizeof(path);
        DWORD type = 0;
        LONG result = RegQueryValueExW(hKey, REG_APP_VAL, NULL, &type, reinterpret_cast<LPBYTE>(path), &pathLen);
        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
    }
    return false;
}

bool SettingsManager::SetStartWithWindows(bool enable) {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring cmd = L"\"" + std::wstring(exePath) + L"\" --minimized";
            RegSetValueExW(hKey, REG_APP_VAL, 0, REG_SZ,
                reinterpret_cast<const BYTE*>(cmd.c_str()),
                static_cast<DWORD>((cmd.length() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, REG_APP_VAL);
        }
        RegCloseKey(hKey);
        return true;
    }
    return false;
}
