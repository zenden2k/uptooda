#include "Toolbar.h"

#include <strsafe.h>

#include "Core/Logging.h"
#include "3rdpart/GdiplusH.h"
#include "Gui/GuiTools.h"
#include "3rdpart/RoundRect.h"
#include "resource.h"
#include "Core/Images/Utils.h"
#include "Func/WinUtils.h"
#include "../Canvas.h"
#include "Core/i18n/Translator.h"
#include "Gui/Helpers/DPIHelper.h"

namespace ImageEditor {

constexpr auto kItemMargin = 3;
constexpr auto kItemHorPadding = 5;
constexpr auto kVertItemHorPadding = 4;
constexpr auto kItemVertPadding = 3;
constexpr auto kIconSize = 20;
constexpr auto kTinyDropdownIconSize = 9;
constexpr auto kDropdownIconSize = 16;

Toolbar::Toolbar(Toolbar::Orientation orientation, bool createSubPanel)
{
    using namespace Gdiplus;
    orientation_ = orientation;
    createSubPanel_ = createSubPanel;
    selectedItemIndex_ = -1;
    trackMouse_ = false;
    //dropDownIcon_ = ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(IDB_DROPDOWNICONPNG),_T("PNG"));

    if ( !WinUtils::IsWine() ) {
        subpanelColor_ = Color(252,252,252);
    } else {
        subpanelColor_.SetFromCOLORREF(GetSysColor(COLOR_BTNFACE));
    }

    subpanelBrush_.CreateSolidBrush(subpanelColor_.ToCOLORREF());
    memset(&buttonsRect_, 0, sizeof(buttonsRect_));
    textRenderingHint_ = Gdiplus::TextRenderingHintAntiAlias;
    oldSelectedBm_ = nullptr;
    itemMargin_ = 0;
    itemHorPadding_ = 0;
    itemVertPadding_ = 0;
    iconSizeX_ = 0;
    iconSizeY_ = 0;
    subpanelHeight_ = 0;
    subpanelLeftOffset_ = 0;
    movable_ = true;
    showButtonText_ = true;
    moveParent_ = false;
}

Toolbar::~Toolbar()
{
    if (!backBufferDc_.IsNull()) {
        backBufferDc_.SelectBitmap(oldSelectedBm_);
    }
    if (!backBuffer_.IsNull()) {
        backBuffer_.DeleteObject();
    }
}

bool Toolbar::Create(HWND parent, bool topMost, bool child, COLORREF backgroundColor, bool roundedBorder)
{
    RECT rc = {0, 0, 1,1};

    if ( orientation_ == orHorizontal ) {
        rc.left = 60;
        rc.right = 70;
    } else {
        rc.top = 60;
        rc.bottom = 70;
    }

    // Move toolbars to the top left corner of parent window in fullscreen mode
    if (!child) {
        CRect parentRect;
        ::GetWindowRect(parent, parentRect);
        OffsetRect(&rc, parentRect.left, parentRect.top);
    }

    transparentColor_.SetFromCOLORREF(backgroundColor);
    roundedBorder_ = roundedBorder;
    DWORD style, exStyle;
    if (child) {
        style = WS_CHILD;
        exStyle = 0;
    } else {
        style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        exStyle = WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
        if (topMost) {
            exStyle |= WS_EX_TOPMOST;
        }
    }

    movable_ = !child;

    HWND wnd = TParent::Create(parent, rc, _T("ImageEditor Toolbar"), style, exStyle);
    if ( !wnd ) {
        LOG(ERROR) << WinUtils::GetLastErrorAsString();
        return false;
    }

    return true;
}

int Toolbar::addButton(const Item& item)
{
    buttons_.push_back(item);
    return buttons_.size()-1;
}

int Toolbar::getItemAtPos(int clientX, int clientY) const
{
    POINT pt = {clientX, clientY};
    for (size_t i = 0; i < buttons_.size(); i++) {
        if ( PtInRect(&buttons_[i].rect,pt ) ) {
            return i;
        }
    }
    return -1;
}

int Toolbar::getItemIndexByCommand(int command) const
{
    for (size_t i = 0; i < buttons_.size(); i++) {
        if (buttons_[i].command == command ) {
            return i;
        }
    }
    return -1;
}

Toolbar::Item* Toolbar::getItem(int index)
{
    return &buttons_[index];
}

void Toolbar::repaintItem(int index)
{
    Item item = buttons_[index];
    InvalidateRect(&item.rect, false);
}

void Toolbar::clickButton(int index)
{
    if ( index < 0 || index >= static_cast<int>(buttons_.size()) ) {
        LOG(ERROR) << "Out of range";
    }
    Item& item = buttons_[index];
    selectedItemIndex_ = index;
    if ( item.checkable ) {

        item.isChecked = item.group!=-1|| !item.isChecked;
        //item.state = item.isChecked ? isChecked : isNormal;
    } else {
        //item.state = isNormal;
    }
    if ( item.group != -1 ) {

        // Uncheck all other buttons with same group id
        for (size_t i = 0; i < buttons_.size(); i++) {
            if ( i != static_cast<size_t>(index) && buttons_[i].group == item.group && buttons_[i].checkable && buttons_[i].isChecked ) {
                buttons_[i].isChecked  = false;
                buttons_[i].state = isNormal;
                InvalidateRect(&buttons_[i].rect, FALSE);
            }
        }
    }

    InvalidateRect(&item.rect, FALSE);
}

LRESULT Toolbar::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    LONG lStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
    if ( lStyle & WS_CHILD ) {
        //transparentColor_.SetFromCOLORREF(GetSysColor(COLOR_APPWORKSPACE));
    } else {
        SetLayeredWindowAttributes(m_hWnd, RGB(transparentColor_.GetR(),transparentColor_.GetG(),transparentColor_.GetB()),225,LWA_COLORKEY/*| LWA_ALPHA*/);
    }
    //lStyle &= ~(WS_CAPTION /*| WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU*/);
    //::SetWindowLong(m_hWnd, GWL_STYLE, lStyle);
    CClientDC hdc(m_hWnd);
    int dpiX = DPIHelper::GetDpiForWindow(m_hWnd);
    int dpiY = dpiX;

    setDPI(dpiX, dpiY);

    backBufferDc_.CreateCompatibleDC(hdc);

    LOGFONT logFont;
    systemFont_.GetLogFont(&logFont);
    if ( logFont.lfQuality & CLEARTYPE_QUALITY ) {
        textRenderingHint_ = Gdiplus::TextRenderingHintClearTypeGridFit;
    } else if ( logFont.lfQuality & ANTIALIASED_QUALITY ) {
        textRenderingHint_ = Gdiplus::TextRenderingHintAntiAliasGridFit;
    }

