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

#include "LogListBox.h"

#include <algorithm>

#include "Gui/GuiTools.h"
#include <Gui/Dialogs/LogWindow.h>

namespace {

const int LLB_VertDivider = 10;
const int LLB_VertMargin = 5;

CString trim(const CString& Str)
{
    CString Result = Str;
    if (!Result.IsEmpty())
        for (int i = Result.GetLength() - 1; i >= 0; i--) {
            if (Result[i] == _T('\n') || Result[i] == _T('\r')) {
                Result.Delete(i);
            }
            else {
                break;
            }
        }
    return Result;
}

}

// CLogListBox
CLogListBox::CLogListBox()
{
    ErrorIcon.LoadIcon(MAKEINTRESOURCE(IDI_ERRORICON));
    WarningIcon.LoadIcon(MAKEINTRESOURCE(IDI_ICONWARNING));
    InfoIcon.LoadIcon(MAKEINTRESOURCE(IDI_ICONINFOBIG));
}

CLogListBox::~CLogListBox()
{
    Detach();
}

LRESULT CLogListBox::OnDrawitem(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled)
{
    LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
    if (!dis) return FALSE;

    LogListBoxItem* item = reinterpret_cast<LogListBoxItem *>(dis->itemData);
    if (!item) return FALSE;

    CDCHandle dc = dis->hDC;
    float dpiScaleX_ = dc.GetDeviceCaps(LOGPIXELSX) / 96.0f;
    float dpiScaleY_ = dc.GetDeviceCaps(LOGPIXELSY) / 96.0f;

    if (dis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {
        COLORREF oldBkColor = dc.SetBkColor(GetSysColor(COLOR_WINDOW));
        COLORREF oldTextColor = dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
        CRect r(dis->rcItem);
        if (!(dis->itemState & ODS_SELECTED)) {
            CBrush br;
            br.CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            dc.FillRect(r, br);
        }
        CRect rct;
        GetClientRect(&rct);

        if (dis->itemState & ODS_SELECTED) {
            CRect rd(dis->rcItem);
            GuiTools::FillRectGradient(dis->hDC, rd, 0xEAE2D9, 0xD3C1AF, false);
        }
        else if (static_cast<int>(dis->itemID) != GetCount() - 1) // If it isn't last item
        {
            CPen pen;
            pen.CreatePen(PS_SOLID, 1, RGB(190,190,190));
            auto oldPen = SelectObject(dc.m_hDC, pen);
            dc.MoveTo(rct.left, r.bottom - 1);
            dc.LineTo(rct.right, r.bottom - 1);
            SelectObject(dc.m_hDC, oldPen);
        }

        int oldBkMode = SetBkMode(dc.m_hDC,TRANSPARENT);

        SIZE TimeLabelDimensions;
        auto oldFont = SelectObject(dc.m_hDC, NormalFont);
        GetTextExtentPoint32(dc, item->Time, item->Time.GetLength(), &TimeLabelDimensions);

        // Writing error time
        int iconSizeW = GetSystemMetrics(SM_CXICON);
        int leftOffset = iconSizeW + static_cast<int>(roundf(dpiScaleX_ * 5));
        ExtTextOutW(dc.m_hDC, rct.right - 5 - TimeLabelDimensions.cx, r.top + LLB_VertMargin, ETO_CLIPPED, r, item->Time, item->Time.GetLength(), 0);
        // Writing error title
        SelectObject(dc.m_hDC, UnderlineFont);
        ExtTextOutW(dc.m_hDC, r.left + leftOffset, r.top + LLB_VertMargin, ETO_CLIPPED, r, item->strTitle, item->strTitle.GetLength(), 0);

        // Writing some info
        SelectObject(dc.m_hDC, NormalFont);
        RECT ItemRect = {r.left + leftOffset, r.top + LLB_VertMargin + LLB_VertDivider + item->TitleHeight,
            r.right - 10, r.bottom - LLB_VertMargin};
        dc.DrawText(item->Info, item->Info.GetLength(), &ItemRect, DT_NOPREFIX);

        // Writing error text with bold (explication of error)
        SelectObject(dc.m_hDC, BoldFont);
        RECT TextRect = {r.left + leftOffset, LLB_VertMargin + r.top + item->TitleHeight + LLB_VertDivider + ((item->Info.GetLength()) ? (item->InfoHeight + LLB_VertDivider) : 0), r.right - 10, r.bottom - LLB_VertMargin};
        dc.DrawText(item->strText, item->strText.GetLength(), &TextRect, DT_NOPREFIX);

        POINT iconPos{ static_cast<int>(roundf(dpiScaleX_ * 5)), r.top + static_cast<int>(roundf(dpiScaleY_ * 5)) };

        CIconHandle* ico = nullptr;
        switch (item->Type) {
            case ILogger::logError:
                ico = &ErrorIcon;
                break;
            case ILogger::logWarning:
                ico = &WarningIcon;
                break;
            case ILogger::logInformation:
                ico = &InfoIcon;
                break;
        }
        if (ico) {
            dc.DrawIcon(iconPos.x, iconPos.y, *ico);
        }
        SelectObject(dc.m_hDC, oldFont);
        dc.SetBkColor(oldBkColor);
        dc.SetTextColor(oldTextColor);
        dc.SetBkMode(oldBkMode);
    }

    bHandled = true;
    return 0;
}

LRESULT CLogListBox::OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled)
{
    auto* lpmis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

    LogListBoxItem * item = reinterpret_cast<LogListBoxItem *>(lpmis->itemData);
    if(!item) {
        return 0;
    }
    HDC dc = GetDC();
    //float dpiScaleX_ = GetDeviceCaps(dc, LOGPIXELSX) / 96.0f;
    float dpiScaleY_ = GetDeviceCaps(dc, LOGPIXELSY) / 96.0f;
    HGDIOBJ oldFont = SelectObject(dc, NormalFont);


    RECT ClientRect;
    GetClientRect(&ClientRect);

    int ItemWidth = ClientRect.right - ClientRect.left - 50;

    RECT Dimensions={0, 0, ItemWidth, 0};
    DrawText(dc, item->strTitle, lstrlen(item->strTitle), &Dimensions,    DT_CALCRECT);
    item->TitleHeight = Dimensions.bottom - Dimensions.top;

    Dimensions.bottom = 0;
    DrawText(dc, item->Info, item->Info.GetLength(), &Dimensions,    DT_CALCRECT);
    item->InfoHeight = Dimensions.bottom - Dimensions.top;

    SelectObject(dc, BoldFont);

    Dimensions.bottom = 0;
    DrawText(dc, item->strText, item->strText.GetLength(), &Dimensions,    DT_CALCRECT);
    item->TextHeight = Dimensions.bottom - Dimensions.top;
    SelectObject(dc, NormalFont);
    lpmis->itemWidth = ItemWidth;
    lpmis->itemHeight = LLB_VertMargin + item->TitleHeight + LLB_VertDivider + item->TextHeight + (item->InfoHeight?(LLB_VertDivider + item->InfoHeight):0) + LLB_VertMargin+2;
    lpmis->itemHeight = std::max(lpmis->itemHeight, static_cast<UINT>(dpiScaleY_ * 70) );
    lpmis->itemHeight = std::min(254u , lpmis->itemHeight);
    SelectObject(dc, oldFont);
    ReleaseDC(dc);
    return 0;
}

