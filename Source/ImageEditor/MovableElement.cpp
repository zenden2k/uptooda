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

#include "MovableElement.h"

#include "3rdpart/GdiplusH.h"
#include "Canvas.h"

namespace ImageEditor {

using namespace Gdiplus;

MovableElement::MovableElement(Canvas* canvas){
    drawDashedRectangle_ = true;
    startPoint_.x = 0;
    startPoint_.y = 0;
    endPoint_.x   = 0;
    endPoint_.y   = 0;

    color_ = Gdiplus::Color( 0, 0, 0 );
    penSize_ = 1;
    isSelected_ = false;
    auto [dpiX, dpiY] = canvas->getDpi();
    gripWidth_ = dpiX * kGripSize;
    gripHeight_ = dpiY * kGripSize;
    grips_.resize(8);
    canvas_ = canvas;
    isMoving_ = false;
    isPenSizeUsed_ = true;
    isBackgroundColorUsed_ = true;
    isColorUsed_ = true;
    drawDashedRectangleWhenSelected_ = false;
}

void MovableElement::render(Painter* gr)
{
}

void MovableElement::renderGrips(Painter* gr)
{
    Gdiplus::Pen pen( Color( 0, 0, 0) );
    //Gdiplus::SolidBrush brush(Color( 255, 255, 255) );
    pen.SetDashStyle(DashStyleDash);
    REAL dashValues[2] = {4, 4};
    pen.SetDashPattern(dashValues, 2);
    //pen.SetBrush(&brush);
    int x = getX();
    int y = getY();
    int width = getWidth()-1;
    int height = getHeight()-1;
    if ( (isSelected_  && drawDashedRectangleWhenSelected_ )|| drawDashedRectangle_ ) {
        gr->DrawRectangle( &pen, x, y, width, height );
        pen.SetColor(Color( 255, 255, 255) );
        pen.SetDashOffset(3);
        REAL dashValues2[2] = {4, 4};
        pen.SetDashPattern(dashValues2, 2);
        gr->DrawRectangle( &pen, x, y, width, height );
    }

    if ( (isSelected_ || getType() == ElementType::etCrop) && isResizable() ) {
        int rectSizeX = gripWidth_;
        int rectSizeY = gripHeight_;
        int halfSizeX = rectSizeX /2 ;
        int halfSizeY = rectSizeY / 2;
        Gdiplus::Pen pen2( Color( 255,255, 255) );
        //pen.SetDashStyle(DashStyleDash);
        /*int x = getX();
        int y = getY();
        int width = getWidth();
        int height = getHeight();*/
        Gdiplus::SolidBrush brush(Color( 10, 10, 10) );

        createGrips();


        for (size_t i = 0; i < grips_.size(); i++) {
            int x = grips_[i].pt.x;
            int y = grips_[i].pt.y;
            gr->FillRectangle( &brush, x-halfSizeX, y-halfSizeY, rectSizeX, rectSizeY );
            gr->DrawRectangle( &pen2, x-halfSizeX-1, y-halfSizeY-1, rectSizeX+1, rectSizeY+1 );
        }
    }
}

void MovableElement::setSelected(bool selected)
{
    isSelected_ = selected;
}

bool MovableElement::isSelected() const
{
    return isSelected_;
}

ElementType MovableElement::getType() const
{
    return ElementType::etUnknown;
}

CursorType MovableElement::GetCursorForBoundary(BoundaryType bt)
{
    switch (bt) {
        case BoundaryType::btTopLeft:
        case BoundaryType::btBottomRight:
            return CursorType::ctResizeDiagonalMain;
        case BoundaryType::btTopRight:
        case BoundaryType::btBottomLeft:
            return CursorType::ctResizeDiagonalAnti;
        case BoundaryType::btTop:
        case BoundaryType::btBottom:
            return CursorType::ctResizeVertical;
        case BoundaryType::btLeft:
        case BoundaryType::btRight:
            return CursorType::ctResizeHorizontal;
        default:
            return CursorType::ctDefault;
    }
}

void MovableElement::setDrawDashedRectangle(bool draw)
{
    drawDashedRectangle_ = draw;
}

int MovableElement::getX()
{
    return getMinPoint(axisX)->x;
}

int MovableElement::getY()
{
    return getMinPoint(axisY)->y;
}

RECT MovableElement::getPaintBoundingRect()
{
    int radius = (std::max<int>)(penSize_, gripWidth_);
    if ( canvas_->hasBlurRectangles() ) {
        radius += static_cast<int>(canvas_->getBlurRadius());
    }
    RECT res = { getX()-radius, getY()-radius, getWidth()+radius*2, getHeight()+radius*2};
    res.right += res.left;
    res.bottom += res.top;
    return res;
}

void MovableElement::setPos(int x, int y) {
    int width = getWidth();
    int height = getHeight();
    int canvasWidth = canvas_->getWidth();
    int canvasHeight = canvas_->getHeigth();


    if (x <= 0) {
        x = 0;
    }
    else if (x + width  >  canvasWidth) {
        x = canvasWidth - width;
    }
    if (y < 0) {
        y = 0;
    }
    else if (y + height  > canvasHeight) {
        y = canvasHeight - height;
    }

    getMinPoint(axisX)->x = x;
    getMinPoint(axisY)->y = y;
    width = getWidth();
    height = getHeight();
    resize(width, height);
}

bool MovableElement::move(int offsetX, int offsetY, bool checkBounds) {
    int canvasWidth = canvas_->getWidth();
    int canvasHeight = canvas_->getHeigth();
    int x = getX();
    int y = getY();
    int right = x + getWidth();
    int bottom = y + getHeight();
    int newX = x + offsetX;
    int newY = y + offsetY;
    int newRight = right + offsetX;
    int newBottom = bottom + offsetY;

    if (checkBounds) {
        if ((newX < 0 || newX >= canvasWidth) && (newRight < 0 || newRight >= canvasWidth)) {
            offsetX = 0;
        }
        if ((newY < 0 || newY >= canvasHeight) && (newBottom < 0 || newBottom >= canvasHeight)) {
            offsetY = 0;
        }
    }

    if (offsetX == 0 && offsetY == 0) {
        return false;
    }
    startPoint_.x += offsetX;
    endPoint_.x += offsetX;
    startPoint_.y += offsetY;
    endPoint_.y += offsetY;
    return true;
}

bool MovableElement::isItemAtPos(int x, int y)
{
    int elementX = getX();
    int elementY = getY();
    int elementWidth = getWidth();
    int elementHeight = getHeight();
    return  ( x+kSelectRadius >= elementX && x  <= elementX + elementWidth + kSelectRadius && y+kSelectRadius>= elementY && y <= elementY + elementHeight+kSelectRadius );
}

void MovableElement::resize(int width, int height)
{
    if ( width  < 1 ) {
        width = 1;
    }
    if ( height < 1 ) {
        height  = 1;
    }
    getMaxPoint(axisX)->x = width-1 + getX();
    getMaxPoint(axisY)->y = height-1 + getY();
}

void MovableElement::createGrips()
{
    int x = getX();
    int y = getY();
    int width = getWidth()-1;
    int height = getHeight()-1;

    // item order is important!!!!
    POINT pts[8] = {
    {x+width,y+height},  {x+width/2, y+height}, {x,y+height},{x+width,y+height/2}, {x,y+height/2}, {x,y}, {x + width / 2, y}, {x+width,y},
    };

    for( int i = 0; i < 8; i++ ) {
        int x = pts[i].x;
        int y = pts[i].y;
        Grip grip;
        grip.pt.x = x;
        grip.pt.y = y;
        if ( x == startPoint_.x && y == startPoint_.y ) {
            grip.gpt = GripPointType::gptStartPoint;
        } else if ( x == endPoint_.x &&   y == endPoint_.y ) {
            grip.gpt = GripPointType::gptEndPoint;
        }
        grip.bt = static_cast<BoundaryType>(i);
        grips_[i] = grip;
    }
}

void MovableElement::beginMove()
{
    isMoving_ = true;
}

void MovableElement::endMove()
{
    isMoving_ = false;
}

bool MovableElement::isMoving() const {
    return isMoving_;
}

bool MovableElement::isPenSizeUsed() const
{
    return isPenSizeUsed_;
}

bool MovableElement::isColorUsed() const
{
    return isColorUsed_;
}

bool MovableElement::isBackgroundColorUsed() const
{
    return isBackgroundColorUsed_;
}

bool MovableElement::isResizable() const {
    return true;
}

bool MovableElement::isEmpty() const {
    return false;
}

POINT* MovableElement::getMaxPoint(Axis axis)
{
    if ( axis == axisX ) {
        return endPoint_.x > startPoint_.x ? &endPoint_ : &startPoint_;
    } else {
        return endPoint_.y > startPoint_.y ? &endPoint_ : &startPoint_;
    }
}

POINT* MovableElement::getMinPoint(Axis axis)
{
    if ( axis == axisX ) {
        return startPoint_.x < endPoint_.x ? &startPoint_ : &endPoint_;
    } else {
        return startPoint_.y < endPoint_.y ? &startPoint_ : &endPoint_;
    }
}

void MovableElement::setDrawBorder(bool enable) {
    drawBorder_ = enable;
}

bool MovableElement::getDrawBorder() const {
    return drawBorder_;
}

}
