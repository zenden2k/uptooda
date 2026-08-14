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

#include "HyperLinkControl.h"

#include <algorithm>

#include "Gui/GuiTools.h"
#include "Gui/Helpers/DPIHelper.h"
#include "HyperLinkControlAccessible.h"

namespace {

constexpr auto VERTICAL_PADDING = 5;

SIZE GetTextDimensions(HDC dc, LPCTSTR Text, HFONT Font) {
    SIZE sz = {};
    if (!Text) {
        return sz;
    }
    HGDIOBJ  OldFont = SelectObject(dc, Font);
    GetTextExtentPoint32(dc, Text, lstrlen(Text), &sz);
    SelectObject(dc, OldFont);
    return sz;
}

}
// CHyperLinkControl
CHyperLinkControl::CHyperLinkControl()
{
    bmpOld_ = {};
    MouseSel = false;
    Track = false;
    BottomY = 0;
    SubItemRightY = -1;
    selectedItemIndex_ = -1;
    mouseDownItemIndex_ = -1;
    hoverItemIndex_ = -1;
    CursorHand = false;
    m_bHyperLinks = true;
    m_BkColor = GetSysColor(COLOR_WINDOW);
    handCursor_ = LoadCursor(nullptr, IDC_HAND); // Loading "Hand" cursor into memory
    arrowCursor_ = LoadCursor(nullptr, IDC_ARROW);
    SetThemeClassList ( L"globals" );
}

CHyperLinkControl::~CHyperLinkControl()
{
    if (dcMem_.m_hDC) {
        SelectObject(dcMem_, bmpOld_);
    }
}

/* CHyperLinkControl::Init
-----------------------
Must be called before adding any items
*/
void CHyperLinkControl::Init(COLORREF BkColor)
{
    createGdiResources();
    m_BkColor = BkColor;
    OpenThemeData();
    createDoubleBuffer();
}

size_t CHyperLinkControl::ItemCount() const {
    return Items.GetCount();
}

CString CHyperLinkControl::GetItemTitle(size_t item) const {
    if (item < Items.GetCount()) {
        return Items[item].szTitle;
    }
    return {};
}

CString CHyperLinkControl::GetItemDescription(size_t item) const {
    if (item < Items.GetCount()) {
        return Items[item].szTip;
    }
    return {};
}

CRect CHyperLinkControl::GetItemRect(size_t itemIndex) const {
    if (itemIndex < Items.GetCount()) {
        return Items[itemIndex].ItemRect;
    }
    return {};
}

int CHyperLinkControl::SelectedIndex() const {
    return selectedItemIndex_;
}

int CHyperLinkControl::ItemFromPoint(POINT pt) const {
    RECT ClientRect;
    GetClientRect(&ClientRect);

    for (size_t i = 0; i< Items.GetCount(); i++) {
        if (!Items[i].Visible) {
            continue;
        }
        CRect rc(Items[i].ItemRect);
        /*if (!m_bHyperLinks && !Items[i].szTip.IsEmpty()) {
            rc.right = ClientRect.right - 6;
            rc.InflateRect(3, 6);
        }*/

        if (rc.PtInRect(pt)) {
            return i;
        }
    }
    return -1;
}

int CHyperLinkControl::GetTextWidth(HDC dc, LPCTSTR Text, HFONT Font)
{
    SIZE sz;
    HGDIOBJ  OldFont = SelectObject(dc, Font);
    GetTextExtentPoint32(dc, Text, lstrlen(Text), &sz);
    SelectObject(dc, OldFont);
    return sz.cx;
}

void CHyperLinkControl::createGdiResources() {
    NormalFont = GetFont();
    if (BoldFont) {
        BoldFont.DeleteObject();
    }
    BoldFont = GuiTools::MakeFontBold(NormalFont);
    if (UnderlineFont) {
        UnderlineFont.DeleteObject();
    }
    UnderlineFont = GuiTools::MakeFontUnderLine(NormalFont);
    if (BoldUnderLineFont) {
        BoldUnderLineFont.DeleteObject();
    }
    BoldUnderLineFont = GuiTools::MakeFontUnderLine(BoldFont);
}

