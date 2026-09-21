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

#include "LogoSettings.h"

#include "Gui/GuiTools.h"
#include "InputDialog.h"
#include "Func/WinUtils.h"
#include "Gui/Components/MyFileDialog.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/Helpers/DPIHelper.h"

CLogoSettings::CLogoSettings()
{
    ZeroMemory(&lf, sizeof(lf));
    memset(&ThumbFont, 0, sizeof(ThumbFont));
    m_CatchChanges = false;
    m_ProfileChanged = false;
}

void CLogoSettings::TranslateUI()
{
    TRC(IDC_CURRENTPROFILELABEL, "Profile:");
    TRC(IDC_FORMATLABEL, "Format:");
    TRC(IDC_QUALITYLABEL, "Quality:");
    TRC(IDC_RESIZEBYWIDTH, "Change width:");
    TRC(IDC_YOURLOGO, "Add watermark");
    TRC(IDC_SMARTCONVERTING, "Skip image processing step if the image has right parameters");
    TRC(IDC_YOURTEXT, "Add text to image");
    TRC(IDC_XLABEL, "and/or height:");
    TRC(IDC_RESIZEMODELABEL, "Resize mode:");
    TRC(IDC_FILENAMELABEL, "Filename:");
    TRC(IDC_LOGOPOSITIONLABEL, "Watermark position:");
    TRC(IDC_LOGOGROUP, "Watermark");
    TRC(IDC_TEXTONIMAGEGROUP, "Text on image");
    TRC(IDC_ENTERYOURTEXTLABEL, "Enter your text:");
    TRC(IDC_TEXTCOLORLABEL, "Text color:");
    TRC(IDC_TEXTSTROKECOLOR, "Stroke color:");
    TRC(IDC_SELECTFONT, "Font...");
    TRC(IDC_TEXTPOSITIONLABEL, "Text position:");
    TRC(IDC_PRESERVE_EXIF, "Preserve metadata (for ex. EXIF)");
    TRC(IDC_SKIPANIMATEDCHECKBOX, "Skip animated");
    SetWindowText(TR("Additional params"));
}

