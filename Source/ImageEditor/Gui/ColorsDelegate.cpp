#include "ColorsDelegate.h"

#include <cassert>
#include <utility>

#include "Gui/GuiTools.h"
#include "Core/i18n/Translator.h"
#include "Core/Images/Utils.h"
#include "resource.h"

namespace ImageEditor {

ColorsDelegate::ColorsDelegate(Toolbar* toolbar, int itemIndex, Canvas* canvas) {
    toolbar_ = toolbar;
    toolbarItemIndex_ = itemIndex;
    canvas_ = canvas;
    RECT rc = {0,0,1,1};
    font_ = GuiTools::GetSystemDialogFont();
    foregroundButton_.Create(toolbar->m_hWnd, rc, 0,WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON);
    foregroundButton_.SetFont(font_);
    foregroundColorButton_.SubclassWindow(foregroundButton_.m_hWnd);
    using namespace std::placeholders;
    foregroundColorButton_.setOnSelChangeCallback([this](auto && color, auto && valid) {
        OnForegroundButtonSelChanged(std::forward<decltype(color)>(color), std::forward<decltype(valid)>(valid));
    });
    foregroundColorButton_.SetCustomText(TR("More colors..."));
    foregroundColorButton_.SetColorCodeText(TR("Get color's code..."));
    foregroundColorButton_.SetListener(this);
    backgroundButton_.Create(toolbar->m_hWnd, rc, 0,WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON);
    backgroundButton_.SetFont(font_);
    backgroundColorButton_.SubclassWindow(backgroundButton_.m_hWnd);
    backgroundColorButton_.setOnSelChangeCallback([this](auto && color, auto && valid) {
        OnBackgroundButtonSelChanged(std::forward<decltype(color)>(color), std::forward<decltype(valid)>(valid));
    });
    backgroundColorButton_.SetCustomText(TR("More colors..."));
    backgroundColorButton_.SetColorCodeText(TR("Get color's code..."));
    backgroundColorButton_.SetListener(this);
    swapColorsIcon_ = ImageUtils::BitmapFromResource(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_SWAPCOLORS), _T("PNG"));
    assert(swapColorsIcon_);
}

SIZE ColorsDelegate::CalcItemSize(Toolbar::Item& item, int x, int y, float dpiScaleX, float dpiScaleY) {
    foregroundRect_ = getForegroundRect(x, y, dpiScaleX, dpiScaleY);
    swapColorsButtonRect_ = getSwapColorsRect(x, y, dpiScaleX, dpiScaleY);
    SIZE res = { static_cast<LONG>((kSquareSize + kOffset +5 + kPadding)* dpiScaleX), static_cast<LONG>((kSquareSize + kOffset + 10)* dpiScaleY ) };
    return res;
}

void ColorsDelegate::DrawItem(Toolbar::Item& item, Gdiplus::Graphics* gr, int x, int y, float dpiScaleX, float dpiScaleY) {
    using namespace Gdiplus;
    Pen borderPen(Color(0,0,0));

    SolidBrush backgroundBrush(backgroundColor_);
    SolidBrush foregroundBrush(foregroundColor_);

    backgroundRect_ = Rect(static_cast<int>(x+(kPadding+kOffset)*dpiScaleX), static_cast<int>(y+(4+kOffset)*dpiScaleY), 
                        static_cast<int>(kSquareSize * dpiScaleX), static_cast<int>(kSquareSize * dpiScaleY));
    gr->FillRectangle(&backgroundBrush, backgroundRect_);
    gr->DrawRectangle(&borderPen, backgroundRect_);

    foregroundRect_ = getForegroundRect(x, y, dpiScaleX, dpiScaleY);
    gr->FillRectangle(&foregroundBrush, foregroundRect_);
    gr->DrawRectangle(&borderPen, foregroundRect_);

    POINT pt = {foregroundRect_.X,foregroundRect_.Y + foregroundRect_.Height};
    //toolbar_->ClientToScreen(&pt);
    foregroundColorButton_.SetWindowPos(0, pt.x, pt.y,0,0,/*SWP_NOSIZE*/0);

    POINT pt2 = {backgroundRect_.X,backgroundRect_.Y + backgroundRect_.Height};
    //toolbar_->ClientToScreen(&pt2);
    backgroundColorButton_.SetWindowPos(0, pt2.x, pt2.y,0,0,/*SWP_NOSIZE*/0);

    swapColorsButtonRect_ = getSwapColorsRect(x, y, dpiScaleX, dpiScaleY);
    gr->DrawImage(swapColorsIcon_.get(), swapColorsButtonRect_);
}


