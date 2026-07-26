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

#include "WizardCommon.h"
#include "atlheaders.h"
#include "Dialogs/WizardDlg.h"

bool CWizardPage::OnShow() {
    EnableNext();
    EnablePrev();
    ShowNext();
    ShowPrev();
    return true;
}

bool CWizardPage::OnNext() {
    return true;
}

void CWizardPage::EnableNext(bool Enable) {
    if (!WizardDlg) {
        return;
    }

    WizardDlg->GetDlgItem(IDC_NEXT).EnableWindow(Enable);
}

void CWizardPage::EnablePrev(bool Enable) {
    if (!WizardDlg) {
        return;
    }

    WizardDlg->GetDlgItem(IDC_PREV).EnableWindow(Enable);
}

void CWizardPage::EnableExit(bool Enable) {
    if (!WizardDlg) {
        return;
    }

    WizardDlg->GetDlgItem(IDCANCEL).EnableWindow(Enable);
}

void CWizardPage::SetNextCaption(LPCTSTR Caption) {
    if (!WizardDlg) {
        return;
    }

    WizardDlg->SetDlgItemText(IDC_NEXT, Caption);
}

void CWizardPage::ShowNext(bool Show) {
    if (!WizardDlg) {
        return;
    }

    WizardDlg->GetDlgItem(IDC_NEXT).ShowWindow(Show ? SW_SHOW : SW_HIDE);
}

void CWizardPage::ShowPrev(bool Show) {
    if (!WizardDlg) {
        return;
    }

    WizardDlg->GetDlgItem(IDC_PREV).ShowWindow(Show ? SW_SHOW : SW_HIDE);
}

void CWizardPage::SetInitialFocus() {
    ::SetFocus(PageWnd);
}

bool CWizardPage::OnHide() {
    return false;
}