LRESULT CLogoSettings::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    convert_profiles_ = settings->ConvertProfiles;
    // Translating controls
    TranslateUI();
    RECT rc = {13, 20, 290, 95};
    img.Create(GetDlgItem(IDC_LOGOGROUP), rc);
    img.ShowWindow(SW_HIDE);
    img.loadImage(nullptr);

    profileCombobox_ = GetDlgItem(IDC_PROFILECOMBO);

    GuiTools::AddComboBoxItems(m_hWnd, IDC_RESIZEMODECOMBO, 3, TR("Fit"), TR("Center"), TR("Stretch"));
    // Items order should be the same as ImageUtils::SaveImageFormat
    GuiTools::AddComboBoxItems(m_hWnd, IDC_FORMATLIST, 6, TR("Auto"), _T("JPEG"), _T("PNG"),_T("GIF"), _T("WebP"), _T("WebP (lossless)"));

    SendDlgItemMessage(IDC_TRANSPIN, UDM_SETRANGE, 0, (LPARAM) MAKELONG((short)100, (short)0));
    SendDlgItemMessage(IDC_QUALITYSPIN,UDM_SETRANGE, 0, (LPARAM) MAKELONG((short)100, (short)1));

    SendDlgItemMessage(IDC_LOGOPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Top left corner"));
    SendDlgItemMessage(IDC_LOGOPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Top center"));
    SendDlgItemMessage(IDC_LOGOPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Top right corner"));
    SendDlgItemMessage(IDC_LOGOPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Bottom left corner"));
    SendDlgItemMessage(IDC_LOGOPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Bottom center"));
    SendDlgItemMessage(IDC_LOGOPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Bottom right corner"));

    SendDlgItemMessage(IDC_TEXTPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Top left corner"));
    SendDlgItemMessage(IDC_TEXTPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Top center"));
    SendDlgItemMessage(IDC_TEXTPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Top right corner"));
    SendDlgItemMessage(IDC_TEXTPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Bottom left corner"));
    SendDlgItemMessage(IDC_TEXTPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Bottom center"));
    SendDlgItemMessage(IDC_TEXTPOSITION, CB_ADDSTRING, 0, (LPARAM)TR("Bottom right corner"));

    TextColor.SubclassWindow(GetDlgItem(IDC_SELECTCOLOR));
    StrokeColor.SubclassWindow(GetDlgItem(IDC_STROKECOLOR));

    createProfileToolbar();
    CString profileName = U2W(settings->DefaultImageUploadParams.ImageProfileName);

    if (convert_profiles_.find(profileName) == convert_profiles_.end()) {
        profileName = _T("Default");
    }
    ShowParams(profileName);
    UpdateProfileList();
    return 1;
}

LRESULT CLogoSettings::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createProfileToolbar();
    return 0;
}

LRESULT CLogoSettings::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CLogoSettings::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CLogoSettings::OnBnClickedLogobrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    IMyFileDialog::FileFilterArray filters = {
        { TR("Images"), _T("*.jpg;*.gif;*.png;*.bmp;*.tiff"), },
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

    SetDlgItemText(IDC_LOGOEDIT, fileName);
    img.loadImage(fileName);
    img.Invalidate();
    ProfileChanged();
    return 0;
}

LRESULT CLogoSettings::OnBnClickedSelectfont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CFontDialog dlg(&lf);
    if(dlg.DoModal(m_hWnd) == IDOK)
       ProfileChanged();
    return 0;
}

LRESULT CLogoSettings::OnBnClickedThumbfont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CFontDialog dlg(&ThumbFont);
    dlg.DoModal(m_hWnd);

    return 0;
}

bool CLogoSettings::apply()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    CString saveToProfile = CurrentProfileName;
    if (CurrentProfileOriginalName == _T("Default"))
        saveToProfile = CurrentProfileOriginalName;

    if (!SaveParams(convert_profiles_[saveToProfile]))
        return false;
    settings->ConvertProfiles = convert_profiles_;
    //Settings.CurrentConvertProfileName  = saveToProfile;
    return TRUE;
}

LRESULT CLogoSettings::OnYourLogoCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    bool bChecked = SendDlgItemMessage(IDC_YOURLOGO, BM_GETCHECK) == BST_CHECKED;
    GuiTools::EnableNextN(hWndCtl, 5, bChecked);
    ProfileChanged();
    return 0;
}

LRESULT CLogoSettings::OnAddTextChecboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    bool bChecked = SendDlgItemMessage(IDC_YOURTEXT, BM_GETCHECK) == BST_CHECKED;
    GuiTools::EnableNextN(hWndCtl, 9, bChecked);
    ProfileChanged();
    return 0;
}

void CLogoSettings::ShowParams(const ImageConvertingParams& params) {
    m_ProfileChanged = false;
    m_CatchChanges = false;
    SetDlgItemText(IDC_LOGOEDIT, U2W(params.LogoFileName));

    if (!params.LogoFileName.empty() && WinUtils::FileExists(U2W(params.LogoFileName)))
        img.loadImage(U2W(params.LogoFileName));

    SetDlgItemText(IDC_EDITYOURTEXT,U2W(params.Text));
    SendDlgItemMessage(IDC_LOGOPOSITION, CB_SETCURSEL, params.LogoPosition);
    SendDlgItemMessage(IDC_TEXTPOSITION, CB_SETCURSEL, params.TextPosition);
    TextColor.SetColor(params.TextColor);
    StrokeColor.SetColor(params.StrokeColor);
    WinUtils::StringToFont(U2W(params.Font), &lf);
    if (params.Quality)
        SetDlgItemInt(IDC_QUALITYEDIT, params.Quality);
    else
        SetDlgItemText(IDC_QUALITYEDIT,_T(""));
    SendDlgItemMessage(IDC_FORMATLIST,CB_SETCURSEL, params.Format);

    SendDlgItemMessage(IDC_RESIZEMODECOMBO,CB_SETCURSEL, static_cast<WPARAM>(params.ResizeMode.toInt()));

    SendDlgItemMessage(IDC_YOURLOGO,BM_SETCHECK, params.AddLogo);
    SendDlgItemMessage(IDC_YOURTEXT,BM_SETCHECK, params.AddText);

    GuiTools::SetCheck(m_hWnd, IDC_SMARTCONVERTING, params.SmartConverting);
    GuiTools::SetCheck(m_hWnd, IDC_SKIPANIMATEDCHECKBOX, params.SkipAnimated);
    SetDlgItemText(IDC_IMAGEWIDTH,U2W(params.strNewWidth));

    SetDlgItemText(IDC_IMAGEHEIGHT,U2W(params.strNewHeight));

    OnYourLogoCheckboxClicked(0, 0, GetDlgItem(IDC_YOURLOGO));
    OnAddTextChecboxClicked(0, 0, GetDlgItem(IDC_YOURTEXT));
    GuiTools::SetCheck(m_hWnd, IDC_PRESERVE_EXIF, params.PreserveExifInformation);
    m_CatchChanges = true;
}

bool CLogoSettings::SaveParams(ImageConvertingParams& params)
{
    bool addLogo = SendDlgItemMessage(IDC_YOURLOGO,BM_GETCHECK) == BST_CHECKED;
    bool addText = SendDlgItemMessage(IDC_YOURTEXT,BM_GETCHECK) == BST_CHECKED;
    int LogoPos = SendDlgItemMessage(IDC_LOGOPOSITION, CB_GETCURSEL);
    int TextPos = SendDlgItemMessage(IDC_TEXTPOSITION, CB_GETCURSEL);

    if(LogoPos == TextPos && addLogo && addText) {
        if(MessageBox(TR("Are you sure to place text and logo in the same position on image?"),TR("Image settings"),MB_ICONQUESTION|MB_YESNO)!=IDYES)
            return false;
    }

    params.LogoPosition = LogoPos;
    params.TextPosition = TextPos;
    params.LogoFileName = W2U(GuiTools::GetWindowText(GetDlgItem(IDC_LOGOEDIT)));
    params.Text = W2U(GuiTools::GetWindowText(GetDlgItem(IDC_EDITYOURTEXT)));
    CString fontString;
    WinUtils::FontToString(&lf, fontString);
    params.Font = W2U(fontString);
    params.AddLogo = addLogo;
    params.AddText = addText;

    GuiTools::GetCheck(m_hWnd, IDC_SMARTCONVERTING, params.SmartConverting);
    GuiTools::GetCheck(m_hWnd, IDC_SKIPANIMATEDCHECKBOX, params.SkipAnimated);
    params.TextColor = TextColor.GetColor();
    params.StrokeColor = StrokeColor.GetColor();
    params.Quality = GetDlgItemInt(IDC_QUALITYEDIT);
    params.Format = SendDlgItemMessage(IDC_FORMATLIST, CB_GETCURSEL);
    params.ResizeMode = static_cast<ImageConvertingParams::ImageResizeMode>(SendDlgItemMessage(IDC_RESIZEMODECOMBO,CB_GETCURSEL));
    params.strNewWidth = W2U(GuiTools::GetWindowText( GetDlgItem(IDC_IMAGEWIDTH)));
    params.strNewHeight = W2U(GuiTools::GetWindowText( GetDlgItem(IDC_IMAGEHEIGHT)));
    GuiTools::GetCheck(m_hWnd, IDC_PRESERVE_EXIF, params.PreserveExifInformation);
    return true;
}

void CLogoSettings::UpdateProfileList() {
    profileCombobox_.ResetContent();

    bool found = false;
    for (const auto& it: convert_profiles_) {
        profileCombobox_.AddString(it.first);
        if (it.first == CurrentProfileName) {
            found = true;
        }
    }
    if (!found) {
        profileCombobox_.AddString(CurrentProfileName);
    }

    profileCombobox_.SelectString(-1, CurrentProfileName);
}

LRESULT CLogoSettings::OnSaveProfile(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    CInputDialog dlg(TR("New Profile Name"), TR("Enter new profile name:"), CurrentProfileOriginalName);
    if(dlg.DoModal(m_hWnd)==IDOK)
    {
         ImageConvertingParams params;
         SaveParams(params);
         convert_profiles_[dlg.getValue()] = params;
         CurrentProfileName = dlg.getValue();
         CurrentProfileOriginalName = dlg.getValue();
         m_ProfileChanged = false;
         UpdateProfileList();
    }
    return 0;
}

LRESULT CLogoSettings::OnProfileComboSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if(m_ProfileChanged)
    {
        if (GuiTools::LocalizedMessageBox(m_hWnd, TR("Current profile's changes weren't saved. Do you want to continue?"), APP_NAME, MB_YESNO | MB_ICONWARNING) != IDYES)
        {
            SendDlgItemMessage(IDC_PROFILECOMBO, CB_SELECTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(CurrentProfileName.GetString()));
            return 0;
        }
    }
    CString profile = GuiTools::GetWindowText(GetDlgItem(IDC_PROFILECOMBO));
    ShowParams(profile);
    UpdateProfileList();
    return 0;
}

void CLogoSettings::ShowParams(const CString& profileName)
{
    if (CurrentProfileName == profileName) {
        return;
    }

    CurrentProfileName = profileName;
    CurrentProfileOriginalName = profileName;
    ShowParams(convert_profiles_[profileName]);
    SendDlgItemMessage(IDC_PROFILECOMBO, CB_SELECTSTRING, static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(profileName.GetString()));
}

LRESULT CLogoSettings::OnProfileEditedCommand(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ProfileChanged();
    return 0;
}

void CLogoSettings::ProfileChanged()
{
    if(!m_CatchChanges) return;

    if(!m_ProfileChanged)
    {
        CurrentProfileOriginalName = CurrentProfileName;
        CurrentProfileName.Replace(CString(_T(" "))+TR("(edited)"), _T(""));
        CurrentProfileName = CurrentProfileName + _T(" ")+ TR("(edited)");
        m_ProfileChanged = true;
        UpdateProfileList();
    }
}

void CLogoSettings::createProfileToolbar() {
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    RECT profileRect;
    ::GetWindowRect(GetDlgItem(IDC_PROFILETOOBLARPLACEBUTTON), &profileRect);
    ScreenToClient(&profileRect);

    if (!m_ProfileEditToolbar) {
        m_ProfileEditToolbar.Create(m_hWnd, profileRect, _T(""), WS_CHILD | WS_VISIBLE | WS_CHILD | WS_TABSTOP | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NORESIZE | /*CCS_BOTTOM |CCS_ADJUSTABLE|*/ CCS_NODIVIDER | TBSTYLE_AUTOSIZE);
        // Put the toolbar after placeholder
        m_ProfileEditToolbar.SetWindowPos(GetDlgItem(IDC_PROFILETOOBLARPLACEBUTTON), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

        m_ProfileEditToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
        m_ProfileEditToolbar.SetButtonStructSize();
    }
    GuiTools::DeleteAllToolbarButtons(m_ProfileEditToolbar);

    CIcon ico;
    ico.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONWHITEPAGE), iconWidth, iconHeight);

    CIcon saveIcon;
    saveIcon.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONSAVE), iconWidth, iconHeight);
    CIcon deleteIcon;
    deleteIcon.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONDELETEBIG), iconWidth, iconHeight);

    if (profileEditToolbarImagelist_) {
        m_ProfileEditToolbar.SetImageList(nullptr);
        profileEditToolbarImagelist_.Destroy();
    }
    profileEditToolbarImagelist_.Create(iconWidth, iconHeight, ILC_COLOR32, 0, 6);
    profileEditToolbarImagelist_.AddIcon(ico);
    profileEditToolbarImagelist_.AddIcon(saveIcon);
    profileEditToolbarImagelist_.AddIcon(deleteIcon);
    m_ProfileEditToolbar.SetImageList(profileEditToolbarImagelist_);
    m_ProfileEditToolbar.AddButton(IDC_NEWPROFILE, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 0, TR("Create Profile"), 0);
    m_ProfileEditToolbar.AddButton(IDC_SAVEPROFILE, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 1, TR("Save Profile"), 0);
    m_ProfileEditToolbar.AddButton(IDC_DELETEPROFILE, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 2, TR("Delete Profile"), 0);
}

LRESULT CLogoSettings::OnProfileEditedNotification(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
   ProfileChanged();
   return 0;
}

LRESULT CLogoSettings::OnNewProfile(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    CString name = TR("New profile");
    CString generatedName = name;
    int i = 1;
    while(convert_profiles_.count(generatedName) > 0)
    {
        generatedName = name  + _T(" ") + WinUtils::IntToStr(i++);
    }

    //CurrentProfileName = CurrentProfileOriginalName = generatedName;
    m_ProfileChanged = true;
    ShowParams(generatedName);
    UpdateProfileList();
    return 0;
}

LRESULT CLogoSettings::OnDeleteProfile(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    CString question;
    question.Format(TR("Are you sure you want to delete profile '%s'?"), CurrentProfileName.GetString());
    if (GuiTools::LocalizedMessageBox(m_hWnd, question, TR("Image settings"), MB_ICONQUESTION | MB_YESNO) != IDYES)
        return 0;
    if(CurrentProfileName=="Default") return 0;
    if(convert_profiles_.count(CurrentProfileName)>0)
        convert_profiles_.erase(CurrentProfileName);

    ShowParams("Default");
    UpdateProfileList();
    return 0;
}
