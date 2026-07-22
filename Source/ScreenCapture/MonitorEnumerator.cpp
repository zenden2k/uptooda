#include "MonitorEnumerator.h"

BOOL MonitorEnumerator::enumDisplayMonitors(HDC hdc, LPCRECT lprcClip) {
    return EnumDisplayMonitors(hdc, lprcClip, monitorEnumProc, reinterpret_cast<LPARAM>(this));
}

size_t MonitorEnumerator::getCount() const {
    return monitors_.size();
}

MonitorEnumerator::MonitorInfo* MonitorEnumerator::getByIndex(size_t index) {
    if (index < monitors_.size()) {
        return &monitors_[index];
    } 
    return nullptr;
}

BOOL CALLBACK MonitorEnumerator::monitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorInfo info;
    auto* pthis = reinterpret_cast<MonitorEnumerator*>(dwData);
    info.monitor = hMonitor;
    if (lprcMonitor) {
        info.rect = *lprcMonitor;
    }
    MONITORINFOEX mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    info.deviceName = mi.szDevice;
    pthis->monitors_.push_back(info);

    return TRUE;
}

std::vector<MonitorEnumerator::MonitorInfo>::const_iterator MonitorEnumerator::begin() const {
    return monitors_.cbegin();
}

std::vector<MonitorEnumerator::MonitorInfo>::const_iterator MonitorEnumerator::end() const {
    return monitors_.cend();
}