std::vector<std::pair<RECT, CString>> ColorsDelegate::getSubItemsHints()
{
    Gdiplus::Rect rc1 = swapColorsButtonRect_;
    RECT rc { rc1.GetLeft(), rc1.GetTop(),
        rc1.GetRight(), rc1.GetBottom()
    };
    std::vector<std::pair<RECT, CString>> res {
        {rc, TR("Swap colors")}
    };
    return res;
}

void ColorsDelegate::setForegroundColor(Gdiplus::Color color ) {
    foregroundColor_ = color;
    foregroundColorButton_.SetColor(color.ToCOLORREF());
}

void ColorsDelegate::setBackgroundColor(Gdiplus::Color color) {
    backgroundColor_ = color;
    backgroundColorButton_.SetColor(color.ToCOLORREF());
}

Gdiplus::Color ColorsDelegate::getForegroundColor() const {
    return foregroundColor_;
}

Gdiplus::Color ColorsDelegate::getBackgroundColor() const {
    return backgroundColor_;
}

int ColorsDelegate::itemIndex() const {
    return toolbarItemIndex_;
}

void ColorsDelegate::OnClick(int x, int y, float dpiScaleX, float dpiScaleY){
    if ( foregroundRect_.Contains(x,y) ) {
        foregroundColorButton_.Click();
    } else if ( backgroundRect_.Contains(x,y)) {
        backgroundColorButton_.Click();
    } else if (swapColorsButtonRect_.Contains(x,y)) {
        swapColors();
    }
}

void ColorsDelegate::OnForegroundButtonSelChanged(COLORREF color, BOOL valid ) {
    foregroundColor_ = Gdiplus::Color(GetRValue(color), GetGValue(color), GetBValue(color));
    toolbar_->repaintItem(toolbarItemIndex_);
    canvas_->setForegroundColor(foregroundColor_);

}

void ColorsDelegate::OnBackgroundButtonSelChanged(COLORREF color, BOOL valid ) {
    backgroundColor_ = Gdiplus::Color(GetRValue(color), GetGValue(color), GetBValue(color));
    toolbar_->repaintItem(toolbarItemIndex_);
    canvas_->setBackgroundColor(backgroundColor_);
}


Gdiplus::Rect ColorsDelegate::getForegroundRect(int x, int y, float dpiScaleX, float dpiScaleY)
{
    return {
        static_cast<int>(kPadding * dpiScaleX + x),
        static_cast<int>(y + dpiScaleY * 4),
        static_cast<int>(kSquareSize * dpiScaleX),
        static_cast<int>(kSquareSize * dpiScaleY)
    };
}

Gdiplus::Rect ColorsDelegate::getSwapColorsRect(int x, int y, float dpiScaleX, float dpiScaleY)
{
    Gdiplus::Rect foregroundRect = getForegroundRect(x, y, dpiScaleX, dpiScaleY);
    return {foregroundRect.GetRight() + 1, foregroundRect.Y - 5, static_cast<int>(12 * dpiScaleX), static_cast<int>(12 * dpiScaleY)};
}

void ColorsDelegate::swapColors() {
    std::swap(foregroundColor_, backgroundColor_);
    canvas_->setForegroundColor(foregroundColor_);
    canvas_->setBackgroundColor(backgroundColor_);
    foregroundColorButton_.SetColor(foregroundColor_.ToCOLORREF());
    backgroundColorButton_.SetColor(backgroundColor_.ToCOLORREF());
    toolbar_->repaintItem(toolbarItemIndex_);
}

void ColorsDelegate::onBeforeDialogOpen(CColorButton* btn, /*out*/ HWND* parent) {
    *parent = toolbar_->GetParent();
}

void ColorsDelegate::onAfterDialogOpen(CColorButton* btn) {
    
}

}
