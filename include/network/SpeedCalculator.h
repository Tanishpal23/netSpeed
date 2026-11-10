#pragma once

#include "Common.h"

class SpeedCalculator {
public:
    SpeedCalculator();
    ~SpeedCalculator() = default;

    // Reset all calculations and baselines
    void Reset();

    // Reset session statistics (totals, peaks, history)
    void ResetSessionStats();

    // Process a new network counter sample
    // Returns true if a valid speed was computed (i.e. not the initial baseline sample)
    bool Update(uint64_t inOctets, uint64_t outOctets, double customDeltaTime = -1.0);

    // Get current calculated speeds in Bytes per second
    double GetCurrentDownloadBps() const;
    double GetCurrentUploadBps() const;

    // Get peak speeds in Bytes per second
    double GetPeakDownloadBps() const;
    double GetPeakUploadBps() const;

    // Get total bytes transferred during session
    uint64_t GetTotalDownloadedBytes() const;
    uint64_t GetTotalUploadedBytes() const;

    // Get rolling history of speed samples (up to maxHistorySamples)
    std::vector<SpeedSample> GetHistory(size_t maxSamples = 60) const;

    // Formatting utilities
    static std::wstring FormatSpeed(double bytesPerSec, SpeedUnit unit, int decimals = 1, bool includeUnit = true);
    static std::wstring FormatBytes(uint64_t totalBytes, int decimals = 2);

private:
    mutable std::mutex m_mutex;

    bool m_isFirstSample = true;
    uint64_t m_prevInOctets = 0;
    uint64_t m_prevOutOctets = 0;
    std::chrono::steady_clock::time_point m_prevTime;

    double m_currentDownloadBps = 0.0;
    double m_currentUploadBps = 0.0;

    double m_peakDownloadBps = 0.0;
    double m_peakUploadBps = 0.0;

    uint64_t m_totalDownloadedBytes = 0;
    uint64_t m_totalUploadedBytes = 0;

    std::deque<SpeedSample> m_history;
    static constexpr size_t MAX_HISTORY_CAPACITY = 300; // 5 minutes at 1s intervals
};
