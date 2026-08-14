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

#include "MovableElements.h"

#include <algorithm>
#include <utility>

#include "3rdpart/GdiplusH.h"
#include "Region.h"
#include "Canvas.h"
#include "Core/Images/Utils.h"
#include "Gui/GuiTools.h"

namespace ImageEditor {

Line::Line(Canvas* canvas, int startX, int startY, int endX, int endY) :MovableElement(canvas) {
    startPoint_.x = startX;
    startPoint_.y = startY;
    endPoint_.x   = endX;
    endPoint_.y   = endY;
    drawDashedRectangle_ = false;
    isBackgroundColorUsed_ = false;
    grips_.resize(2);
}

void Line::setEndPoint(POINT endPoint) {
    int kAccuracy = 7;
    if (abs (endPoint.y-startPoint_.y) < kAccuracy ) {
        endPoint.y = startPoint_.y;
    } else if (abs (endPoint.x-startPoint_.x) < kAccuracy ) {
        endPoint.x = startPoint_.x;
    }
    DrawingElement::setEndPoint( endPoint );
}

void Line::render(Painter* gr) {
    using namespace Gdiplus;
    if ( !gr ) {
        return;
    }
    Gdiplus::Pen pen( color_, static_cast<REAL>(penSize_) );
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    //CustomLineCap cap()
    //pen.SetCustomStartCap()
    gr->DrawLine( &pen, startPoint_.x, startPoint_.y, endPoint_.x, endPoint_.y );
}

void Line::getAffectedSegments( AffectedSegments* segments ) {
    int x0 = startPoint_.x;
    int y0 = startPoint_.y;
    int x1 = endPoint_.x;
    int y1 = endPoint_.y;

    int xStart = ( std::min<>(x0, x1) / AffectedSegments::kSegmentSize +1 ) * AffectedSegments::kSegmentSize;
    int xEnd   = ( std::max<>(x0,x1) / AffectedSegments::kSegmentSize ) * AffectedSegments::kSegmentSize;

    x0 = startPoint_.x;
    x1 = endPoint_.x;
    y0 = startPoint_.y;
    y1 = endPoint_.y;
    segments->markPoint( x0, y0 );
    segments->markPoint( x1, y1 );

    for( int x = xStart; x <= xEnd; x += AffectedSegments::kSegmentSize ) {
        int y = -1;
        if ( x1-x0 != 0) {
            y = y0 + (x-x0)*(y1-y0)/(x1-x0);
        }

        segments->markRect( x - penSize_, y - penSize_, penSize_ * 2, penSize_ * 2  );
    }

    int yStart = ( std::min<>( y0, y1 ) / AffectedSegments::kSegmentSize + 1) * AffectedSegments::kSegmentSize;
    int yEnd   = ( std::max<>( y0, y1 ) / AffectedSegments::kSegmentSize ) * AffectedSegments::kSegmentSize;

    for( int y = yStart; y <= yEnd; y += AffectedSegments::kSegmentSize ) {
        int x = -1;
        if ( y1-y0 != 0) {
            x = x0 + ( y - y0 ) * ( x1 - x0 ) / ( y1-y0 );
        }

        segments->markRect( x - penSize_, y - penSize_, penSize_ * 2, penSize_ * 2  );
    }
}

void Line::createGrips()
{
    Grip grip1;
    Grip grip2;
    grip1.pt.x = startPoint_.x;
    grip1.pt.y = startPoint_.y;
    grip1.gpt = MovableElement::GripPointType::gptStartPoint;
    grip2.pt.x = endPoint_.x;
    grip2.pt.y =  endPoint_.y;
    grip2.gpt = MovableElement::GripPointType::gptEndPoint;

    if ( grip1.pt.x <= grip2.pt.x ) {
        if (  grip1.pt.y <= grip2.pt.y  ) {
            grip1.bt = BoundaryType::btTopLeft;
            grip2.bt = BoundaryType::btBottomRight;
        } else {
            grip1.bt = BoundaryType::btBottomLeft;
            grip2.bt = BoundaryType::btTopRight;
        }
    } else {
        if (  grip2.pt.y > grip1.pt.y  ) {
            grip1.bt = BoundaryType::btTopRight;
            grip2.bt = BoundaryType::btBottomLeft;
        } else {
            grip1.bt = BoundaryType::btBottomRight;
            grip2.bt = BoundaryType::btTopLeft;
        }
    }
    grips_[0] = grip1;
    grips_[1] = grip2;
}

bool Line::isItemAtPos(int x, int y)
{
    int itemX = getX();
    int itemY = getY();
    int itemWidth = getWidth();
    int itemHeight = getHeight();
    int selectRadius = (std::max<int>)(kSelectRadius, penSize_);
    // Line equation
    if ( endPoint_.x - startPoint_.x != 0 ) {
        float k = float(endPoint_.y - startPoint_.y)/float(endPoint_.x - startPoint_.x);
        float b = float(endPoint_.x* startPoint_.y - startPoint_.x*endPoint_.y)/float(endPoint_.x - startPoint_.x);
        if(abs(y - (k*x + b)) <= selectRadius && x >= itemX-selectRadius && x <= itemX + itemWidth +  selectRadius ) {
            return true;
        }
    }

    if ( endPoint_.y - startPoint_.y != 0 ) {
        float k = float(endPoint_.x - startPoint_.x)/float(endPoint_.y - startPoint_.y);
        float b = float(endPoint_.y* startPoint_.x - startPoint_.y*endPoint_.x)/float(endPoint_.y - startPoint_.y);
        if(abs(x - (k*y + b)) <= selectRadius && y >= itemY-selectRadius && y <= itemY + itemHeight +  selectRadius) {
            return true;
        }
    }
    return false;
}

ImageEditor::ElementType Line::getType() const
{
    return ElementType::etLine;
}

TextElement::TextElement(Canvas* canvas, std::shared_ptr<InputBox> inputBox, int startX, int startY, int endX,int endY, bool filled):
                            MovableElement(canvas) {
    startPoint_.x = startX;
    startPoint_.y = startY;
    endPoint_.x   = endX;
    endPoint_.y   = endY;
    inputBox_ = std::move(inputBox);
    isEditing_ = false;
    firstEdit_ = true;
    fillBackground_ = filled;
}

void TextElement::render(Painter* gr) {
    using namespace Gdiplus;
    if ( !gr ) {
        return;
    }

    drawDashedRectangle_ = isSelected() || !inputBox_;
    if (fillBackground_) {
        auto prevSmoothingMode = gr->GetSmoothingMode();
        gr->SetSmoothingMode(SmoothingModeNone);
        SolidBrush br(backgroundColor_);
        gr->FillRectangle(&br, getX(), getY(), getWidth(), getHeight());
        gr->SetSmoothingMode(prevSmoothingMode);
    }
    if ( inputBox_  && !inputBox_->isVisible()) {
        inputBox_->render(gr, canvas_->getBufferBitmap(), Rect(getX()+4,getY()+3,getWidth()-5,getHeight()-6));
    }
}

void TextElement::getAffectedSegments( AffectedSegments* segments ) {
    int x = getX();
    int y = getY();
    int width = getWidth();
    int height = getHeight();
    segments->markRect( x, y, width, height ); // top
}

void TextElement::resize(int width, int height)
{
    MovableElement::resize(width,height);
    if ( inputBox_ ) {
        inputBox_->resize(getX()+3, getY()+3, width-6,height-6, grips_);
        inputBox_->invalidate();
        canvas_->updateView();
    }
}

void TextElement::setInputBox(std::shared_ptr<InputBox> inputBox)
{
    inputBox_ = std::move(inputBox);
    setTextColor();
    using namespace std::placeholders;
    inputBox_->onTextChanged.connect(std::bind(&TextElement::onTextChanged, this, _1));
    inputBox_->onEditCanceled.connect(std::bind(&TextElement::onEditCanceled, this));
    inputBox_->onEditFinished.connect(std::bind(&TextElement::onEditFinished, this));
    inputBox_->onResized.connect(std::bind(&TextElement::onControlResized, this, _1, _2));
    inputBox_->onSelectionChanged.connect(std::bind(&TextElement::onSelectionChanged, this, _1, _2, _3));
}

void TextElement::setFont(LOGFONT font,  DWORD changeMask)
{
    font_ = font;
    if ( inputBox_ ) {
        inputBox_->setFont(font, changeMask);
    }
}

LOGFONT TextElement::getFont() const
{
    return font_;
}

std::shared_ptr<InputBox> TextElement::getInputBox() const
{
    return inputBox_;
}

void TextElement::onTextChanged(LPCTSTR text)
{
    canvas_->updateView(getPaintBoundingRect());
}

void TextElement::onEditCanceled()
{
    inputBox_->show(false);
    endEdit(false);
    setSelected(false);

    canvas_->updateView(getPaintBoundingRect());
}

void TextElement::onEditFinished()
{
    inputBox_->show(false);
    endEdit(true);
    setSelected(false);

    canvas_->updateView(getPaintBoundingRect());
}

void TextElement::onControlResized(int w, int h)
{
    resize(w+6, h+6);
}

void TextElement::setTextColor()
{
    if ( inputBox_ ) {
        if ( !inputBox_->isVisible() ) {
            beginEdit();
        }
        inputBox_->setTextColor(color_);
        if ( !inputBox_->isVisible() ) {
            saveToHistory();
            //endEdit(true); // saving to undo history
        }
    }
}

void TextElement::onSelectionChanged(int min, int max, LOGFONT font)
{
    font_ = font;
    canvas_->onFontChanged(font_);
}

void TextElement::setColor(Gdiplus::Color color)
{
    MovableElement::setColor(color);
    setTextColor();
}

bool TextElement::isEmpty() const {
    return firstEdit_ && inputBox_->isEmpty();
}

ElementType TextElement::getType() const
{
    return ElementType::etText;
}

void TextElement::setSelected(bool selected)
{
    MovableElement::setSelected(selected);
    if ( inputBox_ && !selected ) {
        canvas_->onTextEditFinished(this);
        endEdit(true);

        inputBox_->show(false);
    }
}

void TextElement::beginEdit()
{
    if (! isEditing_) {
        isEditing_ = true;
        if ( inputBox_ ) {
            originalRawText_ = inputBox_->getRawText();
        }
    }
}

void TextElement::endEdit(bool save)
{
    canvas_->setCurrentlyEditedTextElement(nullptr);
    if ( isEditing_ ) {
        firstEdit_ = false;
        isEditing_ = false;
        if (save) {
            saveToHistory();
        }
    }
}

void TextElement::saveToHistory() {
    if (originalRawText_ != inputBox_->getRawText()) {
        auto uhi = std::make_unique<Canvas::UndoHistoryItem>();
        Canvas::UndoHistoryItemElement uhie;
        uhi->type = Canvas::UndoHistoryItemType::uitTextChanged;
        uhie.movableElement = this;
        uhie.rawText = originalRawText_;
        uhi->elements.push_back(uhie);

        canvas_->addUndoHistoryItem(std::move(uhi));
        originalRawText_.clear();
    }
}
void TextElement::setRawText(const std::string& rawText)
{
    if ( inputBox_ ) {
        inputBox_->setRawText(rawText);
    }
}

void TextElement::setFillBackground(bool fill) {
    fillBackground_ = fill;
    isBackgroundColorUsed_ = fill;
}

bool TextElement::getFillBackground() const {
    return fillBackground_;
}

Crop::Crop(Canvas* canvas, int startX, int startY, int endX, int endY):MovableElement(canvas)  {
    startPoint_.x = startX;
    startPoint_.y = startY;
    endPoint_.x   = endX;
    endPoint_.y   = endY;
    isColorUsed_ = false;
    isPenSizeUsed_ = false;
    isBackgroundColorUsed_ = false;
}

void Crop::render(Painter* gr) {
    MovableElement::render(gr);
}

void Crop::getAffectedSegments( AffectedSegments* segments ) {
    int x = getX();
    int y = getY();
    int width = getWidth();
    int height = getHeight();

    //segments->markRect( x, y, width, height );
    segments->markRect( x, y, width, penSize_ ); // top
    segments->markRect( x, y, penSize_, height ); // left
    segments->markRect( x, y + height - penSize_, width, penSize_ ); // bottom
    segments->markRect( x + width - penSize_, y, penSize_, height ); // right
}

ImageEditor::ElementType Crop::getType() const
{
    return ElementType::etCrop;
}

void Crop::setPos(int x, int y) {
    return MovableElement::setPos(x, y);
}

bool Crop::move(int offsetX, int offsetY, bool checkBounds) {
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
        if (newX < 0 || newX >= canvasWidth || newRight < 0 || newRight >= canvasWidth) {
            offsetX = 0;
        }
        if (newY < 0 || newY >= canvasHeight || newBottom < 0 || newBottom >= canvasHeight) {
            offsetY = 0;
        }
    }

