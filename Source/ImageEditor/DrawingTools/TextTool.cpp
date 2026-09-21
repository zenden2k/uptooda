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

#include "TextTool.h"

#include "../Canvas.h"
#include "../Document.h"
#include "../MovableElements.h"
#include "3rdpart/GdiplusH.h"

namespace ImageEditor {

TextTool::TextTool(Canvas* canvas) : MoveAndResizeTool(canvas, ElementType::etText) {
}

void TextTool::beginDraw(int x, int y) {
    allowCreatingElements_ = true;
    auto* textEl = dynamic_cast<TextElement*>(currentElement_);
    if (textEl && textEl->getInputBox()->isVisible()) {
        allowCreatingElements_ = false;
    }
    MoveAndResizeTool::beginDraw(x, y);
    allowCreatingElements_ = true;
    if (currentElement_) {
        auto* textElement = dynamic_cast<TextElement*>(currentElement_);

        if (textElement) {
            auto input = textElement->getInputBox();
            if (input) {
                textElement->beginEdit();
                input->show(true);
                canvas_->onTextEditStarted(textElement);
            }
        }
    }
}

void TextTool::continueDraw(int x, int y, DWORD flags) {
    MoveAndResizeTool::continueDraw(x, y, flags);
}

void TextTool::endDraw(int x, int y) {
    if (!currentElement_) {
        return;
    }

    int width = currentElement_->getWidth();
    int height = currentElement_->getHeight();
    if (elementJustCreated_ && width < 150) {
        width = 150;
    }

    if (height < 30) {
        height = 30;
    }
    currentElement_->resize(width, height);
    int elX = currentElement_->getX();
    int elY = currentElement_->getY();
    RECT inputRect = {elX + 3, elY + 3, elX + currentElement_->getWidth() - 6, elY + currentElement_->getHeight() - 6};

    auto* textElement = dynamic_cast<TextElement*>(currentElement_);
    std::shared_ptr<InputBox> inputBox = textElement ? textElement->getInputBox() : nullptr;
    if (!inputBox) {
        inputBox = canvas_->getInputBox(inputRect);
        textElement->setInputBox(inputBox);
        canvas_->onTextEditStarted(textElement);
    }
    //    currentElement_ = new TextElement(canvas_,inputBox, xStart,yStart, xEnd, yEnd);
    inputBox->show(true);
    textElement->setColor(foregroundColor_);
    canvas_->setCurrentlyEditedTextElement(textElement);
    textElement->setSelected(true);
    inputBox->invalidate();
    textElement->setDrawDashedRectangle(false);
    //currentElement_ = new TextElement(canvas_,inputBox, xStart,yStart, xEnd, yEnd);
    //canvas_->addMovableElement(currentElement_);
    //MoveAndResizeTool::endDraw( x, y);
    canvas_->updateView();
}

void TextTool::render(Painter* gr) {
}

ImageEditor::CursorType TextTool::getCursor(int x, int y) {
    CursorType ct = MoveAndResizeTool::getCursor(x, y);
    auto* textElement = dynamic_cast<TextElement*>(currentElement_);
    auto inputBox = textElement ? textElement->getInputBox() : nullptr;
    if ((ct == CursorType::ctDefault || (ct == CursorType::ctMove && canvas_->getElementAtPosition(x, y) !=
            currentElement_)) &&
        (!inputBox || !inputBox->isVisible())) {
        ct = CursorType::ctEdit;
    }
    return ct;
}

}
