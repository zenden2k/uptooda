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

#include "ColorPickerTool.h"

#include "../Canvas.h"
#include "../Document.h"

#include "3rdpart/GdiplusH.h"

namespace ImageEditor {

ColorPickerTool::ColorPickerTool(Canvas* canvas) :AbstractDrawingTool(canvas)
{

}

void ColorPickerTool::beginDraw(int x, int y)
{

}

void ColorPickerTool::continueDraw(int x, int y, DWORD flags)
{

}

void ColorPickerTool::endDraw(int x, int y)
{
    bool oldValue = canvas_->getCropOnExport();
    canvas_->setCropOnExport(false);
    auto bmp = canvas_->getBitmapForExport();
    canvas_->setCropOnExport(oldValue);
    if (bmp) {
        Gdiplus::Color color;
        if (bmp->GetPixel(x, y, &color) == Gdiplus::Ok) {
            canvas_->setForegroundColor(color);
            canvas_->onForegroundColorChanged(color);
            canvas_->setPreviousDrawingTool();
            // On this line "this" object has been already destroyed
        }
    }
}

void ColorPickerTool::render(Painter* gr)
{

}

CursorType ColorPickerTool::getCursor(int x, int y)
{
    return CursorType::ctColorPicker;
}

void ColorPickerTool::rightButtonClick(int x, int y)
{
    bool oldValue = canvas_->getCropOnExport();
    canvas_->setCropOnExport(false);
    auto bmp = canvas_->getBitmapForExport();
    canvas_->setCropOnExport(oldValue);
    if (bmp) {
        Gdiplus::Color color;
        if (bmp->GetPixel(x, y, &color) == Gdiplus::Ok) {
            canvas_->setBackgroundColor(color);

            canvas_->onBackgroundColorChanged(color);
            canvas_->setPreviousDrawingTool();
            // On this line "this" object has been already destroyed
        }
    }
}

}