    if (offsetX == 0 && offsetY == 0) {
        return false;
    }
    return MovableElement::move(offsetX, offsetY, true);
 }

void Crop::resize(int width, int height) {
    int canvasWidth = canvas_->getWidth();
    int canvasHeight = canvas_->getHeigth();
    int x = getX();
    int y = getY();
    width = (std::min)(width, canvasWidth - x);
    height = (std::min)(height, canvasHeight - y);
    return MovableElement::resize(width, height);
}

RECT Crop::getPaintBoundingRect() {
    RECT rc = MovableElement::getPaintBoundingRect();
    auto [dpiX, dpiY] = canvas_->getDpi();
    // Offset for text with crop dimensions
    rc.left -= static_cast<LONG>(80 * dpiX);
    rc.top -= static_cast<LONG>(80 * dpiY);
    return rc;
}

CropOverlay::CropOverlay(Canvas* canvas, int startX, int startY, int endX,int endY):MovableElement(canvas)
{
    startPoint_.x = startX;
    startPoint_.y = startY;
    endPoint_.x = endX;
    endPoint_.y = endY;
    isColorUsed_ = false;
    isPenSizeUsed_ = false;
    isBackgroundColorUsed_ = false;
    using namespace Gdiplus;
    FontFamily ff(L"Arial");
    font_ = std::make_unique<Font>(&ff, static_cast<REAL>(12), FontStyleBold);
}

