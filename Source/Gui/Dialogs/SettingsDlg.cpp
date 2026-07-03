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

#include "SettingsDlg.h"

#include "TraySettings.h"
#include "HotkeySettingsPage.h"
#include "ScreenshotSettingsPage.h"
#include "ScreenRecordingSettingsPage.h"
#include "ImageEditorSettingsPage.h"
#include "ThumbSettingsPage.h"
#include "GeneralSettings.h"
#include "ConnectionSettingsPage.h"
#include "TransferSettingsPage.h"
#include "VideoGrabberSettingsPage.h"
#include "IntegrationSettings.h"
#include "DefaultServersSettings.h"
#include "Gui/GuiTools.h"

// CSettingsDlg
CSettingsDlg::CSettingsDlg(SettingsPage Page, UploadEngineManager* uploadEngineManager):
    PageToShow(Page), uploadEngineManager_(uploadEngineManager)
{
    CurPage = spNone;
    memset(&Pages, 0, sizeof(Pages));
    backgroundBrush_.CreateSysColorBrush(COLOR_BTNFACE);
}

LRESULT CSettingsDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // center the dialog on the screen
    CenterWindow();
    saveStatusLabelFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_SAVESTATUSLABEL));

    HWND parent = GetParent();
    if(!parent || !::IsWindowVisible(parent))
    {
        SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) & WS_EX_APPWINDOW);
    }

    m_SettingsPagesListBox.SubclassWindow(GetDlgItem(IDC_SETTINGSPAGESLIST));
    fillListBox();
    // set icons

    hIcon = GuiTools::LoadBigIcon(IDR_MAINFRAME);
    hIconSmall = GuiTools::LoadSmallIcon(IDR_MAINFRAME);

    SetIcon(hIcon, TRUE);
    SetIcon(hIconSmall, FALSE);

    TRC(IDOK, "OK");
    TRC(IDCANCEL, "Cancel");
    TRC(IDC_APPLY, "Apply");
    TRC(IDC_SAVESTATUSLABEL, "Settings have been saved.");
    SetWindowText(TR("Settings"));

    m_SettingsPagesListBox.SetCurSel(PageToShow);
    ShowPage(PageToShow);
    return 0;
}

LRESULT CSettingsDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (!apply()) {
        return 0;  // If some tab cannot apply changes - do not close dialog
    }
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->SaveSettings();
    EndDialog(wID);
    return 0;
}

LRESULT CSettingsDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CSettingsDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    for (auto& page : Pages) {
        if (page) {
            ::DestroyWindow(page->PageWnd);
        }
    }
    return 0;
}

LRESULT CSettingsDlg::OnTimer(UINT, WPARAM, LPARAM, BOOL&)
{
    GuiTools::ShowDialogItem(m_hWnd, IDC_SAVESTATUSLABEL, false);
    KillTimer(kStatusLabelTimer);
    return 0;
}

LRESULT CSettingsDlg::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    m_SettingsPagesListBox.SendMessage(WM_MY_DPICHANGED, wParam);
    //fillListBox();
    for (int i = 0; i < SettingsPageCount; i++) {
        const auto& page = Pages[i];
        if (page) {
            ::SendMessage(page->PageWnd, WM_MY_DPICHANGED, wParam, 0);
        }
    }
    return 0;
}

bool CSettingsDlg::ShowPage(SettingsPage idPage)
{
    if(idPage< 0 || idPage> SettingsPageCount-1) return false;

    if(idPage == CurPage) return true;

    if(!Pages[idPage])
        CreatePage(idPage);

    if(Pages[idPage])
        ::ShowWindow(Pages[idPage]->PageWnd, SW_SHOW);

    if (CurPage != spNone && Pages[CurPage]) {
        ::ShowWindow(Pages[CurPage]->PageWnd, SW_HIDE);
    }
    CurPage = idPage;

    m_SettingsPagesListBox.SetCurSel(CurPage);
    return true;
}

LRESULT CSettingsDlg::OnApplyBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (!apply()) {
        return 0;
    }
    GuiTools::ShowDialogItem(m_hWnd, IDC_SAVESTATUSLABEL, true);
    SetTimer(kStatusLabelTimer, 3000);
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->SaveSettings();
    return 0;
}

