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

#pragma once
#include "atlheaders.h"
#include <atlcrack.h>

#include "Gui/Constants.h"

struct HyperLinkControlItem
{
    ~HyperLinkControlItem() {
        DeleteObject(hIcon);
    }
    CString szTitle;
    CString szTip;
    HICON hIcon = nullptr;
    int iconWidth = 0;
    int iconHeight = 0;
    int idCommand = 0;
    RECT ItemRect{};
    bool Hover = false;
    bool Visible = true;
};

// CHyperLinkControl

class CHyperLinkControl :
    public CWindowImpl<CHyperLinkControl>, public CThemeImpl<CHyperLinkControl>
{
public:
    CHyperLinkControl();
    ~CHyperLinkControl();
    DECLARE_WND_SUPERCLASS(_T("CHyperLinkControl"), CListBox::GetWndClassName())

    BEGIN_MSG_MAP(CHyperLinkControl)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkg)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)
        MSG_WM_MOUSEMOVE(OnMouseMove)
        MSG_WM_MOUSELEAVE(OnMouseLeave)
        MSG_WM_KILLFOCUS(OnKillFocus)
        MSG_WM_SETFOCUS(OnSetFocus)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
        MSG_WM_LBUTTONUP(OnLButtonUp)
        MSG_WM_KEYDOWN(OnKeyDown)
        MSG_WM_SETCURSOR(OnSetCursor)
        MESSAGE_HANDLER_EX(WM_GETOBJECT, OnGetObject)
        MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
    END_MSG_MAP()

    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    int AddString(LPCTSTR szTitle,LPCTSTR szTip,int idCommand,HICON hIcon=NULL,bool Visible = true, int Align=0, bool LineBreak = false);
    LRESULT OnMouseMove(UINT Flags, CPoint Pt);
    bool MouseSel,Track;
    LRESULT OnMouseLeave();
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKillFocus(HWND hwndNewFocus);
    LRESULT OnSetFocus(HWND hwndOldFocus);
    LRESULT OnLButtonDown(UINT Flags, CPoint Pt);
    LRESULT OnLButtonUp(UINT Flags, CPoint Pt);
    LRESULT OnKeyDown(TCHAR vk, UINT cRepeat, UINT flags);
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnEraseBkg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    BOOL OnSetCursor(CWindow wnd, UINT nHitTest, UINT message);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetObject(UINT, WPARAM wParam, LPARAM lParam);
    LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void Init(COLORREF BkColor=RGB(255,255,255));
    size_t ItemCount() const;
    CString GetItemTitle(size_t item) const;
    CString GetItemDescription(size_t item) const;
    CRect GetItemRect(size_t itemIndex) const;
    int SelectedIndex() const;
    int ItemFromPoint(POINT pt) const;
    bool m_bHyperLinks;
    int NotifyParent(int nItem);
    void SelectItem(int Index);
    void HoverItem(int Index);
    HyperLinkControlItem* getItemByCommand(int command);
    HyperLinkControlItem* getItem(int index);
    int selectedItemIndex() const;
    int desiredHeight() const;
    void resetContent(bool invalidate = true);

protected:
    CAtlArray<HyperLinkControlItem> Items;
    int BottomY, SubItemRightY;
    CFont BoldFont, UnderlineFont,BoldUnderLineFont;
    int selectedItemIndex_;
    int hoverItemIndex_;
    bool CursorHand;
    HCURSOR handCursor_, arrowCursor_;
    CFontHandle NormalFont;
    COLORREF m_BkColor;
    int mouseDownItemIndex_;
    CComPtr<IAccessible> acc_;
    void createDoubleBuffer();
    CDC dcMem_;
    CBitmap bmMem_;
    HBITMAP bmpOld_;
    static int GetTextWidth(HDC dc, LPCTSTR Text, HFONT Font);
    void createGdiResources();
};