void CropOverlay::render(Painter* gr)
{
    using namespace Gdiplus;
    Gdiplus::SolidBrush brush(Gdiplus::Color( 120, 0, 0, 0) );

    std::vector<MovableElement*> crops;
    canvas_->getElementsByType(ElementType::etCrop, crops);
    Rect rc (0,0, canvas_->getWidth(), canvas_->getHeigth());
    rc.Intersect(canvas_->currentRenderingRect());
    Region rgn(rc.X,rc.Y, rc.Width, rc.Height);
    RectF cropRect;
    bool isCropElementMoving = false;
    for (const auto& crop : crops) {
        rgn = rgn.subtracted(Region(crop->getX(), crop->getY(), crop->getWidth(), crop->getHeight()));

        isCropElementMoving = crop->isMoving();

        cropRect = RectF(static_cast<REAL>(crop->getX()), static_cast<REAL>(crop->getY()),
            static_cast<REAL>(crop->getWidth()), static_cast<REAL>(crop->getHeight()));
    }

    Gdiplus::Region oldRegion;
    gr->GetClip(&oldRegion);
    gr->SetClip(rgn.toNativeRegion().get(), Gdiplus::CombineModeIntersect);
    SmoothingMode prevSmoothingMode = gr->GetSmoothingMode();
    PixelOffsetMode oldPOM = gr->GetPixelOffsetMode();
    gr->SetPixelOffsetMode(PixelOffsetModeHalf);
    gr->SetSmoothingMode(SmoothingModeNone);

    gr->FillRectangle( &brush, rc.X, rc.Y, rc.Width, rc.Height );
    gr->SetClip(&oldRegion);
    gr->SetSmoothingMode(prevSmoothingMode);
    gr->SetPixelOffsetMode(oldPOM);

    // Draw crop dimensions

    if (!cropRect.IsEmptyArea()) {
        SolidBrush textBrush(Color(200, 255, 255, 255));
        SolidBrush textRectBrush(Color(210, 120, 120, 120));

        auto oldTextHint = gr->GetTextRenderingHint();
        gr->SetTextRenderingHint(TextRenderingHintAntiAlias);

        /*StringFormat format;
        format.SetLineAlignment(StringAlignmentCenter);
        format.SetAlignment(StringAlignmentCenter);*/
        CString text;
        text.Format(_T("%dx%d"), static_cast<int>(cropRect.Width), static_cast<int>(cropRect.Height));
        RectF textBoundingBox;
        gr->MeasureString(text, -1, font_.get(), PointF(0, 0), &textBoundingBox);

        if (isCropElementMoving || (cropRect.X - textBoundingBox.Width >= 0 && cropRect.Y - textBoundingBox.Height >= 0)) {
            REAL x = cropRect.X - textBoundingBox.Width < 0 ? cropRect.X : cropRect.X - textBoundingBox.Width;
            REAL y = cropRect.Y - textBoundingBox.Height < 0 ? cropRect.Y : cropRect.Y - textBoundingBox.Height;
            PointF point(x, y);
            RectF textRect = RectF(x, /*cropRect.Y*/y, textBoundingBox.Width, textBoundingBox.Height);

            gr->FillRectangle(&textRectBrush, textRect);
            gr->DrawString(text, -1, font_.get(), point, /*&format,*/ &textBrush);
        }

        gr->SetTextRenderingHint(oldTextHint);
    }
}

