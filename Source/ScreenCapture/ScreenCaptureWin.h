/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

#ifndef IU_CORE_SCREEN_CAPTURE_H
#define IU_CORE_SCREEN_CAPTURE_H

#include <memory>
#include <vector>

#include "Core/Utils/CoreTypes.h"
#include "atlheaders.h"
#include "3rdpart/GdiplusH.h"

namespace ScreenCapture {

enum CaptureMode { cmFullScreen, cmActiveWindow, cmRectangles, cmFreeform, cmWindowHandles, cmLastRegion, cmTopWindowHandles};

enum MonitorMode { kCurrentMonitor = -2, kAllMonitors = -1, kSelectedMonitor = 0 };

class CScreenshotRegion {
public:
    CScreenshotRegion() : m_bFromScreen(false) {
    }

    virtual std::shared_ptr<Gdiplus::Bitmap> GetImage(HDC src) = 0;
    virtual ~CScreenshotRegion() = default;

    virtual bool PrepareShooting(bool fromScreen) {
        m_bFromScreen = fromScreen;
        return true;
    }

    virtual void AfterShooting() {
    }

    virtual bool IsEmpty() {
        return false;
    }

    void setDrawCursor(bool draw) {
        drawCursor_ = draw;
    }

protected:
    bool m_bFromScreen;
    bool drawCursor_ = false;

    void drawCursor(HDC dc, int offsetX, int offsetY) const;
};

class CRectRegion : public CScreenshotRegion {
public:
    CRectRegion();
    CRectRegion(int x, int y, int width, int height);
    explicit CRectRegion(HRGN region);
    std::shared_ptr<Gdiplus::Bitmap> GetImage(HDC src) override;
    bool IsEmpty() override;
    ~CRectRegion() override;
protected:
    CRgn m_ScreenRegion;
};

class CWindowHandlesRegion : public CRectRegion {
public:
    struct WindowCapturingFlags {
        bool RemoveCorners;
        bool AddShadow;
        bool RemoveBackground;
    };

    struct CWindowHandlesRegionItem {
        HWND wnd;
        bool Include;
    };

    CWindowHandlesRegion();
    explicit CWindowHandlesRegion(HWND wnd);
    void AddWindow(HWND wnd, bool Include);
    void RemoveWindow(HWND wnd);
    void Clear();
    void setWindowHidingDelay(int delay);
    void setWindowCapturingFlags(WindowCapturingFlags flags);
    std::shared_ptr<Gdiplus::Bitmap> GetImage(HDC src) override;
    bool IsEmpty() override;
    ~CWindowHandlesRegion() override;
    std::vector<CWindowHandlesRegionItem>::const_iterator cbegin() const;
    std::vector<CWindowHandlesRegionItem>::const_iterator cend() const;
    std::vector<CWindowHandlesRegionItem>::size_type size() const;

protected:


    std::shared_ptr<Gdiplus::Bitmap> CaptureWithTransparencyUsingDWM();
    HWND topWindow;
    int m_WindowHidingDelay;
    bool m_ClearBackground;
    bool m_RemoveCorners;
    bool m_PreserveShadow;
    std::vector<CWindowHandlesRegionItem> m_hWnds;
    void init();
};

class CActiveWindowRegion : public CWindowHandlesRegion {
public:
    CActiveWindowRegion();
    std::shared_ptr<Gdiplus::Bitmap> GetImage(HDC src) override;
};

class CFreeFormRegion : public CRectRegion {
public:
    CFreeFormRegion();
    void AddPoint(POINT point);
    void Clear();
    bool IsEmpty() override;
    std::shared_ptr<Gdiplus::Bitmap> GetImage(HDC src) override;
    ~CFreeFormRegion() override;
protected:
    std::vector<POINT> m_curvePoints;
};

class CScreenCaptureEngine {
public:
    CScreenCaptureEngine();
    ~CScreenCaptureEngine() = default;
    bool captureScreen(bool drawCursor);
    void setSource(HBITMAP src);
    bool captureRegion(CScreenshotRegion* region);
    void setDelay(int msec);
    void setMonitorMode(MonitorMode monitorMode, HMONITOR monitor = nullptr);
    [[nodiscard]] std::shared_ptr<Gdiplus::Bitmap> capturedBitmap() const;

private:
    int m_captureDelay;
    std::shared_ptr<Gdiplus::Bitmap> m_capturedBitmap;
    HBITMAP m_source;
    MonitorMode monitorMode_;
    HMONITOR monitor_;
    DISALLOW_COPY_AND_ASSIGN(CScreenCaptureEngine);
};

}

#endif  // IU_CORE_SCREEN_CAPTURE_H
