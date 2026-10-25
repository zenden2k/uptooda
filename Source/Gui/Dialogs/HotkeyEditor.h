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
#include <atlddx.h>
#include "resource.h"       // main symbols
#include "3rdpart/WinHotkeyCtrl.h"
#include "3rdpart/vkCodes.h"
#include "HotkeySettingsPage.h"
#include "../Models/HotkeyList.h"
#include "Gui/Controls/DialogIndirect.h"

class CHotkeyEditor :
    public CCustomDialogIndirectImpl<CHotkeyEditor>,
    public CWinDataExchange<CHotkeyEditor>
{
    public:
        CHotkeyItem m_data;
        CHotkeyEditor();
        ~CHotkeyEditor();
        CWinHotkeyCtrl localHotkeyCtrl;
        CWinHotkeyCtrl globalHotkeyCtrl;
        enum { IDD = IDD_HOTKEYEDITOR };

        BEGIN_MSG_MAP(CHotkeyEditor)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
        END_MSG_MAP()

        BEGIN_DDX_MAP(CHotkeyEditor)
        END_DDX_MAP()
        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};