// Rectangle
//
Rectangle::Rectangle(Canvas* canvas, int startX, int startY, int endX, int endY, bool filled, bool drawBorder)
    : MovableElement(canvas) {
    startPoint_.x = startX;
    startPoint_.y = startY;
    endPoint_.x   = endX;
    endPoint_.y   = endY;
    drawDashedRectangle_   = false;
    filled_ = filled;
    drawBorder_ = drawBorder;
    isBackgroundColorUsed_ = filled;
}

void Rectangle::render(Painter* gr) {
    using namespace Gdiplus;
    if ( !gr ) {
        return;
    }
    SmoothingMode prevSmoothingMode = gr->GetSmoothingMode();
    gr->SetSmoothingMode(SmoothingModeNone);
    Gdiplus::Pen pen( color_, static_cast<REAL>(penSize_) );
    int x = getX()+penSize_/2/*-(1-penSize_%2)*/;
    int y = getY()+penSize_/2/*-(1-penSize_%2)*/;
    int width = getWidth()-penSize_;
    int height = getHeight()-penSize_;
    if ( filled_ ) {
        SolidBrush br(backgroundColor_);
        gr->FillRectangle(&br, x, y, width, height);
    }
    if (drawBorder_) {
        gr->DrawRectangle(&pen, x, y, width, height);
    }
    gr->SetSmoothingMode(prevSmoothingMode);
}