int CHyperLinkControl::AddString(LPCTSTR szTitle, LPCTSTR szTip, int idCommand, HICON hIcon, bool Visible, int Align, bool LineBreak) {
    // TODO: This shit should be rewritten from scratch
    RECT ClientRect;
    GetClientRect(&ClientRect);

    HyperLinkControlItem item{};
    if (szTip) {
        item.szTip =szTip;
    }
    item.szTitle = szTitle;
    item.hIcon = hIcon;
    if (hIcon) {
        GuiTools::IconInfo iconInfo = GuiTools::GetIconInfo(hIcon);
        item.iconWidth = iconInfo.nWidth;
        item.iconHeight = iconInfo.nHeight;
    }

    item.idCommand = idCommand;
    item.Hover = false;
    item.Visible = Visible;
    CClientDC dc(m_hWnd);

    UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    const int iconSmallWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int iconSmallHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);
    const int iconBigWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXICON, dpi);
    const int iconBigHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYICON, dpi);

    auto scaleX = [dpi](int x) {
        return MulDiv(x, dpi, USER_DEFAULT_SCREEN_DPI);
    };

    auto scaleY = [dpi](int y) {
        return MulDiv(y, dpi, USER_DEFAULT_SCREEN_DPI);
    };

    SIZE tipDimensions = GetTextDimensions(dc, szTip, NormalFont);
    int szTipWidth = szTip ? tipDimensions.cx : 0;
    SIZE titleDimensions = GetTextDimensions(dc, szTitle, BoldFont);
    int TitleWidth = std::max<int>(titleDimensions.cx, szTipWidth);

    auto& itemRect = item.ItemRect;
    if (szTip) {
        if (SubItemRightY != -1) {
            BottomY += scaleY(12);
        }
        itemRect.left = scaleX(5);
        itemRect.top = BottomY;

        if (BottomY && m_bHyperLinks) {
            itemRect.top += scaleY(20);
        }

        itemRect.right = scaleX(10) + itemRect.left + iconBigWidth + TitleWidth + 1 /*ClientRect.right*/;
        int height;
        if (m_bHyperLinks) {
            height = std::max<int>(tipDimensions.cy + titleDimensions.cy + scaleY(3), iconBigHeight);
        } else {
            height = std::max<int>(tipDimensions.cy, iconBigHeight) + scaleY(VERTICAL_PADDING * 2);
        }

        itemRect.bottom = itemRect.top + height;

        BottomY = itemRect.bottom;
        SubItemRightY = -1;
    } else {
        int TextWidth = GetTextWidth(dc, szTitle, NormalFont);
        if (Align == 2) { // right aligned text
            //SubItemRightY = ClientRect.right - TextWidth - sc;
        } else {
            if (SubItemRightY == -1) {
                SubItemRightY = scaleY(35);
            } else {
                SubItemRightY += scaleY(20);
            }
        }
        if (LineBreak) {
            SubItemRightY = scaleY(35);
            BottomY += scaleY(20);
        }
        {
            itemRect.left = SubItemRightY;
            itemRect.top = BottomY + ((BottomY > 1) ? scaleY(6) : 0);
        }
        if (!m_bHyperLinks) {
            itemRect.top += scaleY(5);
        }
        itemRect.right = 1 + GetTextWidth(dc, szTitle, NormalFont) + iconSmallWidth + itemRect.left + scaleY(3);
        itemRect.bottom = itemRect.top + scaleY(20);
        SubItemRightY += itemRect.right - itemRect.left;
    }
    Items.Add(item);
    return TRUE;
}

