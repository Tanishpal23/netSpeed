#include "Common.h"
#include "network/SpeedCalculator.h"
#include "settings/SettingsManager.h"
#include <iostream>
#include <cassert>
#include <cmath>

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::wcerr << L"[FAIL] " << msg << L" (Line " << __LINE__ << L")" << std::endl; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define RUN_TEST(fn) \
    do { \
        std::wcout << L"Running " #fn L"... "; \
        int before = g_testsFailed; \
        fn(); \
        if (g_testsFailed == before) { \
            std::wcout << L"[PASS]" << std::endl; \
            g_testsPassed++; \
        } \
    } while(0)

// Test 1: Basic Speed Calculation with simulated counter data
void TestSpeedCalculation() {
    SpeedCalculator calc;

    // First sample establishes baseline
    bool ret1 = calc.Update(1000000, 500000, 1.0);
    TEST_ASSERT(!ret1, L"First sample must be baseline (returns false)");
    TEST_ASSERT(calc.GetCurrentDownloadBps() == 0.0, L"Initial download speed must be 0");
    TEST_ASSERT(calc.GetCurrentUploadBps() == 0.0, L"Initial upload speed must be 0");

    // Second sample: 1 second later, 1,250,000 bytes down (10 Mbps), 250,000 bytes up (2 Mbps)
    // 1000000 + 1250000 = 2250000 inOctets
    // 500000 + 250000 = 750000 outOctets
    bool ret2 = calc.Update(2250000, 750000, 1.0);
    TEST_ASSERT(ret2, L"Second sample must compute speed");
    TEST_ASSERT(std::abs(calc.GetCurrentDownloadBps() - 1250000.0) < 0.01, L"Download speed must be 1,250,000 B/s");
    TEST_ASSERT(std::abs(calc.GetCurrentUploadBps() - 250000.0) < 0.01, L"Upload speed must be 250,000 B/s");

    // Third sample: 0.5 second delta, 625,000 bytes down, 125,000 bytes up
    // Speed should still be 1,250,000 B/s and 250,000 B/s
    bool ret3 = calc.Update(2250000 + 625000, 750000 + 125000, 0.5);
    TEST_ASSERT(ret3, L"0.5s interval sample must compute speed");
    TEST_ASSERT(std::abs(calc.GetCurrentDownloadBps() - 1250000.0) < 0.01, L"Download speed with 0.5s delta must match");
    TEST_ASSERT(std::abs(calc.GetCurrentUploadBps() - 250000.0) < 0.01, L"Upload speed with 0.5s delta must match");
}

// Test 2: Unit Formatting
void TestUnitFormatting() {
    // 1,250,000 B/s = 10,000,000 bps = 10.0 Mbps
    double speed10Mbps = 1250000.0;

    // Auto unit
    std::wstring strAuto = SpeedCalculator::FormatSpeed(speed10Mbps, SpeedUnit::Auto, 1, true);
    TEST_ASSERT(strAuto == L"10.0 Mbps", L"Auto unit for 10 Mbps failed");

    // Mbps unit
    std::wstring strMbps = SpeedCalculator::FormatSpeed(speed10Mbps, SpeedUnit::Mbps, 1, true);
    TEST_ASSERT(strMbps == L"10.0 Mbps", L"Mbps unit failed");

    // MB/s unit: 1,250,000 / (1024*1024) = 1.192 MB/s -> 1.2 MB/s
    std::wstring strMBps = SpeedCalculator::FormatSpeed(speed10Mbps, SpeedUnit::MBps, 1, true);
    TEST_ASSERT(strMBps == L"1.2 MB/s", L"MB/s unit failed");

    // KB/s unit: 1,250,000 / 1024 = 1220.7 KB/s
    std::wstring strKBps = SpeedCalculator::FormatSpeed(speed10Mbps, SpeedUnit::KBps, 1, true);
    TEST_ASSERT(strKBps == L"1220.7 KB/s", L"KB/s unit failed");

    // Kbps unit: 10,000,000 / 1000 = 10000.0 Kbps
    std::wstring strKbps = SpeedCalculator::FormatSpeed(speed10Mbps, SpeedUnit::Kbps, 0, true);
    TEST_ASSERT(strKbps == L"10000 Kbps", L"Kbps unit failed");

    // Idle 0 speed
    std::wstring strZero = SpeedCalculator::FormatSpeed(0.0, SpeedUnit::Mbps, 1, true);
    TEST_ASSERT(strZero == L"0.0 Mbps", L"Zero Mbps failed");

    // FormatBytes tests
    TEST_ASSERT(SpeedCalculator::FormatBytes(500) == L"500 B", L"FormatBytes 500 B failed");
    TEST_ASSERT(SpeedCalculator::FormatBytes(1048576) == L"1.00 MB", L"FormatBytes 1.00 MB failed");
    TEST_ASSERT(SpeedCalculator::FormatBytes(1073741824ULL * 5ULL) == L"5.00 GB", L"FormatBytes 5.00 GB failed");
}