void Rectangle::getAffectedSegments( AffectedSegments* segments ) {
    int x = getX();
    int y = getY();
    int width = getWidth();
    int height = getHeight();

    //segments->markRect( x, y, width, height );
    segments->markRect( x, y, width, penSize_ ); // top
    segments->markRect( x, y, penSize_, height ); // left
    segments->markRect( x, y + height - penSize_, width, penSize_ ); // bottom
    segments->markRect( x + width - penSize_, y, penSize_, height ); // right*/
}

bool Rectangle::isItemAtPos(int x, int y)
{
    if ( filled_ ) {
        return MovableElement::isItemAtPos(x,y);
    }
    int elementX = getX();
    int elementY = getY();
    int elementWidth = getWidth();
    int elementHeight = getHeight();
    int selectRadius = std::max<int>(kSelectRadius, penSize_);
    return
        ((( x >= elementX - selectRadius && x  <= elementX  + selectRadius )  || ( x >= elementX +elementWidth - selectRadius && x  <= elementX  +elementWidth+ selectRadius ) )
        && y>= elementY - selectRadius && y <= elementY + elementHeight + selectRadius )

        ||
        ((( y >= elementY - selectRadius && y  <= elementY  + selectRadius )  || ( y >= elementY +elementHeight - selectRadius && y  <= elementY  +elementHeight+ selectRadius ) )

        && x>= elementX - selectRadius && x <= elementX + elementWidth + selectRadius );
}

RECT Rectangle::getPaintBoundingRect()
{
    RECT rc = MovableElement::getPaintBoundingRect();
    /*rc.left -= penSize_;
    rc.top -= penSize_;
    rc.right += penSize_;
    rc.bottom+= penSize_;*/
    return rc;
}

ImageEditor::ElementType Rectangle::getType() const
{
    return ElementType::etRectangle;
}

Arrow::Arrow(Canvas* canvas,int startX, int startY, int endX,int endY, ArrowMode mode) :
    Line(canvas, startX, startY, endX, endY),
    mode_(mode)
{
    isBackgroundColorUsed_ = false;
}

void Arrow::render(Painter* gr)
{
    using namespace Gdiplus;
    render(gr, color_, penSize_, startPoint_, endPoint_, mode_);
}

void Arrow::render(Painter* gr, Gdiplus::Color color, int penSize, POINT startPoint, POINT endPoint, ArrowMode mode)
{
    using namespace Gdiplus;

    Gdiplus::Pen pen(/* color_*/ color, REAL(penSize));
    if (mode == ArrowMode::Mode1) {
        AdjustableArrowCap cap1(5, 3, true);
        cap1.SetBaseCap(LineCapTriangle);
        cap1.SetStrokeJoin(LineJoinRound);

        //pen.SetStartCap(LineCapRound);
        pen.SetCustomEndCap(&cap1);

        gr->DrawLine(&pen, startPoint.x, startPoint.y, endPoint.x, endPoint.y);
    } else {
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        gr->DrawLine(&pen, startPoint.x, startPoint.y, endPoint.x, endPoint.y);
        Gdiplus::Pen pen2(color, REAL(penSize));

        int head_length = penSize * 6;
        int head_width = penSize * 4;
        const auto dx = static_cast<float>(endPoint.x - startPoint.x);
        const auto dy = static_cast<float>(endPoint.y - startPoint.y);
        const auto length = std::sqrt(dx * dx + dy * dy);
        if (head_length < 1 || length < head_length / 2.0)
            return;
        // ux,uy is a unit vector parallel to the line.
        const auto ux = dx / length;
        const auto uy = dy / length;

        // vx,vy is a unit vector perpendicular to ux,uy
        const auto vx = -uy;
        const auto vy = ux;

        const auto half_width = 0.5f * head_width;
        Point p1 { int(round(endPoint.x - head_length * ux + half_width * vx)),
            int(round(endPoint.y - head_length * uy + half_width * vy)) };

        Point p2 { int(round(endPoint.x - head_length * ux - half_width * vx)),
            int(round(endPoint.y - head_length * uy - half_width * vy)) };

        pen2.SetEndCap(LineCapRound);
        pen2.SetStartCap(LineCapRound);
        gr->DrawLine(&pen2, endPoint.x, endPoint.y, p1.X, p1.Y);
        gr->DrawLine(&pen2, endPoint.x, endPoint.y, p2.X, p2.Y);
    }
}

