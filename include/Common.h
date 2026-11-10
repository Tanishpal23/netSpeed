#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <dwmapi.h>

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>

// Application Constants
constexpr const wchar_t* APP_NAME = L"NetSpeed";
constexpr const wchar_t* APP_VERSION = L"1.0.0";
constexpr const wchar_t* APP_MUTEX_NAME = L"Global\\NetSpeed_SingleInstance_Mutex_98745";
constexpr const wchar_t* WIDGET_CLASS_NAME = L"NetSpeedTaskbarWidgetClass";
constexpr const wchar_t* DASHBOARD_CLASS_NAME = L"NetSpeedDashboardClass";
constexpr const wchar_t* SETTINGS_CLASS_NAME = L"NetSpeedSettingsClass";

// Custom Windows Messages
constexpr UINT WM_APP_TRAYICON = WM_APP + 101;
constexpr UINT WM_APP_UPDATE_SPEED = WM_APP + 102;
constexpr UINT WM_APP_SETTINGS_CHANGED = WM_APP + 103;

// Display Units
enum class SpeedUnit {
    Auto = 0,   // Automatically choose Kbps, Mbps, Gbps
    Mbps = 1,   // Megabits per second
    MBps = 2,   // Megabytes per second (MB/s)
    KBps = 3,   // Kilobytes per second (KB/s)
    Kbps = 4    // Kilobits per second (Kbps)
};

// Display Layouts
enum class DisplayLayout {
    SingleLine = 0,  // "↓ 12.4 Mbps  ↑ 2.1 Mbps"
    TwoLine = 1      // Line 1: "↓ 12.4 Mbps", Line 2: "↑ 2.1 Mbps"
};

// Window Modes
enum class WindowMode {
    TaskbarDocked = 0, // Docked directly to the Windows taskbar
    Floating = 1       // Floating freely on desktop
};

// Color Themes
enum class ColorTheme {
    Dark = 0,
    Light = 1,
    Custom = 2
};

// Network Adapter Info
struct AdapterInfo {
    ULONG index = 0;
    std::wstring id;
    std::wstring friendlyName;
    std::wstring description;
    DWORD type = 0;             // IF_TYPE_ETHERNET_CSMACD, IF_TYPE_IEEE80211, etc.
    bool isHardware = false;
    bool isOperational = false;
    uint64_t linkSpeed = 0;
};

// Speed Sample
struct SpeedSample {
    double downloadSpeedBps = 0.0; // Bytes per second
    double uploadSpeedBps = 0.0;   // Bytes per second
    std::chrono::steady_clock::time_point timestamp;
};
