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

#ifndef GUI_DIALOGS_CLEARHISTORYDIALOG_H
#define GUI_DIALOGS_CLEARHISTORYDIALOG_H

#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Core/HistoryManager.h"
#include "Gui/Controls/DialogIndirect.h"

// CClearHistoryDlg

class CClearHistoryDlg :
    public CCustomDialogIndirectImpl<CClearHistoryDlg>
{
    public:
        CClearHistoryDlg();
        enum { IDD = IDD_CLEARHISTORYDLG };

        BEGIN_MSG_MAP(CClearHistoryDlg)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
        END_MSG_MAP()

        HistoryClearPeriod GetValue() const;

    protected:
        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

        HistoryClearPeriod value_;
};

#endif // GUI_DIALOGS_CLEARHISTORYDIALOG_H