    tooltip_.Create(m_hWnd, rcDefault, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, WS_EX_TOPMOST);
    tooltip_.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    if (createSubPanel_) {
        subpanelHeight_ = static_cast<int>(27 * dpiScaleY_);
        subpanelLeftOffset_ = static_cast<int>(50 * dpiScaleX_);
        RECT sliderRect = { 0, 0, static_cast<LONG>(100 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 2 * dpiScaleY_) };
        if (orientation_ == orHorizontal) {
            penSizeSlider_.Create(m_hWnd, sliderRect, 0, WS_CHILD | WS_VISIBLE | TBS_NOTICKS);
            createHintForControl(penSizeSlider_.m_hWnd, TR("Line thickness"));
            RECT pixelLabelRect = { 0, 0, static_cast<LONG>(45 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 5 * dpiScaleY_) };
            pixelLabel_.Create(m_hWnd, pixelLabelRect, L"px", WS_CHILD | WS_VISIBLE | SS_NOTIFY);
            pixelLabel_.SetFont(systemFont_);
            createHintForControl(pixelLabel_, TR("Line thickness"));

            RECT radiusSliderRect = { 0, 0, static_cast<LONG>(100 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 2 * dpiScaleY_) };
            roundRadiusSlider_.Create(m_hWnd, radiusSliderRect, 0, WS_CHILD | TBS_NOTICKS);
            createHintForControl(roundRadiusSlider_.m_hWnd, TR("Rounding radius"));
            //RECT radiusLabelRect = { 0, 0, static_cast<LONG>(45 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 5 * dpiScaleY_) };
            roundRadiusLabel_.Create(m_hWnd, pixelLabelRect, L"px", WS_CHILD | SS_NOTIFY);
            roundRadiusLabel_.SetFont(systemFont_);
            createHintForControl(roundRadiusLabel_, TR("Rounding radius"));

            RECT blurRadiusSliderRect = { 0, 0, static_cast<LONG>(100 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 2 * dpiScaleY_) };
            blurRadiusSlider_.Create(m_hWnd, blurRadiusSliderRect, 0, WS_CHILD | TBS_NOTICKS);
            createHintForControl(blurRadiusSlider_.m_hWnd, TR("Blur radius"));
            //RECT radiusLabelRect = { 0, 0, static_cast<LONG>(45 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 5 * dpiScaleY_) };

            blurRadiusLabel_.Create(m_hWnd, pixelLabelRect, L"px", WS_CHILD | SS_NOTIFY);
            blurRadiusLabel_.SetFont(systemFont_);
            createHintForControl(blurRadiusLabel_, TR("Blur radius"));

            RECT fontSizeLabelRect = { 0, 0, static_cast<LONG>(100 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 2 * dpiScaleY_) };

            fontSizeLabel_.Create(m_hWnd, fontSizeLabelRect, TR("Font size:"), WS_CHILD);
            fontSizeLabel_.SetFont(systemFont_);

            RECT fontSizeEditRect = { 0, 0, static_cast<LONG>(63 * dpiScaleX_), static_cast<LONG>(22 * dpiScaleY_) };

            fontSizeEdit_.Create(m_hWnd, fontSizeEditRect, nullptr, WS_CHILD | ES_NUMBER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, (HMENU)ID_FONTSIZEEDITCONTROL);
            fontSizeEdit_.SetFont(systemFont_);

            RECT fontSizeUpDownRect = { 0, 0, static_cast<LONG>(30 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 4 * dpiScaleY_) };

            fontSizeUpDownCtrl_.Create(m_hWnd, fontSizeUpDownRect, _T(""), WS_CHILD | UDS_SETBUDDYINT | /* UDS_ALIGNRIGHT | */ UDS_ARROWKEYS | UDS_HOTTRACK);
            fontSizeUpDownCtrl_.SetRange(1, 100);

            RECT initialValueLabelRect { 0, 0, static_cast<LONG>(100 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 4 * dpiScaleY_) };

            initialValueLabel_.Create(m_hWnd, initialValueLabelRect, TR("Initial value:"), WS_CHILD);
            initialValueLabel_.SetFont(systemFont_);

            RECT initialValueEditRect { 0, 0, static_cast<LONG>(40 * dpiScaleX_), static_cast<LONG>(22 * dpiScaleY_) };

            initialValueEdit_.Create(m_hWnd, initialValueEditRect, _T(""), WS_CHILD | ES_NUMBER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_STEPINITIALVALUE);
            initialValueEdit_.SetFont(systemFont_);

            RECT fillBackgroundCheckboxRect { 0, 0, static_cast<LONG>(100 * dpiScaleX_), static_cast<LONG>(22 * dpiScaleY_) };

            fillBackgroundCheckbox_.Create(m_hWnd, fillBackgroundCheckboxRect, TR("Fill background"), WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 0, ID_FILLBACKGROUNDCHECKBOX);
            fillBackgroundCheckbox_.SetFont(systemFont_);

            RECT invertSelectionCheckboxRect { 0, 0, static_cast<LONG>(200 * dpiScaleX_), static_cast<LONG>(22 * dpiScaleY_) };

            invertSelectionCheckbox_.Create(m_hWnd, invertSelectionCheckboxRect, TR("Invert selection"), WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 0, ID_INVERTSELECTIONCHECKBOX);
            invertSelectionCheckbox_.SetFont(systemFont_);

            RECT drawBorderCheckboxRect { 0, 0, static_cast<LONG>(170 * dpiScaleX_), static_cast<LONG>(22 * dpiScaleY_) };

            drawBorderCheckbox_.Create(m_hWnd, drawBorderCheckboxRect, TR("Draw border"), WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 0, ID_DRAWBORDERCHECKBOX);
            drawBorderCheckbox_.SetFont(systemFont_);

            RECT arrowTypeComboRect { 0, 0, static_cast<LONG>(100 * dpiScaleX_), static_cast<LONG>(22 * dpiScaleY_) };

            arrowTypeCombobox_.Create(m_hWnd, arrowTypeComboRect, _T(""), WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, ID_ARROWTYPECOMBOBOX);
            arrowTypeCombobox_.SetFont(systemFont_);

            createHintForControl(arrowTypeCombobox_, TR("Type of arrow"));
            int itemIndex = arrowTypeCombobox_.AddString(_T(""));
            setArrowComboboxMode(itemIndex, static_cast<int>(Arrow::ArrowMode::Mode1));
            itemIndex = arrowTypeCombobox_.AddString(_T(""));
            setArrowComboboxMode(itemIndex, static_cast<int>(Arrow::ArrowMode::Mode2));

            RECT applyButtonRect { 0, 0, static_cast<LONG>(160 * dpiScaleX_), static_cast<LONG>(22 * dpiScaleY_) };
            applyButton_.Create(m_hWnd, applyButtonRect, TR("Apply crop"), WS_CHILD | BS_PUSHBUTTON | BS_DEFPUSHBUTTON, 0, ID_APPLYBUTTON);
            applyButton_.SetFont(systemFont_);

            RECT cancelButtonRect { 0, 0, static_cast<LONG>(160 * dpiScaleX_), static_cast<LONG>(22 * dpiScaleY_) };
            cancelOperationButton_.Create(m_hWnd, cancelButtonRect, TR("Cancel"), WS_CHILD | BS_PUSHBUTTON, 0, ID_CANCELOPERATIONBUTTON);
            cancelOperationButton_.SetFont(systemFont_);
        }
    }
    return 0;
}

