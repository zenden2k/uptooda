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

#include "MyImage.h"

#include "Core/i18n/Translator.h"
#include "Core/Images/Utils.h"

using namespace Gdiplus;

CMyImage::CMyImage() {
}

CMyImage::~CMyImage()
{
    if (backBufferDc_) {
        SelectObject(backBufferDc_, oldBm_);
        DeleteDC(backBufferDc_);
    }
    if (backBufferBm_) {
        DeleteObject(backBufferBm_);
    }
}

LRESULT CMyImage::OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    CPaintDC dc(m_hWnd);

    bHandled = true;

    if (imageLoaded_)
    {
        RECT rc;
        GetClientRect(&rc);
        dc.SetBkMode(TRANSPARENT);
        dc.BitBlt(0, 0, backBufferWidth_, backBufferHeight_, backBufferDc_, 0, 0, SRCCOPY);
    }

    return 0;
}

LRESULT CMyImage::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    reset();
    return 0;
}

LRESULT CMyImage::OnEraseBkg(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    bHandled = true;
    return TRUE;
}

bool CMyImage::loadImage(LPCTSTR FileName, std::shared_ptr<Gdiplus::Image> img, int ResourceID, bool Bmp, COLORREF transp, bool allowEnlarge, bool whiteBg, bool drawBorder)
{
    reset();
    CRect rc;
    GetClientRect(&rc);

    if (backBufferDc_) {
        SelectObject(backBufferDc_, oldBm_);
        DeleteDC(backBufferDc_);
    }
    backBufferDc_ = nullptr;
    if (backBufferBm_) {
        DeleteObject(backBufferBm_);
    }
    backBufferBm_ = nullptr;
    oldBm_ = nullptr;

    Graphics g(m_hWnd, true);

    backBufferWidth_ = rc.right;
    backBufferHeight_ = rc.bottom;

    isAnimated_ = false;
    int imgwidth = 0, imgheight = 0;
    int newwidth = 0, newheight = 0;
    int width = rc.Width();
    int height = rc.Height();

    if (ResourceID) {
        drawBorder = false;
    }

    if (drawBorder)
    {
        width -=  2;
        height -=  2;
    }

    CClientDC dc(m_hWnd);

    backBufferDc_ = ::CreateCompatibleDC(dc);
    backBufferBm_ = ::CreateCompatibleBitmap(dc, backBufferWidth_, backBufferHeight_);

    oldBm_ = ::SelectObject(backBufferDc_, backBufferBm_);

    Graphics gr(backBufferDc_);
    gr.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    gr.SetPixelOffsetMode(PixelOffsetModeHalf);
    ImageAttributes attr;
    attr.SetWrapMode(WrapModeTileFlipXY);

    std::unique_ptr<Image> newBm;
    std::shared_ptr<Image> bm;
    if (img) {
        bm = img;
    } else if (FileName) {
        std::unique_ptr<GdiPlusImage> srcImg = ImageUtils::LoadImageFromFileExtended(FileName);
        if (srcImg) {
            newBm.reset(srcImg->releaseBitmap());
        }
    } else if (ResourceID){
        if (!Bmp) {
            newBm = ImageUtils::BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(ResourceID), _T("PNG"));
            whiteBg = true;
        } else {
            newBm = std::make_unique<Bitmap>(_Module.GetResourceInstance(), MAKEINTRESOURCE(ResourceID));
        }
    }

    if (newBm) {
        bm = std::move(newBm);
    }

    if (bm){
        imageWidth_ = imgwidth = static_cast<int>(bm->GetWidth());
        imageHeight_ = imgheight = static_cast<int>(bm->GetHeight());

        Size sz = allowEnlarge ? ImageUtils::ProportionalSize(Size(imgwidth, imgheight), Size(width, height)) :
            ImageUtils::AdaptProportionalSize(Size(width, height), Size(imgwidth, imgheight));

        newwidth = sz.Width;
        newheight = sz.Height;
    }

    if (whiteBg) {
        gr.Clear(Color(GetRValue(transp), GetGValue(transp), GetBValue(transp)));
    } else {
        gr.Clear(Color(255, 145, 145, 145));
    }

    RectF bounds(1, 1, float(width), float(height));
    if ((bm) && !bm->GetWidth() && (FileName || ResourceID)) {
        LinearGradientBrush
            br(bounds, Color(255, 255, 255, 255), Color(255, 210, 210, 210), LinearGradientModeBackwardDiagonal);
        gr.FillRectangle(&br, (float)1, (float)1, (float)width, (float)height);

        LinearGradientBrush
            brush(bounds, Color(255, 95, 95, 95), Color(255, 125, 125, 125),
                LinearGradientModeBackwardDiagonal);

        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        Font font(L"Arial", 12, FontStyleBold);

        gr.DrawString(TR("Unable to load picture"), -1, &font, bounds, &format, &brush);
    } else {
        destRect_ = Rect(drawBorder ? 1 + (width - newwidth) / 2 : 0, drawBorder ? 1 + (height - newheight) / 2 : 0, newwidth, newheight);
        if (img && FileName && testForAnimatedGIF(img.get())) {
            animatedImage_ = img;
            initAnimation();
            isAnimated_ = true;
            imageLoaded_ = true;
            Release();
            Start();
            return false;
        }

        LinearGradientBrush br(bounds, Color(255, 255, 255, 255), Color(255, 210, 210, 210),
           LinearGradientModeBackwardDiagonal);

        if (!whiteBg) {
            gr.FillRectangle(&br, (float)1, (float)1, (float)width, (float)height);
        }

        imageLoaded_ = true;

        if (bm) {
            gr.DrawImage(bm.get(), destRect_, 0, 0, imgwidth, imgheight, UnitPixel, &attr);
        }
    }

    imageLoaded_ = true;
    Invalidate();
    return false;
}

