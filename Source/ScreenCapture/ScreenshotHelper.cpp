#include "ScreenshotHelper.h"

#include <dwmapi.h>

#include "Func/WinUtils.h"

namespace ScreenshotHelper {

std::vector<RECT> monitorsRects;

BOOL getActualWindowRect(HWND hWnd, RECT* res, bool maximizedFix) {
    BOOL isEnabled = false;
    if (S_OK == DwmIsCompositionEnabled(&isEnabled)) {
        if (isEnabled) {
            if (S_OK == DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, res, sizeof(RECT))) {
                /*if (maximizedFix)
                    *res = maximizedWindowFix(hWnd, *res);*/
                return TRUE;
            }
        }
    }

    return GetWindowRect(hWnd, res);
}

RECT maximizedWindowFix(HWND handle, RECT windowRect) {
    RECT res = windowRect;
    if (isWindowMaximized(handle)) {
        RECT screenRect = screenFromRectangle(windowRect);
        if (windowRect.left < screenRect.left) {
            windowRect.right -= (screenRect.left - windowRect.left) /** 2*/;
            windowRect.left = screenRect.left;
        }
        if (windowRect.top < screenRect.top) {
            windowRect.bottom -= (screenRect.top - windowRect.top) /** 2*/;
            windowRect.top = screenRect.top;
        }
        IntersectRect(&res, &windowRect, &screenRect);
        //  windowRect.Intersect(screenRect);
    }
    return res;
}

bool isWindowMaximized(HWND handle) {
    WINDOWPLACEMENT wp;
    memset(&wp, 0, sizeof(WINDOWPLACEMENT));
    wp.length = sizeof(WINDOWPLACEMENT);

    if (GetWindowPlacement(handle, &wp)) {
        return wp.showCmd == static_cast<UINT>(SW_MAXIMIZE);
    }
    return false;
}

RECT screenFromRectangle(RECT rc) {
    monitorsRects.clear();
    EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc, 0);
    int max = 0;
    size_t iMax = 0;
    for (size_t i = 0; i < monitorsRects.size(); i++) {
        CRect Bounds = monitorsRects[i];
        Bounds.IntersectRect(&Bounds, &rc);
        if (Bounds.Width() * Bounds.Height() > max) {
            max = Bounds.Width() * Bounds.Height();
            iMax = i;
        }
        // result.UnionRect(result,Bounds);
    }
    return monitorsRects[iMax];
}

BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    if (lprcMonitor) {
        monitorsRects.push_back(*lprcMonitor);
    }
    return TRUE;
}

HRGN getWindowRegion(HWND wnd) {
    RECT WndRect;
    getActualWindowRect(wnd, &WndRect);
    CRgn WindowRgn;
    WindowRgn.CreateRectRgnIndirect(&WndRect);
    if (::GetWindowRgn(wnd, WindowRgn) != ERROR) {
        // WindowRegion.GetRgnBox( &WindowRect);
        WindowRgn.OffsetRgn(WndRect.left, WndRect.top);
    }
    return WindowRgn.Detach();
}

HRGN getWindowVisibleRegion(HWND wnd) {
    CRgn winReg;
    CRect result;
    if (!(GetWindowLong(wnd, GWL_STYLE) & WS_CHILD)) {
        winReg = getWindowRegion(wnd);
        return winReg.Detach();
    }
    getActualWindowRect(wnd, &result);
    while (GetWindowLong(wnd, GWL_STYLE) & WS_CHILD) {
        wnd = GetParent(wnd);
        RECT rc;
        if (GetClientRect(wnd, &rc)) {
            MapWindowPoints(wnd, 0, reinterpret_cast<POINT*>(&rc), 2);
            // parentRgn.CreateRectRgnIndirect(&rc);
        }
        result.IntersectRect(&result, &rc);
    }
    winReg.CreateRectRgnIndirect(&result);
    return winReg.Detach();
}
}
