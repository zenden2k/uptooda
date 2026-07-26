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

#include "Canvas.h"

#include <cassert>
#include <algorithm>

#include "DrawingElement.h"
#include "Document.h"
#include "DrawingTool.h"
#include "InputBox.h"
#include "MovableElements.h"
#include "ImageEditor/Gui/InputBoxControl.h"
#include "Core/Logging.h"
#include "3rdpart/GdiplusH.h"
#include "Core/i18n/Translator.h"

namespace ImageEditor {

Canvas::Canvas( HWND parent ) {
    parentWindow_            = parent;
    oldPoint_.x           = -1;
    oldPoint_.y           = -1;
    callback_             = nullptr;
    drawingToolType_      = DrawingToolType::dtNone;
    previousDrawingTool_ = DrawingToolType::dtNone;
    leftMouseDownPoint_.x = -1;
    leftMouseDownPoint_.y = -1;
    leftMouseUpPoint_ = { -1, -1 };
//    buffer_               = NULL;
    inputBox_             = nullptr;
    currentCursor_    = CursorType::ctDefault;
    scrollOffset_.x = 0;
    scrollOffset_.y = 0;
    overlay_ = nullptr;
    showOverlay_ = false;
    zoomFactor_ = 1;
    currentlyEditedTextElement_ = nullptr;
    foregroundColor_ = Gdiplus::Color(255,0,0);
    backgroundColor_ = Gdiplus::Color(255,255,255);
    stepColorsSet_ = false;
    penSize_ = 12;
    originalPenSize_ = 0;
    roundingRadius_ = 12;
    originalRoundingRadius_ = 0;
    originalBlurRadius_ = 0;
    selection_ = nullptr;
    canvasChanged_ = true;
    fullRender_ = true;
    blurRadius_ = 1.0f;
    blurRectanglesCount_ = 0;
    currentDrawingTool_ = nullptr;
    isDocumentModified_ = false;
    memset(&font_, 0, sizeof(font_));
    doc_ = nullptr;
    canvasWidth_ = 0;
    canvasHeight_ = 0;
    nextNumber_ = 1;
    stepFontSize_ = kDefaultStepFontSize;
    stepInitialValue_ = 1;
    fillTextBackground_ = false;
    cropOnExport_ = true;
    invertSelection_ = false;
    arrowMode_ = Arrow::ArrowMode::Mode1;
    manipulationStarted_ = false;
    dpiX_ = 1.0f;
    dpiY_ = 1.0f;
    createDoubleBuffer();
}

Canvas::~Canvas() {
//    delete buffer_;
    delete currentDrawingTool_;
    delete overlay_;
    for (size_t i = 0; i < elementsToDelete_.size(); i++) {
        delete elementsToDelete_[i];
    }
}

void Canvas::setSize( int x, int y ) {
    if ( x < 1 || x > 10000) {
        x = 1;
    }
    if ( y <1 || y > 10000) {
        y = 1;
    }
    canvasWidth_ = x;
    canvasHeight_ = y;
    createDoubleBuffer();
}

Document* Canvas::currentDocument() const {
    return doc_;
}

void Canvas::setDocument( Document *doc ) {
    doc_ = doc;
    updateView();
}

void Canvas::mouseMove( int x, int y, DWORD flags) {
    bool isLButtonDown = flags & MK_LBUTTON;
    POINT point        = { x, y };
    /*if ( x > canvasWidth_ || y > canvasHeight_ || x <0 || y <0) {
        return;
    }*/
    if ( isLButtonDown  ) {
        assert( currentDrawingTool_ );

        currentDrawingTool_->continueDraw( x,  y, flags );
    }
    CursorType ct = currentDrawingTool_->getCursor(x ,y);
    setCursor(ct);
    /*if (isLButtonDown && currentElement_ != NULL ) {
        AffectedSegments prevSegments;
        currentElement_->getAffectedSegments( &prevSegments );
        CRgn prevRegion = prevSegments.createRegionFromSegments();
        currentElement_->setEndPoint( point );
        AffectedSegments imageSegments;
        currentElement_->getAffectedSegments( &imageSegments );
        CRgn affectedRegion = imageSegments.createRegionFromSegments();
        affectedRegion.CombineRgn( prevRegion, RGN_OR );
        updateView( affectedRegion );
    }
    */

    /*if ( isLButtonDown && oldPoint_.x != -1 && currentDrawingTool_ == dtPen ) {

        int xStart = oldPoint_.x;
        int yStart = oldPoint_.y;
        int xEnd   = point.x;
        int yEnd   = point.y;

        ImageEditor::Line line( xStart, yStart, xEnd, yEnd );
        doc_->addDrawingElement( &line );
        RECT updateRect =  { std::min<>( xStart, xEnd), std::min<>( yStart, yEnd ),
                                    std::max<>( xStart, xEnd) + 1, std::max<>( yStart, yEnd) + 1 };
        updateView( updateRect );

    } else if ( currentDrawingTool_ == dtLine ) {

    }*/
    oldPoint_ = point;
}

void Canvas::mouseDown( int button, int x, int y ) {
    leftMouseDownPoint_.x = x;
    leftMouseDownPoint_.y = y;
    if ( !currentDrawingTool_ ) {
        return;
    }
    currentDrawingTool_->beginDraw( x, y );
    /*if ( currentElement_ == NULL) {
        if ( currentDrawingTool_ == dtLine) {
            currentElement_ = new Line(x, y, x, y);
        } else if ( currentDrawingTool_ == dtRectangle ) {
            currentElement_ = new Rectangle(x, y, x, y);
        }
    }*/
}

void Canvas::mouseUp( int button, int x, int y ) {
    if ( !currentDrawingTool_ ) {
        return;
    }
    if ( button == 0 ) {
        currentDrawingTool_->endDraw( x, y );
    } else {
        currentDrawingTool_->rightButtonClick(x,y);
    }
    /*if (currentDrawingTool_ != dtPen ) {
        if ( currentElement_ != NULL) {
            doc_->addDrawingElement( currentElement_ );
        }
        currentElement_ = NULL;
        updateView();
    }
    currentElement_ = 0;*/
}

void Canvas::mouseDoubleClick(int button, int x, int y)
{
    if ( !currentDrawingTool_ ) {
        return;
    }

    currentDrawingTool_->mouseDoubleClick( x, y );
}

void Canvas::render(HDC dc, const RECT& rectInWindowCoordinates, POINT scrollOffset, SIZE size) {
    using namespace Gdiplus;
    scrollOffset_ = scrollOffset;
    // Updating rect in canvas coordinates
    RECT rect = {rectInWindowCoordinates.left+scrollOffset.x, rectInWindowCoordinates.top+scrollOffset.y,
        /*size.cx*/rectInWindowCoordinates.right - rectInWindowCoordinates.left, /*size.cy*/rectInWindowCoordinates.bottom - rectInWindowCoordinates.top};
    rect.right += rect.left;
    rect.bottom += rect.top;
    /*if ( fullRender_ ) {
    /*    rect.left = 0;
        rect.right = 0;
        rect.bottom = getHeigth();
        rect.right = getWidth();*
    } */{
        RECT canvasRect = {0,0, getWidth(), getHeigth()};
        IntersectRect(&rect, &rect, &canvasRect); // need to check rect dimensions, otherwise renderInBuffer() may fail
        if ( rect.right - rect.left == 0 || rect.bottom - rect.top == 0 ) {
            return;
        }
    }

    if ( canvasChanged_ || fullRender_ ) {
        renderInBuffer(updatedRect_);
    }
    BitmapData bitmapData;
    Rect lockRect(0,0, getWidth(),getHeigth());
    // I hope Gdiplus does not copy data in LockBits
    if ( buffer_->LockBits(&lockRect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) == Ok ) {
        uint8_t * source = (uint8_t *) bitmapData.Scan0;
        unsigned int stride;
        if ( bitmapData.Stride > 0) {
            stride = bitmapData.Stride;
        } else {
            stride = - bitmapData.Stride;
        }
        BITMAPINFO bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biCompression = BI_RGB;

        bi.bmiHeader.biWidth = buffer_->GetWidth();
        bi.bmiHeader.biHeight = -static_cast<LONG>(buffer_->GetHeight());
        bi.bmiHeader.biBitCount = 32;
        // Faster than Graphics::DrawImage and there is no tearing!
        /*int res =*/ SetDIBitsToDevice (dc, rectInWindowCoordinates.left,rectInWindowCoordinates.top,rect.right - rect.left, rect.bottom - rect.top, rect.left, rect.top, rect.top, rect.bottom - rect.top, source + rect.top * stride, &bi, DIB_RGB_COLORS );
        buffer_->UnlockBits(&bitmapData);
    }
    //gr->DrawImage( &*buffer_, rectInWindowCoordinates.left, rectInWindowCoordinates.top, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, UnitPixel );

}

Gdiplus::Rect Canvas::currentRenderingRect() const
{
    return currentRenderingRect_;
}

bool Canvas::isRoundingRectangleSelected() const
{
    for (auto el : elementsOnCanvas_) {
        if (el->isSelected() && (el->getType() == ElementType::etRoundedRectangle || el->getType() == ElementType::etFilledRoundedRectangle ) ) {
            return true;
        }
    }
    return false;
}

bool Canvas::isDocumentModified() const
{
    return isDocumentModified_;
}

void Canvas::setDocumentModified(bool modified)
{
    isDocumentModified_ = modified;
    onDocumentModified();
}

void  Canvas::setCallback(Callback * callback) {
    callback_ = callback;
}

void Canvas::setPenSize(int size) {
    if ( size < 1 || size > kMaxPenSize ) {
        return;
    }
    penSize_ = size;
    if ( currentDrawingTool_ ) {
        currentDrawingTool_->setPenSize(size);
    }
    for (auto& el : elementsOnCanvas_) {
        if (el->isSelected() && el->isPenSizeUsed() ) {
            RECT paintRect = el->getPaintBoundingRect();
            el->setPenSize(size);
            RECT newPaintRect = el->getPaintBoundingRect();
            UnionRect(&paintRect, &paintRect, &newPaintRect);
            updateView(paintRect);
        }
    }

}

int Canvas::getPenSize() const
{
    return penSize_;
}

void Canvas::beginPenSizeChanging()
{
    if ( !originalPenSize_ ) {
        originalPenSize_ = penSize_;
    }
}

void Canvas::endPenSizeChanging(int penSize) {
    penSize_ = penSize;
    if ( originalPenSize_ == 0 || originalPenSize_ == penSize_ ) {
        return ;
    }
    int updatedElementsCount = 0;
    auto uhi = std::make_unique<UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitPenSizeChanged;
    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if ( elementsOnCanvas_[i]->isSelected()) {
            UndoHistoryItemElement uhie;
            uhie.penSize = originalPenSize_;
            RECT paintRect = elementsOnCanvas_[i]->getPaintBoundingRect();
            uhie.movableElement = elementsOnCanvas_[i];
            elementsOnCanvas_[i]->setPenSize(penSize);
            RECT newPaintRect = elementsOnCanvas_[i]->getPaintBoundingRect();
            uhi->elements.push_back(uhie);
            UnionRect(&paintRect, &paintRect, &newPaintRect);
            updatedElementsCount++;
            updateView(paintRect);
        }
    }
    if ( updatedElementsCount ) {
        addUndoHistoryItem(std::move(uhi));
    }
    originalPenSize_= 0;
}


