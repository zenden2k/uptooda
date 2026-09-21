#ifndef __CustomTreeControl__H
#define __CustomTreeControl__H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CCustomTreeControl - A Property List control
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2001-2003 Bjarke Viksoe.
//   Thanks to Pascal Binggeli for fixing the disabled items.
//   Column resize supplied by Remco Verhoef, thanks.
//   Also thanks to Daniel Bowen, Alex Kamenev and others for fixes.
//
// Add the following macro to the parent's message map:
//   REFLECT_NOTIFICATIONS()
//
// This code may be used in compiled form in any way you desire. This
// source file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

#ifndef __cplusplus
  #error WTL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
  #error CustomTreeControl.h requires atlapp.h to be included first
#endif

#ifndef __ATLCTRLS_H__
  #error CustomTreeControl.h requires atlctrls.h to be included first
#endif

#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
  #include <zmouse.h>
#endif //!((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))

// Extended List styles

#define PLS_EX_SHOWSELALWAYS   0x00000008

// Include property base class

#include <vector>
#include <atlctrls.h>
#include <atlmisc.h>
#include <atlframe.h>
class TreeItem;

class TreeItemCallback
{
public:
    virtual void OnTreeItemDelete(TreeItem* item) = 0;
};

class TreeItem
{
public:
    TreeItem(int level) {
        nLevel = level;
        m_isExpanded = false;
        m_callback = 0;
        m_userData = nullptr;
    }

    virtual ~TreeItem() {
        if (m_callback) m_callback->OnTreeItemDelete(this);
        for (auto & m_subItem : m_subItems) {
            delete m_subItem;
        }
    }

    void AddItem(TreeItem* it) {
        m_subItems.push_back(it);
    }

    bool HasItems() {
        return m_subItems.size() != 0;
    }

    void setCallback(TreeItemCallback* c) {
        m_callback = c;
    }

    bool IsExpanded() const {
        return m_isExpanded;
    }

    int level() const {
        return nLevel;
    }

    int ItemCount() const {
        return m_subItems.size();
    }

    TreeItem* item(int index) {
        return m_subItems[index];
    }

    void SetText(const CString text) {
        m_Text = text;
    }

    CString text() const {
        return m_Text;
    }

    void setExpanded(bool exp) {
        m_isExpanded = exp;
    }

    void setUserData(void* data) {
        m_userData = data;
    }

    void* userData() const {
        return m_userData;
    }

protected:
    int nLevel;
    std::vector<TreeItem*> m_subItems;
    bool m_isExpanded;
    CString m_Text;
    void* m_userData;
    TreeItemCallback* m_callback;
};

/////////////////////////////////////////////////////////////////////////////
// CCustomTreeControl control

