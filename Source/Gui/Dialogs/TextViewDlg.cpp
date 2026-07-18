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

#include "TextViewDlg.h"

#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Core/CommonDefs.h"

// CTextViewDlg
CTextViewDlg::CTextViewDlg(const CString &text, const CString &title, const CString &info, const CString &question , const CString &okCaption,const CString &cancelCaption)
{
    m_text = text;
    m_okCaption = okCaption;
    m_cancelCaption = cancelCaption;
    m_question = question;
    m_info = info;
    m_title = title;
}

void CTextViewDlg::setFileDialogOptions(IMyFileDialog::FileFilterArray filters, CString suggestedFileName) {
    fileDialogFilters_ = std::move(filters);
    suggestedFileName_ = suggestedFileName;
}

CTextViewDlg::~CTextViewDlg()
{
}

LRESULT CTextViewDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DlgResize_Init();
    CenterWindow(GetParent());
    SetDlgItemText(IDOK, m_okCaption);
    SetDlgItemText(IDCANCEL, m_cancelCaption);
    const CString normalizedText = WinUtils::NormalizeLineEndings(m_text);

    SetDlgItemText(IDC_TEXTEDIT, normalizedText);
    SetDlgItemText(IDC_QUESTIONLABEL, m_question);
    SetDlgItemText(IDC_TITLETEXT,m_info);
    SetWindowText(m_title);
    return 1;  // Let the system set the focus
}

LRESULT CTextViewDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CTextViewDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CTextViewDlg::OnClickedSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    auto dlg = MyFileDialogFactory::createFileDialog(m_hWnd, CString(), CString(), fileDialogFilters_, false, false);
    dlg->setFileName(suggestedFileName_);
    CString ext = WinUtils::GetFileExt(suggestedFileName_);
    ext.MakeLower();
    dlg->setDefaultExtension(ext);

    if (dlg->DoModal(m_hWnd) == IDOK) {
        try {
            IuCoreUtils::PutFileContents(W2U(dlg->getFile()), W2U(m_text));
        } catch (const std::exception& e) {
            GuiTools::LocalizedMessageBox(m_hWnd, U2WC(e.what()), TR("Error"), MB_ICONERROR);
        }
    }

    return 0;
}