void Canvas::setRoundingRadius(int radius) {
    if ( radius < 1 || radius > kMaxRoundingRadius ) {
        return;
    }
    roundingRadius_ = radius;
    if ( currentDrawingTool_ ) {
        currentDrawingTool_->setRoundingRadius(radius);
    }
    for (const auto& el: elementsOnCanvas_) {
        if (el->isSelected() && el->isPenSizeUsed() ) {
            RECT paintRect = el->getPaintBoundingRect();
            el->setRoundingRadius(radius);
            RECT newPaintRect = el->getPaintBoundingRect();
            UnionRect(&paintRect, &paintRect, &newPaintRect);
            updateView(paintRect);
        }
    }

}

int Canvas::getRoundingRadius() const
{
    return roundingRadius_;
}

void Canvas::beginRoundingRadiusChanging()
{
    if ( !originalRoundingRadius_ ) {
        originalRoundingRadius_ = roundingRadius_;
    }
}

void Canvas::endRoundingRadiusChanging(int radius) {
    roundingRadius_ = radius;
    if ( originalRoundingRadius_ == 0 || originalRoundingRadius_ == roundingRadius_ ) {
        return ;
    }
    int updatedElementsCount = 0;
    auto uhi = std::make_unique<UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitRoundingRadiusChanged;
    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if ( elementsOnCanvas_[i]->isSelected() && (elementsOnCanvas_[i] ->getType() == ElementType::etRoundedRectangle || elementsOnCanvas_[i]->getType() == ElementType::etFilledRoundedRectangle)) {
            UndoHistoryItemElement uhie;
            uhie.penSize = originalRoundingRadius_;
            RECT paintRect = elementsOnCanvas_[i]->getPaintBoundingRect();
            uhie.movableElement = elementsOnCanvas_[i];
            elementsOnCanvas_[i]->setRoundingRadius(radius);
            RECT newPaintRect = elementsOnCanvas_[i]->getPaintBoundingRect();
            uhi->elements.push_back(uhie);
            UnionRect(&paintRect, &paintRect, &newPaintRect);
            updatedElementsCount++;
            updateView(paintRect);
        }
    }
    if ( updatedElementsCount ) {
        addUndoHistoryItem(std::move(uhi));
    }
    originalRoundingRadius_ = 0;
}

