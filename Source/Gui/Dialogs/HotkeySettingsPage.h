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
#ifndef HOTKEYSETTINGS_H
#define HOTKEYSETTINGS_H

#pragma once

#include <vector>

#include "atlheaders.h"
#include <resource.h>       // main symbols
#include "SettingsPage.h"
#include "HotkeyEditor.h"
#include "Gui/Constants.h"

#define IDM_CLEARHOTKEY 10000
#define IDM_CLEARALLHOTKEYS (IDM_CLEARHOTKEY + 1)


class CHotkeyItem;
class CHotkeyList: public std::vector<CHotkeyItem>
{
    public:
        CHotkeyList();
        CHotkeyList(const CHotkeyList&) = delete;
        bool changed() const;
        CHotkeyItem& getByFunc(const CString &func);
        void add(CString name, CString func, DWORD commandId, bool setForegroundWindow = true, WORD Code = 0, WORD modif = 0, int groupId = 0);
        CHotkeyList& operator=(const CHotkeyList& );
        bool operator==( const CHotkeyList& ) const;
        bool operator!=( const CHotkeyList& ) const;
        CString toString() const;
        bool deserialize(const CString &data);
        int getFuncIndex(const CString &func) const;
    private:
        bool m_bChanged;
};

class CHotkeySettingsPage :
    public CDialogImpl<CHotkeySettingsPage>, public CSettingsPage
{
    public:
        bool apply() override;
        enum { IDD = IDD_HOTKEYSETTINGSPAGE};

    protected:
         BEGIN_MSG_MAP(CHotkeySettingsPage)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
            MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)
            COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
            COMMAND_HANDLER(IDC_EDITHOTKEY, BN_CLICKED, OnEditHotkeyBnClicked)
            COMMAND_HANDLER(IDM_CLEARHOTKEY,BN_CLICKED, OnClearHotkey)
            COMMAND_HANDLER(IDM_CLEARALLHOTKEYS,BN_CLICKED, OnClearAllHotkeys)
            NOTIFY_HANDLER_EX(IDC_HOTKEYLIST, NM_DBLCLK, OnHotkeylistNmDblclk)
            NOTIFY_HANDLER(IDC_HOTKEYLIST, LVN_ITEMCHANGED, OnListViewItemChanged)
         END_MSG_MAP()
         // Handler prototypes:
         //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
         //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
         LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
         LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
         LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
         LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         LRESULT OnClearHotkey(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         LRESULT OnClearAllHotkeys(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         LRESULT OnEditHotkeyBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         LRESULT OnHotkeylistNmDblclk(LPNMHDR pnmh);
         LRESULT OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

         CListViewCtrl m_HotkeyList;
         CHotkeyList hotkeyList;
         CFont attentionLabelFont_;
     private:
         void EditHotkey(int index);
         void initListView();
         void updateEditButtonState();
};

#endif // HOTKEYSETTINGS_H


