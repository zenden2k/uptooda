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

#ifndef MYIMAGE_H
#define MYIMAGE_H

#pragma once

#include <memory>

#include "3rdpart/GdiPlusH.h"
#include "Core/Images/GdiPlusImage.h"
#include "atlheaders.h"

class CMyImage : public CWindowImpl<CMyImage> {
public:
    CMyImage();
    ~CMyImage();
    DECLARE_WND_CLASS(_T("CMyImage"))

    BEGIN_MSG_MAP(CMyImage)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkg)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MSG_WM_LBUTTONDOWN(OnLButtonDown)
    MSG_WM_RBUTTONDOWN(OnLButtonDown)
    MSG_WM_MBUTTONUP(OnLButtonDown)
    MSG_WM_TIMER(onTimer)
    END_MSG_MAP()

    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnEraseBkg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnLButtonDown(UINT Flags, CPoint Pt);

    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    bool loadImage(LPCTSTR FileName, std::shared_ptr<Gdiplus::Image> img = NULL, int ResourceID = 0,
                   bool Bitmap = false, COLORREF transp = 0, bool allowEnlarge = false, bool whiteBg = false,
                   bool drawBorder = true);
    void loadPreview(std::unique_ptr<GdiPlusImage> image);

    int imageWidth() const;
    int imageHeight() const;

    void setPause(bool bPause);
    bool isPaused() const;
    void reset();
    void setHideParent(bool hide);

private:
    static constexpr UINT_PTR ANIMATION_TIMER = 1;
    bool hideParent_ = false;
    bool imageLoaded_ = false;
    int imageWidth_ = 0, imageHeight_ = 0;
    HBITMAP backBufferBm_ { };
    HGDIOBJ oldBm_ = nullptr;
    HDC backBufferDc_ { };
    int backBufferWidth_ = 0, backBufferHeight_ = 0;
    std::shared_ptr<GdiPlusImage> animatedImage_;
    bool paused_ = false;
    size_t framePosition_ = 0;
    Gdiplus::Rect destRect_;
    void onTimer(UINT_PTR timerId);
    void scheduleFrame();
    void drawFrame();
};

#endif // MYIMAGE_H
