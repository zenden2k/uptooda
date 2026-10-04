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

#include "SpeechBaloon.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Canvas.h"
#include "InputBox.h"

namespace ImageEditor {

namespace {
constexpr int TEXT_PADDING = 10;
constexpr int MIN_TAIL_DEPTH = 18;
constexpr int MAX_TAIL_DEPTH = 32;
constexpr int TAIL_BASE_HALF_WIDTH = 12;
constexpr int CORNER_RADIUS = 14;

int ClampValue(int value, int minValue, int maxValue) {
    if (maxValue < minValue) {
        return (minValue + maxValue) / 2;
    }
    return (std::max)(minValue, (std::min)(value, maxValue));
}
}

SpeechBaloon::SpeechBaloon(Canvas* canvas, std::shared_ptr<InputBox> inputBox, int startX, int startY, int endX,
                           int endY) : TextElement(canvas, std::move(inputBox), startX, startY, endX, endY, true) {
    fillBackground_ = true;
    isBackgroundColorUsed_ = true;
}

void SpeechBaloon::render(Painter* gr) {
    using namespace Gdiplus;
    if (!gr) {
        return;
    }

    drawDashedRectangle_ = isSelected() || !inputBox_;
    const int maxBorderWidth = (std::max)(1, (std::min)(getWidth() - 1, getHeight() - 1));
    const int borderWidth = (std::min)((std::max)(1, penSize_), maxBorderWidth);
    const int borderInset = (borderWidth + 1) / 2;
    const Geometry geometry = getGeometry(borderInset);
    const int radius = (std::min)(CORNER_RADIUS, (std::min)(geometry.BubbleRect.Width, geometry.BubbleRect.Height) / 3);

    GraphicsPath bubblePath;
    addSpeechBaloonPath(bubblePath, geometry, radius);

    SolidBrush backgroundBrush(backgroundColor_);
    gr->FillPath(&backgroundBrush, &bubblePath);

    Pen borderPen(color_, static_cast<REAL>(borderWidth));
    borderPen.SetLineJoin(LineJoinRound);
    gr->DrawPath(&borderPen, &bubblePath);

    if (inputBox_) {
        inputBox_->render(gr, nullptr, backgroundColor_, getInputBoxRect());
    }
}

void SpeechBaloon::resize(int width, int height) {
    width = (std::max)(1, width);
    height = (std::max)(1, height);
    const int directionX = endPoint_.x < startPoint_.x ? -1 : 1;
    const int directionY = endPoint_.y < startPoint_.y ? -1 : 1;
    endPoint_.x = startPoint_.x + directionX * (width - 1);
    endPoint_.y = startPoint_.y + directionY * (height - 1);

    if (inputBox_) {
        const Gdiplus::Rect inputRect = getInputBoxRect();
        inputBox_->resize(inputRect.X, inputRect.Y, inputRect.Width, inputRect.Height, grips_);
        inputBox_->invalidate();
        canvas_->updateView();
    }
}

ElementType SpeechBaloon::getType() const { return ElementType::etSpeechBaloon; }

Gdiplus::Rect SpeechBaloon::getInputBoxRect() {
    const Gdiplus::Rect bubbleRect = getGeometry().BubbleRect;
    const int availableWidth = (std::max)(1, bubbleRect.Width - TEXT_PADDING * 2);
    const int availableHeight = (std::max)(1, bubbleRect.Height - TEXT_PADDING * 2);
    const int inputWidth = inputWidth_ > 0 ? (std::min)(inputWidth_, availableWidth) : availableWidth;
    const int inputHeight = inputHeight_ > 0 ? (std::min)(inputHeight_, availableHeight) : availableHeight;
    return { bubbleRect.X + (bubbleRect.Width - inputWidth) / 2, bubbleRect.Y + (bubbleRect.Height - inputHeight) / 2,
             inputWidth, inputHeight };
}

void SpeechBaloon::onControlResized(int width, int height) {
    const RECT oldPaintRect = getPaintBoundingRect();
    inputWidth_ = (std::max)(1, width);
    inputHeight_ = (std::max)(1, height);
    const bool horizontalTail = std::abs(endPoint_.x - startPoint_.x) >= std::abs(endPoint_.y - startPoint_.y);
    const int tailDepth
        = (std::min)(MAX_TAIL_DEPTH, (std::max)(MIN_TAIL_DEPTH, (horizontalTail ? getWidth() : getHeight()) / 5));
    const int desiredWidth = width + TEXT_PADDING * 2 + 1 + (horizontalTail ? tailDepth : 0);
    const int desiredHeight = height + TEXT_PADDING * 2 + (horizontalTail ? 0 : tailDepth);

    int newWidth = (std::max)(desiredWidth, 80);
    int newHeight = (std::max)(desiredHeight, 50);

    resize(newWidth, newHeight);

    const RECT newPaintRect = getPaintBoundingRect();
    RECT updateRect;
    UnionRect(&updateRect, &oldPaintRect, &newPaintRect);
    canvas_->updateView(updateRect);
}

SpeechBaloon::Geometry SpeechBaloon::getGeometry(int inset) {
    const int maxInset = (std::max)(0, ((std::min)(getWidth(), getHeight()) - 1) / 2);
    inset = (std::min)((std::max)(0, inset), maxInset);
    const int x = getX() + inset;
    const int y = getY() + inset;
    const int width = (std::max)(1, getWidth() - inset * 2 - 1);
    const int height = (std::max)(1, getHeight() - inset * 2 - 1);
    const bool horizontalTail = std::abs(endPoint_.x - startPoint_.x) >= std::abs(endPoint_.y - startPoint_.y);
    const int primarySize = horizontalTail ? width : height;
    const int tailDepth = (std::min)((std::max)(0, primarySize - 1),
                                     (std::min)(MAX_TAIL_DEPTH, (std::max)(MIN_TAIL_DEPTH, primarySize / 5)));

    Geometry result;
    result.TailTip = { startPoint_.x <= endPoint_.x ? x : x + width, startPoint_.y <= endPoint_.y ? y : y + height };
    if (horizontalTail) {
        const bool pointsRight = startPoint_.x <= endPoint_.x;
        result.Side = pointsRight ? TailSide::LEFT : TailSide::RIGHT;
        result.BubbleRect = pointsRight ? Gdiplus::Rect(x + tailDepth, y, (std::max)(1, width - tailDepth), height)
                                        : Gdiplus::Rect(x, y, (std::max)(1, width - tailDepth), height);
        const int cornerRadius = (std::min)(CORNER_RADIUS, result.BubbleRect.Height / 3);
        const int baseHalfWidth
            = (std::min)(TAIL_BASE_HALF_WIDTH, (std::max)(0, (result.BubbleRect.Height - cornerRadius * 2) / 2));
        const int attachmentY = ClampValue(startPoint_.y, result.BubbleRect.Y + cornerRadius + baseHalfWidth,
                                           result.BubbleRect.GetBottom() - cornerRadius - baseHalfWidth);
        const int attachmentX = pointsRight ? result.BubbleRect.X : result.BubbleRect.GetRight();
        result.TailBaseStart = { attachmentX, attachmentY - baseHalfWidth };
        result.TailBaseEnd = { attachmentX, attachmentY + baseHalfWidth };
    } else {
        const bool pointsDown = startPoint_.y <= endPoint_.y;
        result.Side = pointsDown ? TailSide::TOP : TailSide::BOTTOM;
        result.BubbleRect = pointsDown ? Gdiplus::Rect(x, y + tailDepth, width, (std::max)(1, height - tailDepth))
                                       : Gdiplus::Rect(x, y, width, (std::max)(1, height - tailDepth));
        const int cornerRadius = (std::min)(CORNER_RADIUS, result.BubbleRect.Width / 3);
        const int baseHalfWidth
            = (std::min)(TAIL_BASE_HALF_WIDTH, (std::max)(0, (result.BubbleRect.Width - cornerRadius * 2) / 2));
        const int attachmentX = ClampValue(startPoint_.x, result.BubbleRect.X + cornerRadius + baseHalfWidth,
                                           result.BubbleRect.GetRight() - cornerRadius - baseHalfWidth);
        const int attachmentY = pointsDown ? result.BubbleRect.Y : result.BubbleRect.GetBottom();
        result.TailBaseStart = { attachmentX - baseHalfWidth, attachmentY };
        result.TailBaseEnd = { attachmentX + baseHalfWidth, attachmentY };
    }
    return result;
}

void SpeechBaloon::addSpeechBaloonPath(Gdiplus::GraphicsPath& path, const Geometry& geometry, int radius) {
    const Gdiplus::Rect& rect = geometry.BubbleRect;
    radius = (std::max)(0, (std::min)(radius, (std::min)(rect.Width, rect.Height) / 2));
    const int diameter = radius * 2;
    const int left = rect.X;
    const int top = rect.Y;
    const int right = rect.GetRight();
    const int bottom = rect.GetBottom();

    path.StartFigure();
    path.AddLine(left + radius, top, geometry.Side == TailSide::TOP ? geometry.TailBaseStart.X : right - radius, top);
    if (geometry.Side == TailSide::TOP) {
        path.AddLine(geometry.TailBaseStart, geometry.TailTip);
        path.AddLine(geometry.TailTip, geometry.TailBaseEnd);
        path.AddLine(geometry.TailBaseEnd.X, top, right - radius, top);
    }
    if (radius > 0) {
        path.AddArc(right - diameter, top, diameter, diameter, 270.0f, 90.0f);
    }

    path.AddLine(right, top + radius, right,
                 geometry.Side == TailSide::RIGHT ? geometry.TailBaseStart.Y : bottom - radius);
    if (geometry.Side == TailSide::RIGHT) {
        path.AddLine(geometry.TailBaseStart, geometry.TailTip);
        path.AddLine(geometry.TailTip, geometry.TailBaseEnd);
        path.AddLine(right, geometry.TailBaseEnd.Y, right, bottom - radius);
    }
    if (radius > 0) {
        path.AddArc(right - diameter, bottom - diameter, diameter, diameter, 0.0f, 90.0f);
    }

    path.AddLine(right - radius, bottom, geometry.Side == TailSide::BOTTOM ? geometry.TailBaseEnd.X : left + radius,
                 bottom);
    if (geometry.Side == TailSide::BOTTOM) {
        path.AddLine(geometry.TailBaseEnd, geometry.TailTip);
        path.AddLine(geometry.TailTip, geometry.TailBaseStart);
        path.AddLine(geometry.TailBaseStart.X, bottom, left + radius, bottom);
    }
    if (radius > 0) {
        path.AddArc(left, bottom - diameter, diameter, diameter, 90.0f, 90.0f);
    }

    path.AddLine(left, bottom - radius, left, geometry.Side == TailSide::LEFT ? geometry.TailBaseEnd.Y : top + radius);
    if (geometry.Side == TailSide::LEFT) {
        path.AddLine(geometry.TailBaseEnd, geometry.TailTip);
        path.AddLine(geometry.TailTip, geometry.TailBaseStart);
        path.AddLine(left, geometry.TailBaseStart.Y, left, top + radius);
    }
    if (radius > 0) {
        path.AddArc(left, top, diameter, diameter, 180.0f, 90.0f);
    }
    path.CloseFigure();
}

}