void Canvas::setForegroundColor(Gdiplus::Color color)
{
    if (drawingToolType_ == DrawingToolType::dtStepNumber) {
        stepForegroundColor_ = color;
    } else {
        foregroundColor_ = color;
    }
    if ( currentDrawingTool_ ) {
        currentDrawingTool_->setForegroundColor(color);
    }
    int updatedElementsCount = 0;
    auto uhi = std::make_unique<UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitElementForegroundColorChanged;
    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if ( elementsOnCanvas_[i]->isSelected() && elementsOnCanvas_[i]->isColorUsed() ) {
            UndoHistoryItemElement uhie;
            uhie.color = elementsOnCanvas_[i]->getColor();
            uhie.pos = i;
            uhie.movableElement = elementsOnCanvas_[i];
            elementsOnCanvas_[i]->setColor(color);
            if ( elementsOnCanvas_[i]->getType() != ElementType::etText ) { // TextElements saves it's color by itself
                uhi->elements.push_back(uhie);
                updatedElementsCount++;
            }
        }
    }
    if ( updatedElementsCount ) {
        addUndoHistoryItem(std::move(uhi));
        updateView();
    }
}

void Canvas::setBackgroundColor(Gdiplus::Color color)
{
    if (drawingToolType_ == DrawingToolType::dtStepNumber) {
        stepBackgroundColor_ = color;
    }
    else {
        backgroundColor_ = color;
    }

    if ( currentDrawingTool_ ) {
        currentDrawingTool_->setBackgroundColor(color);
    }
    int updatedElementsCount = 0;
    auto uhi = std::make_unique<UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitElementBackgroundColorChanged;
    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if ( elementsOnCanvas_[i]->isSelected() && elementsOnCanvas_[i]->isBackgroundColorUsed()) {
            UndoHistoryItemElement uhie;
            uhie.color = elementsOnCanvas_[i]->getBackgroundColor();
            uhie.pos = i;
            uhie.movableElement = elementsOnCanvas_[i];
            elementsOnCanvas_[i]->setBackgroundColor(color);
            uhi->elements.push_back(uhie);
            updatedElementsCount++;
        }
    }
    if ( updatedElementsCount ) {
        addUndoHistoryItem(std::move(uhi));
        updateView();
    }
}

Gdiplus::Color Canvas::getForegroundColor() const
{
    return foregroundColor_;
}

Gdiplus::Color Canvas::getBackgroundColor() const
{
    return backgroundColor_;
}

Gdiplus::Color Canvas::getStepForegroundColor() const {
    return stepForegroundColor_;
}

Gdiplus::Color Canvas::getStepBackgroundColor() const {
    return stepBackgroundColor_;
}

void Canvas::setStepColors(Gdiplus::Color fgColor, Gdiplus::Color bgColor) {
    stepForegroundColor_ = fgColor;
    stepBackgroundColor_ = bgColor;
    stepColorsSet_ = true;
}

bool Canvas::isStepColorSet() const {
    return stepColorsSet_;
}

void Canvas::setFont(LOGFONT font, DWORD changeMask)
{
    font_ = font;
    /*if ( currentDrawingTool_ ) {
        currentDrawingTool_->setForegroundColor(color);
    }*/
/*    UndoHistoryItem uhi;
    uhi.type = uitFontChanged;*/
    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if ( elementsOnCanvas_[i]->isSelected() && elementsOnCanvas_[i]->getType() == ElementType::etText ) {
            /*UndoHistoryItemElement uhie;
            uhie.color = elementsOnCanvas_[i]->getColor();
            uhie.pos = i;
            uhie.movableElement = elementsOnCanvas_[i];*/
            auto textItem = dynamic_cast<TextElement*>(elementsOnCanvas_[i]);
            if (textItem) {
                textItem->setFont(font, changeMask);
            }

            /*uhi.elements.push_back(uhie);
            updatedElementsCount++;*/
        }
    }
    /*if ( updatedElementsCount ) {
        addUndoHistoryItem(uhi);
        updateView();
    }*/
}

LOGFONT Canvas::getFont() const
{
    return font_;
}

AbstractDrawingTool* Canvas::setDrawingToolType(DrawingToolType toolType, bool notify ) {
    if ( drawingToolType_ == toolType ) {
        return currentDrawingTool_;
    }
    previousDrawingTool_ = drawingToolType_;
    drawingToolType_ = toolType;
    if ( toolType != DrawingToolType::dtColorPicker ) {
        unselectAllElements();
    }

    //showOverlay(toolType == dtCrop );

    delete currentDrawingTool_;
    currentDrawingTool_ = nullptr;
    if ( toolType == DrawingToolType::dtPen) {
        currentDrawingTool_ = new PenTool( this );
    } else if ( toolType == DrawingToolType::dtBrush) {
        currentDrawingTool_ = new BrushTool( this );
    } else if ( toolType == DrawingToolType::dtMarker) {
        currentDrawingTool_ = new MarkerTool( this );
    }else if ( toolType == DrawingToolType::dtBlur) {
        #if GDIPVER >= 0x0110
        currentDrawingTool_ = new BlurTool( this );
        #else
        LOG(ERROR) << "Blur effect is not supported by current version of GdiPlus.";
        #endif
    }else if ( toolType == DrawingToolType::dtColorPicker) {
        currentDrawingTool_ = new ColorPickerTool( this );
    }else if ( toolType == DrawingToolType::dtText) {
        currentDrawingTool_ = new TextTool( this );
    } else if ( toolType == DrawingToolType::dtCrop ) {
        currentDrawingTool_ = new CropTool( this );
        showOverlay(true);
    }
    else {
        ElementType type;
        bool createVectorTool = true;
        if ( toolType == DrawingToolType::dtLine ) {
            type = ElementType::etLine;
        } else if ( toolType == DrawingToolType::dtArrow ) {
            type = ElementType::etArrow;
        } else if ( toolType == DrawingToolType::dtRectangle ) {
            type = ElementType::etRectangle;
        } else if ( toolType == DrawingToolType::dtBlurringRectangle ) {
            type = ElementType::etBlurringRectangle;
        } else if (toolType == DrawingToolType::dtPixelateRectangle) {
            type = ElementType::etPixelateRectangle;
        } else if ( toolType == DrawingToolType::dtFilledRectangle ) {
            type = ElementType::etFilledRectangle;
        } else if ( toolType == DrawingToolType::dtRoundedRectangle ) {
            type = ElementType::etRoundedRectangle;
        } else if ( toolType == DrawingToolType::dtEllipse ) {
            type = ElementType::etEllipse;
        } else if ( toolType == DrawingToolType::dtFilledRoundedRectangle ) {
            type = ElementType::etFilledRoundedRectangle;
        } else if ( toolType == DrawingToolType::dtFilledEllipse ) {
            type = ElementType::etFilledEllipse;
        }
        else if (toolType == DrawingToolType::dtStepNumber) {
            type = ElementType::etStepNumber;
        }
        else if ( toolType == DrawingToolType::dtMove ) {
            currentDrawingTool_ = new MoveAndResizeTool( this, ElementType::etNone );
            createVectorTool = false;
            /*updateView();
            return currentDrawingTool_;*/
        } else if ( toolType == DrawingToolType::dtSelection ) {
            currentDrawingTool_ = new SelectionTool( this );
            createVectorTool = false;
            /*updateView();
            return currentDrawingTool_;*/
        }
        else {
            LOG(ERROR) << "createElement for toolType=" << static_cast<int>(toolType) << " not implemented.";
            return nullptr;
        }

        if (createVectorTool) {
            currentDrawingTool_ = new VectorElementTool(this, type);
        }

        //currentDrawingTool_ = new VectorElementTool( this, type );
    }
    Gdiplus::Color fgColor = foregroundColor_;
    Gdiplus::Color bgColor = backgroundColor_;

    if (toolType == DrawingToolType::dtStepNumber) {
        if (stepColorsSet_) {
            fgColor = stepForegroundColor_;
            bgColor = stepBackgroundColor_;

            onForegroundColorChanged(fgColor);
            onBackgroundColorChanged(bgColor);
        }
    } else if (previousDrawingTool_ == DrawingToolType::dtStepNumber) {
        onForegroundColorChanged(fgColor);
        onBackgroundColorChanged(bgColor);
    }

    currentDrawingTool_->setPenSize(penSize_);
    currentDrawingTool_->setRoundingRadius(roundingRadius_);
    currentDrawingTool_->setForegroundColor(fgColor);
    currentDrawingTool_->setBackgroundColor(bgColor);
    if ( notify ) {
        onDrawingToolChanged(toolType);
    }

    updateView();
    return currentDrawingTool_;
}

