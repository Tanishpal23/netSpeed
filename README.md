# NetSpeed — Real-Time Windows Internet Speed Monitor


[![Platform: Windows 10 / 11 (x64)](https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011%20(x64)-brightgreen.svg)](#supported-windows-versions)

**NetSpeed** is a lightweight, high-performance Windows desktop application built in native modern C++ and Win32 APIs. It monitors and displays your live download (↓) and upload (↑) internet throughput directly on the Windows taskbar, with minimal memory and CPU footprint.

---

## Features

- **Genuine Real-Time Throughput**: Directly samples Windows network interface counters (`GetIfTable2` / `GetAdaptersAddresses`) at configurable intervals (1s default, down to 250ms). Zero periodic ping or speed-test server traffic.
- **Smart Active Adapter Detection**: Automatically tracks the active primary internet gateway (Wi-Fi, Gigabit Ethernet, 5G Cellular, or VPN) while ignoring virtual machine adapters (Hyper-V, WSL, VirtualBox, VMware) to prevent double-counting.
- **Direct Taskbar Docking & Floating Mode**:
  - **Taskbar Docked Mode**: Automatically anchors directly beside the Windows system tray / notification area (`Shell_TrayWnd`).
  - **Desktop Floating Mode**: Can be dragged anywhere on your desktop and remembers its coordinates across reboots.
- **Compact Layouts**: Toggle between **Single-Line** (`↓ 12.4 Mbps  ↑ 2.1 Mbps`) and **Two-Line** stacked vertical layout.
- **Multiple Units**: Supports **Auto** (Kbps / Mbps / Gbps), **Mbps** (Megabits/sec), **MB/s** (Megabytes/sec), **KB/s**, and **Kbps**.
- **Interactive 60-Second Dashboard**: Double-click the taskbar indicator or system tray icon to reveal a real-time historical throughput graph, session data-usage totals, and peak speed records.
- **Sleek Theming & Customization**: Built-in Dark and Light themes, custom accent colors for download/upload, adjustable font sizes, and opacity settings.
- **Resilient & Low Resource Usage**: Consumes < 8 MB RAM and < 0.1% CPU. Seamlessly recovers from Explorer restarts, multi-monitor rearrangements, DPI scaling changes, and sleep/resume events.
- **Zero Privacy Intrusion**: No telemetry, no background analytics, no ads, and no external server calls.

---

## Supported Windows Versions

- **Windows 11 (64-bit)**: All versions (21H2, 22H2, 23H2, 24H2+). Supports default centered or left-aligned taskbars, auto-hide taskbars, and multi-monitor setups.
- **Windows 10 (64-bit)**: Version 1809 (Build 17763) and higher.

### Windows 11 Taskbar Integration Details

In Windows 11, Microsoft replaced the legacy `Shell_TrayWnd` toolbar mechanism (DeskBands) with a modernized XAML taskbar. NetSpeed achieves seamless taskbar integration by attaching a top-level, non-activating, double-buffered Win32 widget positioned right beside the system tray notifications area (`TrayNotifyWnd`).
- The widget monitors taskbar position, monitor DPI shifts, auto-hide state, and full-screen application events.
- If you prefer a widget placed elsewhere on your screen, switch to **Desktop Floating Mode** in Settings or from the Tray menu.

---

## Installation Guide

### Option 1: Portable Release (Zero Developer Tools Required)

1. Navigate to the **[Releases](https://github.com/your-username/NetSpeed/releases)** page.
2. Download `NetSpeed-v1.0.0-Windows-x64.zip`.
3. Extract the ZIP archive to any folder on your computer.
4. Run `NetSpeed.exe`. The speed monitor will immediately appear on your taskbar and in your system tray.

### Option 2: One-Click User Installer

To integrate NetSpeed into your Windows Start Menu and Apps list without requiring Administrator privileges:
1. Extract the downloaded ZIP.
2. Right-click `install.ps1` and select **Run with PowerShell** (or run `powershell -ExecutionPolicy Bypass -File .\install.ps1`).
3. This installs NetSpeed to `%LOCALAPPDATA%\Programs\NetSpeed`, creates a Start Menu shortcut, and registers it in **Windows Settings → Apps**.

---

## Usage and Configuration

### System Tray Quick Menu
Right-click either the taskbar speed widget or the **NetSpeed** tray icon to access:
- **Dashboard & Graph (Double-Click)**: Opens the 60-second throughput chart and session statistics.
- **Settings...**: Opens the configuration dialog.
- **Display Mode**: Switch between **Taskbar Docked** and **Desktop Floating**.
- **Widget Layout**: Toggle between **Single-Line** and **Two-Line**.
- **Pause Monitoring**: Temporarily pause network polling.
- **Start with Windows**: Toggle automatic startup on Windows boot.
- **Exit**: Cleanly terminates the application.

### Configuring Displayed Speeds
Open **Settings** (via tray menu or the Dashboard button) to configure:
1. **Refresh Interval**: Choose between 250 ms (Ultra High), 500 ms (Fast), 1000 ms (Default), 2000 ms, or 5000 ms (Eco).
2. **Speed Unit**: Auto, Mbps, MB/s, KB/s, or Kbps.
3. **Decimal Precision**: 0, 1, or 2 decimal places.
4. **Network Adapter**: Leave set to *Auto-Detect Active Internet Adapter* or bind to a specific physical Ethernet, Wi-Fi, or VPN device.
5. **Colors & Themes**: Switch between Dark theme, Light theme, or select custom RGB colors for download and upload text.

### Enable Start with Windows
To have NetSpeed launch automatically upon logging into Windows:
- Open Settings and check **"Start automatically with Windows"**, or
- Right-click the Tray icon and click **"Start with Windows"**.

NetSpeed registers in your per-user Windows Run registry (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`) and starts minimized to the taskbar without prompting for admin privileges.

---

## How to Uninstall

### If installed using `install.ps1`:
- Open **Windows Settings → Apps → Installed apps**, locate **NetSpeed**, and click **Uninstall**, or
- Run `powershell -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1`.

### If using the Portable version:
1. Right-click the NetSpeed tray icon and select **Exit**.
2. If "Start with Windows" was enabled, uncheck it prior to exiting (or run `reg delete HKCU\Software\Microsoft\Windows\CurrentVersion\Run /v NetSpeed /f`).
3. Delete the folder containing `NetSpeed.exe`.
4. Delete `%APPDATA%\NetSpeed` to remove saved configuration.

---

## Building from Source

### Prerequisites
- Windows 10 or 11 (64-bit)
- MinGW-w64 / MSYS2 with `g++` (C++17) and `windres`, or Visual Studio 2022 with C++ desktop workload.

### Build using PowerShell
```powershell
# Clone the repository
git clone https://github.com/your-username/NetSpeed.git
cd NetSpeed

# Build Release binaries and run automated test suite
.\build.ps1 -RunTests
```
The compiled self-contained binaries will be placed in `bin\`:
- `bin\NetSpeed.exe`: Main application (statically linked, no runtime dependencies required).
- `bin\NetSpeedTests.exe`: Automated test runner for throughput calculations and persistence.

### Build using Make
```bash
make clean
make all
make test
```

---

## Automated GitHub Releases

This repository includes a fully automated GitHub Actions workflow (`.github/workflows/build-and-release.yml`):
- Whenever a version tag is pushed (e.g. `git tag v1.0.0 && git push origin v1.0.0`):
  1. Spins up a clean Windows runner.
  2. Compiles `NetSpeed.exe` with static runtime linkage.
  3. Executes the full unit test suite.
  4. Packages the portable distribution ZIP with installers and SHA256 checksums.
  5. Publishes a production release on the repository's GitHub Releases page.

---

## Architecture Overview

NetSpeed follows a strictly modular, decoupled Win32 architecture:

```
NetSpeed/
├── include/
│   ├── Common.h                   # Constants, enums, DPI & Win32 definitions
│   ├── network/
│   │   ├── NetworkMonitor.h       # Windows interface enumeration & traffic polling
│   │   └── SpeedCalculator.h      # Differential speed math, units & rolling history
│   ├── settings/
│   │   └── SettingsManager.h      # INI configuration & Windows startup registry
│   └── ui/
│       ├── TaskbarWidget.h        # Double-buffered docked/floating taskbar window
│       ├── TrayIcon.h             # Notification area icon & context menu
│       ├── DashboardWindow.h      # 60s live graph & session statistics dialog
│       └── SettingsWindow.h       # Configuration dialog
├── src/                           # Component implementations
├── tests/
│   └── TestRunner.cpp             # Automated testing for calculation logic & edge cases
├── resources/
│   ├── app.manifest               # Per-Monitor V2 DPI awareness & Common Controls v6
│   ├── resource.h
│   └── resource.rc
├── scripts/
│   ├── install.ps1                # Zero-admin per-user installer
│   └── uninstall.ps1              # Complete clean uninstaller
├── .github/workflows/
│   └── build-and-release.yml      # CI/CD automated build and release pipeline
├── build.ps1                      # Local Windows build script
└── Makefile                       # Standard GNU makefile
```

---
# netSpeed
