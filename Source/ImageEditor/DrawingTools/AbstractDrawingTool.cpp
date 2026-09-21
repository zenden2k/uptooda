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

#include "AbstractDrawingTool.h"

#include "../Canvas.h"
#include "../Document.h"

namespace ImageEditor {

using namespace Gdiplus;

AbstractDrawingTool::AbstractDrawingTool( Canvas *canvas ) {
    startPoint_.x = -1;
    startPoint_.y = -1;
    endPoint_.x   = -1;
    endPoint_.y   = -1;
    //assert( canvas );
    canvas_ = canvas;
    penSize_ = 1;
    roundingRadius_ = penSize_;
}

void AbstractDrawingTool::beginDraw( int x, int y ) {
    startPoint_.x = x;
    startPoint_.y = y;
}

void AbstractDrawingTool::endDraw( int x, int y ) {
    endPoint_.x = x;
    endPoint_.y = y;
}

void AbstractDrawingTool::mouseDoubleClick(int x, int y)
{

}

CursorType AbstractDrawingTool::getCursor(int x, int y)
{
    return CursorType::ctDefault;
}

void AbstractDrawingTool::rightButtonClick(int x, int y)
{

}

void AbstractDrawingTool::setPenSize(int size)
{
    penSize_ = size;
}

void AbstractDrawingTool::setRoundingRadius(int radius)
{
    roundingRadius_  = radius;
}

void AbstractDrawingTool::setForegroundColor(Gdiplus::Color color)
{
    foregroundColor_ = color;
}

void AbstractDrawingTool::setBackgroundColor(Gdiplus::Color color)
{
    backgroundColor_ = color;
}

bool AbstractDrawingTool::applyOperation() {
    return false;
}

void AbstractDrawingTool::cancelOperation() {

}

}