RECT Arrow::getPaintBoundingRect()
{
    RECT res = MovableElement::getPaintBoundingRect();
    res.left -= penSize_ * 2;
    res.top -= penSize_ * 2;
    res.bottom += penSize_*2;
    res.right += penSize_*2;
    return res;
}

ImageEditor::ElementType Arrow::getType() const
{
    return ElementType::etArrow;
}

Selection::Selection(Canvas* canvas, int startX, int startY, int endX,int endY) : MovableElement(canvas)
{
    isColorUsed_ = false;
    isPenSizeUsed_ = false;
    isBackgroundColorUsed_ = false;
}

void Selection::render(Painter* gr)
{

}

ImageEditor::ElementType Selection::getType() const
{
    return ElementType::etSelection;
}

void Selection::createGrips()
{
    throw std::logic_error("The method or operation is not implemented.");
}

FilledRectangle::FilledRectangle(Canvas* canvas, int startX, int startY, int endX, int endY, bool drawBorder)
    : Rectangle(canvas, startX, startY, endX, endY, true, drawBorder) {
}

ElementType FilledRectangle::getType() const
{
    return ElementType::etBlurringRectangle;
}

BlurringRectangle::BlurringRectangle(Canvas* canvas, float blurRadius, int startX, int startY, int endX,int endY,
    bool pixelate, bool invertSelection) : MovableElement(canvas)
{
    blurRadius_ = blurRadius;
    isPenSizeUsed_ = false;
    isBackgroundColorUsed_ = false;
    isColorUsed_ = false;
    pixelate_ = pixelate;
    invertSelection_ = invertSelection;
}

BlurringRectangle::~BlurringRectangle()
{
}

void BlurringRectangle::setBlurRadius(float radius)
{
    blurRadius_ = radius;
}

void BlurringRectangle::setInvertSelection(bool invert) {
    invertSelection_ = invert;
}

bool BlurringRectangle::getInvertSelection() const {
    return invertSelection_;
}

float BlurringRectangle::getBlurRadius() const
{
    return blurRadius_;
}

RECT BlurringRectangle::getPaintBoundingRect() {
    if (invertSelection_) {
        return { 0, 0, canvas_->getWidth(), canvas_->getHeigth() };
    }
    return MovableElement::getPaintBoundingRect();
}

void BlurringRectangle::render(Painter* gr)
{
    drawDashedRectangle_ = isSelected();
    using namespace Gdiplus;
    Bitmap* background = canvas_->getBufferBitmap();
    drawDashedRectangle_ = isMoving_;
    Rect currentRenderingRect = canvas_->currentRenderingRect();
    Rect canvasRect(0, 0, canvas_->getWidth(), canvas_->getHeigth());
    Rect elRect(getX(), getY(), getWidth(), getHeight());
    elRect.Intersect(currentRenderingRect);
    elRect.Intersect(canvasRect);
    if ( elRect.Width < 1 || elRect.Height < 1 ) {
        return;
    }
    // Optimization: do not apply blur while moving or resizing, can hang on slow CPUs
    if ( !isMoving_ /* && !canvas_->manipulationStarted()*/) {
        Rect excludeRect = invertSelection_ ? elRect : Rect();
        Rect applyRect = invertSelection_ ? canvasRect : elRect;
        if(pixelate_) {
            ImageUtils::ApplyPixelateEffect(background, applyRect.X, applyRect.Y, applyRect.Width, applyRect.Height, int(blurRadius_ * 4.0 /*BLUR_RADIUS_PRECISION*/), excludeRect);
        }
        else {
#if GDIPVER >= 0x0110
            Blur blur;
            BlurParams blurParams;
            blurParams.radius = blurRadius_*3;
            blur.SetParameters(&blurParams);
            Matrix matrix;
            Status st;
            RectF sourceRect(elRect.X, elRect.Y, elRect.Width, elRect.Height);

            st = gr->DrawImage(background, &sourceRect, &matrix, &blur, 0, Gdiplus::UnitPixel);
#else
            ImageUtils::ApplyGaussianBlur(background, applyRect.X, applyRect.Y, applyRect.Width, applyRect.Height, blurRadius_, excludeRect);
#endif
        }
    }
}

ImageEditor::ElementType BlurringRectangle::getType() const
{
    return ElementType::etBlurringRectangle;
}


PixelateRectangle::PixelateRectangle(Canvas* canvas, float blurRadius, int startX, int startY, int endX, int endY, bool invertSelection)
            : BlurringRectangle(canvas, blurRadius, startX, startY, endX, endY, true, invertSelection)
{
}

