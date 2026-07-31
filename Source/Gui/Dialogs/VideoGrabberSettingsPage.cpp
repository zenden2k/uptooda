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

#include "VideoGrabberSettingsPage.h"

#include "Gui/GuiTools.h"
#include "Gui/Components/NewStyleFolderDialog.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/Constants.h"
#include "Gui/Helpers/DPIHelper.h"

// CVideoGrabberParams
CVideoGrabberSettingsPage::CVideoGrabberSettingsPage() : m_Font()
{
    memset(&m_Font, 0, sizeof(m_Font));
}

LRESULT CVideoGrabberSettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();

    SetDlgItemInt(IDC_COLUMNSEDIT, Settings.VideoSettings.Columns);
    SetDlgItemInt(IDC_TILEWIDTH, Settings.VideoSettings.TileWidth);
    SetDlgItemInt(IDC_GAPWIDTH, Settings.VideoSettings.GapWidth);
    SetDlgItemInt(IDC_GAPHEIGHT, Settings.VideoSettings.GapHeight);
    WinUtils::StringToFont(Settings.VideoSettings.Font, &m_Font);
    SendDlgItemMessage(IDC_MEDIAINFOONIMAGE, BM_SETCHECK, Settings.VideoSettings.ShowMediaInfo);

    SetWindowText(TR("Appearance options"));
    TRC(IDC_COLUMNSEDITLABEL, "Number of columns:");
    TRC(IDC_PREVIEWWIDTHLABEL, "Thumbnail width:");
    TRC(IDC_INTERVALHORLABEL, "Horizontal interval:");
    TRC(IDC_INTERVALVERTLABEL, "Vertical interval:");
    TRC(IDC_APPEARANCEGROUP, "Frames layout");
    TRC(IDC_MEDIAINFOONIMAGE, "Display video file information on resulting picture");
    TRC(IDC_MEDIAINFOFONT, "Font...");
    TRC(IDC_TEXTCOLORLABEL, "Text color:");
    TRC(IDC_SNAPSHOTFILENAMELABEL, "Filename and path template:");

    TRC(IDC_VIDEOSNAPSHOTSFOLDERLABEL, "Folder for snapshots:");
    TRC(IDC_VIDEOSNAPSHOTSFOLDERBUTTON, "Select...");

    tooltipControl_.Create(m_hWnd);

    filepathTemplateEdit_.Attach(GetDlgItem(IDC_SNAPSHOTFILENAMEEDIT));
    filepathMacrosButton_.Attach(GetDlgItem(IDC_VIDEOGRABBERMACROSBUTTON));
    CString tooltipText = TR("Macros list");
    CToolInfo tip(TTF_SUBCLASS, filepathMacrosButton_, 0, nullptr, const_cast<LPTSTR>(tooltipText.GetString()));
    tooltipControl_.AddTool(tip);

    createResources();

    if (ServiceLocator::instance()->translator()->isRTL()) {
        // Removing WS_EX_RTLREADING style from some controls to look properly when RTL interface language is choosen
        HWND snapshotsFolderEditHwnd = GetDlgItem(IDC_VIDEOSNAPSHOTSFOLDEREDIT);
        LONG styleEx = ::GetWindowLong(snapshotsFolderEditHwnd, GWL_EXSTYLE);
        ::SetWindowLong(snapshotsFolderEditHwnd, GWL_EXSTYLE, styleEx & ~WS_EX_RTLREADING);

        GuiTools::RemoveWindowStyleEx(GetDlgItem(IDC_SNAPSHOTFILENAMEEDIT), WS_EX_RTLREADING);
    }

    textColorButton_.SubclassWindow(GetDlgItem(IDC_TEXTCOLOR));
    textColorButton_.SetColor(Settings.VideoSettings.TextColor);
    SetDlgItemText(IDC_VIDEOSNAPSHOTSFOLDEREDIT, Settings.VideoSettings.SnapshotsFolder);
    SetDlgItemText(IDC_SNAPSHOTFILENAMEEDIT, Settings.VideoSettings.SnapshotFileTemplate);

    BOOL b;
    OnShowMediaInfoTextBnClicked(IDC_MEDIAINFOONIMAGE, 0, 0, b);
    return 1;  // Let the system set the focus
}

