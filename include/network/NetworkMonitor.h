#pragma once

#include "Common.h"

struct NetworkStats {
    uint64_t inOctets = 0;
    uint64_t outOctets = 0;
    bool isConnected = false;
    ULONG activeAdapterIndex = 0;
    std::wstring activeAdapterName;
    std::wstring activeAdapterDescription;
    uint64_t linkSpeedBps = 0;
};

class NetworkMonitor {
public:
    NetworkMonitor();
    ~NetworkMonitor() = default;

    // Refresh the list of all available network adapters on the system
    std::vector<AdapterInfo> EnumerateAdapters();

    // Query traffic statistics for either the auto-detected active adapter or a specific selected adapter
    // selectedAdapterId: empty or "auto" for automatic detection, or the adapter's GUID/ID
    NetworkStats PollStats(const std::wstring& selectedAdapterId = L"auto");

    // Get the name/description of the current active adapter
    std::wstring GetActiveAdapterName() const;

    // Check if network is currently connected
    bool IsConnected() const;

private:
    // Helper to determine if an adapter is virtual
    bool IsVirtualAdapter(const std::wstring& description, const std::wstring& friendlyName) const;

    mutable std::mutex m_mutex;
    std::vector<AdapterInfo> m_cachedAdapters;
    ULONG m_lastActiveIndex = 0;
    std::wstring m_lastActiveName;
    std::wstring m_lastActiveDescription;
    uint64_t m_lastLinkSpeed = 0;
    bool m_isConnected = false;
};
