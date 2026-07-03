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

#include "LangSelect.h"

#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Func/LangClass.h"
#include "Gui/Helpers/LangHelper.h"

LRESULT CLangSelect::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    langListCombo_ = GetDlgItem(IDC_LANGLIST);
    LogoImage.SubclassWindow(GetDlgItem(IDC_STATICLOGO));
    LogoImage.SetWindowPos(0, 0,0, 48, 48, SWP_NOMOVE|SWP_NOZORDER );
    LogoImage.loadImage(0, 0, IDR_ICONMAINNEW, false, GetSysColor(COLOR_BTNFACE));

    boldFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_PLEASECHOOSE));

    auto languageList{ LangHelper::instance()->getLanguageList((WinUtils::GetAppFolder() + "Lang").GetString()) };

    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    std::string selectedLocale = W2U(settings->Language);

    if (settings->Language.IsEmpty()) {
        TCHAR buffer[LOCALE_NAME_MAX_LENGTH];
        GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH);
        CString locale = buffer;
        locale.Replace(_T('-'), _T('_'));
        selectedLocale = W2U(locale);
        if (languageList.find(selectedLocale) == languageList.end()) {
            selectedLocale = selectedLocale.substr(0, selectedLocale.find('_'));
        }
    }

    for (const auto& [key,title] : languageList) {
        int index = langListCombo_.AddString(U2W(title));
        langListCombo_.SetItemDataPtr(index, _strdup(key.c_str()));
        if (key == selectedLocale) {
            langListCombo_.SetCurSel(index);
        }
    }

    if (languageList.empty()) {
        EndDialog(IDOK);
        language_ = _T("en");
        return 0;
    }

    return 1;
}

LRESULT CLangSelect::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int Index = langListCombo_.GetCurSel();
    if (Index < 0) {
        return 0;
    }
    char* key = static_cast<char*>(langListCombo_.GetItemDataPtr(Index));
    if (key) {
        language_ = U2W(key);
    }
    return EndDialog(wID);
}

LRESULT CLangSelect::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

void CLangSelect::SelectLang(LPCTSTR Lang)
{
    int Index = langListCombo_.FindStringExact(0, Lang);
    if (Index < 0) {
        return;
    }
    langListCombo_.SetCurSel(Index);
}

CString CLangSelect::getLanguage() const {
    return language_;
}

LRESULT CLangSelect::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    for (int i = 0; i < langListCombo_.GetCount(); i++) {
        free(langListCombo_.GetItemDataPtr(i));
    }
    langListCombo_.ResetContent();
    return 0;
}