LRESULT CLogListBox::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

    bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x80 ) != 0;

    if (wParam == _T('C') && ctrlPressed) {
        // Ctrl + C has been pressed
        ::SendMessage(GetParent(), WM_COMMAND, MAKELPARAM(CLogWindow::IDC_COPYTEXTTOCLIPBOARD, 0), 0);
    } else if (wParam == _T('A') && ctrlPressed) {
        SelectAll();
    } else {
        bHandled = FALSE;
    }
    return 0;
}

LRESULT CLogListBox::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return 0;
}

int CLogListBox::AddString(ILogger::LogMsgType Type, const CString& strTitle, const CString& strText, const CString& szInfo, const CString& Time)
{
    LogListBoxItem* item = new LogListBoxItem;
    item->Type = Type;



    item->strText = trim(strText);
    item->strTitle = trim(strTitle);
    item->Info = trim(szInfo);
    item->Time = Time;

    SetRedraw(FALSE);
    int nPos = CListBox::AddString((LPCTSTR)item);

    if (nPos < 0) return -1;

    SetItemDataPtr(nPos, item);
    SetTopIndex(nPos - 1);
    SetSel(-1, FALSE); // remove selection
    SetSel(nPos, TRUE); // select new added item
    SetRedraw(TRUE);
    return nPos;
}

LRESULT CLogListBox::OnKillFocus(HWND hwndNewFocus)
{
    SetSel(-1, FALSE);
    return 0;
}

BOOL CLogListBox::SubclassWindow(HWND hWnd)
{
    BOOL Result = CWindowImpl<CLogListBox, CListBox,CControlWinTraits>::SubclassWindow(hWnd);
    Init();
    return Result;
}

void CLogListBox::Init()
{
    NormalFont = GetFont();
    UnderlineFont =  GuiTools::MakeFontUnderLine(NormalFont);
    BoldFont = GuiTools::MakeFontBold(NormalFont);
}

LRESULT CLogListBox::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled)
{
    Clear();
    return 0;
}

void CLogListBox::Clear()
{
    SetRedraw(false);

    int n = GetCount();
    for(int i= 0; i<n; i++){
        auto* item = static_cast<LogListBoxItem *>(GetItemDataPtr(i));
        delete item;
    }
    ResetContent();
    SetRedraw(true);
}

void CLogListBox::SelectAll() {
    //Select all items
    int itemCount = GetCount();
    if (itemCount) {
        SetRedraw(FALSE);
        for (int i = 0; i < itemCount; i++) {
            SetSel(i, TRUE);
        }
        SetRedraw(TRUE);
    }
}

LogListBoxItem* CLogListBox::getItemFromIndex(int index) const {
    return static_cast<LogListBoxItem *>(GetItemDataPtr(index));
}

int CLogListBox::getFirstSelectedItem() const {
    int nSelCount = GetSelCount();
    if (nSelCount <= 0) {
        return LB_ERR;
    }

    std::vector<int> selItems(nSelCount);

    int nActualCount = GetSelItems(nSelCount, selItems.data());

    return (nActualCount > 0) ? selItems[0] : LB_ERR;
}