void Canvas::setPreviousDrawingTool()
{
    if ( previousDrawingTool_ != DrawingToolType::dtNone ) {
        setDrawingToolType(previousDrawingTool_, true);
    }
}

AbstractDrawingTool* Canvas::getCurrentDrawingTool() const
{
    return currentDrawingTool_;
}

void Canvas::addMovableElement(MovableElement* element)
{
    if ( element->getType() == ElementType::etSelection ) {
        delete selection_;
        selection_ = element;
        return;
    }

    auto it = std::find(elementsOnCanvas_.begin(), elementsOnCanvas_.end(), element);
    if (it == elementsOnCanvas_.end()) {

        elementsOnCanvas_.push_back(element);
        if ( element->getType() == ElementType::etBlurringRectangle || element->getType() == ElementType::etPixelateRectangle) {
            blurRectanglesCount_ ++;
        }
        if (element->getType() != ElementType::etCrop) {
            auto historyItem = std::make_unique<UndoHistoryItem>();
            historyItem->type = UndoHistoryItemType::uitElementAdded;
            UndoHistoryItemElement uhie;
            uhie.pos = elementsOnCanvas_.size();
            uhie.movableElement = element;
            historyItem->elements.push_back(uhie);
            addUndoHistoryItem(std::move(historyItem));
            setDocumentModified(true);
        }

        elementsToDelete_.push_back(element);
    }
}

bool Canvas::addDrawingElementToDoc(DrawingElement* element)
{
    currentDocument()->addDrawingElement(element);
    return true;
}

void Canvas::beginDocDrawing() {
    beginManipulation();
    currentDocument()->beginDrawing();
}

void Canvas::endDocDrawing()
{
    currentDocument()->endDrawing();
    auto historyItem = std::make_unique<UndoHistoryItem>();
    historyItem->type = UndoHistoryItemType::uitDocumentChanged;
    addUndoHistoryItem(std::move(historyItem));
}

void Canvas::beginManipulation() {
    manipulationStarted_ = true;
}

void Canvas::endManipulation() {
    manipulationStarted_ = false;
}

bool Canvas::manipulationStarted() const {
    return manipulationStarted_;
}

int Canvas::deleteSelectedElements()
{
    int deletedCount = 0;
    auto uhi = std::make_unique<UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitElementRemoved;
    bool isStepNumberRemoved = false;

    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if ( elementsOnCanvas_[i]->isSelected() && elementsOnCanvas_[i]->getType() != ElementType::etCrop ) {
            UndoHistoryItemElement uhie;
            uhie.movableElement = elementsOnCanvas_[i];
            uhie.pos = i;
            uhi->elements.push_back(uhie );
            elementsOnCanvas_.erase(elementsOnCanvas_.begin() + i);
            if (uhie.movableElement->getType() == ElementType::etStepNumber) {
                isStepNumberRemoved = true;
            }
            i--;
            deletedCount++;
        }
    }
    if ( deletedCount ) {
        addUndoHistoryItem(std::move(uhi));
        if (isStepNumberRemoved) {
            recalcStepNumbers();
        }
        updateView();
        selectionChanged();
    }
    return deletedCount;
}

float Canvas::getBlurRadius() const
{
    return blurRadius_;
}

void Canvas::setBlurRadius(float radius)
{
    blurRadius_ = radius;
}

void Canvas::beginBlurRadiusChanging() {
    if (originalBlurRadius_ == 0) {
        originalBlurRadius_ = blurRadius_;
    }
}

void Canvas::endBlurRadiusChanging(float radius) {
    blurRadius_ = radius;
    if (fabs(originalBlurRadius_) < 0.0001 || fabs(originalBlurRadius_ - blurRadius_) < 0.0001) {
        return;
    }
    int updatedElementsCount = 0;
    auto uhi = std::make_unique<Canvas::UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitBlurRadiusChanged;
    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if (elementsOnCanvas_[i]->isSelected() && (elementsOnCanvas_[i]->getType() == ElementType::etBlurringRectangle || elementsOnCanvas_[i]->getType() == ElementType::etPixelateRectangle)) {
            auto el = dynamic_cast<BlurringRectangle*>(elementsOnCanvas_[i]);
            if (el) {
                UndoHistoryItemElement uhie;
                uhie.floatVal = originalBlurRadius_;
                RECT paintRect = elementsOnCanvas_[i]->getPaintBoundingRect();
                uhie.movableElement = elementsOnCanvas_[i];
                el->setBlurRadius(radius);
                RECT newPaintRect = elementsOnCanvas_[i]->getPaintBoundingRect();
                uhi->elements.push_back(uhie);
                UnionRect(&paintRect, &paintRect, &newPaintRect);
                updatedElementsCount++;
                updateView(paintRect);
            }
        }
    }
    if (updatedElementsCount) {
        addUndoHistoryItem(std::move(uhi));
    }
    originalBlurRadius_ = 0;
}

bool Canvas::hasBlurRectangles() const
{
    return blurRectanglesCount_!=0;
}

void Canvas::showOverlay(bool show)
{
    if (show && !overlay_) {
        overlay_ = new CropOverlay(this, 0,0, getWidth(),getHeigth());
    }
    showOverlay_ = show;
    updateView();
}

