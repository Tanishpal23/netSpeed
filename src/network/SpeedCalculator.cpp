#include "network/SpeedCalculator.h"
#include <cmath>

SpeedCalculator::SpeedCalculator() {
    Reset();
}

void SpeedCalculator::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isFirstSample = true;
    m_prevInOctets = 0;
    m_prevOutOctets = 0;
    m_currentDownloadBps = 0.0;
    m_currentUploadBps = 0.0;
    m_prevTime = std::chrono::steady_clock::now();
}

void SpeedCalculator::ResetSessionStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_totalDownloadedBytes = 0;
    m_totalUploadedBytes = 0;
    m_peakDownloadBps = 0.0;
    m_peakUploadBps = 0.0;
    m_history.clear();
}

bool SpeedCalculator::Update(uint64_t inOctets, uint64_t outOctets, double customDeltaTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();

    if (m_isFirstSample) {
        m_prevInOctets = inOctets;
        m_prevOutOctets = outOctets;
        m_prevTime = now;
        m_isFirstSample = false;
        m_currentDownloadBps = 0.0;
        m_currentUploadBps = 0.0;
        return false;
    }

    double elapsedSeconds = 0.0;
    if (customDeltaTime > 0.0) {
        elapsedSeconds = customDeltaTime;
    } else {
        std::chrono::duration<double> diff = now - m_prevTime;
        elapsedSeconds = diff.count();
    }

    // Ignore tiny or negative time differences to prevent division by zero
    if (elapsedSeconds < 0.05) {
        return false;
    }

    // Handle counter reset or rollover (e.g. adapter reconnect, 32-bit counter overflow)
    if (inOctets < m_prevInOctets || outOctets < m_prevOutOctets) {
        m_prevInOctets = inOctets;
        m_prevOutOctets = outOctets;
        m_prevTime = now;
        m_currentDownloadBps = 0.0;
        m_currentUploadBps = 0.0;
        return true;
    }

    uint64_t deltaIn = inOctets - m_prevInOctets;
    uint64_t deltaOut = outOctets - m_prevOutOctets;

    m_currentDownloadBps = static_cast<double>(deltaIn) / elapsedSeconds;
    m_currentUploadBps = static_cast<double>(deltaOut) / elapsedSeconds;

    // Accumulate session totals
    m_totalDownloadedBytes += deltaIn;
    m_totalUploadedBytes += deltaOut;

    // Update peak speeds
    if (m_currentDownloadBps > m_peakDownloadBps) {
        m_peakDownloadBps = m_currentDownloadBps;
    }
    if (m_currentUploadBps > m_peakUploadBps) {
        m_peakUploadBps = m_currentUploadBps;
    }

    // Update rolling history
    SpeedSample sample;
    sample.downloadSpeedBps = m_currentDownloadBps;
    sample.uploadSpeedBps = m_currentUploadBps;
    sample.timestamp = now;

    m_history.push_back(sample);
    if (m_history.size() > MAX_HISTORY_CAPACITY) {
        m_history.pop_front();
    }

    m_prevInOctets = inOctets;
    m_prevOutOctets = outOctets;
    m_prevTime = now;

    return true;
}

double SpeedCalculator::GetCurrentDownloadBps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentDownloadBps;
}

double SpeedCalculator::GetCurrentUploadBps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentUploadBps;
}

double SpeedCalculator::GetPeakDownloadBps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_peakDownloadBps;
}

double SpeedCalculator::GetPeakUploadBps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_peakUploadBps;
}

uint64_t SpeedCalculator::GetTotalDownloadedBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalDownloadedBytes;
}

uint64_t SpeedCalculator::GetTotalUploadedBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalUploadedBytes;
}

std::vector<SpeedSample> SpeedCalculator::GetHistory(size_t maxSamples) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SpeedSample> result;
    if (m_history.empty()) return result;

    size_t count = (std::min)(maxSamples, m_history.size());
    result.reserve(count);

    auto startIt = m_history.end() - count;
    for (auto it = startIt; it != m_history.end(); ++it) {
        result.push_back(*it);
    }
    return result;
}

std::wstring SpeedCalculator::FormatSpeed(double bytesPerSec, SpeedUnit unit, int decimals, bool includeUnit) {
    if (bytesPerSec < 0.0) bytesPerSec = 0.0;
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(decimals);

    double bitsPerSec = bytesPerSec * 8.0;

    switch (unit) {
    case SpeedUnit::Auto:
        if (bitsPerSec >= 1000000000.0) { // >= 1 Gbps
            ss << (bitsPerSec / 1000000000.0);
            if (includeUnit) ss << L" Gbps";
        } else if (bitsPerSec >= 1000000.0) { // >= 1 Mbps
            ss << (bitsPerSec / 1000000.0);
            if (includeUnit) ss << L" Mbps";
        } else if (bitsPerSec >= 1000.0) { // >= 1 Kbps
            ss << (bitsPerSec / 1000.0);
            if (includeUnit) ss << L" Kbps";
        } else {
            ss << bitsPerSec;
            if (includeUnit) ss << L" bps";
        }
        break;

    case SpeedUnit::Mbps:
        ss << (bitsPerSec / 1000000.0);
        if (includeUnit) ss << L" Mbps";
        break;

    case SpeedUnit::MBps:
        ss << (bytesPerSec / (1024.0 * 1024.0));
        if (includeUnit) ss << L" MB/s";
        break;

    case SpeedUnit::KBps:
        ss << (bytesPerSec / 1024.0);
        if (includeUnit) ss << L" KB/s";
        break;

    case SpeedUnit::Kbps:
        ss << (bitsPerSec / 1000.0);
        if (includeUnit) ss << L" Kbps";
        break;
    }

    return ss.str();
}

std::wstring SpeedCalculator::FormatBytes(uint64_t totalBytes, int decimals) {
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(decimals);

    double dBytes = static_cast<double>(totalBytes);
    constexpr double KB = 1024.0;
    constexpr double MB = 1024.0 * 1024.0;
    constexpr double GB = 1024.0 * 1024.0 * 1024.0;
    constexpr double TB = 1024.0 * 1024.0 * 1024.0 * 1024.0;

    if (dBytes >= TB) {
        ss << (dBytes / TB) << L" TB";
    } else if (dBytes >= GB) {
        ss << (dBytes / GB) << L" GB";
    } else if (dBytes >= MB) {
        ss << (dBytes / MB) << L" MB";
    } else if (dBytes >= KB) {
        ss << (dBytes / KB) << L" KB";
    } else {
        ss << totalBytes << L" B";
    }

    return ss.str();
}