// Test 3: Counter Reset and Rollover Protection
void TestCounterResetAndRollover() {
    SpeedCalculator calc;
    calc.Update(50000000, 50000000, 1.0); // Baseline
    calc.Update(60000000, 55000000, 1.0); // Normal traffic

    TEST_ASSERT(calc.GetCurrentDownloadBps() == 10000000.0, L"Normal traffic download speed mismatch");

    // Adapter disconnects and resets counters to 1000
    calc.Update(1000, 500, 1.0);

    // Should detect counter reset, avoid huge negative/spike value, and report 0
    TEST_ASSERT(calc.GetCurrentDownloadBps() == 0.0, L"Download speed after reset must be 0");
    TEST_ASSERT(calc.GetCurrentUploadBps() == 0.0, L"Upload speed after reset must be 0");

    // Next sample from new baseline: 1000 + 5000 = 6000
    calc.Update(6000, 1500, 1.0);
    TEST_ASSERT(calc.GetCurrentDownloadBps() == 5000.0, L"Subsequent sample must calculate from new baseline");
    TEST_ASSERT(calc.GetCurrentUploadBps() == 1000.0, L"Subsequent sample upload must calculate from new baseline");
}

// Test 4: Disconnected Network Handling
void TestDisconnectedNetworkHandling() {
    SpeedCalculator calc;
    calc.Update(100000, 50000, 1.0);
    calc.Update(200000, 100000, 1.0);

    // Network is disconnected -> 0 bytes transferred
    calc.Update(200000, 100000, 1.0);
    TEST_ASSERT(calc.GetCurrentDownloadBps() == 0.0, L"Disconnected idle speed must be 0");
    TEST_ASSERT(calc.GetCurrentUploadBps() == 0.0, L"Disconnected idle speed must be 0");

    std::wstring idleStr = SpeedCalculator::FormatSpeed(calc.GetCurrentDownloadBps(), SpeedUnit::Mbps, 1, true);
    TEST_ASSERT(idleStr == L"0.0 Mbps", L"Idle speed string must be 0.0 Mbps");
}

