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

#include "InputBoxControl.h"

#include <sstream>
#include "Core/Images/Utils.h"
#include "Gui/GuiTools.h"
#include "ImageEditor/Canvas.h"
#include "Core/i18n/Translator.h"

namespace ImageEditor {
// CLogListBox
InputBoxControl::InputBoxControl(Canvas* canvas) : canvas_(canvas) {
    contextMenuOpened_ = false;
}

InputBoxControl::~InputBoxControl() {
}

LRESULT InputBoxControl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled)
{
    bHandled = false;
    SetEventMask(ENM_CHANGE|ENM_REQUESTRESIZE|ENM_SELCHANGE );
    CFont systemDialogFont = GuiTools::GetSystemDialogFont();
    biggerFont_ = GuiTools::MakeFontBigger(systemDialogFont);
    SetFont(biggerFont_);
    //SetWindowLong(GWL_ID, (LONG)m_hWnd);
    return 0;
}

LRESULT InputBoxControl::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled)
{
    bHandled = true;
    return 1;
}

LRESULT InputBoxControl::OnKillFocus(HWND hwndNewFocus) {
    //ShowWindow( SW_HIDE );
    return 0;
}

BOOL  InputBoxControl::SubclassWindow(HWND hWnd) {
    BOOL Result = Base::SubclassWindow(hWnd);

    return Result;
}

void InputBoxControl::show(bool s)
{
    ShowWindow(s? SW_SHOW: SW_HIDE);
    if ( !s ) {
        ::SetFocus(GetParent());
    }
}

void InputBoxControl::resize(int x, int y, int w,int h, std::vector<MovableElement::Grip> grips)
{
    POINT scrollOffset = canvas_->GetScrollOffset();
    SetWindowPos(/*HWND_TOP*/0, x - scrollOffset.x, y - scrollOffset.y, w, h, 0);
    /*CRgn rgn;
    rgn.CreateRectRgn(0,0,w,h);
    for ( int i = 0; i < grips.size(); i++ ) {
        CRgn gripRgn;
        int gripX = grips[i].pt.x-x;
        int gripY = grips[i].pt.y-y;
        gripRgn.CreateRectRgn(gripX, gripY,gripX + MovableElement::kGripSize, gripY + MovableElement::kGripSize);
        rgn.CombineRgn(gripRgn, RGN_DIFF);
    }
    SetWindowRgn(rgn, true);
    rgn.Detach();*/
}

void InputBoxControl::render(Gdiplus::Graphics* graphics, Gdiplus::Bitmap* background, Gdiplus::Rect layoutArea)
{
    ImageUtils::PrintRichEdit(m_hWnd, graphics, background, layoutArea);
}

bool InputBoxControl::isVisible()
{
    return IsWindowVisible()!=FALSE;
}

void InputBoxControl::invalidate()
{
    Invalidate(false);
}

void InputBoxControl::setTextColor(Gdiplus::Color color)
{
    CHARFORMAT cf;
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.crTextColor = color.ToCOLORREF();
    cf.dwMask = CFM_COLOR;
    //SetSel(0, -1);
    SendMessage(EM_SETCHARFORMAT, IsWindowVisible() ? SCF_SELECTION : SCF_ALL, (LPARAM) &cf);
    //SetSel(-1, 0);
}

void InputBoxControl::setFont(LOGFONT font,  DWORD changeMask)
{
    if ( !IsWindowVisible() ) {
        CFont f;
        f.CreateFontIndirect(&font);
        SetFont(f);
    } else {
        CHARFORMAT cf = GuiTools::LogFontToCharFormat(font);
        cf.dwMask = changeMask;
        SendMessage(EM_SETCHARFORMAT, IsWindowVisible() ? SCF_SELECTION : SCF_ALL, (LPARAM) &cf);
    }
}

void InputBoxControl::setRawText(const std::string& text)
{
    std::stringstream rawRtfText(text);
    EDITSTREAM es = {0};
    es.dwCookie = (DWORD_PTR) &rawRtfText;
    es.pfnCallback = &EditStreamInCallback;
    SendMessage(EM_STREAMIN, SF_RTF, (LPARAM)&es);
}

std::string InputBoxControl::getRawText()
{
    std::stringstream rawRtfText;
    EDITSTREAM es = {0};
    es.dwCookie = (DWORD_PTR) &rawRtfText;
    es.pfnCallback = &EditStreamOutCallback;
    /*int bytes = */SendMessage(EM_STREAMOUT, SF_RTF, (LPARAM)&es);
    return rawRtfText.str();
}