LRESULT CHyperLinkControl::OnMouseMove(UINT Flags, CPoint Pt)
{
    RECT ClientRect;
    GetClientRect(&ClientRect);

    if(!Track) // Capturing mouse
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        ::_TrackMouseEvent(&tme); // We want to receive WM_MOUSELEAVE message
        Track = true;
    }

    for(size_t i=0;i< Items.GetCount(); i++)
    {
        if(!Items[i].Visible) continue;
        CRect rc(Items[i].ItemRect);
        if(!m_bHyperLinks && !Items[i].szTip.IsEmpty())
        {
            rc.right= ClientRect.right - 6;
            rc.InflateRect(3,6);
        }

        if(rc.PtInRect(Pt))
        {
            if(m_bHyperLinks  || Items[i].szTip.IsEmpty())
            {
                if(!CursorHand)
                    SetCursor(handCursor_);  // we need this because system doesn't send WM_CURSOR message immediately
            }
            if(static_cast<size_t>(hoverItemIndex_) == i ) return 0;
            Items[i].Hover = true;
            HoverItem(i);

            return 0;
        }
    }

    HoverItem(-1);

    return 0;
}

LRESULT CHyperLinkControl::OnMouseLeave()
{
    HoverItem(-1);
    Track = false;

    return 0;
}

LRESULT CHyperLinkControl::OnKillFocus(HWND hwndNewFocus)
{
    SelectItem(-1);
    return 0;
}

LRESULT CHyperLinkControl::OnSetFocus(HWND hwndOldFocus)
{
    if (selectedItemIndex_ != -1) {
        return 0;
    }

    /*if (hwndOldFocus == ::GetNextDlgTabItem(::GetParent(GetParent()), m_hWnd, true)
        || hwndOldFocus == ::GetNextDlgTabItem(::GetParent(GetParent()), m_hWnd, false)) {
        SelectItem(0);
    }*/

    return 0;
}

LRESULT CHyperLinkControl::OnLButtonDown(UINT Flags, CPoint Pt) {
    RECT ClientRect;
    GetClientRect(&ClientRect);
    mouseDownItemIndex_ = -1;
    for (size_t i = 0; i < Items.GetCount(); i++)
    {
        const auto& item = Items[i];
        if (!item.Visible) {
            continue;
        }
        CRect rc(item.ItemRect);

        if (!m_bHyperLinks && *item.szTip) {
            rc.right = ClientRect.right - 6;
            rc.InflateRect(3, 6);
        }

        if (rc.PtInRect(Pt)) {
            mouseDownItemIndex_ = i;
            break;
        }
    }
    return 0;
}

LRESULT CHyperLinkControl::OnLButtonUp(UINT Flags, CPoint Pt)
{
    RECT ClientRect;
    GetClientRect(&ClientRect);

    for(size_t i=0;i< Items.GetCount(); i++)
    {
        const auto& item = Items[i];
        if(!item.Visible) continue;
        CRect rc(item.ItemRect);

        if(!m_bHyperLinks && *item.szTip){
            rc.right= ClientRect.right - 6;
            rc.InflateRect(3,6);
        }

        if(rc.PtInRect(Pt)) {
            if (mouseDownItemIndex_ != -1 && mouseDownItemIndex_ == i) {
                NotifyParent(i);
                mouseDownItemIndex_ = -1;
            }
            break;
        }
    }

    return 0;
}

LRESULT CHyperLinkControl::OnKeyDown(TCHAR vk, UINT cRepeat, UINT flags)
{
    if (vk == VK_DOWN || (vk == VK_TAB && !(GetKeyState(VK_SHIFT) & 0x80)))
    {
        bool itemSelected = false;
        for (size_t j = selectedItemIndex_ + 1; j < Items.GetCount(); j++) {
            if (Items[j].Visible) {
                SelectItem(j);
                itemSelected = true;
                break;
            }
        }
        if (!itemSelected) {
            // Set focus to next dialog control
            ::PostMessage(GetParent(), WM_NEXTDLGCTL, 0, FALSE);
        }
    } else if (vk == VK_UP || (vk == VK_TAB && (GetKeyState(VK_SHIFT) & 0x80))) {
        bool itemSelected = false;
        for (int j = selectedItemIndex_ - 1; j >=0 ; j--) {
            if (Items[j].Visible) {
                SelectItem(j);
                itemSelected = true;
                break;
            }
        }
        if (!itemSelected) {
            // Set focus to previous dialog control
            ::PostMessage(GetParent(), WM_NEXTDLGCTL, 1, FALSE);
        }
    }

    if (vk == VK_RETURN || vk == VK_SPACE)
    {
        if (selectedItemIndex_ != -1) {
            NotifyParent(selectedItemIndex_);
        }
    }
    return 0;
}

