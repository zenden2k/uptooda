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

#include "ScreenshotSettingsPage.h"

#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/GuiTools.h"
#include "Gui/Components/NewStyleFolderDialog.h"
#include "Func/MyUtils.h"
#include "Gui/Constants.h"
#include "Gui/Helpers/DPIHelper.h"

LRESULT CScreenshotSettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    TRC(IDC_GROUPPARAMS, "Additional settings");
    TRC(IDC_QUALITYLABEL, "Quality:");
    TRC(IDC_DELAYLABEL, "Timeout:");
    TRC(IDC_FORMATLABEL, "Format:");
    TRC(IDC_SECLABEL, "sec");
    TRC(IDC_MSECLABEL, "msec");
    TRC(IDC_SCREENSHOTSFOLDERSELECT, "Browse...");
    TRC(IDC_SCREENSHOTFOLDERLABEL, "Directory to save screenshots:");
    TRC(IDC_SCREENSHOTFILENAMELABEL, "Filename and path template:");
    TRC(IDC_DELAYLABEL2, "Delay when hiding window:");
    TRC(IDC_ALWAYSCOPYTOCLIPBOARD, "Always copy captured image to clipboard");
    TRC(IDC_SCREENSHOTSAVINGPARAMS, "Saving parameters");
    TRC(IDC_FOREGROUNDWHENSHOOTING, "Bring window to foreground when selected by mouse");
    TRC(IDC_ADDSHADOW, "Capture with shadow");
    CString removeCornersText = TR("Clear window transparent corners")+CString(_T(" (Windows Vista/7)"));
    SetDlgItemText(IDC_REMOVECORNERS, removeCornersText);
    TRC(IDC_REMOVEBACKGROUND, "Clear window's background");
    TRC(IDC_AEROONLY, "Aero only (Windows Vista or later)");
    TRC(IDC_USEOLDREGIONSCREENSHOTMETHOD, "Use old method of rectangular area selection");
    TRC(IDC_CAPTURECURSORCHECKBOX2, "Capture cursor");
    tooltipControl_.Create(m_hWnd);

    if (ServiceLocator::instance()->translator()->isRTL()) {
        // Removing WS_EX_RTLREADING style from some controls to look properly when RTL interface language is choosen
        HWND screenshotFolderEditHwnd = GetDlgItem(IDC_SCREENSHOTFOLDEREDIT);
        LONG styleEx = ::GetWindowLong(screenshotFolderEditHwnd, GWL_EXSTYLE);
        ::SetWindowLong(screenshotFolderEditHwnd, GWL_EXSTYLE, styleEx & ~WS_EX_RTLREADING);
    }

    filepathTemplateEdit_.Attach(GetDlgItem(IDC_SCREENSHOTFILENAMEEDIT));
    filepathMacrosButton_.Attach(GetDlgItem(IDC_SCREENSHOTMACROSBUTTON));
    CString tooltipText = TR("Macros list");
    CToolInfo tip(TTF_SUBCLASS, filepathMacrosButton_, 0, nullptr, const_cast<LPTSTR>(tooltipText.GetString()));
    tooltipControl_.AddTool(tip);

    createResources();

    SetDlgItemText(IDC_SCREENSHOTFILENAMEEDIT, Settings.ScreenshotSettings.FilenameTemplate);

    SetDlgItemText(IDC_SCREENSHOTFOLDEREDIT, Settings.ScreenshotSettings.Folder);
    SendDlgItemMessage(IDC_DELAYSPIN, UDM_SETRANGE, 0, (LPARAM) MAKELONG((short)30, (short)0) );
    SendDlgItemMessage(IDC_QUALITYSPIN, UDM_SETRANGE, 0, (LPARAM) MAKELONG((short)100, (short)1) );

    SendDlgItemMessage(IDC_FORMATLIST, CB_ADDSTRING, 0, (LPARAM)_T("JPEG"));
    SendDlgItemMessage(IDC_FORMATLIST, CB_ADDSTRING, 0, (LPARAM)_T("PNG"));
    SendDlgItemMessage(IDC_FORMATLIST, CB_ADDSTRING, 0, (LPARAM)_T("GIF"));
    SendDlgItemMessage(IDC_FORMATLIST, CB_ADDSTRING, 0, (LPARAM)_T("WebP"));
    SendDlgItemMessage(IDC_FORMATLIST, CB_ADDSTRING, 0, (LPARAM)_T("WebP (lossless)"));
    SendDlgItemMessage(IDC_FOREGROUNDWHENSHOOTING, BM_SETCHECK,Settings.ScreenshotSettings.ShowForeground);
    SendDlgItemMessage(IDC_ALWAYSCOPYTOCLIPBOARD, BM_SETCHECK, Settings.ScreenshotSettings.CopyToClipboard);

    SendDlgItemMessage(IDC_REMOVECORNERS, BM_SETCHECK, Settings.ScreenshotSettings.RemoveCorners);
    SendDlgItemMessage(IDC_ADDSHADOW, BM_SETCHECK, Settings.ScreenshotSettings.AddShadow);
    SendDlgItemMessage(IDC_REMOVEBACKGROUND, BM_SETCHECK, Settings.ScreenshotSettings.RemoveBackground);

    GuiTools::SetCheck(m_hWnd, IDC_USEOLDREGIONSCREENSHOTMETHOD, Settings.ScreenshotSettings.UseOldRegionScreenshotMethod);
    GuiTools::SetCheck(m_hWnd, IDC_CAPTURECURSORCHECKBOX2, Settings.ScreenshotSettings.CaptureCursor);

    int Quality = Settings.ScreenshotSettings.Quality;
    int Format = Settings.ScreenshotSettings.Format;
    int Delay = Settings.ScreenshotSettings.Delay;

    if( Format < 0) Format = 0;
    if( Quality < 0) Quality = 85;
    if( Delay < 0 || Delay > 30) Delay = 2;

    SetDlgItemInt(IDC_QUALITYEDIT, Quality);
    SetDlgItemInt(IDC_DELAYEDIT, Delay);
    SetDlgItemInt(IDC_WINDOWHIDINGDELAY, Settings.ScreenshotSettings.WindowHidingDelay);
    SendDlgItemMessage(IDC_FORMATLIST, CB_SETCURSEL, Format, 0);

    bool isVista = IsWindowsVistaOrGreater();
    ::EnableWindow(GetDlgItem(IDC_AEROONLY), isVista);
    ::EnableWindow(GetDlgItem(IDC_REMOVECORNERS), isVista);
    ::EnableWindow(GetDlgItem(IDC_ADDSHADOW), isVista);
    ::EnableWindow(GetDlgItem(IDC_REMOVEBACKGROUND), isVista);
    return 1;  // Let the system set the focus
}