bool InputBoxControl::isEmpty() {
    int length = GetTextLengthEx(GTL_PRECISE, 1200 /* unicode */);
    return length == 0;
}

DWORD CALLBACK InputBoxControl::EditStreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
    auto* rtf = reinterpret_cast<std::stringstream*>(dwCookie);
    rtf->write(reinterpret_cast<CHAR*>(pbBuff), cb);
    *pcb = cb;
    return 0;
}

DWORD CALLBACK InputBoxControl::EditStreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
    auto* rtf = reinterpret_cast<std::stringstream*>(dwCookie);
    *pcb = static_cast<LONG>(rtf->readsome(reinterpret_cast<CHAR*>(pbBuff), cb));
    return 0;
}

LRESULT InputBoxControl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled) {
    return 0;
}

LRESULT InputBoxControl::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    WPARAM vKey = wParam;
    bHandled = false;
    if ( vKey == VK_ESCAPE ) {
        onEditCanceled();
        bHandled = true;
    } else if ( vKey == VK_RETURN && GetKeyState(VK_CONTROL) & 0x80) {
        onEditFinished();
        bHandled = true;
    }

    return 0;
}
/*
LRESULT InputBoxControl::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WPARAM vKey = wParam;
    return 0;
}
*/
LRESULT InputBoxControl::OnChange(UINT wNotifyCode,int, HWND)
{
    onTextChanged(L"");
    return 0;
}

LRESULT InputBoxControl::OnRequestResize(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    bHandled = true;
    auto * pReqResize = reinterpret_cast<REQRESIZE*>(pnmh);

    SIZE sz = {pReqResize->rc.right - pReqResize->rc.left, pReqResize->rc.bottom - pReqResize->rc.top};
    RECT windowRect;
    GetWindowRect(&windowRect);
    onResized(/*sz.cx*/windowRect.right - windowRect.left, sz.cy);

    //*SetWindowPos(0, 0,0, pReqResize->rc.right - pReqResize->rc.left, pReqResize->rc.bottom - pReqResize->rc.top);
    return 0;
}

LRESULT InputBoxControl::OnSelChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    auto* selChange = reinterpret_cast<SELCHANGE*>(pnmh);
    CHARFORMAT cf;

    GetSelectionCharFormat(cf);
    onSelectionChanged(selChange->chrg.cpMin, selChange->chrg.cpMax, GuiTools::CharFormatToLogFont(cf));

    return 0;
}

LRESULT InputBoxControl::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled)
{
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    CMenu contextMenu;
    POINT pt = {x,y};
//    ClientToScreen(&pt);
    contextMenu.CreatePopupMenu();
    contextMenu.AppendMenu(MF_STRING, IDMM_UNDO, TR("Undo"));
    contextMenu.AppendMenu(MF_STRING, IDMM_REDO, TR("Redo"));
    contextMenu.AppendMenu(MF_SEPARATOR, 1, _T(""));
    contextMenu.AppendMenu(MF_STRING, IDMM_CUT, TR("Cut"));
    contextMenu.AppendMenu(MF_STRING, IDMM_COPY, TR("Copy"));
    contextMenu.AppendMenu(MF_STRING, IDMM_PASTE, TR("Paste"));
    contextMenu.EnableMenuItem(IDMM_PASTE, CanPaste()?MF_ENABLED : MF_DISABLED );
    contextMenu.EnableMenuItem(IDMM_UNDO, CanUndo()?MF_ENABLED : MF_DISABLED  );
    contextMenu.EnableMenuItem(IDMM_REDO, CanRedo()?MF_ENABLED : MF_DISABLED  );
    contextMenuOpened_ = true;
    contextMenu.TrackPopupMenu(TPM_VERTICAL, pt.x, pt.y, m_hWnd);
    contextMenuOpened_ = false;
    return 0;
}

LRESULT InputBoxControl::OnUndo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SendMessage(EM_UNDO, 0, 0);
    return 0;
}

LRESULT InputBoxControl::OnRedo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SendMessage(EM_REDO, 0, 0);
    return 0;
}

LRESULT InputBoxControl::OnCut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SendMessage(WM_CUT, 0, 0);
    return 0;
}

LRESULT InputBoxControl::OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SendMessage(WM_COPY, 0, 0);
    return 0;
}

LRESULT InputBoxControl::OnPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SendMessage(WM_PASTE, 0, 0);
    return 0;
}

LRESULT InputBoxControl::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled)
{
    bHandled = false;
    if ( contextMenuOpened_ ) {
        bHandled = true;
        HCURSOR m_hArrow = LoadCursor(NULL,IDC_ARROW);
        ::SetCursor(m_hArrow);
    }
    return 0;
}

}