void Canvas::selectionChanged()
{
    onSelectionChanged();
}

void Canvas::deleteMovableElement(MovableElement* element)
{
    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if ( elementsOnCanvas_[i] == element ) {

            elementsOnCanvas_.erase(elementsOnCanvas_.begin() + i);
            if ( element->getType() == ElementType::etBlurringRectangle || element->getType() == ElementType::etPixelateRectangle ) {
                blurRectanglesCount_--;
            }
            if ( element->getType() == ElementType::etCrop && (drawingToolType_ != DrawingToolType::dtCrop ) ) {
                showOverlay(false);
            }
            if (element->getType() == ElementType::etStepNumber) {
                recalcStepNumbers();
            }
            //delete element;
            break;
        }
    }
}

void Canvas::updateView() {
    fullRender_ = true;
    RECT rc = { 0, 0, getWidth(), getHeigth() };
    updateView( rc );
}
/*
void Canvas::updateView( const CRgn& region ) {
    canvasChanged_ = true;
    if ( callback_ ) {
        callback_->updateView( this, region );
    }
}*/

void Canvas::updateView( RECT boundingRect ) {
    using namespace Gdiplus;

    Rect newRect(std::max<int>(0,boundingRect.left), std::max<int>(0,boundingRect.top), boundingRect.right - boundingRect.left, boundingRect.bottom - boundingRect.top );
    newRect.Width = min(canvasWidth_ - newRect.X, newRect.Width);
    newRect.Height = min(canvasHeight_ - newRect.Y, newRect.Height);
    if ( newRect.Width <=0 || newRect.Height <=0 ) {
        return;
    }
    canvasChanged_ = true;

    if ( updatedRect_.IsEmptyArea() ) {
        updatedRect_ = newRect;
    } else {
        Rect::Union(updatedRect_,newRect,updatedRect_);
    }

    if ( callback_ ) {
        callback_->updateView(this, updatedRect_, fullRender_);
    }
}

POINT Canvas::GetScrollOffset() const {
    return scrollOffset_;
}

void Canvas::createDoubleBuffer() {
//    delete buffer_;
    buffer_ = std::make_shared<Gdiplus::Bitmap>(canvasWidth_, canvasHeight_, PixelFormat32bppARGB);
    bufferedGr_ = std::make_unique<Gdiplus::Graphics>( buffer_.get() );
    bufferedGr_->Clear(Gdiplus::Color::Transparent);
}

void Canvas::setCursor(CursorType cursorType)
{
    if ( currentCursor_ == cursorType ) {
        return;
    }

    currentCursor_ = cursorType ;
}


void Canvas::renderInBuffer(Gdiplus::Rect rc,bool forExport)
{
    using namespace Gdiplus;
    currentRenderingRect_ = rc;
    if (!fullRender_ && !forExport) {
        Gdiplus::Region reg(rc);
        bufferedGr_->SetClip(&reg);
    } else {
        /* Gdiplus::Region reg;
        bufferedGr_->SetClip(&reg);
        */
        bufferedGr_->ResetClip();
    }
    //LOG(INFO) << "renderInBuffer " << rc.X << " " << rc.Y << " " << rc.Width << " " <<rc.Height << " forExport=" << forExport;
    bufferedGr_->SetPageUnit(Gdiplus::UnitPixel);
    bufferedGr_->SetSmoothingMode(SmoothingModeAntiAlias);

    if ( doc_->hasTransparentPixels() ) {
        if (  !forExport ) {
            SolidBrush whiteBrush(Color(255,255,255));
            bufferedGr_->FillRectangle(&whiteBrush, rc);
            /*int kSquareSize = 40;
            SolidBrush dark(Color(50,50,50));
            SolidBrush light(Color(100,100,100));
            int startX = rc.X - rc.X % kSquareSize;
            int startY = rc.Y - rc.Y % kSquareSize;
            int xCount = ceil(float(rc.Width) / kSquareSize)+1;
            int yCount = ceil(float(rc.Height) / kSquareSize)+1;
            bool isDark =  !(rc.Y / kSquareSize)%2 ;
            isDark =  (rc.X / kSquareSize)%2 == ( isDark ? 0 : 1);
            for (int j = 0; j < yCount; j++) {
                for (int i = 0; i < xCount; i++)
                {
                    bufferedGr_->FillRectangle(isDark ? &dark : &light, startX + i * kSquareSize, startY + j * kSquareSize, kSquareSize, kSquareSize);
                    isDark = !isDark;
                }
                isDark = !isDark;
            }*/
        } else {
            bufferedGr_->Clear(Color(0,0,0,0));
        }
    }

    doc_->render( bufferedGr_.get(), rc );

    if ( currentDrawingTool_ != NULL ) {
        currentDrawingTool_->render( bufferedGr_.get() );
    }

    /*if ( !fullRender_ ) {
            for ( int i=0; i< elementsOnCanvas_.size(); i++) {
                if ( elementsOnCanvas_[i]->getType() != etBlurringRectangle ) {
                    continue;
                }
                RECT paintRect = elementsOnCanvas_[i]->getPaintBoundingRect();
                RECT intersection;
                IntersectRect(&intersection, &paintRect, &rect);
                if ( !(intersection.left == 0 && intersection.right ==0 && intersection.top == 0 && intersection.bottom == 0) ) {
                    UnionRect(&rect, &rect, &paintRect);
                }
            }
        }*/

    for (size_t i = 0; i< elementsOnCanvas_.size(); i++) {
        RECT paintRect = elementsOnCanvas_[i]->getPaintBoundingRect();
        RECT intersection;
        RECT rect = { rc.X, rc.Y, rc.GetRight(), rc.GetBottom()};
        IntersectRect(&intersection, &paintRect, &rect);
        if ( !fullRender_ && intersection.left == 0 && intersection.right ==0 && intersection.top == 0 && intersection.bottom == 0 ) {
            continue;
        }
        elementsOnCanvas_[i]->render(bufferedGr_.get());
    }
    if ( !forExport ) {
        if ( overlay_ && showOverlay_ ) {
            overlay_->render(bufferedGr_.get());
        }

        for (auto& el : elementsOnCanvas_) {
            el->renderGrips(bufferedGr_.get());
        }
    }
    canvasChanged_ = false;
    fullRender_ = false;
    updatedRect_ = Rect();
    bufferedGr_->ResetClip();
}

void Canvas::getElementsByType(ElementType elementType, std::vector<MovableElement*>& out) const
{
    int count = elementsOnCanvas_.size();
    for ( int i = 0; i < count; i++ ) {
        if ( elementType == ElementType::etNone || elementsOnCanvas_[i]->getType() == elementType ) {
            out.push_back(elementsOnCanvas_[i]);
        }
    }
}

void Canvas::setZoomFactor(float zoomFactor)
{
    zoomFactor_ = zoomFactor;
}

Gdiplus::Bitmap* Canvas::getBufferBitmap() const
{
    return buffer_.get();
}

