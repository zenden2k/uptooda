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

#include "DrawingElement.h"
#include "atlheaders.h"
#include "3rdpart/GdiplusH.h"
#include "Core/Logging.h"

namespace ImageEditor {

DrawingElement::DrawingElement() : canvas_(nullptr) {
    startPoint_.x = 0;
    startPoint_.y = 0;
    endPoint_.x = 0;
    endPoint_.y = 0;
    color_ = Gdiplus::Color(0, 0, 0);
    penSize_ = 1;
    roundingRadius_ = penSize_;
}

void DrawingElement::resize(int width, int height) {
    LOG(ERROR) << "Not implemented";
    //dimensions_ = newSize;
}

void DrawingElement::setStartPoint(const POINT startPoint) {
    startPoint_ = startPoint;
}

void DrawingElement::setEndPoint(const POINT endPoint) {
    endPoint_ = endPoint;
}

POINT DrawingElement::getStartPoint() const {
    return startPoint_;
}

POINT DrawingElement::getEndPoint() const {
    return endPoint_;
}

void DrawingElement::setColor(Gdiplus::Color color) {
    color_ = color;
}

void DrawingElement::setBackgroundColor(Gdiplus::Color color) {
    backgroundColor_ = color;
}

Gdiplus::Color DrawingElement::getColor() const {
    return color_;
}

Gdiplus::Color DrawingElement::getBackgroundColor() const {
    return backgroundColor_;
}

void DrawingElement::setCanvas(Canvas* canvas) {
    canvas_ = canvas;
}

void DrawingElement::setPenSize(int penSize) {
    penSize_ = penSize;
}

void DrawingElement::setRoundingRadius(int radius) {
    roundingRadius_ = radius;
}

int DrawingElement::getWidth() const {
    return abs(endPoint_.x - startPoint_.x) + 1;
}

int DrawingElement::getHeight() const {
    return abs(endPoint_.y - startPoint_.y) + 1;
}

void DrawingElement::getAffectedSegments(AffectedSegments* segments) {
    segments->markPoint(startPoint_.x, startPoint_.y);
    segments->markPoint(endPoint_.x, endPoint_.y);
}

AffectedSegments::AffectedSegments() {
    maxWidth_ = -1;
    maxHeight_ = -1;
}

AffectedSegments::AffectedSegments(int maxWidth, int maxHeight) {
    maxWidth_ = maxWidth;
    maxHeight_ = maxHeight;
}

void AffectedSegments::markPoint(int x, int y) {
    if (x >= maxWidth_ || y >= maxHeight_ || x < 0 || y < 0) {
        return;
    }

    int horSegmentIndex = x / kSegmentSize;
    int vertSegmentIndex = y / kSegmentSize;
    unsigned int mapIndex = MAKELONG(horSegmentIndex, vertSegmentIndex);
    segments_[mapIndex] = true;
}

void AffectedSegments::markRect(int x, int y, int width, int height) {
    if (width < 1 || height < 1) {
        return;
    }
    if (x < 0) {
        width += x;
        x = 0;
    }

    if (y < 0) {
        height += y;
        y = 0;
    }

    if (maxWidth_ != -1) {
        width = std::min<>(width, maxWidth_ - x);
    }
    if (maxHeight_ != -1) {
        height = std::min<>(height, maxHeight_ - y);
    }

    if (width < 1 || height < 1) {
        return;
    }

    int xFirstSegmentIndex = x / kSegmentSize;
    int yFirstSegmentIndex = y / kSegmentSize;
    int xLastSegmentIndex = (x + width) / kSegmentSize;
    int yLastSegmentIndex = (y + height) / kSegmentSize;

    for (int i = xFirstSegmentIndex; i <= xLastSegmentIndex; i++) {
        for (int j = yFirstSegmentIndex; j <= yLastSegmentIndex; j++) {
            segments_[MAKELONG(i, j)] = true;
        }
    }
}

void AffectedSegments::markRect(RECT rc) {
    markRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
}

HRGN AffectedSegments::createRegionFromSegments() {
    CRgn rgn;
    rgn.CreateRectRgn(0, 0, 0, 0);

    for (auto i = segments_.begin(); i != segments_.end(); ++i) {
        CRgn tempRegion;
        unsigned int index = i->first;
        int xIndex = LOWORD(index);
        int yIndex = HIWORD(index);
        tempRegion.CreateRectRgn(xIndex * kSegmentSize, yIndex * kSegmentSize, (xIndex + 1) * kSegmentSize,
                                 (yIndex + 1) * kSegmentSize);
        rgn.CombineRgn(tempRegion, RGN_OR);
    }
    return rgn.Detach();
}

void AffectedSegments::getRects(std::deque<RECT>& rects, int maxWidth, int maxHeight) const {
    RECT bounds = {0, 0, maxWidth, maxHeight};
    bool checkBounds = maxWidth && maxHeight;
    for (auto i = segments_.begin(); i != segments_.end(); ++i) {
        unsigned int index = i->first;
        int xIndex = LOWORD(index);
        int yIndex = HIWORD(index);
        RECT rc = {
            xIndex * kSegmentSize, yIndex * kSegmentSize, (xIndex + 1) * kSegmentSize, (yIndex + 1) * kSegmentSize
        };
        if (checkBounds) {
            IntersectRect(&rc, &bounds, &rc);
        }
        rects.push_back(rc);
    }
}

AffectedSegments& AffectedSegments::operator+=(const AffectedSegments& segments) {
    for (auto i = segments.segments_.begin(); i != segments.segments_.end(); ++i) {
        segments_[i->first] = true;
    }
    return *this;
}

void AffectedSegments::clear() {
    segments_.clear();
}

}
