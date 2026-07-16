#ifndef IU_CORE_SCREENCAPTURE_UTILS
#define IU_CORE_SCREENCAPTURE_UTILS

#include <vector>

#include "Core/Utils/CoreTypes.h"
#include "atlheaders.h"

class MonitorEnumerator {

public:
    MonitorEnumerator() = default;
    BOOL enumDisplayMonitors(HDC hdc, LPCRECT lprcClip);
    [[nodiscard]] size_t getCount() const;

    struct MonitorInfo {
        HMONITOR monitor = nullptr;
        CRect rect;
        CString deviceName;
    };

    [[nodiscard]] std::vector<MonitorInfo>::const_iterator begin();
    [[nodiscard]] std::vector<MonitorInfo>::const_iterator end();

    MonitorInfo* getByIndex(size_t index);
protected:
    static BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    std::vector<MonitorInfo> monitors_;
    DISALLOW_COPY_AND_ASSIGN(MonitorEnumerator);
};

#endif