void Canvas::addUndoHistoryItem(std::unique_ptr<UndoHistoryItem> item)
{
    undoHistory_.push(std::move(item));
    setDocumentModified(true);
}

std::shared_ptr<Gdiplus::Bitmap> Canvas::getBitmapForExport()
{
    if (currentlyEditedTextElement_) {
        unselectAllElements();
    }
    using namespace Gdiplus;
    Rect rc(0,0, getWidth(), getHeigth());
    fullRender_ = true;
    renderInBuffer(rc, true);
    Crop * crop = nullptr;

    if (cropOnExport_) {
        // Find first Crop element
        for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
            if (elementsOnCanvas_[i]->getType() == ElementType::etCrop) {
                crop = dynamic_cast<Crop*>(elementsOnCanvas_[i]);
                break;
            }
        }
    }

    if ( !crop )  {
        return buffer_;
    }

    int cropX = crop->getX();
    int cropY = crop->getY();
    int cropWidth = crop->getWidth();
    int cropHeight = crop->getHeight();

    auto bm = std::make_shared<Bitmap>(cropWidth, cropHeight);
    Graphics gr(bm.get());
    gr.DrawImage( buffer_.get(), 0, 0, cropX, cropY, cropWidth, cropHeight, UnitPixel );
    lastAppliedCrop_ = Rect(cropX, cropY, cropWidth, cropHeight);
    return bm;
}

float Canvas::getZoomFactor() const
{
    return zoomFactor_;
}

MovableElement* Canvas::getElementAtPosition(int x, int y, ElementType et)
{
    int count = elementsOnCanvas_.size();
    for ( int i = count-1; i >=0 ; i-- ) {
        if ( elementsOnCanvas_[i]->getType() != ElementType::etCrop && ( et == ElementType::etNone || et == elementsOnCanvas_[i]->getType()) ) {
            if ( elementsOnCanvas_[i]->isItemAtPos(x,y) ) {
                return  elementsOnCanvas_[i];
            }
        }
    }

    for ( int i = count-1; i >=0; i-- ) {
        if ( elementsOnCanvas_[i]->getType() == ElementType::etCrop ) {
            int elementX = elementsOnCanvas_[i]->getX();
            int elementY = elementsOnCanvas_[i]->getY();
            int elementWidth = elementsOnCanvas_[i]->getWidth();
            int elementHeight = elementsOnCanvas_[i]->getHeight();
            if ( x >= elementX && x <= elementX + elementWidth && y>= elementY && y <= elementY + elementHeight ) {
                return  elementsOnCanvas_[i];
            }
        }
    }


    return nullptr;
}

int Canvas::deleteElementsByType(ElementType elementType)
{
    int count = 0;
    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if ( elementsOnCanvas_[i]->getType() == elementType ) {
            elementsOnCanvas_.erase(elementsOnCanvas_.begin() + i);
            i--;
            count++;
        }
    }
    if ( count ) {
        selectionChanged();
    }
    return count;
}

void Canvas::deleteAllElements() {
    for (const auto& it: elementsOnCanvas_) {
        delete it;
    }
    elementsOnCanvas_.clear();
}

int Canvas::getWidth() const
{
    return canvasWidth_;
}

int Canvas::getHeigth() const
{
    return canvasHeight_;
}

CursorType Canvas::getCursor() const
{
    return currentCursor_;
}

bool Canvas::undo() {
    if ( undoHistory_.empty() ) {
        return false;
    }
    bool result = false;
    auto& item = undoHistory_.top();

    result = undoItem(*item.get());

    if ( result ) {
        undoHistory_.pop();
        setDocumentModified(true);
    }
    updateView();
    return result;
}

bool Canvas::undoItem(UndoHistoryItem& item) {
    bool result = false;
    if (item.type == UndoHistoryItemType::uitMultipleChanges) {
        for (auto& change : item.changes) {
            result = undoItem(*change.get()) || result;
        }
        setSize(doc_->getWidth(), doc_->getHeight());
        if (callback_) {
            callback_->canvasSizeChanged();
        }
    } else if (item.type == UndoHistoryItemType::uitDocumentChanged) {
        result = doc_->undo();
    } else if (item.type == UndoHistoryItemType::uitCropApplied) {
        result = doc_->undo();
        canvasWidth_ = doc_->getWidth();
        canvasHeight_ = doc_->getHeight();
        createDoubleBuffer();

        POINT p = item.elements[0].startPoint;

        for (auto& el : elementsOnCanvas_) {
            el->move(p.x, p.y, false);
        }
        if (callback_) {
            callback_->canvasSizeChanged();
        }
    } else if (item.type == UndoHistoryItemType::uitElementAdded) {
        for (size_t i = 0; i < item.elements.size(); i++) {
            deleteMovableElement(item.elements[i].movableElement);
        }
        result = true;
    } else if (item.type == UndoHistoryItemType::uitElementRemoved) {
        int itemCount = item.elements.size();
        // Insert elements in their initial positions
        for (int i = itemCount - 1; i >= 0; i--) {
            elementsOnCanvas_.insert(elementsOnCanvas_.begin() + item.elements[i].pos, item.elements[i].movableElement);
            if (item.elements[i].movableElement->getType() == ElementType::etBlurringRectangle
                || item.elements[i].movableElement->getType() == ElementType::etPixelateRectangle) {
                blurRectanglesCount_++;
            }
        }
        recalcStepNumbers();
        result = true;
    } else if (item.type == UndoHistoryItemType::uitElementForegroundColorChanged) {
        int itemCount = item.elements.size();
        for (int i = itemCount - 1; i >= 0; i--) {
            item.elements[i].movableElement->setColor(item.elements[i].color);
        }
        result = true;
    } else if (item.type == UndoHistoryItemType::uitElementBackgroundColorChanged) {
        int itemCount = item.elements.size();
        for (int i = itemCount - 1; i >= 0; i--) {
            item.elements[i].movableElement->setBackgroundColor(item.elements[i].color);
        }
        result = true;
    } else if (item.type == UndoHistoryItemType::uitPenSizeChanged) {
        int itemCount = item.elements.size();
        for (int i = itemCount - 1; i >= 0; i--) {
            item.elements[i].movableElement->setPenSize(item.elements[i].penSize);
        }
        result = true;
    } else if (item.type == UndoHistoryItemType::uitRoundingRadiusChanged) {
        int itemCount = item.elements.size();
        for (int i = itemCount - 1; i >= 0; i--) {
            if (item.elements[i].movableElement->getType() == ElementType::etFilledRoundedRectangle || item.elements[i].movableElement->getType() == ElementType::etRoundedRectangle) {
                item.elements[i].movableElement->setRoundingRadius(item.elements[i].penSize);
            }
        }
        result = true;
    } else if (item.type == UndoHistoryItemType::uitBlurRadiusChanged) {
        int itemCount = item.elements.size();
        for (int i = itemCount - 1; i >= 0; i--) {
            if (item.elements[i].movableElement->getType() == ElementType::etBlurringRectangle || item.elements[i].movableElement->getType() == ElementType::etPixelateRectangle) {
                auto blurRectangleItem = dynamic_cast<BlurringRectangle*>(item.elements[i].movableElement);
                if (blurRectangleItem) {
                    blurRectangleItem->setBlurRadius(item.elements[i].floatVal);
                }
            }
        }
        result = true;
    } else if (item.type == UndoHistoryItemType::uitElementPositionChanged) {
        int itemCount = item.elements.size();
        // Insert elements in their initial positions
        for (int i = itemCount - 1; i >= 0; i--) {
            MovableElement* el = item.elements[i].movableElement;
            el->setStartPoint(item.elements[i].startPoint);
            el->setEndPoint(item.elements[i].endPoint);
            if (el->getType() == ElementType::etCrop) {
                onCropChanged(el->getX(), el->getY(), el->getWidth(), el->getHeight());
            }
        }
        result = true;
    } else if (item.type == UndoHistoryItemType::uitTextChanged) {
        int itemCount = item.elements.size();
        for (int i = itemCount - 1; i >= 0; i--) {
            dynamic_cast<TextElement*>(item.elements[i].movableElement)->setRawText(item.elements[i].rawText);
        }
        result = true;
    } else if (item.type == UndoHistoryItemType::uitFillBackgroundChanged) {
        int itemCount = item.elements.size();

        for (int i = itemCount - 1; i >= 0; i--) {
            dynamic_cast<TextElement*>(item.elements[i].movableElement)->setFillBackground(item.elements[i].penSize);
        }
        result = true;
    } else if (item.type == UndoHistoryItemType::uitInvertSelectionChanged) {
        int itemCount = item.elements.size();
        for (int i = itemCount - 1; i >= 0; i--) {
            dynamic_cast<BlurringRectangle*>(item.elements[i].movableElement)->setInvertSelection(item.elements[i].penSize);
        }
    } else if (item.type == UndoHistoryItemType::uitDrawBorderChanged) {
        int itemCount = item.elements.size();
        for (int i = itemCount - 1; i >= 0; i--) {
            item.elements[i].movableElement->setDrawBorder(item.elements[i].penSize);
        }
    }
    return result;
}