int CHyperLinkControl::NotifyParent(int nItem)
{
    if(nItem<0) return 0;
    ::SendMessage(GetParent(), WM_COMMAND, (WPARAM)MAKELONG(Items[nItem].idCommand,BN_CLICKED), (LPARAM)m_hWnd);
    SelectItem(-1);
    return 0;
}

LRESULT CHyperLinkControl::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createDoubleBuffer();
    return 0;
}

void CHyperLinkControl::createDoubleBuffer() {
    if (dcMem_.m_hDC) {
        SelectObject(dcMem_, bmpOld_);
        dcMem_.DeleteDC();
        bmMem_.DeleteObject();
    }
    CRect rcClient;
    GetClientRect(rcClient);
    CClientDC dc(m_hWnd);

    dcMem_.CreateCompatibleDC(dc);

    bmMem_.CreateCompatibleBitmap(dc, rcClient.Width(), rcClient.Height());
    bmpOld_ = dcMem_.SelectBitmap(bmMem_);

    dcMem_.FillSolidRect(rcClient, ::GetSysColor(COLOR_WINDOW));
}

LRESULT CHyperLinkControl::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CPaintDC paintDc(m_hWnd);

    int dpiX = DPIHelper::GetDpiForDialog(m_hWnd);
    int dpiY = dpiX;

    auto scaleX = [dpiX](int x) {
        return MulDiv(x, dpiX, 96);
    };

    auto scaleY = [dpiY](int y) {
        return MulDiv(y, dpiY, 96);
    };

    RECT rc;
    GetClientRect(&rc);
    CRect r(rc);
    CBrush br;
    br.CreateSolidBrush(m_BkColor);
    CDC& dc = dcMem_;
    dc.FillRect(r,br);

    dc.SetBkMode(TRANSPARENT);
    const int iconSmallWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpiX);
    const int iconSmallHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpiY);
    const int iconBigWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXICON, dpiX);
    const int iconBigHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYICON, dpiY);

    for(size_t i=0; i<Items.GetCount(); i++)
    {
        const HyperLinkControlItem& item = Items[i];
        if (!item.Visible) {
            continue;
        }
        bool isHighlighted = static_cast<size_t>(selectedItemIndex_) == i || hoverItemIndex_ == i;
        COLORREF oldTextColor = dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));

        if(*(item.szTip)) // If we draw "big" item (with tip)
        {
            CRect itemRect = item.ItemRect;
            RECT TextRect = item.ItemRect;

            if(!m_bHyperLinks && isHighlighted)
            {
                CRect rec = item.ItemRect;
                rec.right = rc.right - scaleX(6);
                //rec.InflateRect(scaleX(3), scaleX(6));
                GuiTools::FillRectGradient(dc.m_hDC, rec, 0xEAE2D9, 0xD3C1AF, false);
            }

            TextRect.left += iconBigWidth + scaleX(5);
            if(!m_bHyperLinks){
                TextRect.left +=scaleX(20);
                TextRect.right +=scaleX(20);
            }
            HFONT oldFont;
            if (isHighlighted && m_bHyperLinks) {
                oldFont = dc.SelectFont(BoldUnderLineFont);
            }
            else {
                oldFont = dc.SelectFont(BoldFont);
            }

            SIZE TextDims;
            GetTextExtentPoint32(dc, item.szTitle, lstrlen(item.szTitle), &TextDims);

            int textY = itemRect.top;
            if(*item.szTip == _T(' ')) {
                textY += (itemRect.Height() - TextDims.cy) / 2;
            }

            dc.ExtTextOutW(TextRect.left, textY, ETO_CLIPPED, &TextRect, item.szTitle, wcslen(item.szTitle), 0);
            dc.SelectFont(oldFont);

            if (isHighlighted && m_bHyperLinks) {
                oldFont = dc.SelectFont(UnderlineFont);
            } else {
                oldFont = dc.SelectFont(GetFont());
            }
            dc.SetTextColor(GetSysColor(COLOR_GRAYTEXT));

            ExtTextOutW(dc.m_hDC, TextRect.left, item.ItemRect.top + TextDims.cy + scaleY(3), ETO_CLIPPED, &TextRect, item.szTip, wcslen(item.szTip), 0);

            dc.SetBkMode(TRANSPARENT);
            if(m_bHyperLinks){
                //dc.DrawIconEx(item.ItemRect.left+1, item.ItemRect.top + 2, item.hIcon, 20, 20);
                dc.DrawIconEx(item.ItemRect.left+1,item.ItemRect.top+2,item.hIcon, iconBigWidth, iconBigHeight);
            } else {
                dc.DrawIconEx(item.ItemRect.left + scaleX(15), item.ItemRect.top + (itemRect.Height() - iconBigHeight) / 2, item.hIcon, iconBigWidth, iconBigHeight);
            }
            dc.SelectFont(oldFont);
        } // End of drawing "big" item

        else
        {
            HFONT oldFont;
            if (isHighlighted) { // If item we draw is selected
                oldFont = dc.SelectFont(UnderlineFont);
            } else {
                oldFont = dc.SelectFont(GetFont());
            }

            dc.DrawIconEx(item.ItemRect.left,item.ItemRect.top+1,item.hIcon, iconSmallWidth, iconSmallHeight);

            CRect textRect = item.ItemRect;
            textRect.left += iconSmallWidth + scaleX(3);
            textRect.top += 1;
            dc.ExtTextOut(textRect.left, textRect.top, ETO_CLIPPED /*|ETO_OPAQUE/**/, &item.ItemRect, item.szTitle, wcslen(item.szTitle), 0);
            dc.SelectFont(oldFont);
        }
        dc.SetTextColor(oldTextColor);
    }

    paintDc.BitBlt(0, 0, rc.right - rc.left, rc.bottom - rc.top, dcMem_, 0, 0, SRCCOPY);
    return 0;
}