// Test 5: Session Totals and Peak Tracking
void TestSessionTotalsAndPeaks() {
    SpeedCalculator calc;
    calc.Update(1000, 1000, 1.0); // Baseline

    // Step 1: 5000 bytes down, 2000 bytes up in 1s -> speed 5000, 2000
    calc.Update(6000, 3000, 1.0);
    TEST_ASSERT(calc.GetTotalDownloadedBytes() == 5000, L"Total downloaded mismatch");
    TEST_ASSERT(calc.GetTotalUploadedBytes() == 2000, L"Total uploaded mismatch");
    TEST_ASSERT(calc.GetPeakDownloadBps() == 5000.0, L"Peak download mismatch");
    TEST_ASSERT(calc.GetPeakUploadBps() == 2000.0, L"Peak upload mismatch");

    // Step 2: 15000 bytes down, 1000 bytes up in 1s -> speed 15000, 1000
    calc.Update(21000, 4000, 1.0);
    TEST_ASSERT(calc.GetTotalDownloadedBytes() == 20000, L"Total downloaded step 2 mismatch");
    TEST_ASSERT(calc.GetTotalUploadedBytes() == 3000, L"Total uploaded step 2 mismatch");
    TEST_ASSERT(calc.GetPeakDownloadBps() == 15000.0, L"New peak download mismatch");
    TEST_ASSERT(calc.GetPeakUploadBps() == 2000.0, L"Peak upload should remain 2000");

    // Step 3: Reset session stats
    calc.ResetSessionStats();
    TEST_ASSERT(calc.GetTotalDownloadedBytes() == 0, L"Total downloaded after reset must be 0");
    TEST_ASSERT(calc.GetTotalUploadedBytes() == 0, L"Total uploaded after reset must be 0");
    TEST_ASSERT(calc.GetPeakDownloadBps() == 0.0, L"Peak download after reset must be 0");
    TEST_ASSERT(calc.GetPeakUploadBps() == 0.0, L"Peak upload after reset must be 0");

    // Verify history buffer
    auto hist = calc.GetHistory();
    TEST_ASSERT(hist.empty(), L"History must be cleared after reset");
}

// Test 6: Settings Manager Serialization and Defaults
void TestSettingsPersistence() {
    SettingsManager mgr;
    AppSettings s = mgr.GetSettings();

    // Verify valid defaults
    TEST_ASSERT(s.updateIntervalMs >= 200, L"Default update interval must be >= 200");
    TEST_ASSERT(s.decimalPlaces >= 0 && s.decimalPlaces <= 2, L"Decimal places must be 0-2");

    // Mutate and save
    s.updateIntervalMs = 500;
    s.speedUnit = SpeedUnit::MBps;
    s.displayLayout = DisplayLayout::TwoLine;
    s.decimalPlaces = 2;
    s.showArrows = false;

    mgr.UpdateSettings(s);

    // Reload from file
    SettingsManager reloadMgr;
    AppSettings reloaded = reloadMgr.GetSettings();

    TEST_ASSERT(reloaded.updateIntervalMs == 500, L"Persisted update interval mismatch");
    TEST_ASSERT(reloaded.speedUnit == SpeedUnit::MBps, L"Persisted speed unit mismatch");
    TEST_ASSERT(reloaded.displayLayout == DisplayLayout::TwoLine, L"Persisted layout mismatch");
    TEST_ASSERT(reloaded.decimalPlaces == 2, L"Persisted decimals mismatch");
    TEST_ASSERT(!reloaded.showArrows, L"Persisted show arrows mismatch");

    // Restore standard defaults
    s.updateIntervalMs = 1000;
    s.speedUnit = SpeedUnit::Auto;
    s.displayLayout = DisplayLayout::SingleLine;
    s.decimalPlaces = 1;
    s.showArrows = true;
    mgr.UpdateSettings(s);
}

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"   NetSpeed Test Suite - Execution      " << std::endl;
    std::wcout << L"========================================" << std::endl;

    RUN_TEST(TestSpeedCalculation);
    RUN_TEST(TestUnitFormatting);
    RUN_TEST(TestCounterResetAndRollover);
    RUN_TEST(TestDisconnectedNetworkHandling);
    RUN_TEST(TestSessionTotalsAndPeaks);
    RUN_TEST(TestSettingsPersistence);

    std::wcout << L"========================================" << std::endl;
    std::wcout << L"Results: " << g_testsPassed << L" Passed, " << g_testsFailed << L" Failed." << std::endl;
    std::wcout << L"========================================" << std::endl;

    return (g_testsFailed == 0) ? 0 : 1;
}
