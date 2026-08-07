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

#include "MarkerTool.h"

#include "../Canvas.h"
#include "../Document.h"

#include "Core/Utils/CoreUtils.h"

#include <cmath>
#include <cassert>
#include "3rdpart/GdiplusH.h"

namespace ImageEditor {
using namespace Gdiplus;


MarkerTool::MarkerTool(Canvas* canvas) : AbstractDrawingTool(canvas) {
    circleStride_ = 0;
    circleData_ = 0;
    oldPoint_.x = -1;
    oldPoint_.y = -1;
    createCircle();
}

void MarkerTool::beginDraw(int x, int y) {
    Gdiplus::Region exclRegion(Gdiplus::Rect(0, 0, canvas_->getWidth(), canvas_->getHeigth()));
    affectedRegion_.Exclude(&exclRegion);
    canvas_->beginDocDrawing();
    oldPoint_.x = x;
    oldPoint_.y = y;
    startPoint_.x = x;
    startPoint_.y = y;
}

void MarkerTool::continueDraw(int x, int y, DWORD flags) {
    if (flags & MK_CONTROL) {
        y = oldPoint_.y;
    }
    if (startPoint_.x != -1 || startPoint_.y != -1) {
        drawLine(oldPoint_.x, oldPoint_.y, x, y);
    }

    oldPoint_.x = x;
    oldPoint_.y = y;
}

void MarkerTool::endDraw(int x, int y) {
    if (x == startPoint_.x && y == startPoint_.y && x == oldPoint_.x && y == oldPoint_.y) {
        drawLine(x, y, x, y);
    }
    startPoint_.x = -1;
    startPoint_.y = -1;
    endPoint_ = oldPoint_ = startPoint_;
    canvas_->currentDocument()->addAffectedSegments(segments_);
    canvas_->endDocDrawing();
    segments_.clear();
}

void MarkerTool::render(Painter* gr) {
}

ImageEditor::CursorType MarkerTool::getCursor(int x, int y) {
    return CursorType::ctBrush;
}

void MarkerTool::drawLine(int x0, int y0, int x1, int y1) {
    using namespace Gdiplus;
    Graphics* gr = canvas_->currentDocument()->getGraphicsObject();
    gr->SetSmoothingMode(SmoothingModeAntiAlias);
    Pen pen(foregroundColor_, static_cast<Gdiplus::REAL>(penSize_));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    //gr->DrawLine(&pen, x0,y0, x1, y1);

    RECT updatedRect = {0, 0, 0, 0};

    if (y1 < y0) {
        std::swap(y0, y1);
        std::swap(x0, x1);
    }
    int xStart = min(x0, x1)/*( min(x0, x1) / AffectedSegments::kSegmentSize +1 ) * AffectedSegments::kSegmentSize*/;
    int xEnd = max(x0, x1)/*( max(x0, x1) / AffectedSegments::kSegmentSize ) * AffectedSegments::kSegmentSize*/;

    int yStart = min(y0, y1);
    int yEnd = max(y0, y1);

    float len = sqrt(pow((float)x1 - x0, 2) + pow((float)y1 - y0, 2));
    if (((int)len) == 0) {
        len = 1;
    }
    float sinA = (x1 - x0) / len;
    float cosA = sqrt(1 - sinA * sinA);

    float x = static_cast<float>(x0);
    float y = static_cast<float>(y0);
    SolidBrush br(foregroundColor_);
    if (x1 == x0) {
        for (int y = yStart; y <= yEnd; y++) {
            x = static_cast<float>(x0);
            RECT rc = {static_cast<LONG>(x - penSize_ / 2), static_cast<LONG>(y - penSize_ / 2), penSize_, penSize_};
            rc.right += rc.left;
            rc.bottom += rc.top;
            UnionRect(&updatedRect, &updatedRect, &rc);
            highlightRegion(rc);


            /*RectF sourceRect(rc.left, rc.top, rc.right-rc.left, rc.bottom - rc.top);
            GraphicsPath path;
            path.AddEllipse(sourceRect);
            path.SetFillMode(FillModeAlternate);
            Gdiplus::Region reg(&path);

            affectedRegion_.Union(&reg);*/

            UnionRect(&updatedRect, &updatedRect, &rc);
            //delete circle;
        }
    } else if (y1 == y0) {
        for (int x = xStart; x <= xEnd; x++) {
            int y = y0;
            RECT rc = {x - penSize_ / 2, y - penSize_ / 2, penSize_, penSize_};
            rc.right += rc.left;
            rc.bottom += rc.top;
            UnionRect(&updatedRect, &updatedRect, &rc);
            highlightRegion(rc);
            segments_.markRect(rc);
        }
    } else {
        for (int a = 0; a <= len; a++) {
            x = x0 + a * sinA;
            y = y0 + a * cosA;

            RECT rc = {static_cast<LONG>(x - penSize_ / 2), static_cast<LONG>(y - penSize_ / 2), penSize_, penSize_};
            rc.right += rc.left;
            rc.bottom += rc.top;
            UnionRect(&updatedRect, &updatedRect, &rc);
            highlightRegion(rc);
            segments_.markRect(rc);
        }
    }

    canvas_->updateView(updatedRect);
}

void MarkerTool::highlightRegion(RECT rc) {
    Bitmap* canvasBm = canvas_->currentDocument()->getBitmap();
    BitmapData canvasData;
    int w = std::min(static_cast<int>(canvasBm->GetWidth() - rc.left), static_cast<int>(rc.right - rc.left));
    int h = std::min(static_cast<int>(canvasBm->GetHeight() - rc.top), static_cast<int>(rc.bottom - rc.top));
    rc.left = max<LONG>(0, rc.left);
    rc.top = max<LONG>(0, rc.top);
    Rect rc2(rc.left, rc.top, w, h);
    segments_.markRect(rc);
    if (canvasBm->LockBits(&rc2, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &canvasData) == Ok) {
        UINT stride;
        auto* source = static_cast<uint8_t*>(canvasData.Scan0);
        uint8_t* brSource = circleData_.get();
        if (canvasData.Stride > 0) {
            stride = canvasData.Stride;
        } else {
            stride = -canvasData.Stride;
        }

        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                int offset = i * stride + j * 4;
                int circleOffset = i * circleStride_ + j * 4;
                int Y = static_cast<int>(0.299 * source[offset] + 0.587 * source[offset + 1] + 0.114 * source[offset +
                    2]);

                auto srcA = static_cast<float>(pow(brSource[circleOffset + 3] / 255.0 * (Y / 255.0), 15));
                // why pow 15 ?? I don't know
                uint8_t srcR = brSource[circleOffset];
                uint8_t srcG = brSource[circleOffset + 1];
                uint8_t srcB = brSource[circleOffset + 2];
                /*if ( Y != 255 ) {
                    srcA = srcA;
                }*/

                auto dstA = static_cast<float>(source[offset + 3] / 255.0);
                uint8_t dstR = source[offset];
                uint8_t dstG = source[offset + 1];
                uint8_t dstB = source[offset + 2];
                float outA = srcA + dstA * (1 - srcA);
                auto outR = static_cast<uint8_t>((srcR * srcA + dstR * dstA * (1 - srcA)) / outA);
                auto outG = static_cast<uint8_t>((srcG * srcA + dstG * dstA * (1 - srcA)) / outA);
                auto outB = static_cast<uint8_t>((srcB * srcA + dstB * dstA * (1 - srcA)) / outA);
                source[offset] = outR;
                source[offset + 1] = outG;
                source[offset + 2] = outB;
                source[offset + 3] = static_cast<uint8_t>(outA * 255);
            }
        }

