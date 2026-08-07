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

#include "BrushTool.h"

#include <cmath>

#include "../Canvas.h"
#include "../Document.h"
#include "Core/Utils/CoreUtils.h"

namespace ImageEditor {

BrushTool::BrushTool(Canvas* canvas) : AbstractDrawingTool(canvas) {
    oldPoint_.x = -1;
    oldPoint_.y = -1;
}

void BrushTool::beginDraw(int x, int y) {
    Gdiplus::Region exclRegion(Gdiplus::Rect(0, 0, canvas_->getWidth(), canvas_->getHeigth()));
    affectedRegion_.Exclude(&exclRegion);
    canvas_->beginDocDrawing();
    oldPoint_.x = x;
    oldPoint_.y = y;
    startPoint_.x = x;
    startPoint_.y = y;
}

void BrushTool::continueDraw(int x, int y, DWORD flags) {
    if (flags & MK_CONTROL) {
        y = oldPoint_.y;
    }
    if (startPoint_.x != -1 || startPoint_.y != -1) {
        drawLine(oldPoint_.x, oldPoint_.y, x, y);
    }


    oldPoint_.x = x;
    oldPoint_.y = y;
}

void BrushTool::endDraw(int x, int y) {
    if (x == startPoint_.x && y == startPoint_.y && x == oldPoint_.x && y == oldPoint_.y) {
        // If user just clicked, but not moved pointer
        drawLine(x, y, x, y);
    }
    startPoint_.x = -1;
    startPoint_.y = -1;
    endPoint_ = oldPoint_ = startPoint_;

    canvas_->currentDocument()->addAffectedSegments(segments_);
    canvas_->endDocDrawing();
    segments_.clear();
}

void BrushTool::render(Painter* gr) {
}

ImageEditor::CursorType BrushTool::getCursor(int x, int y) {
    return CursorType::ctBrush;
}

void BrushTool::drawLine(int x0, int y0, int x1, int y1) {
    using namespace Gdiplus;
    Graphics* gr = canvas_->currentDocument()->getGraphicsObject();
    gr->SetSmoothingMode(SmoothingModeAntiAlias);
    Pen pen(foregroundColor_, static_cast<Gdiplus::REAL>(penSize_));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    if (x0 == x1 && y0 == y1) {
        SolidBrush br(foregroundColor_);
        gr->FillEllipse(&br, x0 - penSize_ / 2, y0 - penSize_ / 2, penSize_, penSize_);
    } else {
        gr->DrawLine(&pen, x0, y0, x1, y1);
    }

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
            RECT rc = {
                static_cast<LONG>(x) - penSize_ - 1, static_cast<LONG>(y) - penSize_ - 1, penSize_ * 2 + 1,
                penSize_ * 2 + 2
            };
            rc.right += rc.left;
            rc.bottom += rc.top;
            UnionRect(&updatedRect, &updatedRect, &rc);
            //gr->FillEllipse( &br, (int)x, y, penSize_, penSize_ );
            segments_.markRect(rc);
        }
    } else if (y1 == y0) {
        for (int x = xStart; x <= xEnd; x++) {
            int y = y0;
            RECT rc = {x - penSize_ - 1, y - penSize_ - 1, penSize_ * 2, penSize_ * 2 + 2};
            rc.right += rc.left;
            rc.bottom += rc.top;
            UnionRect(&updatedRect, &updatedRect, &rc);
            //gr->FillEllipse( &br, (int)x, y, penSize_, penSize_ );
            segments_.markRect(rc);
        }
    } else {
        // Why not simple draw line ? O_o
        for (int a = 0; a <= len; a++) {
            x = x0 + a * sinA;
            y = y0 + a * cosA;

            RECT rc = {
                static_cast<LONG>(x) - penSize_ - 1, static_cast<LONG>(y) - penSize_ - 1, penSize_ * 2 + 2,
                penSize_ * 2 + 2
            };
            rc.right += rc.left;
            rc.bottom += rc.top;
            UnionRect(&updatedRect, &updatedRect, &rc);
            //gr->FillEllipse( &br, (int)x, y, penSize_, penSize_ );
            segments_.markRect(rc);
        }
    }

    canvas_->updateView(updatedRect);
}
}
