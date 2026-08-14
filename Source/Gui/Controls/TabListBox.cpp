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

#include "TabListBox.h"

#include "Gui/Helpers/DPIHelper.h"
#include "Gui/GuiTools.h"

LRESULT CTabListBox::OnDrawitem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* lpdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
    int iItemIndex = static_cast<int>(lpdis->itemID);
    CDCHandle dc(lpdis->hDC);
    UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    CString itemText;
    GetText(iItemIndex, itemText);
    dc.SetBkMode(TRANSPARENT);
    if(lpdis->itemState & ODS_SELECTED )
    {
        CRect roundedRect(lpdis->rcItem);
        int padding = MulDiv(3, dpi, USER_DEFAULT_SCREEN_DPI);
        roundedRect.DeflateRect(padding, padding);
        HPEN oldPen = 0;
        CPen pen;
        dc.FillRect(&lpdis->rcItem, COLOR_WINDOW);

        /* if (lpdis->itemState & ODS_FOCUS) {
            dc.DrawFocusRect(roundedRect);
        } else {*/
            pen.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_HIGHLIGHT));
            oldPen = dc.SelectPen(pen);
            dc.RoundRect(roundedRect, CPoint(2, 2));
        //}
        CRect rc(lpdis->rcItem);
        int padding2 = padding + 1;
        rc.DeflateRect(padding2, padding2);

        COLORREF baseColor = GetSysColor(COLOR_HIGHLIGHT);

        // Extract RGB components
        BYTE r = GetRValue(baseColor);
        BYTE g = GetGValue(baseColor);
        BYTE b = GetBValue(baseColor);

        // Convert to TRIVERTEX format (16-bit per channel)
        auto to16 = [](BYTE x) -> USHORT {
            return (USHORT)((x << 8) | x); // duplicate 8 bits: 0xA0 => 0xA0A0
        };

        // Slight lightening/darkening for gradient effect
        int lighten = 20;
        int darken = -30;

        // Clamp and adjust function
        auto adjust = [](int val, int delta) -> BYTE {
            int res = val + delta;
            if (res < 0)
                res = 0;
            if (res > 255)
                res = 255;
            return (BYTE)res;
        };

        // Create TRIVERTEX array for gradient
        TRIVERTEX vertex[2] = {
            { rc.left, rc.top,
                to16(adjust(r, lighten)),
                to16(adjust(g, lighten)),
                to16(adjust(b, lighten)),
                0 },

            { rc.right, rc.bottom,
                to16(adjust(r, darken)),
                to16(adjust(g, darken)),
                to16(adjust(b, darken)),
                0 }
        };

        GRADIENT_RECT gRect = { 0, 1 };

        // Draw vertical gradient
        dc.GradientFill(vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
        dc.SelectPen(oldPen);
    }
    else
    {
        CBrush br;
        br.CreateSolidBrush(GetSysColor(COLOR_WINDOW));
        dc.FillRect(&lpdis->rcItem,br);
    }

    COLORREF oldColor = dc.SetTextColor(GetSysColor((lpdis->itemState & ODS_SELECTED)? COLOR_HIGHLIGHTTEXT: COLOR_WINDOWTEXT));
    dc.DrawText(itemText, itemText.GetLength(), &lpdis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    dc.SetTextColor(oldColor);
    return 0;
}

/* LRESULT CTabListBox::OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    auto* lpmis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
    CClientDC dc(m_hWnd);
    int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    int iItemIndex = lpmis->itemID;
    CString buf;
    RECT r = { 0, 0, 0, 0 };
    GetText(iItemIndex, buf);
    dc.DrawText(buf, buf.GetLength(), &r, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);
    lpmis->itemHeight = r.bottom - r.top + MulDiv(15, dpi, USER_DEFAULT_SCREEN_DPI);
    bHandled = true;
    return TRUE;
}*/

LRESULT CTabListBox::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    init();
    return 0;
}

void CTabListBox::init() {
    CClientDC dc(m_hWnd);
    UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    CString buf;
    int res = GetText(0, buf);
    if (res <= 0) {
        buf = _T("SOMETHING");
    }
    RECT r = { 0, 0, 0, 0 };
    CFont font = GuiTools::GetSystemDialogFont(dpi);
    HFONT oldFont = dc.SelectFont(font);
    dc.DrawText(buf, buf.GetLength(), &r, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);
    dc.SelectFont(oldFont);
    int height = r.bottom - r.top + MulDiv(15, dpi, USER_DEFAULT_SCREEN_DPI);
    SetItemHeight(0, height);
}

BOOL CTabListBox::SubclassWindow(HWND hWnd) {
    BOOL Result = CWindowImpl<CTabListBox, CListBox, CControlWinTraits>::SubclassWindow(hWnd);
    init();
    return Result;
}
