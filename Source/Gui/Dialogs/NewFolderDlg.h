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
#ifndef NEWFOLDERDLG_H
#define NEWFOLDERDLG_H


#pragma once
#include "atlheaders.h"
#include "resource.h"       // main symbols
#include <string>
#include <vector>
#include "Gui/Controls/DialogIndirect.h"

class CFolderItem;

class CNewFolderDlg :
    public CCustomDialogIndirectImpl<CNewFolderDlg>,
    public CDialogResize<CNewFolderDlg>
{
    public:
        CNewFolderDlg(CFolderItem &folder, bool CreateNewFolder, const std::vector<std::string>& accessTypeList);
        enum { IDD = IDD_NEWFOLDERDLG };
    protected:
         BEGIN_MSG_MAP(CNewFolderDlg)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
              COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
              COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
            CHAIN_MSG_MAP(CDialogResize<CNewFolderDlg>)
         END_MSG_MAP()

        BEGIN_DLGRESIZE_MAP(CNewFolderDlg)
            DLGRESIZE_CONTROL(IDC_FOLDERTITLEEDIT, DLSZ_SIZE_X)
            DLGRESIZE_CONTROL(IDC_FOLDERDESCREDIT, DLSZ_SIZE_X|DLSZ_SIZE_Y)
            DLGRESIZE_CONTROL(IDC_ACCESSTYPECOMBO,  DLSZ_SIZE_X | DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_ACCESSTYPELABEL,  DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X| DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X|DLSZ_MOVE_Y)
        END_DLGRESIZE_MAP()
         // Handler prototypes:
         //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
         //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        CString m_sTitle;
        bool m_bCreateNewFolder;
        CString m_sDescription;
        CFolderItem &m_folder;
        std::vector<std::string> m_accessTypeList;
};

#endif // NEWFOLDERDLG_H