LRESULT Toolbar::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    if ( (HWND)lParam == penSizeSlider_.m_hWnd ||   (HWND)lParam == roundRadiusSlider_.m_hWnd
        || (HWND)lParam == blurRadiusSlider_.m_hWnd) {
        ::SendMessage(GetParent(),uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT Toolbar::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if ( WinUtils::IsWine() ) {
        HDC dc = (HDC) wParam;
        SetBkMode(dc, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    return (LRESULT)(HBRUSH)subpanelBrush_;
}

LRESULT Toolbar::OnActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /*if ( wParam == WA_CLICKACTIVE  || wParam == WA_ACTIVE) {
        ::SetActiveWindow((HWND)lParam);
        bHandled = true;
    }*/
    return 0;
}


LRESULT Toolbar::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    using namespace Gdiplus;
    CDC &dc = backBufferDc_;

    CRect clientRect;
    bHandled = true;
    GetClientRect(&clientRect);
    Gdiplus::Graphics gr(dc);
    gr.SetInterpolationMode(InterpolationModeHighQualityBicubic );
    gr.SetPageUnit(Gdiplus::UnitPixel);

    Rect rect( 0, 0, clientRect.right, clientRect.bottom);
    SolidBrush br1(transparentColor_);
    gr.FillRectangle(&br1, rect);
        Pen p(Color(1,87,124));
    LinearGradientBrush br (RectF(float(0), float(-0.5 ), float(buttonsRect_.right),
            /*rect.top+*/ float(buttonsRect_.bottom) ), Color(252,252,252), Color(
            200,200,200), orientation_ == orHorizontal ? LinearGradientModeVertical : LinearGradientModeHorizontal);

    rect = Rect(subpanelLeftOffset_, static_cast<INT>(buttonsRect_.bottom - 2 * dpiScaleY_), static_cast<INT>(kSubpanelWidth*dpiScaleX_), clientRect.bottom);
    rect.Height -= rect.Y;

    SolidBrush br2 (subpanelColor_);
    rect.Width--;
    rect.Height --;
    ImageUtils::DrawRoundedRectangle(&gr,rect,7,&p, &br2);


    rect = Rect( 0, 0, buttonsRect_.right, buttonsRect_.bottom);
    rect.Width--;
    rect.Height --;

    //Gdiplus::SolidBrush br(Color(200,2,146,209));

    gr.SetSmoothingMode(GetStyle()&WS_CHILD ? SmoothingModeAntiAlias : SmoothingModeDefault);
    if (roundedBorder_) {
        ImageUtils::DrawRoundedRectangle(&gr, rect, 7, &p, &br);
    } else {
        gr.FillRectangle(&br, rect);
        gr.DrawRectangle(&p, rect);
    }

    //

    gr.SetSmoothingMode(SmoothingModeAntiAlias);

    //gr.FillRectangle(&br,rect);
    int x = itemMargin_;
    int y = itemMargin_;
    for (size_t i = 0; i < buttons_.size(); i++) {
        SIZE s = CalcItemSize(&gr, i, x, y);
        drawItem(i, &gr, x, y);

        if ( orientation_ == orHorizontal ) {
            x += s.cx + itemMargin_;
        } else {
            y += s.cy + itemMargin_;
        }
        //y+= s.cy;cli
    }

    if ( !(GetStyle()&WS_CHILD)  ) { // fix artefacts
        SolidBrush br(transparentColor_);
        gr.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        gr.FillRectangle(&br, Rect(buttonsRect_.right-1, buttonsRect_.bottom-1, 1,1));
    }

    CPaintDC realDc(m_hWnd);
    realDc.BitBlt(0, 0, clientRect.Width(), clientRect.Height(), dc, 0, 0, SRCCOPY);

    return 0;
}

LRESULT Toolbar::OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);

    if(!trackMouse_) // Capturing mouse
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        ::_TrackMouseEvent(&tme); // We want to receive WM_MOUSELEAVE message
        trackMouse_ = true;
    }

    if (selectedItemIndex_ != -1) {
        const auto& button = buttons_[selectedItemIndex_];
        if (button.state == isDropDown) {
            return 0;
        }
    }

    int oldSelectedIndex  = selectedItemIndex_;

    selectedItemIndex_ = getItemAtPos(xPos, yPos);

    if (oldSelectedIndex != selectedItemIndex_ ) {
        if (selectedItemIndex_ != -1) {
            auto& button = buttons_[selectedItemIndex_];
            if (button.enabled) {
                buttons_[selectedItemIndex_].state = isHover;
                InvalidateRect(&buttons_[selectedItemIndex_].rect, false);
            }
        }
        if (oldSelectedIndex != -1) {
            auto& button = buttons_[oldSelectedIndex];
            button.state = isNormal;
            InvalidateRect(&button.rect, false);
        }
    }
    return 0;
}

LRESULT Toolbar::OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    /*int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam); */
    trackMouse_ = false;
    if (selectedItemIndex_ >= 0 && selectedItemIndex_ < buttons_.size()) {
        const auto& button = buttons_[selectedItemIndex_];
        if (button.state == isDropDown) {
            return 0;
        }
        
        buttons_[selectedItemIndex_].state = isNormal;

        InvalidateRect(&buttons_[selectedItemIndex_].rect, false);
        selectedItemIndex_ = -1;
        trackMouse_ = false;
    }

    return 0;
}

LRESULT Toolbar::OnLButtonDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    bool moveParent = moveParent_;
    HWND parent = GetParent();

    selectedItemIndex_ = getItemAtPos(xPos, yPos);

    if (selectedItemIndex_ != -1) {
        Item& item = buttons_[selectedItemIndex_];
        if (item.enabled) {
            if (item.type == Toolbar::itComboButton && isPointInComboButtonDropdownArea(xPos, yPos, item)) {
                item.state = isDropDown;
            } else if (item.type == Toolbar::itTinyCombo) {
                if (isPointInTinyComboButtonDropdownArea(xPos, yPos, item)) {
                    item.state = isDropDown;
                } else {
                    SetTimer(kTinyComboDropdownTimer, 600);
                    item.state = isDown;
                }
            } else {
                item.state = isDown;
            }

            moveParent = moveParent && item.itemDelegate && !item.itemDelegate->needClick();

            InvalidateRect(&item.rect, false);

            if (item.state == isDropDown && (item.type == Toolbar::itComboButton || item.type == Toolbar::itTinyCombo)) {
                // Showing a popup menu in the parent class
                ::SendMessage(parent, MTBM_DROPDOWNMOUSEDOWN, (WPARAM)&item, (LPARAM)m_hWnd);
                // Restoring button state
                item.state = isNormal;
                InvalidateRect(&item.rect, false);
            }   
        }
    }

    if (moveParent) {
        ReleaseCapture();
        SendMessage(parent, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }

    return 0;
}

LRESULT Toolbar::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (wParam == kTinyComboDropdownTimer && selectedItemIndex_ != -1) {
        Item& item = buttons_[selectedItemIndex_];
        if (item.type == Toolbar::itTinyCombo) {
            ::PostMessage(GetParent(), MTBM_DROPDOWNMOUSEDOWN, (WPARAM)&item,(LPARAM)m_hWnd);
        }
    }
    KillTimer(kTinyComboDropdownTimer);
    return 0;
}