        canvasBm->UnlockBits(&canvasData);
    }
}

void MarkerTool::setPenSize(int size) {
    AbstractDrawingTool::setPenSize(size);
    createCircle();
}

void MarkerTool::createCircle() {
    using namespace Gdiplus;
    circleData_ = 0;
    circleStride_ = 0;
    Bitmap circle(penSize_, penSize_, PixelFormat32bppARGB);
    Graphics gr2(&circle);
    SolidBrush br(Color(255, 255, 0));
    if (penSize_ == 1) {
        gr2.FillRectangle(&br, 0, 0, 1, 1);
    } else {
        gr2.FillEllipse(&br, 0, 0, circle.GetWidth(), circle.GetHeight());
    }

    BitmapData circleData;

    Rect lc(0, 0, circle.GetWidth(), circle.GetHeight());
    if (circle.LockBits(&lc, ImageLockModeRead, PixelFormat32bppARGB, &circleData) == Ok) {
        if (circleData.Stride > 0) {
            circleStride_ = circleData.Stride;
        } else {
            circleStride_ = -circleData.Stride;
        }
        size_t dataSize = circleStride_ * circle.GetHeight();
        circleData_ = std::make_unique<uint8_t[]>(dataSize);
        memcpy(circleData_.get(), circleData.Scan0, dataSize);
        circle.UnlockBits(&circleData);
    }
}
}
