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

#include "MoveAndResizeTool.h"

#include <cmath>
#include <cassert>

#include "../Canvas.h"
#include "../Document.h"
#include "../MovableElements.h"
#include "Core/Utils/CoreUtils.h"
#include "3rdpart/GdiplusH.h"

namespace ImageEditor {

bool PointsEqual(const POINT& a, const POINT& b) {
    return a.x == b.x && a.y == b.y;
}

CropOverlay* MoveAndResizeTool::cropOverlay_ = nullptr;

MoveAndResizeTool::MoveAndResizeTool(Canvas* canvas, ElementType type) : AbstractDrawingTool( canvas ) {
    currentElement_       = nullptr;
    elementType_          = type;
//    draggedBoundary_. = btNone;
    isMoving_ = false;
    allowCreatingElements_ = true;
    prevPaintBoundingRect_.left = -1;
    prevPaintBoundingRect_.right = -1;
    prevPaintBoundingRect_.top = -1;
    prevPaintBoundingRect_.bottom = -1;
    originalStartPoint_.x = -1;
    originalStartPoint_.y = -1;
    originalEndPoint_.x = -1;
    originalEndPoint_.y = -1;
    allowMovingElements_ = true;
    elementJustCreated_ = false;
    startPoint_.x = -1;
    startPoint_.y = -1;
}

void MoveAndResizeTool::beginDraw( int x, int y ) {
    elementJustCreated_ = false;
    if ( allowMovingElements_ ) {
        draggedBoundary_ = checkElementsBoundaries(x,y, &currentElement_);
        if ( draggedBoundary_.bt!= BoundaryType::btNone ) {
            canvas_->unselectAllElements();
            currentElement_->setSelected(true);
            canvas_->selectionChanged();
            currentElement_->beginMove();
            originalStartPoint_ = currentElement_->getStartPoint();
            originalEndPoint_ = currentElement_->getEndPoint();
            prevPaintBoundingRect_ = currentElement_->getPaintBoundingRect();
            return;
        }
        MovableElement* el = canvas_->getElementAtPosition(x, y, elementType_== ElementType::etCrop ? ElementType::etCrop : ElementType::etNone );
        if ( el && ( elementType_ == el->getType() ||  ( elementType_== ElementType::etNone  && el->getType()  != ElementType::etCrop )) ) {
            //currentElement_->setSelected(true);
            canvas_->unselectAllElements();
            el->setSelected(true);
            canvas_->selectionChanged();
            originalStartPoint_ = el->getStartPoint();
            originalEndPoint_ = el->getEndPoint();
            isMoving_ = true;

            currentElement_ = el;
            currentElement_->beginMove();
            prevPaintBoundingRect_ = currentElement_->getPaintBoundingRect();
            startPoint_.x = x;
            startPoint_.y = y;
            return;
        }
    }
    if ( elementType_== ElementType::etCrop  ) {
        canvas_->deleteElementsByType(elementType_);
    }

    if (currentElement_ && currentElement_->isEmpty()) {
        canvas_->unselectElement(currentElement_);
        canvas_->deleteMovableElement(currentElement_);
    }
    canvas_->unselectAllElements();

    if ( allowCreatingElements_ ) {
        POINT pt = { x, y };
        startPoint_.x = x;
        startPoint_.y = y;

        createElement();
        elementJustCreated_ = true;

        if ( currentElement_ ) {
            if (currentElement_->isResizable()) {
                currentElement_->setStartPoint( pt );
                currentElement_->setEndPoint(pt);
            }
            prevPaintBoundingRect_ = currentElement_->getPaintBoundingRect();
            canvas_->addMovableElement( currentElement_ );
            currentElement_->beginMove();

        }
        if ( !currentElement_ || currentElement_->getType() == ElementType::etCrop ) {
            //canvas_->updateView(); // update the whole canvas
        } else {
            canvas_->updateView(currentElement_->getPaintBoundingRect());
        }
    } else {

        currentElement_ = nullptr;
        if ( canvas_->unselectAllElements() ) {
            //canvas_->updateView();
        }
    }
}

void MoveAndResizeTool::continueDraw( int x, int y, DWORD flags ) {

    if ( currentElement_ && currentElement_->isResizable() && draggedBoundary_.bt!= BoundaryType::btNone ) {
        POINT* elementBasePoint = 0;
        if ( draggedBoundary_.gpt == MovableElement::GripPointType::gptStartPoint ) {
            elementBasePoint = &currentElement_->startPoint_;
        } else if ( draggedBoundary_.gpt == MovableElement::GripPointType::gptEndPoint ) {
            elementBasePoint = &currentElement_->endPoint_;
        }
        int elWidth  = 0;
        int elHeight = 0;
        int elX = 0;
        int elY  = 0;
        if ( elementBasePoint ) {
            elementBasePoint->x = x;
            elementBasePoint->y = y;
            elWidth = currentElement_->getWidth();
            elHeight = currentElement_->getHeight();
            elX = currentElement_->getX();
            elY  = currentElement_->getY();
        } else {
            elWidth = currentElement_->getWidth();
            elHeight = currentElement_->getHeight();
            elX = currentElement_->getX();
            elY  = currentElement_->getY();
            switch ( draggedBoundary_.bt ) {
                case BoundaryType::btBottomRight:
                    elWidth = x - elX+1;
                    elHeight = y - elY+1;
                    break;
                case BoundaryType::btBottom:
                    elHeight = y - elY+1;
                    break;
                case BoundaryType::btRight:
                    elWidth = x - elX+1;
                    break;
                case BoundaryType::btTopLeft:

                    elWidth =  elX - x + elWidth;
                    elHeight = elY - y + elHeight;
                    elX = x;
                    elY = y;
                    break;
                case BoundaryType::btLeft:

                    elWidth = elX - x + elWidth;
                    elX = x;
                    break;
                case BoundaryType::btTop:
                    elHeight =  elY - y + elHeight;
                    elY = y;
                    break;
                case BoundaryType::btBottomLeft:

                    elWidth = elX - x + elWidth;
                    elHeight = y - elY+1;
                    elX = x;
                    break;
                case BoundaryType::btTopRight:

                    elWidth =  x - elX+1;
                    elHeight = elY - y + elHeight;
                    elY = y;
            }


            currentElement_->setPos(elX, elY);
            if (draggedBoundary_.bt != BoundaryType::btNone) {
                currentElement_->resize(elWidth, elHeight);
            }
        }

        if (currentElement_->getType() == ElementType::etCrop) {
            canvas_->onCropChanged(currentElement_->getX(), currentElement_->getY(), currentElement_->getWidth(), currentElement_->getHeight());
        }
        RECT paintBoundingRect = currentElement_->getPaintBoundingRect();
        RECT updateRect;
        UnionRect(&updateRect, &paintBoundingRect, &prevPaintBoundingRect_);

        canvas_->updateView(updateRect);
        prevPaintBoundingRect_ = paintBoundingRect;

        return;
    }

    if ( isMoving_ && currentElement_ ) {
        int deltaX = x - startPoint_.x;
        int deltaY = y - startPoint_.y;
        startPoint_.x = x;
        startPoint_.y = y;

        if (currentElement_->move(deltaX, deltaY) ) {
            RECT paintBoundingRect = currentElement_->getPaintBoundingRect();
            RECT updateRect;
            UnionRect(&updateRect, &paintBoundingRect, &prevPaintBoundingRect_);
            canvas_->updateView(updateRect);
            if (currentElement_ && currentElement_->getType() == ElementType::etCrop) {
                canvas_->onCropChanged(currentElement_->getX(), currentElement_->getY(), currentElement_->getWidth(), currentElement_->getHeight());
            }
            prevPaintBoundingRect_ = currentElement_->getPaintBoundingRect();
        }

        return;
    }

    if ( currentElement_ ) {
        POINT pt = { x, y };
        if (currentElement_->isResizable()) {
            currentElement_->setEndPoint(pt);
        }
        RECT paintBoundingRect = currentElement_->getPaintBoundingRect();
        RECT updateRect;
        UnionRect(&updateRect, &paintBoundingRect, &prevPaintBoundingRect_);

        canvas_->updateView(updateRect);
        prevPaintBoundingRect_ = currentElement_->getPaintBoundingRect();
    }
}

void MoveAndResizeTool::endDraw( int x, int y ) {
    if ( currentElement_ ) {
        currentElement_->endMove();
        if ( currentElement_->getType() == ElementType::etCrop) {
            canvas_->onCropFinished(currentElement_->getX(), currentElement_->getY(), currentElement_->getWidth(), currentElement_->getHeight());
        }
        if ( currentElement_->getType() != ElementType::etCrop) {
            currentElement_->setDrawDashedRectangle(false);
        }

        POINT newStartPoint_ = currentElement_->getStartPoint();
        POINT newEndPoint_ = currentElement_->getEndPoint();

        if (!elementJustCreated_ && currentElement_->getType() != ElementType::etCrop &&
            (!PointsEqual(newStartPoint_, originalStartPoint_) || !PointsEqual(newEndPoint_, originalEndPoint_))) {

            auto uhi = std::make_unique<Canvas::UndoHistoryItem>();
            uhi->type = Canvas::UndoHistoryItemType::uitElementPositionChanged;
            Canvas::UndoHistoryItemElement uhie;
            uhie.startPoint = originalStartPoint_;
            uhie.endPoint = originalEndPoint_;
            uhie.movableElement = currentElement_;
            uhi->elements.push_back(uhie);
            canvas_->addUndoHistoryItem(std::move(uhi));
        }

        elementJustCreated_ = false;
        RECT paintBoundingRect = currentElement_->getPaintBoundingRect();
        RECT updateRect;
        UnionRect(&updateRect, &paintBoundingRect, &prevPaintBoundingRect_);
        currentElement_ = nullptr;
        prevPaintBoundingRect_.bottom = -1;
        prevPaintBoundingRect_.left = -1;
        prevPaintBoundingRect_.right = -1;
        prevPaintBoundingRect_.top = -1;
        canvas_->updateView(updateRect);
        //currentElement_->setSelected(true);

    }

    startPoint_.x = -1;
    startPoint_.y = -1;
    endPoint_.x   = -1;
    endPoint_.y   = -1;

    if ( draggedBoundary_.bt!= BoundaryType::btNone ) {

        draggedBoundary_.bt = BoundaryType::btNone;
        return;
    }
    if ( isMoving_ ) {
        isMoving_ = false;
    }
    canvas_->endManipulation();
}

void MoveAndResizeTool::render( Painter* gr ) {
    if ( currentElement_ ) {
        //currentElement_->render( gr );
    }
}

void MoveAndResizeTool::createElement() {
    //delete currentElement_;
    currentElement_ = 0;
    switch( elementType_ ) {
        case ElementType::etArrow:
            currentElement_ = new Arrow(canvas_, startPoint_.x,startPoint_.y, endPoint_.x, endPoint_.y, canvas_->getArrowMode());
            break;
        case ElementType::etLine:
            currentElement_ = new Line(canvas_, startPoint_.x,startPoint_.y, endPoint_.x, endPoint_.y);
            break;
        case ElementType::etText:
            currentElement_ = new TextElement(canvas_, 0, startPoint_.x, startPoint_.y, endPoint_.x, endPoint_.y, canvas_->getFillTextBackground());
            break;
        case ElementType::etSelection:
            currentElement_ = new Selection(canvas_, startPoint_.x,startPoint_.y, endPoint_.x, endPoint_.y);
            break;
        case ElementType::etCrop:
            /*if ( !cropOverlay_ ) {
                cropOverlay_ = new CropOverlay(canvas_, 0,0, canvas_->getWidth(), canvas_->getHeigth());

                atexit(&cleanUp);
            }*/
            canvas_->showOverlay(true);
            currentElement_ = new Crop(canvas_, 0, 0, 0, 0 );
            break;
        case ElementType::etRectangle:
            currentElement_ = new Rectangle(canvas_, startPoint_.x,startPoint_.y, endPoint_.x, endPoint_.y);
            break;
        case ElementType::etRoundedRectangle:
            currentElement_ = new RoundedRectangle(canvas_, startPoint_.x,startPoint_.y, endPoint_.x, endPoint_.y);
            currentElement_->setRoundingRadius(roundingRadius_);
            break;
        case ElementType::etFilledRoundedRectangle:
            currentElement_ = new FilledRoundedRectangle(canvas_, startPoint_.x, startPoint_.y, endPoint_.x, endPoint_.y, canvas_->getDrawBorder());
            currentElement_->setRoundingRadius(roundingRadius_);
            break;
        case ElementType::etEllipse:
            currentElement_ = new Ellipse(canvas_);
            break;
        case ElementType::etFilledEllipse:
            currentElement_ = new FilledEllipse(canvas_, canvas_->getDrawBorder());
            break;
        case ElementType::etPixelateRectangle:
            currentElement_ = new PixelateRectangle(canvas_, /*float(canvas_->getPixelateBlockSize())*/canvas_->getBlurRadius(), startPoint_.x, startPoint_.y, endPoint_.x, endPoint_.y, canvas_->getInvertSelection());
            break;
        case ElementType::etBlurringRectangle:
            currentElement_ = new BlurringRectangle(canvas_, canvas_->getBlurRadius(), startPoint_.x,startPoint_.y, endPoint_.x, endPoint_.y, false, canvas_->getInvertSelection());
            break;
        case ElementType::etFilledRectangle:
            currentElement_ = new FilledRectangle(canvas_, startPoint_.x,startPoint_.y, endPoint_.x, endPoint_.y, canvas_->getDrawBorder());
            break;
        case ElementType::etStepNumber:
            currentElement_ = new StepNumber(canvas_, startPoint_.x, startPoint_.y, endPoint_.x, endPoint_.y, canvas_->getNextNumber(), canvas_->getStepFontSize());
            break;
        /*default:
            LOG(ERROR) << "Unsupported element type";*/
    }
    if ( currentElement_ ) {
        currentElement_->setPenSize(penSize_);
        Gdiplus::Color fgColor = foregroundColor_;
        Gdiplus::Color bgColor = backgroundColor_;
        if (elementType_ == ElementType::etStepNumber && !canvas_->isStepColorSet()) {
            canvas_->setStepColors(fgColor, bgColor);
        }
        currentElement_->setColor(fgColor);
        currentElement_->setBackgroundColor(bgColor);
    }
}

MovableElement::Grip MoveAndResizeTool::checkElementsBoundaries( int x, int y, MovableElement** elem)
{
    std::vector<MovableElement*> cropElements;
    canvas_->getElementsByType(elementType_, cropElements);
    int count = cropElements.size();
    for( int i  = count-1; i>= 0; i-- ) {
        if ( !cropElements[i]->isSelected() && cropElements[i]->getType() != ElementType::etCrop ) {
            continue;
        }
        MovableElement::Grip grip = checkElementBoundaries(cropElements[i], x , y);
        if ( grip.bt != BoundaryType::btNone ) {
            if ( elem ) {
                *elem = cropElements[i];
            }
            return grip;
        }
    }

    return {};
}

MovableElement::Grip  MoveAndResizeTool::checkElementBoundaries(MovableElement* element, int x, int y)
{
    for (size_t i = 0; i < element->grips_.size(); i++) {
        if ( abs (x - element->grips_[i].pt.x) <= element->gripWidth_ + 2 &&  abs (y - element->grips_[i].pt.y) <= element->gripHeight_ + 2 ) {
            return element->grips_[i];
        }
    }
    return {};
}

void MoveAndResizeTool::cleanUp()
{
    delete cropOverlay_;
}

CursorType MoveAndResizeTool::getCursor(int x, int y)
{
    CursorType  ct = CursorType::ctDefault;

    switch( elementType_ ) {
        case ElementType::etArrow:
            //currentElement_ = new Line( 0, 0, 0, 0 );
            break;
        case ElementType::etCrop:
            ct = CursorType::ctCross;
            break;
    }
    MovableElement::Grip grip = checkElementsBoundaries( x, y);
    if ( grip.bt != BoundaryType::btNone ) {
        return MovableElement::GetCursorForBoundary(grip.bt);
    }
    MovableElement* el = canvas_->getElementAtPosition(x,y);
    if ( el &&  (el->getType() != ElementType::etCrop || elementType_ == ElementType::etCrop)  ) {

        return CursorType::ctMove;
    }
    return ct;
}

void MoveAndResizeTool::mouseDoubleClick(int x, int y)
{
    MovableElement* el = canvas_->getElementAtPosition(x,y);
    if ( el &&  el->getType() == ElementType::etText && elementType_ != ElementType::etText ) {
        // WARNING!!! After this line "this" is DELETED!!!!
        AbstractDrawingTool* dtool = canvas_->setDrawingToolType(DrawingToolType::dtText, true);
        if ( dtool ) {
            dtool->beginDraw(x,y);
        }

    }
}

}
