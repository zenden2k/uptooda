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
#include "resource.h"       // main symbols
#include "Gui/Controls/DialogIndirect.h"
#include "Gui/Components/MyFileDialog.h"

// CTextViewDlg

class CTextViewDlg :
    public CCustomDialogIndirectImpl<CTextViewDlg>,
    public CDialogResize<CTextViewDlg>
{
    public:
        CTextViewDlg(const CString& text, const CString& title, const CString& info, const CString& question,
                     const CString& okCaption = TR(
                           "OK"), const CString& cancelCaption = TR("Cancel"));

        void setFileDialogOptions(IMyFileDialog::FileFilterArray filters, CString suggestedFileName);
        enum { IDD = IDD_TEXTVIEWDLG };

    protected:
        BEGIN_MSG_MAP(CTextViewDlg)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
            COMMAND_HANDLER(IDC_SAVE, BN_CLICKED, OnClickedSave)
            CHAIN_MSG_MAP(CDialogResize<CTextViewDlg>)
        END_MSG_MAP()

        BEGIN_DLGRESIZE_MAP(CTextViewDlg)
            DLGRESIZE_CONTROL(IDC_QUESTIONLABEL, DLSZ_MOVE_Y | DLSZ_SIZE_X)
            DLGRESIZE_CONTROL(IDC_TEXTEDIT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
            DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
            //DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        END_DLGRESIZE_MAP()

        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        CString m_okCaption, m_cancelCaption, m_text, m_title, m_info, m_question;

        IMyFileDialog::FileFilterArray fileDialogFilters_;
        CString suggestedFileName_;
};
