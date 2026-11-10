# NetSpeed One-Click Installer for Windows
# Does not require Administrator privileges! Installs per-user to %LOCALAPPDATA%\Programs\NetSpeed
param (
    [switch]$NoLaunch,
    [switch]$CreateDesktopShortcut
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   Installing NetSpeed for Windows      " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$CurrentDir = $PSScriptRoot
$SourceExe = Join-Path $CurrentDir "..\bin\NetSpeed.exe"
if (-not (Test-Path $SourceExe)) {
    $SourceExe = Join-Path $CurrentDir "NetSpeed.exe"
}
if (-not (Test-Path $SourceExe)) {
    throw "NetSpeed.exe not found! Please compile first or extract from release zip."
}

$InstallDir = Join-Path $env:LOCALAPPDATA "Programs\NetSpeed"
$TargetExe = Join-Path $InstallDir "NetSpeed.exe"
$UninstallScript = Join-Path $InstallDir "uninstall.ps1"

# Stop existing instance if running
$runningProc = Get-Process -Name "NetSpeed" -ErrorAction SilentlyContinue
if ($runningProc) {
    Write-Host "Stopping running NetSpeed process..." -ForegroundColor Yellow
    $runningProc | Stop-Process -Force
    Start-Sleep -Milliseconds 500
}

# Create installation directory
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

# Copy binary & uninstall script
Copy-Item -Path $SourceExe -Destination $TargetExe -Force
$sourceUninstall = Join-Path $CurrentDir "uninstall.ps1"
if (Test-Path $sourceUninstall) {
    Copy-Item -Path $sourceUninstall -Destination $UninstallScript -Force
}

Write-Host "Installed files to: $InstallDir" -ForegroundColor Green

# Create Start Menu shortcut
$StartMenuDir = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\NetSpeed"
New-Item -ItemType Directory -Force -Path $StartMenuDir | Out-Null
$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut((Join-Path $StartMenuDir "NetSpeed.lnk"))
$Shortcut.TargetPath = $TargetExe
$Shortcut.WorkingDirectory = $InstallDir
$Shortcut.Description = "Real-Time Internet Speed Monitor for Windows Taskbar"
$Shortcut.Save()
Write-Host "Created Start Menu shortcut." -ForegroundColor Green

# Optional Desktop shortcut
if ($CreateDesktopShortcut) {
    $DesktopDir = [Environment]::GetFolderPath("Desktop")
    $DeskShortcut = $WshShell.CreateShortcut((Join-Path $DesktopDir "NetSpeed.lnk"))
    $DeskShortcut.TargetPath = $TargetExe
    $DeskShortcut.WorkingDirectory = $InstallDir
    $DeskShortcut.Description = "Real-Time Internet Speed Monitor"
    $DeskShortcut.Save()
    Write-Host "Created Desktop shortcut." -ForegroundColor Green
}

# Register with Windows Apps & Features (Uninstall Registry Entry)
$UninstallRegKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\NetSpeed"
New-Item -Path $UninstallRegKey -Force | Out-Null
Set-ItemProperty -Path $UninstallRegKey -Name "DisplayName" -Value "NetSpeed - Real-Time Network Monitor"
Set-ItemProperty -Path $UninstallRegKey -Name "DisplayVersion" -Value "1.0.0"
Set-ItemProperty -Path $UninstallRegKey -Name "Publisher" -Value "NetSpeed Open Source"
Set-ItemProperty -Path $UninstallRegKey -Name "InstallLocation" -Value $InstallDir
Set-ItemProperty -Path $UninstallRegKey -Name "DisplayIcon" -Value $TargetExe
Set-ItemProperty -Path $UninstallRegKey -Name "UninstallString" -Value "powershell.exe -ExecutionPolicy Bypass -File `"$UninstallScript`""
Set-ItemProperty -Path $UninstallRegKey -Name "NoModify" -Value 1 -Type DWord
Set-ItemProperty -Path $UninstallRegKey -Name "NoRepair" -Value 1 -Type DWord

Write-Host "Registered in Windows Settings -> Apps." -ForegroundColor Green

# Launch
if (-not $NoLaunch) {
    Write-Host "Launching NetSpeed..." -ForegroundColor Cyan
    Start-Process -FilePath $TargetExe
}

Write-Host "========================================" -ForegroundColor Green
Write-Host "   NetSpeed installation complete!      " -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
