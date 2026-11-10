# NetSpeed Uninstaller for Windows
$ErrorActionPreference = "SilentlyContinue"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   Uninstalling NetSpeed...             " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 1. Stop any running process
$runningProc = Get-Process -Name "NetSpeed" -ErrorAction SilentlyContinue
if ($runningProc) {
    Write-Host "Stopping running NetSpeed process..." -ForegroundColor Yellow
    $runningProc | Stop-Process -Force
    Start-Sleep -Milliseconds 500
}

# 2. Remove Startup Registry Key
$RunRegKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
Remove-ItemProperty -Path $RunRegKey -Name "NetSpeed" -ErrorAction SilentlyContinue

# 3. Remove Start Menu Shortcut & folder
$StartMenuDir = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\NetSpeed"
if (Test-Path $StartMenuDir) {
    Remove-Item -Recurse -Force $StartMenuDir
}

# 4. Remove Desktop Shortcut
$DesktopDir = [Environment]::GetFolderPath("Desktop")
$DeskShortcut = Join-Path $DesktopDir "NetSpeed.lnk"
if (Test-Path $DeskShortcut) {
    Remove-Item -Force $DeskShortcut
}

# 5. Remove Uninstall Registry Key
$UninstallRegKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\NetSpeed"
Remove-Item -Path $UninstallRegKey -Recurse -Force -ErrorAction SilentlyContinue

# 6. Delete Installation Directory
$InstallDir = Join-Path $env:LOCALAPPDATA "Programs\NetSpeed"
if (Test-Path $InstallDir) {
    # If uninstaller itself is running from here, schedule self-deletion
    Start-Process -WindowStyle Hidden cmd.exe -ArgumentList "/c timeout /t 1 /nobreak > NUL & rmdir /s /q `"$InstallDir`""
}

Write-Host "========================================" -ForegroundColor Green
Write-Host "   NetSpeed has been uninstalled.       " -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