void CMyImage::initAnimation() {
    exitEvent_.Close();
    pauseEvent_.Close();
    exitEvent_.Create(0, TRUE, FALSE);
    pauseEvent_.Create(0, TRUE, TRUE);
}

bool CMyImage::testForAnimatedGIF(Gdiplus::Image* img) {
    UINT count = 0;
    count = img->GetFrameDimensionsCount();
    auto pDimensionIDs = std::make_unique<GUID[]>(count);

    // Get the list of frame dimensions from the Image object.
    img->GetFrameDimensionsList(pDimensionIDs.get(), count);

    // Get the number of frames in the first dimension.
    frameCount_ = img->GetFrameCount(&pDimensionIDs[0]);

    // Assume that the image has a property item of type PropertyTagFrameDelay.
    // Get the size of that property item.
    int nSize = img->GetPropertyItemSize(PropertyTagFrameDelay);

    // Allocate a buffer to receive the property item.
    propertyItem_ = make_unique_malloc<PropertyItem>(nSize);

    img->GetPropertyItem(PropertyTagFrameDelay, nSize, propertyItem_.get());

    return frameCount_ > 1;
}

DWORD CMyImage::Run() {
    framePosition_ = 0;

    bool bExit = false;
    while (bExit == false)
    {
        bExit = drawFrame();
    }
    return 0;
}

bool CMyImage::drawFrame() {
    CRect clientRect;
    GetClientRect(&clientRect);
    pauseEvent_.WaitForEvent(INFINITE);

    GUID pageGuid = FrameDimensionTime;

    long hmWidth = animatedImage_->GetWidth();
    long hmHeight = animatedImage_->GetHeight();

    Graphics graphics(backBufferDc_);
    Color backgroundColor(255, 210, 210, 210);
    Rect rc(clientRect.left, clientRect.top, clientRect.Width()-1, clientRect.Height()-1);

    graphics.Clear(backgroundColor);
    Gdiplus::Pen pen(Color(255, 145, 145, 145));
    graphics.DrawRectangle(&pen, rc);
    graphics.DrawImage(animatedImage_.get(), destRect_);
    Invalidate();
    animatedImage_->SelectActiveFrame(&pageGuid, framePosition_++);

    if (framePosition_ == frameCount_) {
        framePosition_ = 0;
    }

    long lPause = static_cast<long*>(propertyItem_->value)[framePosition_] * 10;
    if (lPause < 50) {
        lPause = 100;
    }
    return exitEvent_.WaitForEvent(lPause);
}

LRESULT CMyImage::OnLButtonDown(UINT Flags, CPoint Pt)
{
    if (hideParent_) {
        ::ShowWindow(GetParent(), SW_HIDE);
    }
    return 0;
}

LRESULT CMyImage::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
    return SendMessage(GetParent(), uMsg, wParam, lParam);
}

int CMyImage::imageWidth() const {
    return imageWidth_;
}

int CMyImage::imageHeight() const {
    return imageHeight_;
}

void CMyImage::setPause(bool bPause) {
    if (!isAnimated_)
        return;

    if (bPause && !paused_) {
        pauseEvent_.ResetEvent();
    } else {
        if (paused_ && !bPause) {
            pauseEvent_.ResetEvent();
        }
    }

    paused_ = bPause;
}

bool CMyImage::isPaused() const {
    return paused_;
}

void CMyImage::reset() {
    if (isAnimated_ && IsRunning()) {
        exitEvent_.SetEvent();
        WaitForThread();
    }
    animatedImage_ = nullptr;
    frameCount_ = 0;
    propertyItem_ = nullptr;
}

void CMyImage::setHideParent(bool hide) {
    hideParent_ = hide;
}