LRESULT CHyperLinkControl::OnEraseBkg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    bHandled = true;
    return 1;
}

void CHyperLinkControl::SelectItem(int Index)
{
    if(Index == selectedItemIndex_) return;

    RECT ClientRect;
    GetClientRect(&ClientRect);

    if(selectedItemIndex_ != -1) // Repainting previous selection
    {
        CRect temp = Items[selectedItemIndex_].ItemRect;
        if(!m_bHyperLinks)
        {
            temp.right= ClientRect.right;
            temp.InflateRect(3,6);
        }
        if(!Items[selectedItemIndex_].szTip.IsEmpty())
            //temp.left+=40;
            InvalidateRect(&temp,false);
        else InvalidateRect(&Items[selectedItemIndex_].ItemRect,false);
    }

    selectedItemIndex_ = Index;

    if(selectedItemIndex_!=-1)  //Repainting selected hyperlink
    {
        CRect temp = Items[Index].ItemRect;
        if(!m_bHyperLinks)
        {
            temp.right= ClientRect.right;
            temp.InflateRect(3,6);
        }
        if(!Items[Index].szTip.IsEmpty())
            //temp.left+=40;
            InvalidateRect(&temp,false);
        else InvalidateRect(&Items[Index].ItemRect,false);
        NotifyWinEvent(EVENT_OBJECT_FOCUS, m_hWnd, OBJID_CLIENT, Index + 1);
    }
}