template <class T, class TBase = CListBox, class TWinTraits = CWinTraitsOR<LBS_OWNERDRAWVARIABLE | LBS_NOTIFY>>
class ATL_NO_VTABLE CCustomTreeControlImpl :
    public CWindowImpl<T, TBase, TWinTraits>,
    public COwnerDraw<T>
{
public:
    DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

    enum { CATEGORY_INDENT = 16 };


    DWORD m_dwExtStyle;


    CCustomTreeControlImpl() :
        m_dwExtStyle(0UL)
    //m_iMiddle(0),
    //m_bColumnFixed(false),
    //m_iPrevious(0),
    //m_iPrevXGhostBar(0)
    {
        m_memDC = 0;
        m_memBM = 0;
        enableEraseBkgnd = false;

        m_doubleBufferSize.cx = 0;
        m_doubleBufferSize.cy = 0;
    }

    virtual ~CCustomTreeControlImpl() {
        if (m_memBM) DeleteObject(m_memBM);
        if (m_memDC)DeleteDC(m_memDC);
    }

    BOOL SubclassWindow(HWND hWnd) {
        ATLASSERT(m_hWnd==NULL);
        ATLASSERT(::IsWindow(hWnd));
        BOOL bRet = CWindowImpl<T, TBase, TWinTraits>::SubclassWindow(hWnd);
        if (bRet) _Init();
        return bRet;
    }

    void SetExtendedListStyle(DWORD dwExtStyle) {
        ATLASSERT(::IsWindow(m_hWnd));
        // Assign styles
        if ((dwExtStyle & PLS_EX_SORTED) != 0) {
            ATLASSERT((dwExtStyle & PLS_EX_CATEGORIZED)==0); // We don't support sorted categories!
            ATLASSERT(GetStyle() & LBS_SORT);
            ATLASSERT(GetStyle() & LBS_HASSTRINGS);
        }
        m_dwExtStyle = dwExtStyle;
        // Recalc colours and fonts
        SendMessage(WM_SETTINGCHANGE);
    }

    DWORD GetExtendedListStyle() const {
        return m_dwExtStyle;
    }

    void ResetContent() {
        ATLASSERT(::IsWindow(m_hWnd));

        TBase::ResetContent();
    }

    TreeItem* AddItem(CString name, void* userData = 0) {
        ATLASSERT(::IsWindow(m_hWnd));
        TreeItem* item = new TreeItem(0);
        item->setUserData(userData);
        int nItem = TBase::AddString((LPCTSTR)item);
        if (nItem == LB_ERR) return NULL;

        item->SetText(name);
        //  TBase::SetItemData(nItem, (DWORD_PTR) item);
        return item;
    }

    /* TreeItem*  AddItem(CString name)
     {
        ATLASSERT(::IsWindow(m_hWnd));
          TreeItem* item = new TreeItem(0);
        int nItem = 0;//TBase::AddString((LPCTSTR)item);
        if( nItem == LB_ERR ) return NULL;
      
        item->SetText(name);
        //TBase::SetItemData(nItem, (DWORD_PTR) item);
        return item;
     }*/

    TreeItem* AddSubItem(CString name, TreeItem* parent, void* userData, bool autoExpand = true) {
        ATLASSERT(::IsWindow(m_hWnd));
        // int nItem = TBase::AddString(name);
        // if( nItem == LB_ERR ) return NULL;
        TreeItem* item = new TreeItem(parent->level() + 1);
        item->setUserData(userData);
        item->SetText(name);
        parent->AddItem(item);
        if (autoExpand) {
            /*int nItem =*/
            TBase::AddString((LPCTSTR)item);
            parent->setExpanded(true);
        }
        return item;
    }

    int FindVisibleItemIndex(TreeItem* item) {
        ATLASSERT(::IsWindow(m_hWnd));

        for (int i = 0; i < GetCount(); i++) {

            if (TBase::GetItemData(i) == (DWORD_PTR)item) return i;
        }
        return -1;
    }

    bool ExpandItem(int index) {
        TreeItem* item = GetItem(index);
        if (!item) return false;
        if (!item->HasItems() || item->IsExpanded()) return false;

        //SetRedraw(false);
        for (int i = 0; i < item->ItemCount(); i++) {

            /*int insertedIndex =  */
            TBase::InsertString(++index, (LPCTSTR)item->item(i));
            //TBase::SetItemData(insertedIndex, (DWORD_PTR) item->item(i));
        }
        item->setExpanded(true);
        //SetRedraw(true);
        //Invalidate();
        return true;
    }

    BOOL ExpandItem(TreeItem* item) {
        int index = FindVisibleItemIndex(item);
        if (index == -1) return FALSE;
        ExpandItem(index);
        return TRUE;
    }

    bool ToggleItem(TreeItem* item) {
        if (!item->HasItems()) return false;
        if (item->IsExpanded())
            return CollapseItem(FindVisibleItemIndex(item)) != 0;
        else
            return ExpandItem(FindVisibleItemIndex(item)) != 0;
    }

    BOOL CollapseItem(int index) {
        TreeItem* item = GetItem(index);
        if (!item) return false;
        if (!item->HasItems() || !item->IsExpanded()) return false;

        SetRedraw(false);
        index++;
        while (index < GetCount()) {
            TreeItem* itemToHide = GetItem(index);
            if (itemToHide->level() < item->level() + 1) break;
            TBase::SetItemData(index, 0L); // Clear data now, so WM_DELETEITEM doesn't delete
            // the IProperty in the DeleteString() call below
            TBase::DeleteString(index);
            //m_arrItems.Add(prop);
        }
        item->setExpanded(false);
        SetRedraw(true);
        Invalidate();
        return TRUE;
    }

    void InvalidateItem(int idx) {
        ATLASSERT(::IsWindow(m_hWnd));
        if (idx == -1) return;
        RECT rc = {0};
        GetItemRect(idx, &rc);
        InvalidateRect(&rc);
    }

    // Unsupported methods
    int AddString(LPCTSTR /*lpszItem*/) {
        ATLASSERT(false);
        return LB_ERR;
    }

    int InsertString(int /*nIndex*/, LPCTSTR /*lpszItem*/) {
        ATLASSERT(false);
        return LB_ERR;
    }

    int DeleteString(UINT /*nIndex*/) {
        ATLASSERT(false);
        return LB_ERR;
    }

    // Implementation

    void _Init() {
        ATLASSERT(::IsWindow(m_hWnd));
        // Needs LBS_OWNERDRAWVARIABLE and LBS_NOTIFY flags,
        // but don't want multiselect or multicolumn flags.
        ATLASSERT(GetStyle() & LBS_OWNERDRAWVARIABLE);
        ATLASSERT(GetStyle() & LBS_NOTIFY);
        ATLASSERT((GetStyle() & (LBS_MULTIPLESEL|LBS_NODATA|LBS_MULTICOLUMN))==0);
        SendMessage(WM_SIZE);
        SendMessage(WM_SETTINGCHANGE);
    }

    // Message map and handlers
BEGIN_MSG_MAP(CCustomTreeControlImpl)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
        MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        ///    MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        //    MESSAGE_HANDLER(WM_MOUSEWHEEL, OnVScroll)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnDblClick)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
        CHAIN_MSG_MAP_ALT(COwnerDraw<T>, 1)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

    HDC m_memDC;
    HBITMAP m_memBM;
    SIZE m_doubleBufferSize;
    bool enableEraseBkgnd;

    void CreateDoubleBuffer() {
        RECT rc;
        GetClientRect(&rc);
        if (m_doubleBufferSize.cx >= rc.right && m_doubleBufferSize.cy >= rc.bottom) {
            if (m_doubleBufferSize.cx - rc.right < 100 && m_doubleBufferSize.cy - rc.bottom < 100)
                return;
        }

        if (m_memBM) DeleteObject(m_memBM);
        if (m_memDC)DeleteDC(m_memDC);
        HDC dc = GetDC();
        m_memDC = CreateCompatibleDC(dc);
        rc.right += 100;
        rc.bottom += 100;
        m_memBM = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
        m_doubleBufferSize.cx = rc.right;
        m_doubleBufferSize.cy = rc.bottom;

        ReleaseDC(dc);
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
        LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);
        _Init();
        return lRes;
    }


    LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
        SetRedraw(false);
        LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);
        SetRedraw(true);
        Invalidate();
        bHandled = true;
        return lRes;
    }

    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
        if (enableEraseBkgnd) {
            RECT rc;
            GetClientRect(&rc);
            HDC dc = (HDC)wParam;
            CBrush br;
            br.CreateSolidBrush(RGB(255, 255, 255));
            HGDIOBJ oldBrush = SelectObject(dc, br);
            PatBlt(dc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
            SelectObject(dc, oldBrush);
        }
        OnPaint(0, 0, 0, bHandled);
        bHandled = true;
        return 1;
    }

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
        RECT rc;
        GetClientRect(&rc);
        CPaintDC dc(m_hWnd); 
        RECT paintRect = dc.m_ps.rcPaint;
        CreateDoubleBuffer();
        HGDIOBJ oldObj = SelectObject(m_memDC, m_memBM);

        CBrush br;
        br.CreateSolidBrush(RGB(255, 255, 255));
        HGDIOBJ oldBrush = SelectObject(m_memDC, br);
        //    PatBlt(m_memDC,paintRect.left,paintRect.top,paintRect.right-paintRect.left, paintRect.bottom-paintRect.top, PATCOPY);

        PatBlt(m_memDC, 0, 0, m_doubleBufferSize.cx, m_doubleBufferSize.cy, PATCOPY);
        SelectObject(m_memDC, oldBrush);
        HRGN rgn = CreateRectRgnIndirect(&paintRect);
        SelectClipRgn(m_memDC, rgn);

        //PatBlt(memDC,t2,t1+t2,RC1.right-RC1.left, RC1.bottom-RC1.top, PATCOPY); 
        //----------- 
        enableEraseBkgnd = true;
        SendMessage(WM_PRINT, (WPARAM)m_memDC,
                    (LPARAM)(PRF_CLIENT | PRF_CHILDREN | PRF_CHECKVISIBLE/*|PRF_ERASEBKGND*/ | PRF_OWNED));
        enableEraseBkgnd = false;
        //----- 
        SelectClipRgn(m_memDC, 0);
        //BitBlt(dc, paintRect.left, paintRect.top, paintRect.right-paintRect.left, paintRect.bottom-paintRect.top, m_memDC,paintRect.left, paintRect.top, SRCCOPY); 
        BitBlt(dc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, m_memDC, 0, 0, SRCCOPY);
        SelectObject(m_memDC, oldObj);
        DeleteObject(rgn);
        bHandled = true;
        return 0;
    }


    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
        // Make sure to delete editor and item-data memory
        // FIX: Thanks to Ilya Markevich for spotting this memory leak
        ResetContent();
        bHandled = TRUE;
        return 0;
    }

    TreeItem* GetItem(int index) {
        auto itemData = TBase::GetItemData(index);
        if (itemData == LB_ERR) {
            return nullptr;
        }
        return reinterpret_cast<TreeItem*>(TBase::GetItemData(index));
    }

    LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
        LRESULT lRes = DefWindowProc(WM_LBUTTONDOWN, wParam, lParam);
        return lRes;
    }

    LRESULT OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
        LRESULT lRes = DefWindowProc(WM_LBUTTONUP, wParam, lParam);
        DefWindowProc(WM_RBUTTONUP, wParam, lParam);
        return lRes;
    }

    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
        switch (LOWORD(wParam)) {
        case VK_TAB:
            {
                int idx = GetCurSel();
                if (idx != -1) {
                    /*               IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(idx));
                                   ATLASSERT(prop);
                                   prop->Activate(PACT_TAB, 0);*/
                }
            }
            break;
        }
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
        // Kill the nasty BEEP sound!
        if (wParam == _T(' ')) return 0;

        if (wParam == _T('+')) {
            int idx = GetCurSel();
            if (idx != -1) {
                TreeItem* it = GetItem(idx);

                ExpandItem(it);

            }
        } else if (wParam == _T('-')) {
            int idx = GetCurSel();
            if (idx != -1) {
                /*TreeItem * it = */
                GetItem(idx);

                CollapseItem(idx);

            }
        }
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled**/) {
        if (::GetFocus() != m_hWnd) SetFocus();

        LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);

        // Should we do some column resize?
        // NOTE: Apparently the ListBox control will internally do SetCapture() to
        //       capture all mouse-movements.
        //      m_iPrevious = 0;
        //      int iIndent = CATEGORY_INDENT;
        // if( (m_dwExtStyle & PLS_EX_NOCOLUMNRESIZE) == 0 &&
        //    GET_X_LPARAM(lParam) == m_iMiddle + iIndent ) 
        //{
        // int m_iPrevious = GET_X_LPARAM(lParam);
        // }

        int idx = GetCurSel();
        if (idx != -1) {
            TreeItem* it = GetItem(idx);
            //IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(idx));
            // ATLASSERT(prop);
            // Ask owner first
            //NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_CLICK, prop };
            //if( ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh) == 0 ) 
            {
                // Translate into action
                if (GET_X_LPARAM(lParam) < CATEGORY_INDENT) {
                    ToggleItem(it);
                    //prop->Activate(PACT_EXPAND, 0);
                }
                /* else {
                    if( prop->IsEnabled() ) prop->Activate(PACT_CLICK, lParam);
                 }*/
            }
        }
        return lRes;
    }

    virtual LRESULT OnDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
        LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);
        int idx = GetCurSel();
        if (idx != -1) {
            TreeItem* prop = GetItem(idx);
            ATLASSERT(prop);
            ToggleItem(prop);
            // Ask owner first
            {
                // Send DblClick action
                // if( prop->() ) prop->Activate(PACT_DBLCLICK, lParam);
            }
        }
        return lRes;
    }

    LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
        // Custom styles
        /*      m_di.dwExtStyle = m_dwExtStyle;
              // Standard colors
              m_di.clrText = ::GetSysColor(COLOR_WINDOWTEXT);
              m_di.clrBack = ::GetSysColor(COLOR_WINDOW);
              m_di.clrSelText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
              m_di.clrSelBack = ::GetSysColor(COLOR_HIGHLIGHT);
              m_di.clrDisabled = ::GetSysColor(COLOR_GRAYTEXT);
              // Border
              m_di.clrBorder = ::GetSysColor(COLOR_BTNFACE);
              if( !m_BorderPen.IsNull() ) m_BorderPen.DeleteObject();
              m_di.Border = m_BorderPen.CreatePen(PS_SOLID, 1, m_di.clrBorder);
              // Fonts
              if( !m_TextFont.IsNull() ) m_TextFont.DeleteObject();
              if( !m_CategoryFont.IsNull() ) m_CategoryFont.DeleteObject();*/
        /*LOGFONT lf;
        HFONT hFont = (HFONT)::SendMessage(GetParent(), WM_GETFONT, 0, 0);
        if( hFont == NULL ) hFont = AtlGetDefaultGuiFont();
        ::GetObject(hFont, sizeof(lf), &lf);*/
        // m_di.TextFont = m_TextFont.CreateFontIndirect(&lf);
        //      SetFont(m_di.TextFont);
        /* if( (m_dwExtStyle & PLS_EX_XPLOOK) == 0 ) 
             lf.lfWeight += FW_BOLD;*/
        //m_di.CategoryFont = m_CategoryFont.CreateFontIndirect(&lf);
        // Text metrics
        //CClientDC dc(m_hWnd);
        /*      HFONT hOldFont = dc.SelectFont(m_di.TextFont);
              dc.GetTextMetrics(&m_di.tmText);
              dc.SelectFont(hOldFont);*/
        // Repaint
        Invalidate();
        return 0;
    }

    LRESULT OnNavigate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
        switch (wParam) {
        case VK_UP:
        case VK_DOWN:
            {
                SetCurSel(GetCurSel() + (wParam == VK_UP ? -1 : 1));
                BOOL bDummy = FALSE;
                //            OnSelChange(0, 0, NULL, bDummy);
            }
            break;
        }
        return 0;
    }

    LRESULT OnExpand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
        ATLASSERT(lParam);
        //      ExpandItem(reinterpret_cast<IProperty*>(lParam));
        return 0;
    }

    LRESULT OnCollapse(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
        ATLASSERT(lParam);
        // CollapseItem(reinterpret_cast<IProperty*>(lParam));
        return 0;
    }

    // Owner draw methods
    void DeleteItem(LPDELETEITEMSTRUCT lpDIS) {
        if (lpDIS->itemData != 0) {
            TreeItem* item = reinterpret_cast<TreeItem*>(lpDIS->itemData);
            if (item->level() == 0)
                delete item;
        }
    }

    void MeasureItem(LPMEASUREITEMSTRUCT lpMIS) {
        SIZE sz = {0, 0};
        TreeItem* item = reinterpret_cast<TreeItem*>(lpMIS->itemData);
        TreeItemSize(item, &sz);
        lpMIS->itemHeight = sz.cy;
    }

    void DrawName(CString name, HDC ddc, DWORD state, RECT rc) {
        CDCHandle dc(ddc);
        COLORREF clrBack, clrFront;
        /* if( (state & ODS_DISABLED) != 0 ) {
            clrFront = di.clrDisabled;
            clrBack = di.clrBack;
         }
         else */
        if ((state & ODS_SELECTED) != 0) {
            /*clrFront = di.clrSelText;
            clrBack = di.clrSelBack;*/
            clrFront = RGB(255, 255, 255);
            clrBack = RGB(0, 0, 200);
        } else {
            clrFront = RGB(0, 0, 0);
            clrBack = RGB(255, 255, 255);
        }
        RECT rcItem = rc;
        dc.FillSolidRect(&rcItem, clrBack);
        rcItem.left += 2; // Indent text
        dc.SetBkMode(TRANSPARENT);
        COLORREF oldBkColor = dc.SetBkColor(clrBack);
        COLORREF oldTextColor = dc.SetTextColor(clrFront);
        dc.DrawText(name, -1, &rcItem, DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL | DT_NOPREFIX | DT_VCENTER);
        dc.SetBkColor(oldBkColor);
        dc.SetTextColor(oldTextColor);
    }

    void DrawItem(LPDRAWITEMSTRUCT lpDIS) {
        if (lpDIS->itemID == -1) return; // If there are no list box items, skip this message. 

        CDCHandle dc(lpDIS->hDC);
        RECT rc = lpDIS->rcItem;

        TreeItem* item = reinterpret_cast<TreeItem*>(lpDIS->itemData);
        // IProperty* prop = reinterpret_cast<IProperty*>(lpDIS->itemData);
        //      ATLASSERT(prop);
        //BYTE kind = prop->GetKind();

        // Customize item
        /*PROPERTYDRAWINFO di = m_di;
        di.hDC = dc;
        di.state = lpDIS->itemState & ~ODS_DISABLED;
        if( lpDIS->itemID == (UINT) m_iInplaceIndex ) di.state |= ODS_COMBOBOXEDIT;
  
        // Special style for removing selection when control hasn't got focus
        if( (di.dwExtStyle & PLS_EX_SHOWSELALWAYS) == 0 && (::GetFocus() != m_hWnd) ) {
           di.state &= ~ODS_SELECTED;
        }*/

        // Prepare drawing
        //  HFONT hOldFont = dc.SelectFont(di.TextFont);

        // If this control is painted with categories
        //      if( (m_dwExtStyle & PLS_EX_CATEGORIZED) != 0 )

        // We paint a nice border in the gap with the plus/minus signs
        //         HPEN hOldPen = dc.SelectPen(di.Border);
        /*dc.MoveTo(rc.left + CATEGORY_INDENT - 1, rc.top);
        dc.LineTo(rc.left + CATEGORY_INDENT - 1, rc.bottom);
        if( (m_dwExtStyle & PLS_EX_XPLOOK) != 0 ) {
           RECT rcIndent = { rc.left, rc.top, rc.left + CATEGORY_INDENT, rc.bottom };
           dc.FillRect(&rcIndent, ::GetSysColorBrush(COLOR_3DFACE));
        }*/
        // Paint plus/minus sign if it's actually a category item
#if 0
          if( item->HasItems() ) {
//            dc.SelectFont(di.CategoryFont);
//            CCategoryProperty* pCategory = static_cast<CCategoryProperty*>(prop);
            POINT ptMiddle = { rc.left + (CATEGORY_INDENT / 2), rc.top + ((rc.bottom - rc.top) / 2) };
            RECT rcSymbol = { ptMiddle.x - 4, ptMiddle.y - 4, ptMiddle.x + 5, ptMiddle.y + 5 };
            dc.SelectStockPen(BLACK_PEN);
            HBRUSH hOldBrush = dc.SelectStockBrush(NULL_BRUSH);
            dc.Rectangle(&rcSymbol);
            dc.SelectBrush(hOldBrush);
            if( !item->IsExpanded() ) 
            {
               dc.MoveTo(ptMiddle.x, ptMiddle.y - 2);
               dc.LineTo(ptMiddle.x, ptMiddle.y + 3);
            }
            dc.MoveTo(ptMiddle.x - 2, ptMiddle.y);
            dc.LineTo(ptMiddle.x + 3, ptMiddle.y);
         }
         //dc.SelectPen(hOldPen);
         rc.left += CATEGORY_INDENT;
      }
