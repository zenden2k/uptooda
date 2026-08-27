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

#include "GeneralSettings.h"

#include "LogWindow.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Gui/Components/MyFileDialog.h"
#include "Core/i18n/Translator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/Helpers/LangHelper.h"

LRESULT CGeneralSettings::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    // Translating controls
    TRC(IDC_CHANGESWILLBE, "Please note that program needs to be restarted for new language settings to take affect.");
    TRC(IDC_LANGUAGELABEL, "Interface language:");
    TRC(IDC_VIEWLOG, "Show Error Log");
    TRC(IDC_LOGGROUP, "Error control");
    TRC(IDC_IMAGEEDITLABEL, "Images editor:");
    TRC(IDC_AUTOSHOWLOG, "Show automatically log window in case of error");
    TRC(IDC_CONFIRMONEXIT, "Ask confirmation on exit");
    TRC(IDC_DROPVIDEOFILESTOTHELIST, "Add video files to the list after Drag'n'Drop");
    TRC(IDC_DEVELOPERMODE, "Developer mode");
    TRC(IDC_CHECKUPDATES, "Automatically check for updates");
    TRC(IDC_ENABLETOASTS, "Enable Toast notifications (Windows 8+)");
    TRC(IDC_THUMBNAILSFORVIDEOCHECKBOX, "Show preview for video files in file list");
    TRC(IDC_CLEARSERVERSETTINGS, "Clear server settings");
    SetDlgItemText(IDC_IMAGEEDITORPATH, U2W(settings->ImageEditorPath));

    if (ServiceLocator::instance()->translator()->isRTL()) {
        // Removing WS_EX_RTLREADING style from some controls to look properly when RTL interface language is chosen
        HWND imageEditorPathHwnd = GetDlgItem(IDC_IMAGEEDITORPATH);
        LONG styleEx = ::GetWindowLong(imageEditorPathHwnd, GWL_EXSTYLE);
        ::SetWindowLong(imageEditorPathHwnd, GWL_EXSTYLE, styleEx & ~WS_EX_RTLREADING);
    }
    langListCombo_ = GetDlgItem(IDC_LANGLIST);

    toolTipCtrl_ = GuiTools::CreateToolTipForWindow(GetDlgItem(IDC_BROWSEBUTTON), TR("Choose executable file"));

    auto languageList{ LangHelper::instance()->getLanguageList((WinUtils::GetAppFolder() + "Lang").GetString()) };

    const std::string& selectedLocale = settings->Language;

    int selectedIndex = -1;
    for (const auto& [key, title] : languageList) {
        int index = langListCombo_.AddString(U2W(title));
        langListCombo_.SetItemDataPtr(index, _strdup(key.c_str()));
        if (key == selectedLocale) {
            selectedIndex = index;
        }
    }
    langListCombo_.SetCurSel(selectedIndex);

    SendDlgItemMessage(IDC_CONFIRMONEXIT, BM_SETCHECK, settings->ConfirmOnExit);
    SendDlgItemMessage(IDC_AUTOSHOWLOG, BM_SETCHECK, settings->AutoShowLog);
    GuiTools::SetCheck(m_hWnd, IDC_DROPVIDEOFILESTOTHELIST, settings->DropVideoFilesToTheList);
    GuiTools::SetCheck(m_hWnd, IDC_DEVELOPERMODE, settings->DeveloperMode);
    GuiTools::SetCheck(m_hWnd, IDC_CHECKUPDATES, settings->AutomaticallyCheckUpdates);
    GuiTools::SetCheck(m_hWnd, IDC_ENABLETOASTS, settings->EnableToastNotifications);
    GuiTools::SetCheck(m_hWnd, IDC_THUMBNAILSFORVIDEOCHECKBOX, settings->ShowPreviewForVideoFiles);
    GuiTools::EnableDialogItem(m_hWnd, IDC_THUMBNAILSFORVIDEOCHECKBOX, WtlGuiSettings::IsFFmpegAvailable());
    return 1;  // Let the system set the focus
}

LRESULT CGeneralSettings::OnBnClickedBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    IMyFileDialog::FileFilterArray filters = {
        { CString(TR("Executables")), _T("*.exe;*.com;*.bat;*.cmd;"), },
        { TR("All files"), _T("*.*") }
    };

    auto dlg = MyFileDialogFactory::createFileDialog(m_hWnd, WinUtils::GetAppFolder(), TR("Choose program"), filters, false);
    if (dlg->DoModal(m_hWnd) != IDOK) {
        return 0;
    }

    CString fileName = dlg->getFile();
    if (fileName.IsEmpty()) {
        return 0;
    }

    CString cmdLine = CString(_T("\"")) + fileName + CString(_T("\""));
    cmdLine += _T(" \"%1\"");

    SetDlgItemText(IDC_IMAGEEDITORPATH, cmdLine);
    return 0;
}


bool CGeneralSettings::apply()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    int index = langListCombo_.GetCurSel();

    if (index != -1) {
        char* key = static_cast<char*>(langListCombo_.GetItemDataPtr(index));
        if (key) {
            settings->Language = key;
        }
    }

    settings->ImageEditorPath = W2U(GuiTools::GetWindowText(GetDlgItem(IDC_IMAGEEDITORPATH)));

    settings->AutoShowLog = SendDlgItemMessage(IDC_AUTOSHOWLOG,  BM_GETCHECK )==BST_CHECKED;
    settings->ConfirmOnExit = SendDlgItemMessage(IDC_CONFIRMONEXIT, BM_GETCHECK)==BST_CHECKED;
    settings->ShowPreviewForVideoFiles = SendDlgItemMessage(IDC_THUMBNAILSFORVIDEOCHECKBOX, BM_GETCHECK)==BST_CHECKED;
    settings->DropVideoFilesToTheList = GuiTools::GetCheck(m_hWnd, IDC_DROPVIDEOFILESTOTHELIST);
    GuiTools::GetCheck(m_hWnd, IDC_DEVELOPERMODE, settings->DeveloperMode);
    GuiTools::GetCheck(m_hWnd, IDC_CHECKUPDATES, settings->AutomaticallyCheckUpdates);
    GuiTools::GetCheck(m_hWnd, IDC_ENABLETOASTS, settings->EnableToastNotifications);
    return true;
}


LRESULT CGeneralSettings::OnBnClickedViewLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ServiceLocator::instance()->logWindow()->Show();
    return 0;
}

LRESULT CGeneralSettings::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    for (int i = 0; i < langListCombo_.GetCount(); i++) {
        free(static_cast<char*>(langListCombo_.GetItemDataPtr(i)));
    }
    langListCombo_.ResetContent();
    return 0;
}

LRESULT CGeneralSettings::OnBnClickedClearServerSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    auto builtInScripts = CUploadEngineListBase::builtInScripts();

    // 'Directory' server type has no account
    auto newEnd = std::remove_if(builtInScripts.begin(), builtInScripts.end(), [](auto&& s) { return s == CUploadEngineListBase::CORE_SCRIPT_DIRECTORY; });
    builtInScripts.erase(newEnd, builtInScripts.end());

    std::string msg = str(boost::format(
        _("All account data will be deleted, including logins, passwords and other settings. "
          "The following server types' settings will also be deleted: %s.")
        )
        % IuStringUtils::Join(builtInScripts, ", ")
    );
    if (GuiTools::LocalizedMessageBox(m_hWnd, U2WC(msg), APP_NAME, MB_OKCANCEL | MB_ICONWARNING) == IDOK) {
        BasicSettings* settings = ServiceLocator::instance()->basicSettings();
        settings->clearServerSettings();
        settings->SaveSettings();
    }
    return 0;
}