ElementType PixelateRectangle::getType() const
{
    return ElementType::etPixelateRectangle;
}


RoundedRectangle::RoundedRectangle(Canvas* canvas, int startX, int startY, int endX, int endY, bool filled /*= false */, bool drawBorder)
                : Rectangle(canvas, startX, startY, endX,endY, filled, drawBorder)
{
    isBackgroundColorUsed_ = filled;
}

void RoundedRectangle::render(Painter* gr)
{
    using namespace Gdiplus;
    Gdiplus::Pen pen( color_, static_cast<REAL>(penSize_) );
    int x = getX() + penSize_/2 /*- (1-penSize_%2)*/;
    int y = getY() + penSize_/2 /*- (1-penSize_%2)*/;
    int width = max(getWidth()-penSize_,1);
    int height = max(getHeight()-penSize_,1);
    SolidBrush br(backgroundColor_);

    Region rgn(max(getX(),0),max(0,getY()), getWidth(), getHeight());

    gr->SetClip(rgn.toNativeRegion().get(), Gdiplus::CombineModeIntersect); // the drawed ellipse can exceed in some cases the bounding rectangle, setting the clip
    ImageUtils::DrawRoundedRectangle(gr, Rect(x,y,width,height), roundingRadius_ *2 , drawBorder_ ? &pen: nullptr, filled_ ? &br : 0);
    gr->SetClip(canvas_->currentRenderingRect()); // restoring clip
}

ImageEditor::ElementType RoundedRectangle::getType() const
{
    return ElementType::etRoundedRectangle;
}

FilledRoundedRectangle::FilledRoundedRectangle(Canvas* canvas, int startX, int startY, int endX,int endY, bool drawBorder)
    : RoundedRectangle(canvas, startX, startY, endX,endY, true, drawBorder)
{

}

ImageEditor::ElementType FilledRoundedRectangle::getType() const
{
    return ElementType::etFilledRoundedRectangle;
}

Ellipse::Ellipse(Canvas* canvas, bool filled /*= false */, bool drawBorder)
    : MovableElement(canvas) {
    filled_ = filled;
    setDrawBorder(drawBorder);
    isBackgroundColorUsed_ = filled;
}

void Ellipse::render(Painter* gr)
{
    using namespace Gdiplus;
    Gdiplus::Pen pen( color_, static_cast<REAL>(penSize_) );
    int x = getX() + penSize_/2 /*- (1-penSize_%2)*/;
    int y = getY() + penSize_/2 /*- (1-penSize_%2)*/;
    int width = getWidth()-penSize_;
    int height = getHeight()-penSize_;
    SolidBrush br(backgroundColor_);
    Region rgn(max(getX(),0),max(0,getY()), getWidth(), getHeight());
    gr->SetClip(rgn.toNativeRegion().get(), Gdiplus::CombineModeIntersect);

    if ( filled_ ) {
        gr->FillEllipse(&br, x,y,width,height);
    }
    if (getDrawBorder()) {
        gr->DrawEllipse(&pen, x, y, width, height);
    }
    gr->SetClip(canvas_->currentRenderingRect()); // restoring clip
}

bool Ellipse::containsPoint(Gdiplus::Rect ellipse, const Gdiplus::Point& location) {
    using namespace Gdiplus;
    Point center(
        ellipse.GetLeft() + (ellipse.Width / 2),
        ellipse.GetTop() + (ellipse.Height / 2));

    double _xRadius = ellipse.Width / 2.0;
    double _yRadius = ellipse.Height / 2.0;


    if (_xRadius <= 0.0 || _yRadius <= 0.0)
        return false;
    /* This is a more general form of the circle equation
     *
     * X^2/a^2 + Y^2/b^2 <= 1
     */

    Point normalized(location.X - center.X,
                     location.Y - center.Y);

    return ((double)(normalized.X * normalized.X)
            / (_xRadius * _xRadius)) + ((double)(normalized.Y * normalized.Y) / (_yRadius * _yRadius))
        <= 1.0;
}


void Ellipse::createGrips()
{
    grips_.resize(8);
    MovableElement::createGrips();
    //btBottomRight, btBottom,  btBottomLeft,  btRight,  btLeft,  btTopLeft, btTop, btTopRight,
        grips_.erase(grips_.begin() + static_cast<int>(BoundaryType::btTopRight));
        grips_.erase(grips_.begin() + static_cast<int>(BoundaryType::btTopLeft));
            grips_.erase(grips_.begin() + static_cast<int>(BoundaryType::btBottomLeft));
    grips_.erase(grips_.begin() + static_cast<int>(BoundaryType::btBottomRight));
}

