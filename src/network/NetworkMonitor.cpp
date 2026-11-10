#include "network/NetworkMonitor.h"
#include <algorithm>

NetworkMonitor::NetworkMonitor() {
    EnumerateAdapters();
}

bool NetworkMonitor::IsVirtualAdapter(const std::wstring& description, const std::wstring& friendlyName) const {
    std::wstring lowerDesc = description;
    std::wstring lowerName = friendlyName;
    std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::towlower);
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

    const wchar_t* virtualKeywords[] = {
        L"hyper-v", L"vethernet", L"virtualbox", L"vmware",
        L"tap-windows", L"wsl", L"pseudo", L"loopback",
        L"npcap", L"pcap", L"wireguard tunnel"
    };

    for (const auto* kw : virtualKeywords) {
        if (lowerDesc.find(kw) != std::wstring::npos || lowerName.find(kw) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

std::vector<AdapterInfo> NetworkMonitor::EnumerateAdapters() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AdapterInfo> adapters;

    ULONG outBufLen = 15000;
    std::vector<BYTE> buffer(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_PREFIX;
    ULONG dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(outBufLen);
        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES curr = pAddresses; curr != NULL; curr = curr->Next) {
            // Exclude software loopback
            if (curr->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
                continue;
            }

            AdapterInfo info;
            info.index = curr->IfIndex;
            if (curr->AdapterName) {
                // AdapterName is ANSI string GUID
                int len = MultiByteToWideChar(CP_ACP, 0, curr->AdapterName, -1, NULL, 0);
                if (len > 0) {
                    std::wstring wname(len - 1, 0);
                    MultiByteToWideChar(CP_ACP, 0, curr->AdapterName, -1, &wname[0], len);
                    info.id = wname;
                }
            }
            if (curr->FriendlyName) {
                info.friendlyName = curr->FriendlyName;
            }
            if (curr->Description) {
                info.description = curr->Description;
            }
            info.type = curr->IfType;
            info.isOperational = (curr->OperStatus == IfOperStatusUp);
            info.linkSpeed = curr->ReceiveLinkSpeed;

            // Check if hardware
            bool isVirtual = IsVirtualAdapter(info.description, info.friendlyName);
            info.isHardware = !isVirtual && (curr->IfType == IF_TYPE_ETHERNET_CSMACD ||
                                             curr->IfType == IF_TYPE_IEEE80211 ||
                                             curr->IfType == IF_TYPE_WWANPP ||
                                             curr->IfType == IF_TYPE_WWANPP2 ||
                                             curr->IfType == IF_TYPE_PPP);

            adapters.push_back(info);
        }
    }

    m_cachedAdapters = adapters;
    return adapters;
}

NetworkStats NetworkMonitor::PollStats(const std::wstring& selectedAdapterId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    NetworkStats stats;
    stats.isConnected = false;

    PMIB_IF_TABLE2 pTable = NULL;
    if (GetIfTable2(&pTable) != NO_ERROR || !pTable) {
        return stats;
    }

    bool isAuto = (selectedAdapterId.empty() || selectedAdapterId == L"auto");

    // Gather gateway info to find default internet route
    ULONG bestIfIndex = 0;
    MIB_IPFORWARD_ROW2 bestRoute;
    SOCKADDR_INET destAddr;
    ZeroMemory(&destAddr, sizeof(destAddr));
    destAddr.si_family = AF_INET;
    destAddr.Ipv4.sin_addr.s_addr = inet_addr("8.8.8.8");

    if (GetBestRoute2(NULL, 0, NULL, &destAddr, 0, &bestRoute, &destAddr) == NO_ERROR) {
        bestIfIndex = bestRoute.InterfaceIndex;
    }

    // Match selected or active adapter
    const MIB_IF_ROW2* targetRow = NULL;
    std::wstring foundName;
    std::wstring foundDesc;

    if (!isAuto) {
        // User selected a specific adapter ID
        for (ULONG i = 0; i < pTable->NumEntries; ++i) {
            const MIB_IF_ROW2& row = pTable->Table[i];
            // Match against cached adapter info
            for (const auto& ca : m_cachedAdapters) {
                if (ca.id == selectedAdapterId && ca.index == row.InterfaceIndex) {
                    targetRow = &row;
                    foundName = ca.friendlyName;
                    foundDesc = ca.description;
                    break;
                }
            }
            if (targetRow) break;
        }
    }

    if (!targetRow && bestIfIndex > 0) {
        // Find the best route adapter in the table
        for (ULONG i = 0; i < pTable->NumEntries; ++i) {
            const MIB_IF_ROW2& row = pTable->Table[i];
            if (row.InterfaceIndex == bestIfIndex && row.OperStatus == IfOperStatusUp) {
                targetRow = &row;
                break;
            }
        }
    }

    // Fallback: If best route not found or not operational, pick the first operational hardware adapter
    if (!targetRow) {
        uint64_t maxOctets = 0;
        for (ULONG i = 0; i < pTable->NumEntries; ++i) {
            const MIB_IF_ROW2& row = pTable->Table[i];
            if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            if (row.OperStatus != IfOperStatusUp) continue;

            // Find description from cache
            std::wstring desc = row.Description ? row.Description : L"";
            std::wstring fname = row.Alias ? row.Alias : L"";
            if (IsVirtualAdapter(desc, fname)) continue;

            uint64_t totalOctets = row.InOctets + row.OutOctets;
            if (totalOctets >= maxOctets) {
                maxOctets = totalOctets;
                targetRow = &row;
            }
        }
    }

    if (targetRow) {
        stats.isConnected = (targetRow->OperStatus == IfOperStatusUp);
        stats.inOctets = targetRow->InOctets;
        stats.outOctets = targetRow->OutOctets;
        stats.activeAdapterIndex = targetRow->InterfaceIndex;
        stats.linkSpeedBps = targetRow->ReceiveLinkSpeed;

        if (foundName.empty()) {
            // Find friendly name from cached list
            for (const auto& ca : m_cachedAdapters) {
                if (ca.index == targetRow->InterfaceIndex) {
                    foundName = ca.friendlyName;
                    foundDesc = ca.description;
                    break;
                }
            }
            if (foundName.empty()) {
                foundName = targetRow->Alias ? targetRow->Alias : L"Network Adapter";
                foundDesc = targetRow->Description ? targetRow->Description : L"";
            }
        }

        stats.activeAdapterName = foundName;
        stats.activeAdapterDescription = foundDesc;

        m_lastActiveIndex = stats.activeAdapterIndex;
        m_lastActiveName = stats.activeAdapterName;
        m_lastActiveDescription = stats.activeAdapterDescription;
        m_lastLinkSpeed = stats.linkSpeedBps;
        m_isConnected = stats.isConnected;
    } else {
        m_isConnected = false;
    }

    FreeMibTable(pTable);
    return stats;
}

std::wstring NetworkMonitor::GetActiveAdapterName() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastActiveName;
}

bool NetworkMonitor::IsConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isConnected;
}