bool CScreenshotSettingsPage::apply()
{
    const CWindow fileNameTemplateEdit = GetDlgItem(IDC_SCREENSHOTFILENAMEEDIT);
    const CString fileNameTemplate = GuiTools::GetWindowText(fileNameTemplateEdit);

    if (fileNameTemplate.IsEmpty()) {
        throw ValidationException(TR("The filename template cannot be empty!"), fileNameTemplateEdit);
    }

    if (fileNameTemplate.FindOneOf(FORBIDDEN_FILEPATH_TEMPLATE_CHARACTERS) != -1) {
        throw ValidationException(TR("The filename template contains forbidden characters!"), fileNameTemplateEdit);
    }

    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    Settings.ScreenshotSettings.FilenameTemplate = fileNameTemplate;

    Settings.ScreenshotSettings.Format = SendDlgItemMessage(IDC_FORMATLIST,CB_GETCURSEL,0,0);
    Settings.ScreenshotSettings.Quality = GetDlgItemInt(IDC_QUALITYEDIT);
    Settings.ScreenshotSettings.Delay = GetDlgItemInt(IDC_DELAYEDIT);
    Settings.ScreenshotSettings.ShowForeground = SendDlgItemMessage(IDC_FOREGROUNDWHENSHOOTING, BM_GETCHECK) == BST_CHECKED;

    Settings.ScreenshotSettings.Folder = GuiTools::GetWindowText(GetDlgItem(IDC_SCREENSHOTFOLDEREDIT));
    Settings.ScreenshotSettings.Format = SendDlgItemMessage(IDC_FORMATLIST,CB_GETCURSEL,0,0);
    Settings.ScreenshotSettings.Quality = GetDlgItemInt(IDC_QUALITYEDIT);
    Settings.ScreenshotSettings.Delay = GetDlgItemInt(IDC_DELAYEDIT);
    Settings.ScreenshotSettings.ShowForeground = SendDlgItemMessage(IDC_FOREGROUNDWHENSHOOTING, BM_GETCHECK) == BST_CHECKED;
    Settings.ScreenshotSettings.CopyToClipboard =  SendDlgItemMessage(IDC_ALWAYSCOPYTOCLIPBOARD, BM_GETCHECK) == BST_CHECKED;

    Settings.ScreenshotSettings.WindowHidingDelay = GetDlgItemInt(IDC_WINDOWHIDINGDELAY);

    Settings.ScreenshotSettings.RemoveCorners = SendDlgItemMessage(IDC_REMOVECORNERS, BM_GETCHECK)!=0;
    Settings.ScreenshotSettings.AddShadow = SendDlgItemMessage(IDC_ADDSHADOW, BM_GETCHECK)!=0;
    Settings.ScreenshotSettings.RemoveBackground = SendDlgItemMessage(IDC_REMOVEBACKGROUND, BM_GETCHECK)!=0;
    Settings.ScreenshotSettings.UseOldRegionScreenshotMethod = GuiTools::GetCheck(m_hWnd, IDC_USEOLDREGIONSCREENSHOTMETHOD);
    Settings.ScreenshotSettings.CaptureCursor = GuiTools::GetCheck(m_hWnd, IDC_CAPTURECURSORCHECKBOX2);

    return true;
}

