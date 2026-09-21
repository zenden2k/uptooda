
#include "TransferSettingsPage.h"

#include "Core/CommonDefs.h"
#include "WizardDlg.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Gui/Components/MyFileDialog.h"
#include "Core/Settings/WtlGuiSettings.h"

void CTransferSettingsPage::TranslateUI()
{
    TRC(IDC_AUTOCOPYTOCLIPBOARD, "Copy automatically results to clipboard");
    TRC(IDC_UPLOADERRORLABEL, "Uploading errors");
    TRC(IDC_RETRIES1LABEL, "Num of retries per file:");
    TRC(IDC_RETRIES2LABEL, "Num of retries per action:");
    TRC(IDC_MAXTHREADSLABEL, "Threads number:");
    TRC(IDC_EXECUTESCRIPTCHECKBOX, "Execute Squirrel script for each task (file)");
    TRC(IDC_QUICKUPLOADLABEL, "Uploading initiated from tray");
    TRC(IDC_GENERATEDCODELABEL, "Copy to the clipboard:");
    TRC(IDC_JUSTURLRADIO, "Just URL");
    TRC(IDC_USELASTCODETYPERADIO, "Last code type selected");
    TRC(IDC_CHECKFILETYPESCHECKBOX, "Check file types before upload");
}
    
LRESULT CTransferSettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    TranslateUI();

    BOOL temp;
    DoDataExchange(FALSE);

    SetDlgItemInt(IDC_MAXTHREADSEDIT, Settings.MaxThreads);

    SetDlgItemInt(IDC_FILERETRYLIMIT, Settings.FileRetryLimit);
    SetDlgItemInt(IDC_ACTIONRETRYLIMIT, Settings.ActionRetryLimit);
    SendDlgItemMessage(IDC_AUTOCOPYTOCLIPBOARD, BM_SETCHECK, (WPARAM)Settings.AutoCopyToClipboard);

    GuiTools::SetCheck(m_hWnd, IDC_EXECUTESCRIPTCHECKBOX, Settings.ExecuteScript);
    GuiTools::SetCheck(m_hWnd, IDC_CHECKFILETYPESCHECKBOX, Settings.CheckFileTypesBeforeUpload);
    executeScriptCheckboxChanged();
    SetDlgItemText(IDC_SCRIPTFILENAMEEDIT, IuCoreUtils::Utf8ToWstring(Settings.ScriptFileName).c_str());

    useLastCodeTypeRadioButton_.SetCheck(Settings.TrayResult == WtlGuiSettings::trLastCodeType);
    justURLRadioButton_.SetCheck(Settings.TrayResult == WtlGuiSettings::trJustURL);
	
    return 1;  // Let the system set the focus
}

LRESULT CTransferSettingsPage::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CTransferSettingsPage::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}


LRESULT CTransferSettingsPage::OnExecuteScriptCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    executeScriptCheckboxChanged();
    return 0;
}

bool CTransferSettingsPage::apply()
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    checkBounds(IDC_FILERETRYLIMIT, 1, 10, IDC_RETRIES1LABEL);
    checkBounds(IDC_ACTIONRETRYLIMIT, 1, 10, IDC_RETRIES2LABEL);
    
    DoDataExchange(TRUE);
    Settings.FileRetryLimit = GetDlgItemInt(IDC_FILERETRYLIMIT);
    Settings.ActionRetryLimit = GetDlgItemInt(IDC_ACTIONRETRYLIMIT);

    Settings.AutoCopyToClipboard = SendDlgItemMessage(IDC_AUTOCOPYTOCLIPBOARD, BM_GETCHECK)!=0;
    Settings.CheckFileTypesBeforeUpload = SendDlgItemMessage(IDC_CHECKFILETYPESCHECKBOX, BM_GETCHECK)== BST_CHECKED;

    Settings.TrayResult = useLastCodeTypeRadioButton_.GetCheck() == BST_CHECKED;
    checkBounds(IDC_MAXTHREADSEDIT, 1, 20, IDC_MAXTHREADSLABEL);
    Settings.MaxThreads = GetDlgItemInt(IDC_MAXTHREADSEDIT);
    if (Settings.MaxThreads <= 0 || Settings.MaxThreads > 50 )
    {
        Settings.MaxThreads = 3;
    }

    GuiTools::GetCheck(m_hWnd, IDC_EXECUTESCRIPTCHECKBOX, Settings.ExecuteScript);
    CString scriptFile = GuiTools::GetDlgItemText(m_hWnd, IDC_SCRIPTFILENAMEEDIT);

    if (scriptFile.IsEmpty()) {
        Settings.ExecuteScript = false;
    } else if (Settings.ExecuteScript && !WinUtils::FileExists(scriptFile)) {
        CString message1, message2;
        CString fieldTitle = GuiTools::GetDlgItemText(m_hWnd, IDC_EXECUTESCRIPTCHECKBOX);
        message1.Format(TR("Error in the field '%s':\n"), fieldTitle.GetString());
        message2.Format(TR("File %s doesn't exist"), static_cast<LPCTSTR>(scriptFile));
        throw ValidationException(message1+message2, GetDlgItem(IDC_SCRIPTFILENAMEEDIT));
    }
    Settings.ScriptFileName = W2U(scriptFile);

    return true;
}

LRESULT CTransferSettingsPage::OnBnClickedBrowseScriptButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    IMyFileDialog::FileFilterArray filters = {
        { CString(_T("Squirrel 3 script (.nut)")), _T("*.nut;"), },
        { TR("All files"), _T("*.*") }
    };

    auto dlg = MyFileDialogFactory::createFileDialog(m_hWnd, WinUtils::GetAppFolder(), CString(), filters, false);

    if (dlg->DoModal(m_hWnd) != IDOK) {
        return 0;
    }

    CString fileName = dlg->getFile();

    if (fileName.IsEmpty()) {
        return 0;
    }

    SetDlgItemText(IDC_SCRIPTFILENAMEEDIT, fileName);

    return 0;
}

void CTransferSettingsPage::executeScriptCheckboxChanged() {
    bool Checked = SendDlgItemMessage(IDC_EXECUTESCRIPTCHECKBOX, BM_GETCHECK) != 0;
    GuiTools::EnableNextN(GetDlgItem(IDC_EXECUTESCRIPTCHECKBOX), 2, Checked);
}