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

#include "NewFolderDlg.h"

#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"

CNewFolderDlg::CNewFolderDlg(CFolderItem &folder, bool CreateNewFolder, const std::vector<std::string>& accessTypeList):
    m_bCreateNewFolder(CreateNewFolder), m_folder(folder), m_accessTypeList(accessTypeList)
{
}

LRESULT CNewFolderDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRC(IDC_FOLDERNAMELABEL, "Folder name:");
    TRC(IDC_FOLDERDESCRLABEL, "Summary:");
    TRC(IDC_ACCESSTYPELABEL, "Access:");
    TRC(IDCANCEL, "Cancel");
    TRC(IDOK, "OK");

    DlgResize_Init();
    CenterWindow(GetParent());
    if (m_bCreateNewFolder) {
        SetWindowText(TR("Create folder (album)"));
    } else {
        SetWindowText(TR("Edit folder"));
    }
    SetDlgItemText(IDC_FOLDERTITLEEDIT, Utf8ToWCstring(m_folder.title));
    CString text = U2W(m_folder.summary);
    text.Replace(_T("\r"), _T(""));
    text.Replace(_T("\n"), _T("\r\n"));
    SetDlgItemText(IDC_FOLDERDESCREDIT, text);
    for (const auto& i : m_accessTypeList) {
        SendDlgItemMessage(IDC_ACCESSTYPECOMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(U2W(i).GetString()));
    }
    SendDlgItemMessage(IDC_ACCESSTYPECOMBO, CB_SETCURSEL, m_folder.accessType);

    ::SetFocus(GetDlgItem(IDC_FOLDERTITLEEDIT));
    return 0;  // Let the system set the focus
}

LRESULT CNewFolderDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_sTitle = GuiTools::GetWindowText(GetDlgItem(IDC_FOLDERTITLEEDIT));
    m_folder.title = W2U(m_sTitle);
    m_sDescription = GuiTools::GetWindowText(GetDlgItem(IDC_FOLDERDESCREDIT));
    m_folder.summary = W2U(m_sDescription);
    int nAccessType = SendDlgItemMessage(IDC_ACCESSTYPECOMBO, CB_GETCURSEL);
    if(nAccessType >= 0) {
        m_folder.accessType = nAccessType;
    }
    EndDialog(wID);
    return 0;
}

LRESULT CNewFolderDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}