void Canvas::rotate(Gdiplus::RotateFlipType angle) {
    if (angle == Gdiplus::RotateNoneFlipNone) {
        return;
    }
    bool oldCropValue = cropOnExport_;
    setCropOnExport(false);
    std::shared_ptr<Gdiplus::Bitmap> bmp = getBitmapForExport();
    bool justFlip = (angle == Gdiplus::RotateNoneFlipX || angle == Gdiplus::RotateNoneFlipY
        || angle == Gdiplus::RotateNoneFlipXY);

    bmp->RotateFlip(angle);
    bmp = std::shared_ptr<Gdiplus::Bitmap>(bmp->Clone(0, 0, bmp->GetWidth(), bmp->GetHeight(), PixelFormat32bppARGB));
    doc_->updateBitmap(bmp);

    // Deleting all elements on canvas
    int deletedCount = 0;
    auto uhi = std::make_unique<UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitElementRemoved;
    bool isStepNumberRemoved = false;

    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if (!justFlip || elementsOnCanvas_[i]->getType() != ElementType::etCrop)
        {
            UndoHistoryItemElement uhie;
            uhie.movableElement = elementsOnCanvas_[i];
            uhie.pos = i;
            uhi->elements.push_back(std::move(uhie));
            elementsOnCanvas_.erase(elementsOnCanvas_.begin() + i);
            i--;
            deletedCount++;
        }
    }

    auto historyItem = std::make_unique<UndoHistoryItem>();
    historyItem->type = UndoHistoryItemType::uitDocumentChanged;

    auto multiItem = std::make_unique<UndoHistoryItem>();
    multiItem->type = UndoHistoryItemType::uitMultipleChanges;
    if (deletedCount) {
        multiItem->changes.push_back(std::move(uhi));
    }
    multiItem->changes.push_back(std::move(historyItem));

    addUndoHistoryItem(std::move(multiItem));
    if (!justFlip) {
        showOverlay(false);
        setSize(bmp->GetWidth(), bmp->GetHeight());
        if (callback_) {
            callback_->canvasSizeChanged();
        }
    }

    if (deletedCount) {
        selectionChanged();
    }
    updateView();
    setCropOnExport(oldCropValue);
}

