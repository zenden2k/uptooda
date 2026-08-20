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

#include "MediaInfoDlg.h"

#include <boost/format.hpp>

#include "Func/MediaInfoHelper.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/CommonDefs.h"

// CMediaInfoDlg
CMediaInfoDlg::CMediaInfoDlg()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    infoType_ = static_cast<InfoType>(settings->MediaInfoSettings.InfoType);
    generateTextInEnglish_ = !settings->MediaInfoSettings.EnableLocalization;
}

LRESULT CMediaInfoDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CenterWindow(GetParent());
    boldFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_FILEINFOLABEL));
    DlgResize_Init(false, true, 0); // resizable dialog without "griper"
    LOGFONT logFont;
    WinUtils::StringToFont(_T("Courier New,8,,204"), &logFont);

    editFont_.CreateFontIndirect(&logFont);
    SendDlgItemMessage(IDC_FILEINFOEDIT, WM_SETFONT, reinterpret_cast<WPARAM>(editFont_.m_hFont), MAKELPARAM(false, 0));
    editControl_.AttachToDlgItem(m_hWnd, IDC_FILEINFOEDIT);

    // Translating controls' text
    TRC(IDOK, "OK");
    SetWindowText(TR("Information about file"));
    TRC(IDC_COPYALL, "Copy to clipboard");
    TRC(IDC_SUMMARYRADIOBUTTON, "Summary");
    TRC(IDC_FULLINFORADIOBUTTON, "Full information");
    TRC(IDC_GENERATETEXTINENGLISHCHECKBOX, "Disable localization");

    SetDlgItemText(IDC_FILEINFOEDIT, TR("Loading..."));

    GuiTools::SetCheck(m_hWnd, infoType_ == InfoType::itFullInformation ? IDC_FULLINFORADIOBUTTON: IDC_SUMMARYRADIOBUTTON, true);

    GuiTools::SetCheck(m_hWnd, IDC_GENERATETEXTINENGLISHCHECKBOX, generateTextInEnglish_);

    ::SetFocus(GetDlgItem(IDC_FILEINFOEDIT));
    FixEditRTL();
    GenerateInfo();
    return 0;
}

LRESULT CMediaInfoDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(!IsRunning())  EndDialog(wID); // Don't allow user to close dialog before thread finishes
    return 0;
}

LRESULT CMediaInfoDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(!IsRunning())  EndDialog(wID);
    return 0;
}

void CMediaInfoDlg::ShowInfo(HWND parentWnd, LPCTSTR FileName)
{
    m_FileName = FileName;
    DoModal(parentWnd);
}

DWORD CMediaInfoDlg::Run()
{
    CString ShortFileName = WinUtils::TrimString(WinUtils::DoExtractFileName(m_FileName), 40);
    if(!WinUtils::FileExists(m_FileName))
    {
        SetDlgItemText(IDC_FILEINFOLABEL, CString(TR("Error:")));
        std::wstring infoEditText;
        try {
            infoEditText = str(boost::wformat(TR("File \"%s\" not found!")) % ShortFileName.GetString());
        }
        catch (std::exception& ex) {
            LOG(ERROR) << ex.what();
        }
        SetDlgItemText(IDC_FILEINFOEDIT, infoEditText.c_str());
        return 0;
    }

    std::wstring fileInfoLabel;
    try {
        fileInfoLabel = str(boost::wformat(TR("Information about file \"%s\" :")) % ShortFileName.GetString());
    } catch (std::exception& ex) {
        LOG(ERROR) << ex.what();
    }

    SetDlgItemText(IDC_FILEINFOLABEL, fileInfoLabel.c_str());

    MediaInfoHelper::GetMediaFileInfo(W2U(m_FileName), summary_, fullInfo_, !generateTextInEnglish_);
    bool fullInfo = GuiTools::GetCheck(m_hWnd, IDC_FULLINFORADIOBUTTON);
    SetDlgItemText(IDC_FILEINFOEDIT, U2W(fullInfo ? fullInfo_ : summary_));
    return 0;
}

void CMediaInfoDlg::GenerateInfo() {
    if (!IsRunning()) {
        generateTextInEnglish_ = GuiTools::GetCheck(m_hWnd, IDC_GENERATETEXTINENGLISHCHECKBOX);
        Release();
        Start(); // Starting thread which will load in background information about file m_FileName
    }
}

void CMediaInfoDlg::FixEditRTL() {
    HWND editControl = GetDlgItem(IDC_FILEINFOEDIT);

    if (ServiceLocator::instance()->translator()->isRTL()) {
        LONG styleEx = ::GetWindowLong(editControl, GWL_EXSTYLE);
        LONG style = ::GetWindowLong(editControl, GWL_STYLE);
        //DWORD isStyle = style & ES_RIGHT;

        if (generateTextInEnglish_) {
            styleEx = styleEx & ~(WS_EX_RTLREADING | WS_EX_LAYOUTRTL | WS_EX_RIGHT);
            style = style & (~ES_RIGHT);
        } else {
            styleEx = styleEx | WS_EX_RTLREADING | WS_EX_LAYOUTRTL | WS_EX_RIGHT;
            style = style | ES_RIGHT;
        }
        ::SetWindowLong(editControl, GWL_EXSTYLE, styleEx);
        ::SetWindowLong(editControl, GWL_STYLE, style);
    }
}

LRESULT CMediaInfoDlg::OnBnClickedCopyall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Copying text to clipboard
    SendDlgItemMessage(IDC_FILEINFOEDIT, EM_SETSEL, 0, -1);
    SendDlgItemMessage(IDC_FILEINFOEDIT, WM_COPY, 0, 0);
    SendDlgItemMessage(IDC_FILEINFOEDIT, EM_SETSEL, static_cast<WPARAM>(-1), 0);
    return 0;
}

LRESULT CMediaInfoDlg::OnInfoRadioButtonClicked(WORD, WORD, HWND, BOOL&) {
    bool fullInfo = GuiTools::GetCheck(m_hWnd, IDC_FULLINFORADIOBUTTON);
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    SetDlgItemText(IDC_FILEINFOEDIT, U2W(fullInfo ? fullInfo_ : summary_));
    settings->MediaInfoSettings.InfoType = fullInfo ? 1 : 0;
    return 0;
}

LRESULT CMediaInfoDlg::OnShowInEnglishCheckboxClicked(WORD, WORD, HWND, BOOL&) {
    generateTextInEnglish_ = GuiTools::GetCheck(m_hWnd, IDC_GENERATETEXTINENGLISHCHECKBOX);

    FixEditRTL();
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->MediaInfoSettings.EnableLocalization = !generateTextInEnglish_;
    GenerateInfo();
    return 0;
}
