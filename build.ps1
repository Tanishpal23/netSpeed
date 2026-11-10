# NetSpeed Build Script for Windows (PowerShell)
param (
    [string]$Configuration = "Release",
    [switch]$RunTests,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$RootDir = $PSScriptRoot
$BuildDir = Join-Path $RootDir "build"
$BinDir = Join-Path $RootDir "bin"

if ($Clean) {
    Write-Host "Cleaning build and bin directories..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
    if (Test-Path $BinDir) { Remove-Item -Recurse -Force $BinDir }
    Write-Host "Clean completed." -ForegroundColor Green
    return
}

# Ensure directories exist
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $BinDir | Out-Null

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   Building NetSpeed ($Configuration)   " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 1. Compile Resources
Write-Host "[1/4] Compiling Windows Resources (Manifest & Version Info)..." -ForegroundColor Gray
$rcInput = Join-Path $RootDir "resources\resource.rc"
$rcOutput = Join-Path $BuildDir "resource.o"
$resDir = Join-Path $RootDir "resources"
& windres -I $resDir $rcInput -O coff -o $rcOutput
if ($LASTEXITCODE -ne 0) { throw "Resource compilation failed." }

# 2. Source Files
$sources = @(
    "src\main.cpp",
    "src\network\NetworkMonitor.cpp",
    "src\network\SpeedCalculator.cpp",
    "src\settings\SettingsManager.cpp",
    "src\ui\TaskbarWidget.cpp",
    "src\ui\TrayIcon.cpp",
    "src\ui\DashboardWindow.cpp",
    "src\ui\SettingsWindow.cpp"
) | ForEach-Object { Join-Path $RootDir $_ }

$cxxFlags = @(
    "-std=c++17",
    "-O2",
    "-DUNICODE",
    "-D_UNICODE",
    "-DWIN32_LEAN_AND_MEAN",
    "-I$($RootDir)\include",
    "-static",
    "-static-libgcc",
    "-static-libstdc++",
    "-mwindows",
    "-municode"
)

$ldLibs = @(
    "-liphlpapi",
    "-lws2_32",
    "-lgdi32",
    "-luser32",
    "-lcomctl32",
    "-lcomdlg32",
    "-lshell32",
    "-ladvapi32",
    "-ldwmapi",
    "-luxtheme"
)

# 3. Compile Main Binary
$mainExe = Join-Path $BinDir "NetSpeed.exe"
Write-Host "[2/4] Compiling NetSpeed.exe (Self-contained x64)..." -ForegroundColor Gray
$compileArgs = $cxxFlags + $sources + $rcOutput + $ldLibs + @("-o", $mainExe)
& g++ @compileArgs
if ($LASTEXITCODE -ne 0) { throw "Compilation of NetSpeed.exe failed." }
Write-Host "  -> Successfully generated: $mainExe" -ForegroundColor Green

# 4. Compile Unit Tests
$testExe = Join-Path $BinDir "NetSpeedTests.exe"
Write-Host "[3/4] Compiling NetSpeedTests.exe..." -ForegroundColor Gray
$testSources = @(
    "tests\TestRunner.cpp",
    "src\network\SpeedCalculator.cpp",
    "src\settings\SettingsManager.cpp"
) | ForEach-Object { Join-Path $RootDir $_ }

$testFlags = @(
    "-std=c++17",
    "-O2",
    "-DUNICODE",
    "-D_UNICODE",
    "-DWIN32_LEAN_AND_MEAN",
    "-I$($RootDir)\include",
    "-static",
    "-static-libgcc",
    "-static-libstdc++",
    "-mconsole"
)

$testLibs = @(
    "-liphlpapi",
    "-lws2_32",
    "-luser32",
    "-ladvapi32",
    "-lshell32"
)

$testArgs = $testFlags + $testSources + $testLibs + @("-o", $testExe)
& g++ @testArgs
if ($LASTEXITCODE -ne 0) { throw "Compilation of NetSpeedTests.exe failed." }
Write-Host "  -> Successfully generated: $testExe" -ForegroundColor Green

# 5. Run Tests
if ($RunTests) {
    Write-Host "[4/4] Running Automated Unit Tests..." -ForegroundColor Cyan
    & $testExe
    if ($LASTEXITCODE -ne 0) { throw "Unit tests failed!" }
}

Write-Host "========================================" -ForegroundColor Green
Write-Host "   Build completed successfully!        " -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