#endif
        // Calculate rectangles for the two sides
        RECT rcName = rc;
        // RECT rcValue = rc;
        // Special handling of XP-like categories
        /*      if( kind == PROPKIND_CATEGORY /*&& (m_dwExtStyle & PLS_EX_XPLOOK) != 0 *) {
                 rcName.right = rcValue.left = rc.right;
              }*/
        //
        // ExtTextOut(dc, rcName.left, rcName.top, 0, &rcName, item->text(), item->text().GetLength(),0);
        // Draw name
        //      di.rcItem = rcName;
        rcName.left += CATEGORY_INDENT * item->level();
        DrawTreeItem(dc, rcName, lpDIS->itemState, item);

        // Paint borders
        //  HPEN hOldPen = dc.SelectPen(di.Border);
        //      dc.SelectPen(hOldPen);
        // dc.SelectFont(hOldFont);
    }

    virtual void DrawTreeItem(HDC dc, RECT rc, UINT itemState, TreeItem* item) {
        DrawName(item->text(), dc, itemState, rc);
    }

    virtual void TreeItemSize(TreeItem* item, SIZE* sz) {
        sz->cy = 20 + 3;
    }
};

class CCustomTreeControlCtrl : public CCustomTreeControlImpl<CCustomTreeControlCtrl>
{
public:
    DECLARE_WND_SUPERCLASS(_T("WTL_CustomTreeControl"), GetWndClassName())
};


#endif // __CustomTreeControl__H