void CHyperLinkControl::HoverItem(int Index)
{
    if (Index == hoverItemIndex_) {
        return;
    }

    RECT ClientRect;
    GetClientRect(&ClientRect);

    if (hoverItemIndex_ != -1) // Repainting previous selection
    {
        const auto& item = Items[hoverItemIndex_];
        CRect temp = item.ItemRect;
        if (!m_bHyperLinks)
        {
            temp.right = ClientRect.right;
            temp.InflateRect(3, 6);
        }
        if (!Items[hoverItemIndex_].szTip.IsEmpty())
            InvalidateRect(&temp, false);
        else InvalidateRect(&Items[hoverItemIndex_].ItemRect, false);
    }

    hoverItemIndex_ = Index;

    if (hoverItemIndex_ != -1)  //Repainting selected hyperlink
    {
        const auto& item = Items[Index];
        CRect temp = item.ItemRect;
        if (!m_bHyperLinks)
        {
            temp.right = ClientRect.right;
            temp.InflateRect(3, 6);
        }
        if (!item.szTip.IsEmpty())
            InvalidateRect(&temp, false);
        else InvalidateRect(&item.ItemRect, false);
    }
}

HyperLinkControlItem* CHyperLinkControl::getItemByCommand(int command) {
    for (size_t i = 0; i < Items.GetCount(); i++) {
        if (Items[i].idCommand == command) {
            return &Items[i];
        }
    }
    return nullptr;
}

HyperLinkControlItem* CHyperLinkControl::getItem(int index) {
    if (index >=0 && index < Items.GetCount()) {
        return &Items[index];
    }
    return nullptr;
}

int CHyperLinkControl::selectedItemIndex() const {
    return selectedItemIndex_;
}

int CHyperLinkControl::desiredHeight() const {
    int dpiY = DPIHelper::GetDpiForDialog(m_hWnd);

    auto scaleY = [dpiY](int y) {
        return MulDiv(y, dpiY, USER_DEFAULT_SCREEN_DPI);
    };

    return BottomY + scaleY(3);
}

void CHyperLinkControl::resetContent(bool invalidate) {
    for (size_t i = 0; i < Items.GetCount(); i++) {
        if (Items[i].hIcon) {
            DestroyIcon(Items[i].hIcon);
        }
    }
    Items.RemoveAll();
    BottomY = 0;
    SubItemRightY = -1;
    selectedItemIndex_ = -1;
    mouseDownItemIndex_ = -1;
    hoverItemIndex_ = -1;
    if (invalidate) {
        Invalidate();
    }
}

BOOL CHyperLinkControl::OnSetCursor(CWindow/* wnd*/, UINT/* nHitTest*/, UINT/* message*/)
{
    bool SubItem = false;
    if (hoverItemIndex_ != -1 && Items[hoverItemIndex_].szTip.IsEmpty()) {
        SubItem = true;
    }
    CursorHand = (hoverItemIndex_ != -1) && (m_bHyperLinks || SubItem);
    SetCursor(CursorHand ? handCursor_ : arrowCursor_);

    return TRUE;
}

LRESULT CHyperLinkControl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    for (size_t i = 0; i < Items.GetCount(); i++) {
        if (Items[i].hIcon) {
            DestroyIcon(Items[i].hIcon);
        }
    }
    return 0;
}

LRESULT CHyperLinkControl::OnGetObject(UINT, WPARAM wParam, LPARAM lParam) {
    if (lParam == OBJID_CLIENT) {
        if (acc_ == nullptr) {
            CHyperLinkControlAccessible::Create(this, &acc_);
        }
        return LresultFromObject(__uuidof(*acc_), wParam, acc_);
    } else {
        SetMsgHandled(FALSE);
        return 0;
    }
}

LRESULT CHyperLinkControl::OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    LRESULT retFlags = DLGC_WANTARROWS | DLGC_DEFPUSHBUTTON | DLGC_WANTTAB;
    if (wParam == VK_RETURN ) {
        if (selectedItemIndex_ != -1) {
            //NotifyParent(Selected);
            retFlags |= DLGC_DEFPUSHBUTTON;
        }
    }
    return retFlags;
}

LRESULT CHyperLinkControl::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createGdiResources();
    createDoubleBuffer();
    return 0;
}