RECT Ellipse::getPaintBoundingRect()
{
    RECT rc = MovableElement::getPaintBoundingRect();
    int width = getWidth();
    int height = getHeight();
    if ( width < penSize_ * 2 ||  height < penSize_ * 2  ) {
        rc.left -= penSize_;
        rc.right += penSize_;
        rc.top -= penSize_;
        rc.bottom += penSize_;
    }
    return rc;
}

ImageEditor::ElementType Ellipse::getType() const
{
    return ElementType::etEllipse;
}

bool Ellipse::isItemAtPos(int x, int y)
{
    using namespace Gdiplus;
    int elX = getX();
    int elY = getY();
    int width = getWidth();
    int height = getHeight();
    int selectRadius = std::max<int>(penSize_, kSelectRadius);
    if ( filled_ ) {
        return containsPoint(Rect(elX,elY, width, height), Point(x,y));
    } else {
        return containsPoint(Rect(elX-selectRadius,elY-selectRadius, width+selectRadius*2, height+selectRadius*2), Point(x,y))
            && !containsPoint(Rect(elX+selectRadius,elY+selectRadius, width-selectRadius*2, height-selectRadius*2), Point(x,y) );
    }
}

FilledEllipse::FilledEllipse(Canvas* canvas, bool drawBorder) : Ellipse(canvas, true, drawBorder)
{

}

ImageEditor::ElementType FilledEllipse::getType() const
{
    return ElementType::etFilledEllipse;
}

StepNumber::StepNumber(Canvas* canvas, int startX, int startY, int endX, int endY, int number, int fontSize): MovableElement(canvas) {
    using namespace Gdiplus;
    number_ = number;
    fontSize_ = fontSize;
    int radius = recalcRadius();

    startPoint_.x = startX - radius;
    startPoint_.y = startY - radius;
    endPoint_.x = startX + radius;
    endPoint_.y = startY + radius;
    drawDashedRectangle_ = true;
    drawDashedRectangleWhenSelected_ = true;
    isBackgroundColorUsed_ = true;
}

void StepNumber::render(Painter* gr) {
    using namespace Gdiplus;
    int x = getX();
    int y = getY();
    int width = getWidth();
    int height = getHeight();
    SolidBrush br(backgroundColor_);
    Region rgn(max(x, 0), max(0, y), width, height);
    gr->SetClip(rgn.toNativeRegion().get(), CombineModeIntersect);
    gr->FillEllipse(&br, x, y, width-1, height-1);
    SolidBrush textBrush(color_);

    FontFamily ff(L"Arial");
    Font font(&ff, static_cast<REAL>(fontSize_), FontStyleBold);

    StringFormat format;
    format.SetLineAlignment(StringAlignmentCenter);
    format.SetAlignment(StringAlignmentCenter);

    RectF textRect(static_cast<REAL>(x), static_cast<REAL>(y), static_cast<REAL>(width), static_cast<REAL>(height));
    CString s;
    s.Format(L"%d", number_);
    auto oldTextHint = gr->GetTextRenderingHint();
    gr->SetTextRenderingHint(TextRenderingHintAntiAlias);
    gr->DrawString(s, -1, &font, textRect,  &format, &textBrush );
    gr->SetTextRenderingHint(oldTextHint);
    gr->SetClip(canvas_->currentRenderingRect()); // restoring clip

}

RECT StepNumber::getPaintBoundingRect() {
    return MovableElement::getPaintBoundingRect();
}

ElementType StepNumber::getType() const {
    return ElementType::etStepNumber;
}

int StepNumber::recalcRadius() {
    using namespace Gdiplus;
    Graphics* gr = canvas_->getGraphicsDevice();
    auto oldTextHint = gr->GetTextRenderingHint();
    gr->SetTextRenderingHint(TextRenderingHintAntiAlias);
    FontFamily ff(L"Arial");
    Font font(&ff, static_cast<REAL>(fontSize_), FontStyleBold);
    CString s;
    s.Format(L"%d", number_);
    RectF boundingBox;
    gr->MeasureString(s, -1, &font, PointF(static_cast<REAL>(startPoint_.x), static_cast<REAL>(startPoint_.y)), &boundingBox);
    gr->SetTextRenderingHint(oldTextHint);
    int radius = static_cast<int>(std::max<>(boundingBox.Width, boundingBox.Height) / 2 + 6);
    return radius;
}

void StepNumber::setNumber(int number) {
    if (number_ != number) {
        number_ = number;
        int radius = recalcRadius();
        resize(radius * 2, radius * 2);
    }
}

bool StepNumber::isResizable() const {
    return false;
}

void StepNumber::setFontSize(int size) {
    if (fontSize_ != size) {
        fontSize_ = size;
        int radius = recalcRadius();
        resize(radius * 2, radius * 2);
    }
}

}