LRESULT CScreenshotSettingsPage::OnScreenshotsFolderSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CString path = GuiTools::GetWindowText(GetDlgItem(IDC_SCREENSHOTFOLDEREDIT));

    CNewStyleFolderDialog fd(m_hWnd, path, TR("Select folder") );

    fd.SetFolder(path);

    if (fd.DoModal(m_hWnd) == IDOK)
    {
        SetDlgItemText(IDC_SCREENSHOTFOLDEREDIT, fd.GetFolderPath());
        return true;
    }

    return 0;
}

LRESULT CScreenshotSettingsPage::OnMacrosButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    const std::vector<std::pair<CString, CString>> items{
        {_T("%y"), TR("year")},
        {_T("%m"), TR("month")},
        {_T("%d"), TR("day")},
        {_T("%h"), TR("hour")},
        {_T("%n"), TR("minute")},
        {_T("%s"), TR("second")},
        {_T("%i"), TR("index")},
        {_T("%width%"), TR("image width")},
        {_T("%height%"), TR("image height")},
    };

    RECT rc {};
    ::GetWindowRect(hWndCtl, &rc);
    POINT menuOrigin { rc.left, rc.bottom };

    CMenu macrosMenu;

    int id = 1;
    macrosMenu.CreatePopupMenu();

    for (const auto& item : items) {
        CString title = item.first + _T(" - ") + item.second;
        macrosMenu.AppendMenu(MF_STRING, id++, title);
    }

    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;

    int result = macrosMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, menuOrigin.x, menuOrigin.y, m_hWnd, &excludeArea);
    if (result && (result - 1 < items.size())) {
        filepathTemplateEdit_.ReplaceSel(items[result - 1].first, TRUE);
    }

    return 0;
}

void CScreenshotSettingsPage::createResources() {
    const UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    if (iconInfo_) {
        iconInfo_.DestroyIcon();
    }

    if (SUCCEEDED(iconInfo_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONINFO), iconWidth, iconHeight))) {
        filepathMacrosButton_.SetIcon(iconInfo_);
    }
}

LRESULT CScreenshotSettingsPage::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createResources();
    return 0;
}