bool CSettingsDlg::apply()  {
    for (int i = 0; i < SettingsPageCount; i++) {
        const auto& page = Pages[i];
        if (!page) {
            continue;
        }
        page->clearErrors();
        if (!page->validate()) {
            ShowPage(static_cast<SettingsPage>(i));
            const auto& errors = page->errors();
            if (!errors.empty()) {
                CString msg;
                for (const auto& error : errors) {
                    msg += error.Message;
                    msg += _T("\r\n");
                }
                LocalizedMessageBox(msg, TR("Error"), MB_ICONERROR);
                if (errors[0].Control) {
                    ::SetFocus(errors[0].Control);
                }
            }
            return false;
        }
    }
    for (int i = 0; i < SettingsPageCount; i++) {
        try {
            if (Pages[i] && !Pages[i]->apply()) {
                ShowPage(static_cast<SettingsPage>(i));
                return false;
            }
        } catch (ValidationException& ex) {
            ShowPage(static_cast<SettingsPage>(i));
            if (!ex.errors_.empty()) {
                LocalizedMessageBox(ex.errors_[0].Message, TR("Error"), MB_ICONERROR);
                if (ex.errors_[0].Control) {
                    ::SetFocus(ex.errors_[0].Control);
                }
            }

            return false;
            // If some tab cannot apply changes - do not close dialog
        }
    }
    return true;
}

void CSettingsDlg::fillListBox() {
    m_SettingsPagesListBox.ResetContent();
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "General")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Servers")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Images")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Thumbnails")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Screen Capture")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Image Editor")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Screen Recording")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Video")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Connection")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Uploading")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Integration")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Tray icon")));
    m_SettingsPagesListBox.AddString(U2WC(_c("settings.menu", "Hotkeys")));
}

template<typename T, typename... Args> std::unique_ptr<T> createPageObject(HWND hWnd, RECT& rc, Args&&... args) {
    auto dlg = std::make_unique<T>(std::forward<Args>(args)...);
    dlg->Create(hWnd, rc);
    dlg->PageWnd = dlg->m_hWnd;
    return dlg;
}

bool CSettingsDlg::CreatePage(SettingsPage pageId)
{
    RECT rc = { 150,3,636,400 };
    auto createObject = [&]() -> std::unique_ptr<CSettingsPage> {
        switch (pageId) {
            case spGeneral:
                return createPageObject<CGeneralSettings>(m_hWnd, rc);
            case spServers:
                return createPageObject<CDefaultServersSettings>(m_hWnd, rc, uploadEngineManager_);
            case spImages:
                return createPageObject<CLogoSettings>(m_hWnd, rc);
            case spThumbnails:
                return createPageObject<CThumbSettingsPage>(m_hWnd, rc);
            case spScreenshot:
                return createPageObject<CScreenshotSettingsPage>(m_hWnd, rc);
            case spImageEditor:
                return createPageObject<CImageEditorSettingsPage>(m_hWnd, rc);
            case spScreenRecording:
                return createPageObject<CScreenRecordingSettingsPage>(m_hWnd, rc);
            case spVideo:
                return createPageObject<CVideoGrabberSettingsPage>(m_hWnd, rc);
            case spConnection:
                return createPageObject<CConnectionSettingsPage>(m_hWnd, rc);
            case spUploading:
                return createPageObject<CTransferSettingsPage>(m_hWnd, rc);
            case spIntegration:
                return createPageObject<CIntegrationSettings>(m_hWnd, rc, uploadEngineManager_);
            case spTrayIcon:
                return createPageObject<CTraySettingsPage>(m_hWnd, rc);
            case spHotkeys:
                return createPageObject<CHotkeySettingsPage>(m_hWnd, rc);
            default:
                LOG(ERROR) << "No such page " << pageId;
                return {};
        }
    };

    std::unique_ptr<CSettingsPage> page = createObject();
    Pages[pageId] = std::move(page);

    WINDOWPLACEMENT wp;
    ::GetWindowPlacement(GetDlgItem(IDC_TABCONTROL), &wp);
    TabCtrl_AdjustRect(GetDlgItem(IDC_TABCONTROL),FALSE, &wp.rcNormalPosition);
    ::SetWindowPos(Pages[pageId]->PageWnd, m_SettingsPagesListBox, wp.rcNormalPosition.left, wp.rcNormalPosition.top, -wp.rcNormalPosition.left + wp.rcNormalPosition.right, -wp.rcNormalPosition.top + wp.rcNormalPosition.bottom, 0);

    Pages[pageId]->fixBackground();
    return true;
}

LRESULT CSettingsDlg::OnTabChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    int Index = TabCtrl_GetCurSel(GetDlgItem(idCtrl));
    ShowPage(static_cast<SettingsPage>(Index));
    return 0;
}

LRESULT CSettingsDlg::OnSettingsPagesSelChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int iPageIndex = m_SettingsPagesListBox.GetCurSel();
    ShowPage(static_cast<SettingsPage>(iPageIndex));

    return 0;
}

LRESULT CSettingsDlg::OnCtlColorStatic(HDC hdc, HWND hwndChild)
{
    if (hwndChild == GetDlgItem(IDC_SAVESTATUSLABEL)) {
        SetTextColor(hdc, RGB(0, 180, 0));
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(static_cast<HBRUSH>(backgroundBrush_));
    }
    return 0;
}