std::shared_ptr<InputBox> Canvas::getInputBox( const RECT& rect ) {
    inputBox_ = std::make_shared<InputBoxControl>(this);
    RECT rc = rect;
    rc.left++;
    rc.top++;

    rc.top -= scrollOffset_.y;
    rc.left -= scrollOffset_.x;
    DWORD rtlStyle = ServiceLocator::instance()->translator()->isRTL() ? (WS_EX_LAYOUTRTL | WS_EX_RTLREADING) : 0;
    /*HWND wnd =*/ inputBox_->Create( parentWindow_, rc, _T(""), WS_CHILD |ES_MULTILINE|/*ES_AUTOHSCROLL|*/ES_AUTOVSCROLL|  ES_WANTRETURN
        | ES_NOHIDESEL /*| ES_LEFT */, WS_EX_TRANSPARENT | rtlStyle);

    inputBox_->SetWindowPos(HWND_TOP,0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
    inputBox_->setFont(font_,  CFM_FACE | CFM_SIZE | CFM_CHARSET
        | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_OFFSET);
    inputBox_->SetFocus();
    return inputBox_;
}

TextElement* Canvas::getCurrentlyEditedTextElement() const
{
    return currentlyEditedTextElement_;
}

void Canvas::setCurrentlyEditedTextElement(TextElement* textElement)
{
    currentlyEditedTextElement_ = textElement;
}

int Canvas::unselectAllElements()
{
    size_t count = 0;
    for (size_t i = 0; i < elementsOnCanvas_.size(); i++) {
        if ( elementsOnCanvas_[i]->isSelected() ) {
            elementsOnCanvas_[i]->setSelected(false);
            updateView(elementsOnCanvas_[i]->getPaintBoundingRect());
            count++;
        }
    }
    selectionChanged();
    return count;
}

bool Canvas::unselectElement(MovableElement* element) {
    if (element->isSelected()) {
        element->setSelected(false);
        updateView(element->getPaintBoundingRect());
        return true;
    }
    selectionChanged();
    return false;
}

HWND Canvas::getRichEditControl() const
{
    return inputBox_?inputBox_->m_hWnd : 0;
}

int Canvas::getNextNumber() {
    return nextNumber_++;
}

void Canvas::recalcStepNumbers() {
    nextNumber_ = stepInitialValue_;
    int count = 0;
    for (const auto& el : elementsOnCanvas_) {
        if (el->getType() == ElementType::etStepNumber) {
            auto* stepNumberElement = dynamic_cast<StepNumber*>(el);
            if (stepNumberElement) {
                stepNumberElement->setNumber(nextNumber_++);
                count++;
            }
        }
    }
    if (count) {
        updateView();
    }
}

void Canvas::setStepInitialValue(int value) {
    stepInitialValue_ = value;
    recalcStepNumbers();
}

Gdiplus::Graphics* Canvas::getGraphicsDevice() const {
    return bufferedGr_.get();
}

Gdiplus::Rect Canvas::lastAppliedCrop() const {
    return lastAppliedCrop_;
}

Gdiplus::Rect Canvas::lastCrop() const {
    for (const auto& el: elementsOnCanvas_) {
        if (el->getType() == ElementType::etCrop) {
            auto crop = dynamic_cast<Crop*>(el);
            if (crop) {
                return Gdiplus::Rect(crop->getX(), crop->getY(), crop->getWidth(), crop->getHeight());
            }
        }
    }
    return lastAppliedCrop();
}

void Canvas::setStepFontSize(int fontSize) {
    stepFontSize_ = fontSize;
    for ( const auto& el: elementsOnCanvas_) {
        if (el->getType() == ElementType::etStepNumber) {
            auto stepItem = dynamic_cast<StepNumber*>(el);
            if (stepItem) {
                stepItem->setFontSize(fontSize);
            }
        }
    }
    updateView();
}

int Canvas::getStepFontSize() const {
    return stepFontSize_;
}

void Canvas::setFillTextBackground(bool fill) {
    fillTextBackground_ = fill;
    int count = 0;
    auto uhi = std::make_unique<UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitFillBackgroundChanged;

    for (auto& el: elementsOnCanvas_) {
        if (el->isSelected() && el->getType() == ElementType::etText) {
            auto* textEl = dynamic_cast<TextElement*>(el);
            if (textEl) {
                bool prev = textEl->getFillBackground();
                if (prev == fill) {
                    continue;
                }
                textEl->setFillBackground(fill);

                UndoHistoryItemElement uhie;
                uhie.pos = 0;
                uhie.movableElement = el;
                uhie.penSize = prev;
                uhi->elements.push_back(uhie);
                count++;
            }
        }
    }
    if (count) {
        addUndoHistoryItem(std::move(uhi));
        updateView();
    }
}

bool Canvas::getFillTextBackground() const {
    return fillTextBackground_;
}

void Canvas::setInvertSelection(bool invert) {
    invertSelection_ = invert;
    int count = 0;
    auto uhi = std::make_unique<UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitInvertSelectionChanged;

    for (auto& el : elementsOnCanvas_) {
        if (el->isSelected() && (el->getType() == ElementType::etBlurringRectangle || el->getType() == ElementType::etPixelateRectangle)) {
            auto* blurEl = dynamic_cast<BlurringRectangle*>(el);
            if (blurEl) {
                bool prev = blurEl->getInvertSelection();
                if (prev == invert) {
                    continue;
                }
                blurEl->setInvertSelection(invert);

                UndoHistoryItemElement uhie;
                uhie.pos = 0;
                uhie.movableElement = el;
                uhie.penSize = prev;
                uhi->elements.push_back(uhie);
                count++;
            }
        }
    }
    if (count) {
        addUndoHistoryItem(std::move(uhi));
        updateView();
    }
}

bool Canvas::getInvertSelection() const {
    return invertSelection_;
}

void Canvas::setDrawBorder(bool enable) {
    drawBorder_ = enable;
    int count = 0;
    auto uhi = std::make_unique<UndoHistoryItem>();
    uhi->type = UndoHistoryItemType::uitDrawBorderChanged;

    for (auto& el : elementsOnCanvas_) {
        if (el->isSelected() && (el->getType() == ElementType::etFilledRectangle || el->getType() == ElementType::etFilledRoundedRectangle
            || el->getType() == ElementType::etEllipse)) {
            auto* blurEl = dynamic_cast<Rectangle*>(el);
            if (blurEl) {
                bool prev = blurEl->getDrawBorder();
                if (prev == enable) {
                    continue;
                }
                blurEl->setDrawBorder(enable);

                UndoHistoryItemElement uhie;
                uhie.pos = 0;
                uhie.movableElement = el;
                uhie.penSize = prev;
                uhi->elements.push_back(uhie);
                count++;
            }
        }
    }
    if (count) {
        addUndoHistoryItem(std::move(uhi));
        updateView();
    }
}

bool Canvas::getDrawBorder() const {
    return drawBorder_;
}

void Canvas::setArrowMode(Arrow::ArrowMode arrowMode) {
    arrowMode_ = arrowMode;
}

Arrow::ArrowMode Canvas::getArrowMode() const {
    return arrowMode_;
}

void Canvas::applyCurrentOperation() {
    currentDrawingTool_->applyOperation();
}

void Canvas::cancelCurrentOperation() {
    currentDrawingTool_->cancelOperation();
}

void Canvas::applyCrop(Crop* crop) {
    using namespace Gdiplus;
    int cropX = crop->getX();
    int cropY = crop->getY();
    int cropWidth = crop->getWidth();
    int cropHeight = crop->getHeight();

    doc_->applyCrop(cropX, cropY, cropWidth, cropHeight);

    auto bm = std::make_shared<Bitmap>(cropWidth, cropHeight);
    Graphics gr(bm.get());
    gr.DrawImage(buffer_.get(), 0, 0, cropX, cropY, cropWidth, cropHeight, UnitPixel);
    lastAppliedCrop_ = Rect(cropX, cropY, cropWidth, cropHeight);

    auto historyItem = std::make_unique<UndoHistoryItem>();
    historyItem->type = UndoHistoryItemType::uitCropApplied;
    UndoHistoryItemElement element;
    element.startPoint = { cropX , cropY };
    element.endPoint = { cropX + cropWidth, cropY + cropHeight };
    historyItem->elements.push_back(element);
    addUndoHistoryItem(std::move(historyItem));
    canvasWidth_ = cropWidth;
    canvasHeight_ = cropHeight;
    for (auto& el : elementsOnCanvas_) {
        if (el != crop) {
            el->move(-cropX, -cropY, false);
        }
    }
    createDoubleBuffer();
    if (callback_) {
        callback_->canvasSizeChanged();
    }
}

bool Canvas::hasElementOfType(ElementType type) const {
    for (const auto& el : elementsOnCanvas_) {
        if (el->getType() == type) {
            return true;
        }
    }
    return false;
}

void Canvas::setCropOnExport(bool crop) {
    cropOnExport_ = crop;
}

bool Canvas::getCropOnExport() const {
    return cropOnExport_;
}

void Canvas::setDpi(float dpiX, float dpiY) {
    dpiX_ = dpiX;
    dpiY_ = dpiY;
}

std::pair<float, float> Canvas::getDpi() const {
    return { dpiX_, dpiY_ };
}

}