bool CVideoGrabberSettingsPage::apply()
{
    const int columns = GetDlgItemInt(IDC_COLUMNSEDIT);
    if (columns <= 0) {
        CString fieldName = GuiTools::GetDlgItemText(m_hWnd, IDC_COLUMNSEDITLABEL);
        CString message;
        message.Format(TR("Error in the field '%s': value should be greater than zero"), fieldName.GetString());
        throw ValidationException(message, GetDlgItem(IDC_COLUMNSEDIT));
    }

    const CWindow fileNameTemplateEdit = GetDlgItem(IDC_SNAPSHOTFILENAMEEDIT);
    const CString fileNameTemplate = GuiTools::GetWindowText(fileNameTemplateEdit);

    if (fileNameTemplate.IsEmpty()) {
        throw ValidationException(TR("The filename template cannot be empty!"), fileNameTemplateEdit);
    }

    if (fileNameTemplate.FindOneOf(FORBIDDEN_FILEPATH_TEMPLATE_CHARACTERS) != -1) {
        throw ValidationException(TR("The filename template contains forbidden characters!"), fileNameTemplateEdit);
    }

    checkBounds(IDC_TILEWIDTH, 10, 1024, IDC_PREVIEWWIDTHLABEL);
    checkBounds(IDC_GAPWIDTH, 0, 200, IDC_INTERVALHORLABEL);
    checkBounds(IDC_GAPHEIGHT, 0, 200, IDC_INTERVALVERTLABEL);
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    Settings.VideoSettings.Columns = columns;
    Settings.VideoSettings.TileWidth = GetDlgItemInt(IDC_TILEWIDTH);
    Settings.VideoSettings.GapWidth = GetDlgItemInt(IDC_GAPWIDTH);
    Settings.VideoSettings.GapHeight = GetDlgItemInt(IDC_GAPHEIGHT);
    Settings.VideoSettings.ShowMediaInfo = SendDlgItemMessage(IDC_MEDIAINFOONIMAGE, BM_GETCHECK);
    WinUtils::FontToString(&m_Font, Settings.VideoSettings.Font);
    Settings.VideoSettings.SnapshotsFolder = GuiTools::GetDlgItemText(m_hWnd, IDC_VIDEOSNAPSHOTSFOLDEREDIT);
    Settings.VideoSettings.SnapshotFileTemplate = GuiTools::GetDlgItemText(m_hWnd, IDC_SNAPSHOTFILENAMEEDIT);

    Settings.VideoSettings.TextColor = textColorButton_.GetColor();

    return true;
}

LRESULT CVideoGrabberSettingsPage::OnMediaInfoFontClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    CFontDialog dlg(&m_Font);
    dlg.DoModal(m_hWnd);
    return 0;
}

LRESULT CVideoGrabberSettingsPage::OnShowMediaInfoTextBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bool bChecked = SendDlgItemMessage(wID, BM_GETCHECK) == BST_CHECKED;
    GuiTools::EnableNextN(GetDlgItem(wID), 3, bChecked);
    return 0;
}

LRESULT CVideoGrabberSettingsPage::OnVideoSnapshotsFolderButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    CString path = GuiTools::GetWindowText(GetDlgItem(IDC_VIDEOSNAPSHOTSFOLDEREDIT));

    CNewStyleFolderDialog fd(m_hWnd, path, TR("Select folder"));

    fd.SetFolder(path);

    if (fd.DoModal(m_hWnd) == IDOK) {
        SetDlgItemText(IDC_VIDEOSNAPSHOTSFOLDEREDIT,fd.GetFolderPath());
        return true;
    }
    return 0;
}

LRESULT CVideoGrabberSettingsPage::OnMacrosButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    const std::vector<std::pair<CString, CString>> items{
        {_T("%f%"),   TR("video file name without extension")},
        {_T("%fe%"),  TR("video file name")},
        {_T("%ext%"), TR("video file extension")},
        {_T("%y%"),   TR("year")},
        {_T("%m%"),   TR("month")},
        {_T("%d%"),   TR("day")},
        {_T("%h%"),   TR("hour")},
        {_T("%n%"),   TR("minute")},
        {_T("%s%"),   TR("second")},
        {_T("%i%"),   TR("index")},
        {_T("%cx%"),  TR("video width")},
        {_T("%cy%"),  TR("video height")},
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

void CVideoGrabberSettingsPage::createResources() {
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

LRESULT CVideoGrabberSettingsPage::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createResources();
    return 0;
}