LRESULT Toolbar::OnLButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    RECT clientRect;
    KillTimer(kTinyComboDropdownTimer);
    GetClientRect(&clientRect);
    POINT pt = { xPos, yPos };
    if (!::PtInRect(&clientRect, pt))  {
        return 0;
    }
    if (selectedItemIndex_ != -1) {
        selectedItemIndex_ = getItemAtPos(xPos, yPos);
        Item& item = buttons_[selectedItemIndex_];

        if (item.enabled) {
            if (item.itemDelegate) {
                clickButton(selectedItemIndex_);
                item.itemDelegate->OnClick(xPos, yPos, dpiScaleX_, dpiScaleY_);
            } else {
                HWND parent = GetParent();
                
                bool isDropDownArea = false;

                if (item.type == Toolbar::itComboButton) {
                    isDropDownArea = isPointInComboButtonDropdownArea(xPos, yPos, item);
                } else if (item.type == Toolbar::itTinyCombo) {
                    isDropDownArea = isPointInTinyComboButtonDropdownArea(xPos, yPos, item);
                }
                
                if (isDropDownArea) {
                    ::SendMessage(parent, MTBM_DROPDOWNCLICKED, (WPARAM)&item, (LPARAM)m_hWnd);
                } else {
                    ::SendMessage(parent, WM_COMMAND, MAKEWPARAM(item.command, BN_CLICKED), (LPARAM)m_hWnd);
                    selectedItemIndex_ = -1;
                    OnMouseMove(WM_MOUSEMOVE, wParam, lParam, bHandled);      
                }               
            }
        } else {
            selectedItemIndex_ = -1;
        }
    }

    return 0;
}


LRESULT Toolbar::OnRButtonDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    const int xPos = GET_X_LPARAM(lParam);
    const int yPos = GET_Y_LPARAM(lParam);

    selectedItemIndex_ = getItemAtPos(xPos, yPos);
    if (selectedItemIndex_ == -1) {
        return 0;
    }

    Item& item = buttons_[selectedItemIndex_];
    if (item.type == Toolbar::itTinyCombo) {
        item.state = isDropDown;
        InvalidateRect(&item.rect, false);
        HWND parent = GetParent();
        ::SendMessage(parent, MTBM_DROPDOWNMOUSEDOWN, (WPARAM)&item, (LPARAM)m_hWnd);

        item.state = isNormal;
        selectedItemIndex_ = -1;
        InvalidateRect(&item.rect, false);
    }
    return 0;
}

LRESULT Toolbar::OnRButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT Toolbar::OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    return 1; // avoid flicker
}

LRESULT Toolbar::OnNcHitTest(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    bHandled = false;
    if (movable_ && ::GetKeyState(VK_MENU) & 0x8000) {
        // User can drag toolbars with ALT pressed
        bHandled = true;
        return HTCAPTION;
    }
    return 0;
}

SIZE Toolbar::CalcItemSize(Gdiplus::Graphics* gr, int index, int x, int y, bool expand) {
    using namespace Gdiplus;
    SIZE res={0,0};
    Item &item = buttons_[index];

    if (item.itemDelegate) {
        return item.itemDelegate->CalcItemSize(item, x, y, dpiScaleX_, dpiScaleY_);
    }

    if (showButtonText_ && item.title.GetLength()) {
        PointF origin(0,0);
        RectF textBoundingBox;
        if (  gr->MeasureString(item.title, item.title.GetLength(), font_.get(), origin, &textBoundingBox) == Ok ) {
            res.cx = static_cast<LONG>(textBoundingBox.Width);
            res.cy = static_cast<LONG>(textBoundingBox.Height);
        }
    }

    if (item.icon) {
        res.cx += iconSizeX_ + ((item.title.IsEmpty() || item.type == itTinyCombo) ? 0 : itemHorPadding_);
        res.cy = std::max<int>(iconSizeY_, res.cy);
    }

    if (item.type == itComboButton) {
        res.cx += static_cast<int>(kDropdownIconSize * dpiScaleX_);
    } else if (item.type == itTinyCombo) {
        //res.cx += static_cast<int>(kTinyDropdownIconSize * dpiScaleX_);
    }

    res.cx += itemHorPadding_ * 2;
    res.cy += itemVertPadding_ * 2;

    if (expand) {
        if (orientation_ == orVertical) {
            res.cx = max(res.cx, buttonsRect_.right - buttonsRect_.left - itemMargin_ * 2);
        } else {
            res.cy = max(res.cy, buttonsRect_.bottom - buttonsRect_.top - itemMargin_ * 2);
        }
    }

    return res;
}

int Toolbar::AutoSize() {
    int width = 0;
    int height = 0;
    CClientDC dc(m_hWnd);
    Gdiplus::Graphics gr(dc);

    if (!dpiScaleX_ || !dpiScaleY_) {
        setDPI(dc.GetDeviceCaps(LOGPIXELSX), dc.GetDeviceCaps(LOGPIXELSY));
    }

    int x = itemMargin_;
    int y = itemMargin_;

    for (size_t i = 0; i < buttons_.size(); i++) {
        SIZE s = CalcItemSize(&gr, i, x, y);
        Item& item = buttons_[i];
        //Gdiplus::RectF bounds(static_cast<Gdiplus::REAL>(x), static_cast<Gdiplus::REAL>(y), float(s.cx), float(s.cy));
        item.rect.left = x;
        item.rect.top = y;
        item.rect.right = s.cx + x;
        item.rect.bottom = s.cy + y;

        if (orientation_ == orHorizontal) {
            x += s.cx + itemMargin_;
            width = std::max<int>(x, subpanelLeftOffset_ + static_cast<int>((kSubpanelWidth + 20) * dpiScaleX_));
            height = std::max<int>(s.cy + itemMargin_ * 2, height);
        }
        else {
            y += s.cy + itemMargin_;
            height = y;
            width = std::max<int>(s.cx + itemMargin_ * 2, width);
        }
    }

    SetWindowPos(0, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);

    GetClientRect(&buttonsRect_);

    for (size_t i = 0; i < buttons_.size(); i++) {
        const RECT& rc = buttonsRect_;
        Item& item = buttons_[i];
        if (orientation_ == orVertical) {
            int width = std::max(item.rect.right - item.rect.left, rc.right - itemMargin_ * 2);
            item.rect.right = width + item.rect.left;
        } else {
            int height = std::max(item.rect.bottom - item.rect.top, rc.bottom - itemMargin_ * 2);
            item.rect.bottom = height + item.rect.top;
        }
    }



    if ( orientation_ == orHorizontal && createSubPanel_) {
        SetWindowPos(0, 0,0,width,height + subpanelHeight_,SWP_NOMOVE|SWP_NOZORDER);
        penSizeSlider_.SetWindowPos(0, subpanelLeftOffset_ + static_cast<int>(3 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + dpiScaleY_), 0, 0, SWP_NOSIZE|SWP_NOZORDER);
        penSizeSlider_.SetRange(1,Canvas::kMaxPenSize);
        RECT penSizeSliderRect;
        penSizeSlider_.GetClientRect(&penSizeSliderRect);
        penSizeSlider_.ClientToScreen(&penSizeSliderRect);
        ScreenToClient(&penSizeSliderRect);
        pixelLabel_.SetWindowPos(0, penSizeSliderRect.right, static_cast<int>(buttonsRect_.bottom + 3 * dpiScaleY_), 0, 0, SWP_NOSIZE);

        RECT pixelLabelRect_;
        pixelLabel_.GetClientRect(&pixelLabelRect_);
        pixelLabel_.ClientToScreen(&pixelLabelRect_);
        ScreenToClient(&pixelLabelRect_);

        roundRadiusSlider_.SetWindowPos(0, subpanelLeftOffset_ + static_cast<int>(150 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + 1 * dpiScaleY_), 0, 0, SWP_NOSIZE| SWP_NOZORDER);
        roundRadiusSlider_.SetRange(1,Canvas::kMaxRoundingRadius);
        RECT radiusSliderRect;
        roundRadiusSlider_.GetClientRect(&radiusSliderRect);
        roundRadiusSlider_.ClientToScreen(&radiusSliderRect);
        ScreenToClient(&radiusSliderRect);
        roundRadiusLabel_.SetWindowPos(0, radiusSliderRect.right, buttonsRect_.bottom + static_cast<int>(3 * dpiScaleY_), 0, 0, SWP_NOSIZE| SWP_NOZORDER);

        RECT roundRadiusLabelRect;
        roundRadiusLabel_.GetClientRect(&roundRadiusLabelRect);
        roundRadiusLabel_.ClientToScreen(&roundRadiusLabelRect);
        ScreenToClient(&roundRadiusLabelRect);

        // Blur radius
        blurRadiusSlider_.SetWindowPos(0, subpanelLeftOffset_ + static_cast<int>(3 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + 1 * dpiScaleY_), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        blurRadiusSlider_.SetRange(BLUR_RADIUS_PRECISION, Canvas::kMaxBlurRadius * BLUR_RADIUS_PRECISION);
        RECT blurRadiusSliderRect;
        blurRadiusSlider_.GetClientRect(&blurRadiusSliderRect);
        blurRadiusSlider_.ClientToScreen(&blurRadiusSliderRect);
        ScreenToClient(&blurRadiusSliderRect);
        blurRadiusLabel_.SetWindowPos(0, blurRadiusSliderRect.right, buttonsRect_.bottom + static_cast<int>(3 * dpiScaleY_), 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        RECT blurRadiusLabelRect;
        blurRadiusLabel_.GetClientRect(&blurRadiusLabelRect);
        blurRadiusLabel_.ClientToScreen(&blurRadiusLabelRect);
        ScreenToClient(&blurRadiusLabelRect);
        //RECT fontSizeLabelRect = { 0, 0, static_cast<LONG>(100 * dpiScaleX_), static_cast<LONG>(subpanelHeight_ - 2 * dpiScaleY_) };

        GuiTools::AutoSizeStaticControl(fontSizeLabel_);
        RECT fontSizeLabelRect;
        fontSizeLabel_.GetWindowRect(&fontSizeLabelRect);
        ScreenToClient(&fontSizeLabelRect);
        fontSizeLabel_.SetWindowPos(0, subpanelLeftOffset_ + static_cast<int>(6 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + (subpanelHeight_ - fontSizeLabelRect.bottom + fontSizeLabelRect.top)/2), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        fontSizeLabel_.GetWindowRect(&fontSizeLabelRect);
        ScreenToClient(&fontSizeLabelRect);

        fontSizeEdit_.SetWindowPos(0, fontSizeLabelRect.right + static_cast<int>(8 * dpiScaleX_), buttonsRect_.bottom + static_cast<int>(2 * dpiScaleY_), 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        if (!fontSizeUpDownCtrl_.GetBuddy()) {
            fontSizeUpDownCtrl_.SetBuddy(fontSizeEdit_);
        }
        CRect editRect;
        fontSizeEdit_.GetWindowRect(&editRect);
        ScreenToClient(&editRect);

        int spinWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXVSCROLL, dpiX_);
        fontSizeUpDownCtrl_.SetWindowPos(nullptr,
            editRect.right, editRect.top,
            spinWidth, editRect.Height(),
            SWP_NOZORDER | SWP_NOACTIVATE);

        RECT upDownRect, rect;
        fontSizeUpDownCtrl_.GetWindowRect(&upDownRect);
        ScreenToClient(&upDownRect);
        GuiTools::AutoSizeStaticControl(initialValueLabel_);
        initialValueLabel_.GetWindowRect(&rect);
        ScreenToClient(&rect);

        initialValueLabel_.SetWindowPos(nullptr, upDownRect.right + static_cast<int>(38 * dpiScaleX_), buttonsRect_.bottom + (subpanelHeight_ - rect.bottom + rect.top) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        initialValueLabel_.GetWindowRect(&rect);
        ScreenToClient(&rect);

        initialValueEdit_.SetWindowPos(0, rect.right + static_cast<int>(8 * dpiScaleX_), buttonsRect_.bottom + static_cast<int>(2 * dpiScaleY_), 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        // Moving fill background checkbox
        fillBackgroundCheckbox_.SetWindowPos(0, subpanelLeftOffset_ + static_cast<int>(3 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + dpiScaleY_), 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        invertSelectionCheckbox_.SetWindowPos(0, blurRadiusLabelRect.right + static_cast<int>(6 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + dpiScaleY_), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        drawBorderCheckbox_.SetWindowPos(0, pixelLabelRect_.right + static_cast<int>(15 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + dpiScaleY_), 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        arrowTypeCombobox_.SetWindowPos(0, pixelLabelRect_.right+ int(3 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + dpiScaleY_), 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        applyButton_.SetWindowPos(0, subpanelLeftOffset_ + static_cast<int>(8 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + dpiScaleY_*2), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        cancelOperationButton_.SetWindowPos(0, subpanelLeftOffset_ + static_cast<int>(175 * dpiScaleX_), static_cast<int>(buttonsRect_.bottom + dpiScaleY_ * 2), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    for (size_t i = 0; i < buttons_.size(); i++) {
        CreateToolTipForItem(i);
    }
    tooltip_.SetMaxTipWidth(MulDiv(300, dpiX_, USER_DEFAULT_SCREEN_DPI));

    CRect clientRect;
    GetClientRect(&clientRect);

    if (backBuffer_.m_hBitmap) {
        backBufferDc_.SelectBitmap(oldSelectedBm_);
        backBuffer_.DeleteObject();
    }

    backBuffer_.CreateCompatibleBitmap(dc, clientRect.Width(), clientRect.Height());
    oldSelectedBm_ = backBufferDc_.SelectBitmap(backBuffer_);
    return 1;
}

void Toolbar::setDPI(int newDpiX, int newDpiY) {
    if (newDpiX && newDpiY) {
        int oldDpiX = dpiX_;
        int oldDpiY = dpiY_;

        dpiX_ = newDpiX;
        dpiY_ = newDpiY;
        dpiScaleX_ = newDpiX / 96.0f;
        dpiScaleY_ = newDpiY / 96.0f;
        iconSizeX_ = static_cast<int>(kIconSize * dpiScaleX_);
        iconSizeY_ = static_cast<int>(kIconSize * dpiScaleY_);
        systemFont_ = GuiTools::GetSystemDialogFont(newDpiX);
        CClientDC dc(m_hWnd);
        font_ = std::make_unique<Gdiplus::Font>(dc, systemFont_);
        if (createSubPanel_) {
            subpanelHeight_ = static_cast<int>(27 * dpiScaleY_);
            subpanelLeftOffset_ = static_cast<int>(50 * dpiScaleX_);
        }

        itemMargin_ = static_cast<int>(kItemMargin * dpiScaleX_);
        itemHorPadding_ = orientation_ == orHorizontal ? static_cast<int>(kItemHorPadding * dpiScaleX_) : static_cast<int>(kVertItemHorPadding * dpiScaleX_);
        //vertItemHorPadding_ = static_cast<int>(kVertItemHorPadding * dpiScaleX_);
        itemVertPadding_ = static_cast<int>(kItemVertPadding * dpiScaleY_);

        dropDownIcon_ = GuiTools::CreateDropDownArrowBitmap(m_hWnd, kDropdownIconSize * dpiScaleX_, kDropdownIconSize * dpiScaleY_);
        tinyDropDownIcon_ = GuiTools::CreateDropDownArrowBitmap(m_hWnd, kTinyDropdownIconSize * dpiScaleX_, kTinyDropdownIconSize * dpiScaleY_);

        if (orientation_ == orHorizontal && createSubPanel_ && oldDpiX && oldDpiY) {
            struct SomeInfo {
                Toolbar* pthis;
                HFONT newFont;
                int oldDpiX, oldDpiY;
                int newDpiX, newDpiY;
            } si = {
                this, systemFont_, oldDpiX, oldDpiY, newDpiX, newDpiY
            };

            EnumChildWindows(m_hWnd, [](HWND hChild, LPARAM lParam) -> BOOL {
                auto si = reinterpret_cast<SomeInfo*>(lParam);
                ::SendMessage(hChild, WM_SETFONT, (WPARAM)si->newFont, TRUE);

                if (si->oldDpiX && si->oldDpiY) {
                    CRect rc {};

                    if (::GetWindowRect(hChild, &rc)) {
                        si->pthis->ScreenToClient(&rc);
                        int x = MulDiv(rc.left, si->newDpiX, si->oldDpiX);
                        int y = MulDiv(rc.top, si->newDpiY, si->oldDpiY);
                        int width = MulDiv(rc.Width(), si->newDpiX, si->oldDpiX);
                        int height = MulDiv(rc.Height(), si->newDpiY, si->oldDpiY);
                        ::SetWindowPos(hChild, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                }

                return TRUE; 
            },
            reinterpret_cast<LPARAM>(&si));

            if (fontSizeUpDownCtrl_ && fontSizeEdit_) {
                CRect editRect;
                fontSizeEdit_.GetWindowRect(&editRect);
                ScreenToClient(&editRect);

                int spinWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXVSCROLL, newDpiX);
                fontSizeUpDownCtrl_.SetWindowPos(nullptr,
                    editRect.right, editRect.top,
                    spinWidth, editRect.Height(),
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
            /*pixelLabel_.SetFont(systemFont_);
            roundRadiusLabel_.SetFont(systemFont_);
            blurRadiusLabel_.SetFont(systemFont_);
            fontSizeLabel_.SetFont(systemFont_);
            fontSizeEdit_.SetFont(systemFont_);
            initialValueLabel_.SetFont(systemFont_);
            initialValueEdit_.SetFont(systemFont_);
            fillBackgroundCheckbox_.SetFont(systemFont_);
            invertSelectionCheckbox_.SetFont(systemFont_);
            arrowTypeCombobox_.SetFont(systemFont_);
            applyButton_.SetFont(systemFont_);
            cancelOperationButton_.SetFont(systemFont_);*/
        }
    }
}

void Toolbar::drawItem(int itemIndex, Gdiplus::Graphics* gr, int x, int y)
{
    using namespace Gdiplus;
    SIZE size = CalcItemSize(gr, itemIndex, x, y, true);

    Item& item = buttons_[itemIndex];

    if ( item.itemDelegate ) {
        item.itemDelegate->DrawItem(item, gr, x, y, dpiScaleX_, dpiScaleY_);
        return;
    }

    RectF bounds(static_cast<Gdiplus::REAL>(x), static_cast<Gdiplus::REAL>(y), float(size.cx), float(size.cy));
    item.rect.left = x;
    item.rect.top = y;
    item.rect.right = size.cx + x;
    item.rect.bottom = size.cy + y;

    SolidBrush brush(item.enabled ? Color(0, 0, 0) : Color(120, 120, 120));

    if ( item.state == isHover ||  item.state == isDown ||  item.state == isDropDown || item.isChecked) {
            Pen p(Color(198,196,197));
            Color gradientColor1 = item.isChecked ?  Color(170,170,170) : Color(232,232,232);
            Color gradientColor2 = item.isChecked ? Color(130,130,130) : Color(170,170,170);
            LinearGradientBrush br (RectF(float(x), float(y ), float( x+size.cx),
                /*rect.top+*/ float(y+size.cy ) ), gradientColor1, gradientColor2, LinearGradientModeVertical);

            CRoundRect roundRect;
            roundRect.FillRoundRect(gr,&br,Rect(x, y, size.cx, size.cy),Color(198,196,197),4);

            if ( item.type == itComboButton ) {
                gr->DrawLine(&p, bounds.X + bounds.Width - kDropdownIconSize * dpiScaleX_ - 2*dpiScaleX_ ,  bounds.Y+ 1, bounds.X + bounds.Width - kDropdownIconSize * dpiScaleX_ -2 * dpiScaleX_, bounds.Y + bounds.Height - 1);
            }

    } /*else if ( item.state == isChecked ) {
        Pen p(Color(198,196,197));
        Color gradientColor1 = Color(200,200,200);
        Color gradientColor2 = Color(140,140,140);
        LinearGradientBrush br (RectF(float(0), float(0.5 ), float( size.cx),
            /*rect.top+* float(size.cy ) ), gradientColor1, gradientColor2, LinearGradientModeVertical);
        //    gr->FillRectangle( &brush, Rect(x, y, size.cx, size.cy));
        //br.TranslateTransform(x,y);
        //br.SetWrapMode(WrapModeTile);
        /*CRoundRect roundRect;
        roundRect.FillRoundRect(gr,&br,Rect(x, y, size.cx, size.cy),Color(198,196,197),4);
        //roundRect.DrawRoundRect(gr,Rect(x, y, size.cx, size.cy),Color(198,196,197),7, 1);
        //DrawRoundedRectangle(gr,Rect(x, y, size.cx, size.cy),8,&p, 0);
        /*if ( item.type == itComboButton ) {
            gr->DrawLine(&p, bounds.X + bounds.Width - dropDownIcon_->GetWidth()-3 ,  bounds.Y+1 , bounds.X + bounds.Width - dropDownIcon_->GetWidth()-3, bounds.Y + bounds.Height -1 );
        }*/

//    }

    REAL iconX = itemHorPadding_ + bounds.X + (item.state == isDown ? 1 : 0);

    if ( item.type == itComboButton ) {
        gr->DrawImage(dropDownIcon_.get(), bounds.GetRight() - kDropdownIconSize * dpiScaleX_ + (item.state == isDropDown ? 1 : 0), bounds.Y + (bounds.Height - kDropdownIconSize * dpiScaleY_) / 2 + (item.state == isDropDown ? 1 : 0) /* kDropdownIconSize * dpiScaleX_, kDropdownIconSize * dpiScaleY_*/);
    } else if ( item.type == itTinyCombo ) {
        int offsetX = 0;
        if (item.state == isDown) {
            offsetX = -1;
        } else if (item.state == isDropDown) {
            offsetX = 1;
        }

        gr->DrawImage(tinyDropDownIcon_.get(), iconX + iconSizeX_ - 1 + offsetX, bounds.Y + bounds.Height - kTinyDropdownIconSize * dpiScaleY_ + (item.state == isDropDown ? 1 : 0) /*, kTinyDropdownIconSize * dpiScaleX_, kTinyDropdownIconSize * dpiScaleY_*/);
    }

    if (item.icon) {
        Gdiplus::ImageAttributes imageAttr;
    
        // Create ColorMatrix for desaturation and transparency reduction
        Gdiplus::ColorMatrix colorMatrix = {
            0.3f, 0.3f, 0.3f, 0.0f, 0.0f, // Red
            0.3f, 0.3f, 0.3f, 0.0f, 0.0f, // Green
            0.3f, 0.3f, 0.3f, 0.0f, 0.0f, // Blue
            0.0f, 0.0f, 0.0f, 0.8f, 0.0f, // Alpha (50% opacity)
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f  // Translation
        };

        imageAttr.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

        int x = static_cast<int>(iconX);
        int y = static_cast<int>(bounds.Y + (item.state == isDown ? 1 : 0) + (bounds.Height - iconSizeY_) / 2);

        if (item.enabled) {
            gr->DrawImage(item.icon.get(), x, y, iconSizeX_, iconSizeY_);
        } else {
            gr->DrawImage(item.icon.get(), Gdiplus::Rect(x, y, iconSizeX_, iconSizeY_),
                0, 0, item.icon->GetWidth(), item.icon->GetHeight(), Gdiplus::UnitPixel, &imageAttr
            );
        }
    }

    if (showButtonText_) {
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        //gr->SetTextRenderingHint(textRenderingHint_);

        int iconWidth = (item.icon ? iconSizeX_/*+ itemHorPadding_*/ : 0);
        int textOffsetX = iconWidth + itemHorPadding_;
        RectF textBounds(textOffsetX + bounds.X + (item.state == isDown ? 1 : 0), bounds.Y + (item.state == isDown ? 1 : 0), bounds.Width - textOffsetX - (item.type == itComboButton ? 16 * dpiScaleX_ : 0), bounds.Height);
        if (orientation_ == orVertical) {
            //LOG(INFO) << "textBounds x="<<textBounds.X << " y = " << textBounds.Y << "   "<<item.title;
        }

        gr->DrawString(item.title, -1, font_.get(), textBounds, &format, &brush);
    }
}

void Toolbar::createHintForControl(HWND slider, CString hint) {
    TOOLINFO ti = {};
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_SUBCLASS;
    ti.hwnd     = slider;
    ti.hinst    = _Module.GetModuleInstance();
    auto textBuffer = std::make_unique<TCHAR[]>(hint.GetLength() + 1);
    StringCchCopy(textBuffer.get(), hint.GetLength() + 1, hint);
    ti.lpszText = textBuffer.get();
    ::GetClientRect(slider, &ti.rect);

    tooltip_.AddTool(&ti);
}

void Toolbar::CreateToolTipForItem(size_t index)
{
    Item& item = buttons_[index];

    std::vector<std::pair<RECT, CString>> hints;

    if (!item.hint.IsEmpty()) {
        hints.emplace_back(item.rect, item.hint);
    } else if (item.itemDelegate) {
        hints = item.itemDelegate->getSubItemsHints();
    } else {
        return;
    }

    
    for (const auto& hint : hints) {
        TOOLINFO ti = {};
        ti.cbSize = sizeof(TOOLINFO);
        ti.uFlags = TTF_SUBCLASS;
        ti.hwnd = m_hWnd;
        ti.hinst = _Module.GetModuleInstance();
        //CString textBuffer = item.hint;
        auto textBuffer = std::make_unique<TCHAR[]>(hint.second.GetLength() + 1);
        StringCchCopy(textBuffer.get(), hint.second.GetLength() + 1, hint.second);
        ti.lpszText = textBuffer.get();
        ti.rect = hint.first;
        ti.uId = static_cast<UINT_PTR>(index);
        tooltip_.AddTool(&ti);
    }
}

void Toolbar::updateTooltipForItem(size_t index) {
    Item& item = buttons_[index];

    tooltip_.UpdateTipText(item.hint.GetString(), m_hWnd, static_cast<UINT_PTR>(index));
}

LRESULT Toolbar::OnFontSizeEditControlChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ::SendMessage(GetParent(), MTBM_FONTSIZECHANGE, 0, 0);
    return 0;
}

int Toolbar::getFontSize() const {
    CString text = GuiTools::GetWindowText(fontSizeEdit_);
    return _ttoi(text);
}

void Toolbar::setStepFontSize(int fontSize) {
    fontSizeEdit_.SetWindowText(WinUtils::IntToStr(fontSize));
}

void Toolbar::showStepFontSize(bool show) {
    fontSizeEdit_.ShowWindow(show ? SW_SHOW : SW_HIDE);
    fontSizeLabel_.ShowWindow(show ? SW_SHOW : SW_HIDE);
    fontSizeUpDownCtrl_.ShowWindow(show ? SW_SHOW : SW_HIDE);

    initialValueLabel_.ShowWindow(show ? SW_SHOW : SW_HIDE);
    initialValueEdit_.ShowWindow(show ? SW_SHOW : SW_HIDE);
}

void Toolbar::showPenSize(bool show) {
    penSizeSlider_.ShowWindow(show ? SW_SHOW : SW_HIDE);
    pixelLabel_.ShowWindow(show ? SW_SHOW : SW_HIDE);
}

void Toolbar::setStepInitialValue(int value) {
    initialValueEdit_.SetWindowText(WinUtils::IntToStr(value));
}

int Toolbar::getStepInitialValue() const {
    CString text = GuiTools::GetWindowText(initialValueEdit_);
    return _ttoi(text);
}

LRESULT Toolbar::OnStepInitialValueChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ::SendMessage(GetParent(), MTBM_STEPINITIALVALUECHANGE, 0, 0);
    return 0;
}

LRESULT Toolbar::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    return 0;
}

SIZE Toolbar::getArrowComboBoxBitmapSize(int dpiX, int dpiY) {
    return {
        MulDiv(70, dpiX, USER_DEFAULT_SCREEN_DPI),
        MulDiv(18, dpiY, USER_DEFAULT_SCREEN_DPI)
    };
}

void Toolbar::setArrowComboboxMode(int itemIndex, int arrowType) {
    arrowTypeCombobox_.SetItemData(itemIndex, static_cast<DWORD_PTR>(arrowType));
}


bool Toolbar::isPointInComboButtonDropdownArea(int x, int y, const Item& item) {
    return x > static_cast<int>(item.rect.right - dropDownIcon_->GetWidth() - itemMargin_);
}


bool Toolbar::isPointInTinyComboButtonDropdownArea(int x, int y, const Item& item) {
    return x > item.rect.right - 6 * dpiScaleX_ - itemMargin_ && y > item.rect.bottom - 6 * dpiScaleY_ - itemMargin_;
}

LRESULT Toolbar::OnMeasureItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    LPMEASUREITEMSTRUCT lpmis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
    //CClientDC dc(m_hWnd);
    int dpiX = DPIHelper::GetDpiForWindow(m_hWnd);
    int dpiY = dpiX;  //dc.GetDeviceCaps(LOGPIXELSY);
    if (wParam == ID_ARROWTYPECOMBOBOX) {

        SIZE sz = getArrowComboBoxBitmapSize(dpiX, dpiY);
        int paddingY = /*/ MulDiv(2, dpiX, USER_DEFAULT_SCREEN_DPI)*/0;
        lpmis->itemWidth = std::max<int>(MulDiv(200, dpiX, USER_DEFAULT_SCREEN_DPI), sz.cx);

        if (lpmis->itemHeight < sz.cy + paddingY) {
            lpmis->itemHeight = sz.cy + paddingY;
        }

        return TRUE;
    }
    return FALSE;
}

LRESULT Toolbar::OnDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    using namespace Gdiplus;
    LPDRAWITEMSTRUCT lpdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

    if (wParam == ID_ARROWTYPECOMBOBOX) {
        COLORREF clrBackground;
        COLORREF clrForeground;
        TEXTMETRIC tm;
        int x;
        int y;
        HRESULT hr;
        size_t itemLength, cch;

        if (lpdis->itemID == -1) { // Empty item
            return FALSE;
        }

        auto arrowMode = static_cast<Arrow::ArrowMode>(lpdis->itemData);

        // The colors depend on whether the item is selected.
        clrForeground = SetTextColor(lpdis->hDC,
            GetSysColor((lpdis->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

        clrBackground = SetBkColor(lpdis->hDC,
            GetSysColor(lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : COLOR_WINDOW));

        // Calculate the vertical and horizontal position.
        GetTextMetrics(lpdis->hDC, &tm);
        y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
        x = LOWORD(GetDialogBaseUnits()) / 4;

        itemLength = ::SendMessage(lpdis->hwndItem, CB_GETLBTEXTLEN, lpdis->itemID, 0);
        if (itemLength == CB_ERR) {
            return FALSE;
        }
        CString buf;
        auto b = buf.GetBuffer(itemLength + 1);
        // Get and display the text for the list item.
        ::SendMessage(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID, (LPARAM)b);
        buf.ReleaseBuffer(itemLength);
        int dpiX = DPIHelper::GetDpiForWindow(m_hWnd);
        int dpiY = dpiX; 
        SIZE bitmapSize = getArrowComboBoxBitmapSize(dpiX, dpiY);
        ExtTextOut(lpdis->hDC, bitmapSize.cx + 2 * x, y,
            ETO_CLIPPED | ETO_OPAQUE, &lpdis->rcItem,
            buf, buf.GetLength(), NULL);

        {
            Graphics gr(lpdis->hDC);
            gr.SetPageUnit(Gdiplus::UnitPixel);
            gr.SetSmoothingMode(SmoothingModeAntiAlias);
            Color clr;
            clr.SetFromCOLORREF(GetSysColor((lpdis->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

            int bitmapY = (lpdis->rcItem.bottom + lpdis->rcItem.top) / 2;
            Arrow::render(&gr, clr, 3, { 1, bitmapY }, { bitmapSize.cx + 1, bitmapY }, arrowMode);
        }

        // Restore the previous colors.
        SetTextColor(lpdis->hDC, clrForeground);
        SetBkColor(lpdis->hDC, clrBackground);

        // If the item has the focus, draw the focus rectangle.
        if (lpdis->itemState & ODS_FOCUS) {
            DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
        }

        return TRUE;
    }
    return FALSE;
}


LRESULT Toolbar::OnDPICHanged(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    int dpiX = LOWORD(wParam);
    int dpiY = HIWORD(wParam);
    setDPI(dpiX, dpiY);
    AutoSize();
    update();
    return 0;
}

void Toolbar::showFillBackgroundCheckbox(bool show) {
    fillBackgroundCheckbox_.ShowWindow(show ? SW_SHOW : SW_HIDE);
}

void Toolbar::showInvertSelectionCheckbox(bool show) {
    invertSelectionCheckbox_.ShowWindow(show ? SW_SHOW : SW_HIDE);
}

void Toolbar::showDrawBorderCheckbox(bool show) {
    drawBorderCheckbox_.ShowWindow(show ? SW_SHOW : SW_HIDE);
}

void Toolbar::showArrowTypeCombo(bool show) {
    arrowTypeCombobox_.ShowWindow(show ? SW_SHOW : SW_HIDE);
}

LRESULT Toolbar::OnFillBackgroundCheckboxClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ::SendMessage(GetParent(), MTBM_FILLBACKGROUNDCHANGE, 0, 0);
    return 0;
}

LRESULT Toolbar::OnInvertSelectionCheckboxClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ::SendMessage(GetParent(), MTBM_INVERTSELECTIONCHANGE, 0, 0);
    return 0;
}


LRESULT Toolbar::OnDrawBorderCheckboxClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ::SendMessage(GetParent(), MTBM_DRAWBORDERCHANGE, 0, 0);
    return 0;
}

LRESULT Toolbar::OnArrowTypeComboChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ::SendMessage(GetParent(), MTBM_ARROWTYPECHANGE, 0, 0);
    return 0;
}

LRESULT Toolbar::OnApplyButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ::SendMessage(GetParent(), MTBM_APPLY, 0, 0);
    return 0;
}

LRESULT Toolbar::OnCancelOperationButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ::SendMessage(GetParent(), MTBM_CANCEL, 0, 0);
    return 0;
}

bool Toolbar::isFillBackgroundChecked() const {
    return fillBackgroundCheckbox_.GetCheck() == BST_CHECKED;
}

bool Toolbar::isInvertSelectionChecked() const {
    return invertSelectionCheckbox_.GetCheck() == BST_CHECKED;
}

int Toolbar::getArrowType() const {
    return arrowTypeCombobox_.GetCurSel();
}

void Toolbar::setArrowType(int type) {
    arrowTypeCombobox_.SetCurSel(type);
}

void Toolbar::setMovable(bool value) {
    movable_ = value;
}

void Toolbar::showApplyButtons(bool show) {
    applyButton_.ShowWindow(show ? SW_SHOW : SW_HIDE);
    cancelOperationButton_.ShowWindow(show ? SW_SHOW : SW_HIDE);
}

void Toolbar::setFillBackgroundCheckbox(bool fill) {
    fillBackgroundCheckbox_.SetCheck(fill ? BST_CHECKED : BST_UNCHECKED);
}

void Toolbar::setInvertSelectionCheckbox(bool invert) {
    invertSelectionCheckbox_.SetCheck(invert ? BST_CHECKED : BST_UNCHECKED);
}

bool Toolbar::isDrawBorderChecked() const {
    return drawBorderCheckbox_.GetCheck() == BST_CHECKED;
}

void Toolbar::setDrawBorderCheckbox(bool enable) {
    drawBorderCheckbox_.SetCheck(enable ? BST_CHECKED : BST_UNCHECKED);
}

void Toolbar::setShowButtonText(bool show) {
    showButtonText_ = show;
}

void Toolbar::enableButton(int index, bool enable) {
    if (index < 0 || index >= buttons_.size()) {
        return;
    }
    auto& button = buttons_[index];
    button.enabled = enable;
    InvalidateRect(&button.rect, FALSE);
}

Toolbar::Orientation Toolbar::orientation() const {
    return orientation_;
}

void Toolbar::setMoveParent(bool move) {
    moveParent_ = move;
}


void Toolbar::update() {
    AutoSize();
    Invalidate();
}

}
