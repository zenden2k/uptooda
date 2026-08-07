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
#include "UploadSettings.h"

#include <boost/format.hpp>

#include "atlheaders.h"
#include "ServerFolderSelect.h"
#include "NewFolderDlg.h"
#include "ServerParamsDlg.h"
#include "Gui/GuiTools.h"
#include "Func/MyEngineList.h"
#include "Gui/Dialogs/SettingsDlg.h"
#include "Gui/IconBitmapUtils.h"
#include "Func/WinUtils.h"
#include "AddFtpServerDialog.h"
#include "AddDirectoryServerDialog.h"
#include "Gui/Controls/ServerSelectorControl.h"
#include "Gui/Dialogs/WizardDlg.h"
#include "LoginDlg.h"
#include "ServerProfileGroupSelectDialog.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/WinServerIconCache.h"
#include "Func/IuCommonFunctions.h"
#include "StatusDlg.h"
#include "Core/FileTypeCheckTask.h"
#include "Gui/Dialogs/FileFormatCheckErrorDlg.h"
#include "Gui/Helpers/DPIHelper.h"
#include "Gui/Dialogs/ServerListPopup.h"

namespace {
    struct ResizePreset {
        std::wstring width;
        std::wstring height;
        std::wstring title;
        int menuGroup;
    };

    // Maximum number of presets is limited to 100 items
    // (IDC_RESIZEPRESETMENU_LAST_ID - IDC_RESIZEPRESETMENU_FIRST_ID)
    ResizePreset resizePresets[] = {
        {L"",     L"",     L"", 0}, // No resize at all
        {L"800",  L"600",  L"", 1}, // If title is empty, it is being formatted like "WxH"
        {L"1024", L"768",  L"", 1},
        {L"1600", L"1200", L"", 1},
        {L"1280",  L"720",  L"", 2},
        {L"1600", L"900",  L"", 2},
        {L"1920", L"1080", L"", 2},
        {L"25%",  L"25%",  L"25%", 3},
        {L"50%",  L"50%",  L"50%", 3},
        {L"75%",  L"75%",  L"75%", 3}
    };

    void CenterToolbarInGroupBoxThemed(HWND hDialog, HWND hGroupBox, HWND hToolbar) {
        int dpi = DPIHelper::GetDpiForWindow(hDialog);

        RECT rcGroup;
        GetWindowRect(hGroupBox, &rcGroup);
        ScreenToClient(hDialog, (LPPOINT)&rcGroup.left);
        ScreenToClient(hDialog, (LPPOINT)&rcGroup.right);

        RECT rcToolbar;
        GetWindowRect(hToolbar, &rcToolbar);
        int toolbarWidth = rcToolbar.right - rcToolbar.left;
        int toolbarHeight = rcToolbar.bottom - rcToolbar.top;

        HDC hdc = GetDC(hGroupBox);
        HFONT hFont = (HFONT)SendMessage(hGroupBox, WM_GETFONT, 0, 0);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        int captionHeight = tm.tmHeight + MulDiv(2, dpi, USER_DEFAULT_SCREEN_DPI); // +4 для отступов

        SelectObject(hdc, hOldFont);
        ReleaseDC(hGroupBox, hdc);

        int borderWidth = MulDiv(2, dpi, USER_DEFAULT_SCREEN_DPI);

        int contentLeft = rcGroup.left + borderWidth;
        int contentTop = rcGroup.top + captionHeight;
        int contentWidth = (rcGroup.right - rcGroup.left) - (borderWidth * 2);
        int contentHeight = (rcGroup.bottom - rcGroup.top) - captionHeight - borderWidth;

        int x = contentLeft + (contentWidth - toolbarWidth) / 2;
        int y = contentTop + (contentHeight - toolbarHeight) / 2;

        SetWindowPos(hToolbar, NULL, x, y, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    }

CUploadSettings::CUploadSettings(CMyEngineList* EngineList, UploadEngineManager* uploadEngineManager, WinServerIconCache* iconCache)
    :convert_profiles_(ServiceLocator::instance()->settings<WtlGuiSettings>()->ConvertProfiles),
    iconCache_(iconCache)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    nImageIndex = nFileIndex = -1;
    m_EngineList = EngineList;
    m_ProfileChanged  = false;
    m_CatchChanges = false;
    iconBitmapUtils_ = std::make_unique<IconBitmapUtils>();
    useServerThumbnailsTooltip_ = nullptr;
    uploadEngineManager_ = uploadEngineManager;
    using namespace std::placeholders;
    settingsChangedConnection_ = settings->onChange.connect(std::bind(&CUploadSettings::settingsChanged, this, _1));
}

CUploadSettings::~CUploadSettings() {
    try {
        settingsChangedConnection_.disconnect();
    } catch ( const std::exception& ex) {
        LOG(ERROR) << ex.what();
    }
}

void CUploadSettings::settingsChanged(BasicSettings* settingsBase)
{
    //auto* settings = dynamic_cast<CommonGuiSettings*>(settingsBase);
}

void CUploadSettings::updateButtonIcons() {
    const UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);

    int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    if (iconDropdown_) {
        iconDropdown_.DestroyIcon();
    }
    iconDropdown_ = GuiTools::CreateDropDownArrowIcon(m_ResizePresetIconButton, dpi, GuiTools::ARROW_DOWN);
    m_ResizePresetIconButton.SetIcon(iconDropdown_);
    m_ShorteningServerButton.SetIcon(iconDropdown_);

    if (iconEdit_) {
        iconEdit_.DestroyIcon();
    }
    iconEdit_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONEDIT), iconWidth, iconHeight);

    RECT profileRect;
    ::GetWindowRect(GetDlgItem(IDC_EDITPROFILE), &profileRect);
    ScreenToClient(&profileRect);

    if (!m_ProfileEditToolbar) {
        m_ProfileEditToolbar.Create(m_hWnd, profileRect, _T(""), WS_CHILD | WS_VISIBLE | WS_CHILD | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_NORESIZE | /*CCS_BOTTOM |CCS_ADJUSTABLE|*/ TBSTYLE_TOOLTIPS | CCS_NODIVIDER | TBSTYLE_AUTOSIZE);
        m_ProfileEditToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
        m_ProfileEditToolbar.SetButtonStructSize();
    }
    m_ProfileEditToolbar.SetButtonSize(iconWidth + 2, iconHeight + 2);
    if (m_profileEditToolbarImageList) {
        m_profileEditToolbarImageList.Destroy();
    }
    m_profileEditToolbarImageList.Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 0, 6);
    m_profileEditToolbarImageList.AddIcon(iconEdit_);

    m_ProfileEditToolbar.SetImageList(m_profileEditToolbarImageList);
    int buttonCount = m_ProfileEditToolbar.GetButtonCount();

    for (int i = buttonCount - 1; i >= 0; i--) {
        m_ProfileEditToolbar.DeleteButton(i);
    }
    m_ProfileEditToolbar.AddButton(IDC_EDITPROFILE, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 0, TR("Edit Profile"), 0);
}

void CUploadSettings::TranslateUI()
{
    TRC(IDC_FORMATLABEL, "Format:");
    TRC(IDC_QUALITYLABEL, "Quality:");
    TRC(IDC_RESIZEBYWIDTH, "Change width:");
    TRC(IDC_XLABEL, "and/or height:");
    TRC(IDC_PROFILELABEL, "Profile:");
    TRC(IDC_IMAGEPARAMETERS, "Image settings");
    TRC(IDC_KEEPASIS, "Process images before upload");
    TRC(IDC_THUMBSETTINGS, "Thumbnails");
    TRC(IDC_CREATETHUMBNAILS, "Create thumbnails");
    TRC(IDC_IMAGESERVERGROUPBOX, "Server to host your images");
    TRC(IDC_USESERVERTHUMBNAILS, "Use server-side thumbnails");
    TRC(IDC_WIDTHLABEL, "Thumbnail's size:");
    TRC(IDC_ADDFILESIZE, "Add text on thumbnail");
    TRC(IDC_PRESSUPLOADBUTTON, "Click button \"Upload\" for starting process of uploading.");
    TRC(IDC_FILESERVERGROUPBOX, "Server to host other file formats");
    TRC(IDC_SKIPANIMATEDCHECKBOX, "Skip animated");
    useServerThumbnailsTooltip_ = GuiTools::CreateToolTipForWindow(GetDlgItem(IDC_USESERVERTHUMBNAILS), TR("This means that the thumbnail will be created by site, not the program."));
}

LRESULT CUploadSettings::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    PageWnd = m_hWnd;
    sessionImageServer_ = WizardDlg->getSessionImageServer();
    sessionFileServer_ = WizardDlg->getSessionFileServer();

    moreImageServersLink_.SubclassWindow(GetDlgItem(IDC_CHOOSEMOREIMAGESERVERSLABEL));
    moreImageServersLink_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER | HLINK_COMMANDBUTTON;
    moreImageServersLink_.SetLabel(TR("Choose more servers..."));
    moreImageServersLink_.SetToolTipText(TR("Choose more servers..."));
    moreImageServersLink_.m_clrLink = GetSysColor(COLOR_WINDOWTEXT);

    moreFileServersLink_.SubclassWindow(GetDlgItem(IDC_CHOOSEMOREFILESERVERS));
    moreFileServersLink_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER | HLINK_COMMANDBUTTON;
    moreFileServersLink_.SetLabel(TR("Choose more servers..."));
    moreFileServersLink_.SetToolTipText(TR("Choose more servers..."));
    moreFileServersLink_.m_clrLink = GetSysColor(COLOR_WINDOWTEXT);

    //m_ThumbSizeEdit.SubclassWindow(GetDlgItem(IDC_QUALITYEDIT));
    TranslateUI();

    m_ResizePresetIconButton.m_hWnd = GetDlgItem(IDC_RESIZEPRESETSBUTTON);
    m_ShorteningServerButton.m_hWnd = GetDlgItem(IDC_SHORTENINGURLSERVERBUTTON);

    updateButtonIcons();
    initToolbars();

    Toolbar.SetWindowLong(GWL_ID, IDC_IMAGETOOLBAR);
    FileServerSelectBar.SetWindowLong(GWL_ID, IDC_FILETOOLBAR);

    SendDlgItemMessage(IDC_QUALITYSPIN,UDM_SETRANGE,0,(LPARAM) MAKELONG((short)100, (short)1));
    SendDlgItemMessage(IDC_THUMBQUALITYSPIN,UDM_SETRANGE,0,(LPARAM) MAKELONG((short)100, (short)1));

    GuiTools::AddComboBoxItems(m_hWnd, IDC_FORMATLIST, 6, TR("Auto"), _T("JPEG"), _T("PNG"), _T("GIF"), _T("WebP"), _T("WebP (lossless)"));

    ShowParams();
    CString profileName = U2W(settings->DefaultImageUploadParams.ImageProfileName);
    if (convert_profiles_.find(profileName) == convert_profiles_.end()) {
        profileName = _T("Default");
    }

    ShowParams(profileName);
    UpdateProfileList();
    UpdateAllPlaceSelectors();
    return FALSE;
}

LRESULT CUploadSettings::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    initToolbars();
    UpdateAllPlaceSelectors();
    UpdateToolbarIcons();
    updateButtonIcons();
    return 0;
}

LRESULT CUploadSettings::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CUploadSettings::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CUploadSettings::OnBnClickedKeepasis(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool checked = SendDlgItemMessage(IDC_KEEPASIS, BM_GETCHECK, 0, 0)!=FALSE;
    GuiTools::EnableNextN(GetDlgItem(IDC_KEEPASIS), 15, checked);
    m_ProfileEditToolbar.EnableWindow(checked);
    return 0;
}

void CUploadSettings::ShowParams(/*UPLOADPARAMS params*/)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    const ImageUploadParams& params = settings->DefaultImageUploadParams;
    SendDlgItemMessage(IDC_KEEPASIS, BM_SETCHECK, params.ProcessImages);
    SendDlgItemMessage(IDC_THUMBFORMATLIST, CB_SETCURSEL, static_cast<int>(params.getThumb().Format));
    SendDlgItemMessage(IDC_CREATETHUMBNAILS, BM_SETCHECK, params.CreateThumbs);
    SendDlgItemMessage(IDC_ADDFILESIZE, BM_SETCHECK, params.getThumb().AddImageSize);
    SendDlgItemMessage(IDC_USESERVERTHUMBNAILS, BM_SETCHECK, params.UseServerThumbs);

    bool shortenImages = getSessionImageServerItem().shortenLinks();
    bool shortenFiles = getSessionFileServerItem().shortenLinks();
    int shortenLinks = BST_INDETERMINATE;
    int checkboxStyle = BS_AUTO3STATE;
    HWND checkbox = GetDlgItem(IDC_SHORTENLINKSCHECKBOX);
    if (shortenImages == shortenFiles)
    {
        shortenLinks = shortenImages ? BST_CHECKED : BST_UNCHECKED;
        checkboxStyle = BS_AUTOCHECKBOX;
    }
    ::SetWindowLong(checkbox, GWL_STYLE, (::GetWindowLong(checkbox, GWL_STYLE) & ~(BS_AUTO3STATE | BS_AUTOCHECKBOX)) | checkboxStyle);
    SendDlgItemMessage(IDC_SHORTENLINKSCHECKBOX, BM_SETCHECK, shortenLinks);
    updateUrlShorteningCheckboxLabel();
}

LRESULT CUploadSettings::OnBnClickedCreateThumbnails(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    BOOL checked = SendDlgItemMessage(IDC_CREATETHUMBNAILS, BM_GETCHECK, 0, 0);

    ::EnableWindow(GetDlgItem(IDC_THUMBWIDTH), checked);
    ::EnableWindow(GetDlgItem(IDC_THUMBHEIGHT), checked);
    ::EnableWindow(GetDlgItem(IDC_ADDFILESIZE), checked);
    ::EnableWindow(GetDlgItem(IDC_WIDTHLABEL), checked);
    ::EnableWindow(GetDlgItem(IDC_USESERVERTHUMBNAILS), checked);
    ::EnableWindow(GetDlgItem(IDC_PXLABEL), checked);

    return 0;
}

bool CUploadSettings::OnNext()
{
    auto *settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    auto& sessionImageServer = getSessionImageServerItem();
    if (sessionImageServer.isNull()) {
        GuiTools::LocalizedMessageBox(m_hWnd, TR("You have not selected an image server"), TR("Error"), MB_ICONERROR);
        return false;
    }

    if(!sessionImageServer.serverName().empty())
    {
        const CUploadEngineData *ue = sessionImageServer.uploadEngineData();
        if (ue->NeedAuthorization == CUploadEngineData::naObligatory && sessionImageServer.profileName().empty())
        {
            CString errorMsg;
            errorMsg.Format(TR("Upload to server '%s' is impossible without account.\nYou should sign Up on this server and specify your account details in program settings."), (LPCTSTR)Utf8ToWCstring(m_EngineList->getServerDisplayName(ue)));
            GuiTools::LocalizedMessageBox(m_hWnd, errorMsg, APP_NAME, MB_ICONERROR);
            return false;
        }
    }

    auto& sessionFileServer = getSessionFileServerItem();

    if (sessionFileServer.isNull()) {
        GuiTools::LocalizedMessageBox(m_hWnd, TR("You have not selected an file server"), TR("Error"), MB_ICONERROR);
        return false;
    }
    if(!sessionFileServer.serverName().empty())
    {
        const CUploadEngineData *ue2 = sessionFileServer.uploadEngineData();
        if (ue2->NeedAuthorization == CUploadEngineData::naObligatory && sessionFileServer.profileName().empty())
        {
            CString errorMsg;
            errorMsg.Format(TR("Please specify authentication settings for '%s' server!"), static_cast<LPCTSTR>(U2W(m_EngineList->getServerDisplayName(ue2))));
            GuiTools::LocalizedMessageBox(m_hWnd, errorMsg, APP_NAME, MB_ICONWARNING);
            return false;
        }
    }

    ImageUploadParams imageUploadParams = settings->DefaultImageUploadParams;
    imageUploadParams.ProcessImages = SendDlgItemMessage(IDC_KEEPASIS, BM_GETCHECK, 0) == BST_CHECKED;
    imageUploadParams.CreateThumbs = GuiTools::IsChecked(m_hWnd, IDC_CREATETHUMBNAILS);
    imageUploadParams.UseServerThumbs = GuiTools::IsChecked(m_hWnd, IDC_USESERVERTHUMBNAILS);
    imageUploadParams.getThumbRef().AddImageSize = GuiTools::IsChecked(m_hWnd, IDC_ADDFILESIZE);

    int thumbWidth = GetDlgItemInt(IDC_THUMBWIDTH);
    int thumbHeight = GetDlgItemInt(IDC_THUMBHEIGHT);

    if (!thumbWidth && !thumbHeight) {
        thumbWidth = ThumbCreatingParams::DEFAULT_THUMB_WIDTH;
    }

    auto& thumb = imageUploadParams.getThumbRef();
    thumb.Width = thumbWidth;
    thumb.Height = thumbHeight;

    if (thumb.Width && thumb.Height) {
        thumb.ResizeMode = ThumbCreatingParams::trByBoth;
    } else if (thumb.Height) {
        thumb.ResizeMode = ThumbCreatingParams::trByHeight;
    } else {
        thumb.ResizeMode = ThumbCreatingParams::trByWidth;
    }

    int shortenLinks = SendDlgItemMessage(IDC_SHORTENLINKSCHECKBOX, BM_GETCHECK);
    if (shortenLinks != BST_INDETERMINATE)
    {
        bool shorten = shortenLinks == BST_CHECKED;
        sessionImageServer.setShortenLinks(shorten);
        sessionFileServer.setShortenLinks(shorten);
    }

    if (settings->CheckFileTypesBeforeUpload && !WizardDlg->checkFileFormats(sessionImageServer_, sessionFileServer_, imageUploadParams)) {
        return false;
    }

    WizardDlg->setSessionImageServer(sessionImageServer_);
    WizardDlg->setSessionFileServer(sessionFileServer_);
    if ( settings->RememberImageServer ) {
        settings->imageServer = sessionImageServer_;
    }
    if ( settings->RememberFileServer ) {
        settings->fileServer = sessionFileServer_;
    }
    settings->DefaultImageUploadParams = imageUploadParams;

    SaveCurrentProfile();

    return true;
}

bool CUploadSettings::OnShow()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    ShowPrev();
    ShowNext();
    BOOL temp;

    if (WizardDlg->serversChanged()) {
        sessionImageServer_ = WizardDlg->getSessionImageServer();
        sessionFileServer_ = WizardDlg->getSessionFileServer();
        WizardDlg->setServersChanged(false);
        ShowParams();
    }

    auto* mainDlg = WizardDlg->getPage<CMainDlg>(CWizardDlg::wpMainPage);

    // Reset skip flag, which can be set when checkFileFormats on file list is called
    // CAtlArray doesn't have begin() and end() methods.
    for (size_t i = 0; i < mainDlg->FileList.GetCount(); i++) {
        mainDlg->FileList[i].setSkipped(false);
    }

    CString profileName = U2W(settings->DefaultImageUploadParams.ImageProfileName);
    if (convert_profiles_.find(profileName) == convert_profiles_.end()) {
        profileName = _T("Default");
    }
    ShowParams(profileName);
    UpdateProfileList();
    UpdateAllPlaceSelectors();
    OnBnClickedCreateThumbnails(0, 0, 0, temp);
    OnBnClickedKeepasis(0, 0, 0, temp);
    SetNextCaption(TR("&Upload"));
    EnableNext();
    return true;
}

LRESULT CUploadSettings::OnBnClickedLogin(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
    bool ImageServer = (wID % 2)!=0;

    ServerProfile & serverProfile = ImageServer? getSessionImageServerItem() : getSessionFileServerItem();
    CLoginDlg dlg(serverProfile, uploadEngineManager_);

    BasicSettings* Settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* serverSettings = Settings->getServerSettings(ImageServer ? getSessionImageServerItem() : getSessionFileServerItem());

    std::string UserName = serverSettings ? serverSettings->authData.Login : std::string();
    bool prevAuthEnabled = serverSettings ? serverSettings->authData.DoAuth : false;
    UINT dlgResult = dlg.DoModal(m_hWnd);

    if (dlgResult  == IDOK) {
        ServerSettingsStruct* ss = Settings->getServerSettings(ImageServer ? getSessionImageServerItem() : getSessionFileServerItem());
        std::string newUserName = ss ? ss->authData.Login : std::string();
        bool newAuth = ss ? ss->authData.DoAuth : false;
        serverProfile.setProfileName(WCstringToUtf8(dlg.accountName()));
        if(UserName != newUserName || newAuth != prevAuthEnabled) {
            serverProfile.clearFolderInfo();
        }

        UpdateAllPlaceSelectors();
    } else if (dlgResult == CLoginDlg::ID_DELETEACCOUNT) {
        serverProfile.clearFolderInfo();
        serverProfile.setProfileName("");
        UpdateAllPlaceSelectors();
    }
    return 0;
}

LRESULT CUploadSettings::OnBnClickedUseServerThumbnails(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    //BOOL checked=SendDlgItemMessage(IDC_USESERVERTHUMBNAILS, BM_GETCHECK, 0, 0);
    return 0;
}

LRESULT CUploadSettings::OnBnClickedSelectFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
{
	bool ImageServer = (hWndCtl == Toolbar.m_hWnd);

    ServerProfile& serverProfile = ImageServer ? getSessionImageServerItem() : getSessionFileServerItem();
    const CUploadEngineData *ue = serverProfile.uploadEngineData();

	if (!ue) {
		LOG(ERROR) << "serverProfile.uploadEngineData() cannot be NULL";
		return 0;
	}

    if(ue->SupportsFolders){
        CServerFolderSelect as(serverProfile, uploadEngineManager_);

        as.m_SelectedFolder.id = serverProfile.folderId();
        as.m_SelectedFolder.parentIds = serverProfile.parentIds();

        if(as.DoModal(m_hWnd) == IDOK){
            BasicSettings* settings = ServiceLocator::instance()->basicSettings();
            ServerSettingsStruct* serverSettings = settings->getServerSettings(serverProfile);

            if(!as.m_SelectedFolder.id.empty()){
                if (serverSettings) {
                    serverSettings->defaultFolder = as.m_SelectedFolder;
                }
                serverProfile.setFolder(as.m_SelectedFolder);
            }
            else {
                if (serverSettings) {
                    serverSettings->defaultFolder = CFolderItem();
                }
                serverProfile.clearFolderInfo();
            }
            UpdateAllPlaceSelectors();
        }
    }
    return 0;
}

LRESULT CUploadSettings::OnBnClickedSelectServer(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/) {
    bool isImageServer = (hWndCtl == Toolbar.m_hWnd);
    CToolBarCtrl& CurrentToolbar = isImageServer ? Toolbar : FileServerSelectBar;
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();

    RECT buttonRect {};
    CurrentToolbar.GetRect(ID_SERVERBUTTON, &buttonRect);
    CurrentToolbar.ClientToScreen(&buttonRect);
    CurrentToolbar.SetButtonInfo(ID_SERVERBUTTON, TBIF_STATE, 0, TBSTATE_ENABLED | TBSTATE_PRESSED, nullptr, 0, 0, 0, 0);
    ServerProfile& serverProfile = isImageServer ? getSessionImageServerItem() : getSessionFileServerItem();
    CString serverName = U2W(serverProfile.serverName());
    int serverIndex = myEngineList->getUploadEngineIndex(serverName);
    int serverMask = isImageServer ? CUploadEngineData::TypeImageServer | CUploadEngineData::TypeFileServer : CUploadEngineData::TypeFileServer | CUploadEngineData::TypeVideoServer;
    int selectedServerType = isImageServer ? CUploadEngineData::TypeImageServer : CUploadEngineListBase::ALL_SERVERS;
    CServerListPopup serverListPopup(ServiceLocator::instance()->myEngineList(), iconCache_, serverMask, selectedServerType, serverIndex);
   
    if (serverListPopup.showPopup(m_hWnd, buttonRect) == IDOK) {
        int newServerIndex = serverListPopup.serverIndex();
        if (newServerIndex != -1) {
            selectServer(serverProfile, newServerIndex);
            UpdateAllPlaceSelectors();
        }
    };

    CurrentToolbar.SetButtonInfo(ID_SERVERBUTTON, TBIF_STATE, 0, TBSTATE_ENABLED, nullptr, 0, 0, 0, 0);

    return 0;
}

void CUploadSettings::UpdateToolbarIcons()
{
    HICON hImageIcon = NULL, hFileIcon = NULL;
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);

    if (!getSessionImageServerItem().isNull()) {
        hImageIcon = iconCache_->getIconForServer(getSessionImageServerItem().serverName(), dpi);
    }

    if (!getSessionFileServerItem().isNull()) {
        hFileIcon = iconCache_->getIconForServer(getSessionFileServerItem().serverName(), dpi);
    }

    if(hImageIcon)
    {
        if(nImageIndex == -1)
        {
            nImageIndex = m_PlaceSelectorImageList.AddIcon( hImageIcon);
        }
        else nImageIndex= m_PlaceSelectorImageList.ReplaceIcon(nImageIndex, hImageIcon);
    } else nImageIndex =-1;

    if(hFileIcon)
    {
        if(nFileIndex == -1)
        {
            nFileIndex = m_PlaceSelectorImageList.AddIcon( hFileIcon);
        }
        else
            nFileIndex = m_PlaceSelectorImageList.ReplaceIcon(nFileIndex, hFileIcon);
    } else nFileIndex = -1;

    Toolbar.ChangeBitmap(ID_SERVERBUTTON, nImageIndex);
    FileServerSelectBar.ChangeBitmap(ID_SERVERBUTTON, nFileIndex);
}

void CUploadSettings::initToolbars() {
    int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
    int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    if (m_PlaceSelectorImageList) {
        m_PlaceSelectorImageList.Destroy();
    }
    m_PlaceSelectorImageList.Create(iconWidth, iconHeight, ILC_COLOR32, 0, 6);
    CIcon iconUser;
    iconUser.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONUSER), iconWidth, iconHeight);
    m_PlaceSelectorImageList.AddIcon(iconUser);
    CIcon iconFolder;
    iconFolder.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONFOLDER2), iconWidth, iconHeight);
    m_PlaceSelectorImageList.AddIcon(iconFolder);

    CIcon iconSeparator = GuiTools::CreateDropDownArrowIcon(m_hWnd, dpi, GuiTools::ARROW_RIGHT);

    m_PlaceSelectorImageList.AddIcon(iconSeparator);

    RECT Toolbar1Rect;
    ::GetWindowRect(GetDlgItem(IDC_IMAGESERVERGROUPBOX), &Toolbar1Rect);

    ::MapWindowPoints(0, m_hWnd, reinterpret_cast<LPPOINT>(&Toolbar1Rect), 2);
    Toolbar1Rect.top += MulDiv(9, dpi, USER_DEFAULT_SCREEN_DPI);
    Toolbar1Rect.bottom -= MulDiv(3, dpi, USER_DEFAULT_SCREEN_DPI);
    Toolbar1Rect.left += MulDiv(6, dpi, USER_DEFAULT_SCREEN_DPI);
    Toolbar1Rect.right -= MulDiv(6, dpi, USER_DEFAULT_SCREEN_DPI);

    RECT Toolbar2Rect;
    ::GetWindowRect(GetDlgItem(IDC_FILESERVERGROUPBOX), &Toolbar2Rect);
    ::MapWindowPoints(0, m_hWnd, reinterpret_cast<LPPOINT>(&Toolbar2Rect), 2);
    Toolbar2Rect.top += MulDiv(9, dpi, USER_DEFAULT_SCREEN_DPI);
    Toolbar2Rect.bottom -= MulDiv(3, dpi, USER_DEFAULT_SCREEN_DPI);
    Toolbar2Rect.left += MulDiv(6, dpi, USER_DEFAULT_SCREEN_DPI);
    Toolbar2Rect.right -= MulDiv(6, dpi, USER_DEFAULT_SCREEN_DPI);

    for (int i = 0; i < 2; i++) {
        CToolBarCtrl& CurrentToolbar = (i == 0) ? Toolbar : FileServerSelectBar;
        if (!CurrentToolbar) {
            CurrentToolbar.Create(m_hWnd, i ? Toolbar2Rect : Toolbar1Rect, _T(""), WS_CHILD | WS_VISIBLE | WS_CHILD | WS_TABSTOP | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_NORESIZE | /*CCS_BOTTOM |CCS_ADJUSTABLE|*/ CCS_NODIVIDER | TBSTYLE_AUTOSIZE);
            CurrentToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DOUBLEBUFFER);
            CurrentToolbar.SetButtonStructSize();
        }
        int buttonCount = CurrentToolbar.GetButtonCount();

        for (int i = buttonCount - 1; i >= 0; i--) {
            CurrentToolbar.DeleteButton(i);
        }
        CurrentToolbar.SetImageList(m_PlaceSelectorImageList);

        CurrentToolbar.AddButton(ID_SERVERBUTTON, TBSTYLE_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, TBSTATE_ENABLED, -1, TR("Choose server..."), 0);
        CurrentToolbar.AddButton(IDC_TOOLBARSEPARATOR1, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 2, _T(""), 0);

        CurrentToolbar.AddButton(IDC_LOGINTOOLBUTTON + !i, /*TBSTYLE_BUTTON*/ TBSTYLE_DROPDOWN | BTNS_AUTOSIZE | BTNS_SHOWTEXT, TBSTATE_ENABLED, 0, _T(""), 0);
        CurrentToolbar.AddButton(IDC_TOOLBARSEPARATOR2, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, 2, _T(""), 0);

        CurrentToolbar.AddButton(IDC_SELECTFOLDER, TBSTYLE_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, TBSTATE_ENABLED, 1, TR("Choose folder..."), 0);
        //CurrentToolbar.SetButtonSize(MulDiv(20, dpiX, USER_DEFAULT_SCREEN_DPI), MulDiv(24, dpiY, USER_DEFAULT_SCREEN_DPI));
        CurrentToolbar.AutoSize();
        CurrentToolbar.SetWindowPos(i == 0 ? GetDlgItem(IDC_IMAGESERVERGROUPBOX) : Toolbar.m_hWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        SIZE toolbarSize;
        if (CurrentToolbar.GetMaxSize(&toolbarSize)) {

            RECT rc = i ? Toolbar2Rect : Toolbar1Rect;
            //rc.right = rc.left + toolbarSize.cx;
            rc.bottom = rc.top + toolbarSize.cy;
           // rc.top = rc.top + (rc.bottom - rc.top - toolbarSize.cy) / 2;
            CurrentToolbar.SetWindowPos(nullptr, &rc, SWP_NOZORDER);
        }
        CenterToolbarInGroupBoxThemed(m_hWnd, GetDlgItem(i ? IDC_FILESERVERGROUPBOX : IDC_IMAGESERVERGROUPBOX), CurrentToolbar);
    }
}

void CUploadSettings::UpdatePlaceSelector(bool ImageServer)
{
    TBBUTTONINFO bi;
    CToolBarCtrl& currentToolbar = ImageServer ? Toolbar: FileServerSelectBar;

    const ServerProfileGroup& serverProfileGroup = ImageServer ? sessionImageServer_: sessionFileServer_;
    ServerProfile& serverProfile = ImageServer ? getSessionImageServerItem() : getSessionFileServerItem();
    const CUploadEngineData* uploadEngine = ServiceLocator::instance()->engineList()->byName(serverProfile.serverName());

    std::wstring serverButtonTitleWideStr;
    if (serverProfile.isNull() || !uploadEngine) {
        serverButtonTitleWideStr = TR("Choose server");
    } else {
        if (serverProfileGroup.getCount() == 1) {
            serverButtonTitleWideStr = IuCoreUtils::Utf8ToWstring(CMyEngineList::getServerDisplayName(uploadEngine));
        } else {
            serverButtonTitleWideStr = IuCoreUtils::Utf8ToWstring(str(
                IuStringUtils::FormatNoExcept(_c("upload_settings_wizard_page.server_button", "%1% + %2%"))
                    % CMyEngineList::getServerDisplayName(uploadEngine)
                % (serverProfileGroup.getCount() - 1)
            ));
        }
    }

    ZeroMemory(&bi, sizeof(bi));
    bi.cbSize = sizeof(bi);
    bi.dwMask = TBIF_TEXT;
    bi.pszText = const_cast<LPWSTR>(serverButtonTitleWideStr.c_str());
    currentToolbar.SetButtonInfo(ID_SERVERBUTTON, &bi);

    if(serverProfile.isNull())
    {
        currentToolbar.HideButton(IDC_LOGINTOOLBUTTON + ImageServer ,true);
        currentToolbar.HideButton(IDC_TOOLBARSEPARATOR1, true);
        currentToolbar.HideButton(IDC_SELECTFOLDER, true);
        currentToolbar.HideButton(IDC_TOOLBARSEPARATOR2, true);
        return;
    }

    if (!uploadEngine) {
        LOG(ERROR) << "uploadEngine cannot be NULL";
        return;
    }

    BasicSettings* Settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* res = Settings->getServerSettings(serverProfile);
    ServerSettingsStruct serverSettings;
    if (res) {
        serverSettings = *res;
    }
    LoginInfo& li = serverSettings.authData;
    CString login = WinUtils::TrimString(Utf8ToWCstring(li.Login),23);

    currentToolbar.SetImageList(m_PlaceSelectorImageList);
    currentToolbar.HideButton(IDC_LOGINTOOLBUTTON + ImageServer,(bool)!uploadEngine->NeedAuthorization);
    currentToolbar.HideButton(IDC_TOOLBARSEPARATOR1,(bool)!uploadEngine->NeedAuthorization);

    bool ShowLoginButton = !login.IsEmpty() && li.DoAuth;
    if(!ShowLoginButton)
    {
        if(uploadEngine->NeedAuthorization == 2)
            login = TR("Specify account...");
        else
            login = TR("Account is not enabled");

    }
    bi.pszText = const_cast<LPWSTR>(login.GetString());
    currentToolbar.SetButtonInfo(IDC_LOGINTOOLBUTTON+ImageServer, &bi);

    bool ShowFolderButton = uploadEngine->SupportsFolders && ShowLoginButton;

    currentToolbar.HideButton(IDC_SELECTFOLDER,!ShowFolderButton);
    currentToolbar.HideButton(IDC_TOOLBARSEPARATOR2,!ShowFolderButton);

    CString title = WinUtils::TrimString(Utf8ToWCstring(serverProfile.folderTitle()), 27);
    if(title.IsEmpty()) title = TR("No Folder Selected");
    bi.pszText = const_cast<LPWSTR>(title.GetString());
    currentToolbar.SetButtonInfo(IDC_SELECTFOLDER, &bi);

}
void CUploadSettings::UpdateAllPlaceSelectors()
{
    UpdatePlaceSelector(false); // Update server selector (file hosting)
    UpdatePlaceSelector(true); // Update server selector (image hosting)
    UpdateToolbarIcons();
    updateMoreServersLink(false); // file hosting
    updateMoreServersLink(true); // image hosting
}

LRESULT CUploadSettings::OnImageServerSelect(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int nServerIndex = static_cast<int>(wID - IDC_IMAGESERVER_FIRST_ID);
    selectServer(getSessionImageServerItem(), nServerIndex);

    UpdateAllPlaceSelectors();
    return 0;
}

LRESULT CUploadSettings::OnFileServerSelect(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int nServerIndex = static_cast<int>(wID - IDC_FILESERVER_FIRST_ID);

    selectServer(getSessionFileServerItem(), nServerIndex);
    UpdateAllPlaceSelectors();
    return 0;
}

LRESULT CUploadSettings::OnServerDropDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    auto* pnmtb = reinterpret_cast<NMTOOLBAR *>(pnmh);

    bool isImageServer = idCtrl == IDC_IMAGETOOLBAR;
    ServerProfile & serverProfile = isImageServer ? getSessionImageServerItem() : getSessionFileServerItem();
    std::vector<HBITMAP> bitmaps;
    const CUploadEngineData *uploadEngine = nullptr;
    if(!serverProfile.isNull())
    {
        uploadEngine = serverProfile.uploadEngineData();
    }

    CToolBarCtrl& CurrentToolbar = isImageServer ? Toolbar: FileServerSelectBar;

    CMenu sub;
    MENUITEMINFO mi;
    ZeroMemory(&mi,sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_TYPE|MIIM_ID;
    mi.fType = MFT_STRING;
    sub.CreatePopupMenu();

    if (/*pnmtb->iItem == IDC_SELECTFOLDER*/true) {
        // Show popup menu with list of accounts
        const std::map <std::string, ServerSettingsStruct>& serverUsers = Settings.ServersSettings[serverProfile.serverName()];

        // Ignore account list if there is just one key "" in map (empty string, meaning no authentication at all)
        if(!serverUsers.empty() && (serverUsers.size() > 1 || serverUsers.find("") == serverUsers.end()) )
        {
            bool addedSeparator = false;
            auto plug = std::dynamic_pointer_cast<CAdvancedUploadEngine>(uploadEngineManager_->getUploadEngine(serverProfile));
            /*if(!plug) return TBDDRET_TREATPRESSED;*/

            int i =0;
            if ( serverUsers.size() && !serverProfile.profileName().empty() ) {
                mi.wID = IDC_LOGINTOOLBUTTON + static_cast<int>(isImageServer);
                CString changeAccountSettingsStr = TR("Edit login credentials...");
                mi.dwTypeData = const_cast<LPWSTR>(changeAccountSettingsStr.GetString());
                mi.cch = changeAccountSettingsStr.GetLength();
                sub.InsertMenuItem(i++, true, &mi);
            } else {
                addedSeparator = true;
            }

            menuOpenedUserNames_.clear();
            menuOpenedIsImageServer_ = isImageServer;

            if(plug && plug->supportsSettings()) {
                mi.wID = IDC_SERVERPARAMS + static_cast<int>(isImageServer);
                CString serverSettingsStr = TR("Server settings...");
                mi.dwTypeData = const_cast<LPWSTR>(serverSettingsStr.GetString());
                mi.cch = serverSettingsStr.GetLength();
                sub.InsertMenuItem(i++, true, &mi);
            }
            int command = IDC_USERNAME_FIRST_ID;
            HICON userIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICONUSER));

            for( const auto& it: serverUsers) {
                CString login = U2W(it.first);
                if (!login.IsEmpty() )/*&& it->second.authData.DoAuth**/ {
                    if ( !addedSeparator ) {
                        ZeroMemory(&mi,sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE|MIIM_ID;
                        mi.wID = IDC_FILESERVER_LAST_ID + 1;
                        mi.fType = MFT_SEPARATOR;

                        sub.InsertMenuItem(i++, true, &mi);
                        addedSeparator =  true;
                    }
                    ZeroMemory(&mi,sizeof(mi));
                    mi.cbSize = sizeof(mi);

                    mi.fMask = MIIM_FTYPE |MIIM_ID | MIIM_STRING;
                    mi.fType = MFT_STRING;
                    mi.wID = command;

                    mi.dwTypeData  = const_cast<LPWSTR>(login.GetString());
                    mi.cch = login.GetLength();
                    HBITMAP bm = iconBitmapUtils_->HIconToBitmapPARGB32(userIcon, dpi);
                    bitmaps.push_back(bm);


                    mi.hbmpItem = bm;
                    if ( mi.hbmpItem ) {
                        mi.fMask |= MIIM_BITMAP;
                    }
                    menuOpenedUserNames_.push_back(login);
                    sub.InsertMenuItem(i++, true, &mi);
                    command++;
                }

            }
            if ( uploadEngine->NeedAuthorization != CUploadEngineData::naObligatory ) {
                ZeroMemory(&mi,sizeof(mi));
                mi.cbSize = sizeof(mi);
                mi.fMask = MIIM_FTYPE |MIIM_ID | MIIM_STRING;
                mi.fType = MFT_STRING;
                mi.wID = IDC_NO_ACCOUNT + !isImageServer;
                CString noAthenticationStr = TR("<no authentication>");
                mi.dwTypeData  = const_cast<LPWSTR>(noAthenticationStr.GetString());
                mi.cch = noAthenticationStr.GetLength();
                sub.InsertMenuItem(i++, true, &mi);
            }

            ZeroMemory(&mi,sizeof(mi));
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_TYPE|MIIM_ID;
            mi.wID = IDC_FILESERVER_LAST_ID + 1;
            mi.fType = MFT_SEPARATOR;

            sub.InsertMenuItem(i++, true, &mi);
            ZeroMemory(&mi,sizeof(mi));
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_FTYPE |MIIM_ID | MIIM_STRING;
            mi.fType = MFT_STRING;
            mi.wID = IDC_ADD_ACCOUNT + !isImageServer;
            CString newAccountStr = TR("Add account...");
            mi.dwTypeData  = const_cast<LPWSTR>(newAccountStr.GetString());
            mi.cch = newAccountStr.GetLength();
            sub.InsertMenuItem(i++, true, &mi);
            sub.SetMenuDefaultItem(0,TRUE);
        }
        else
        {
            return TBDDRET_TREATPRESSED;
        }
    }

    RECT rc;
    CurrentToolbar.GetRect(pnmtb->iItem, &rc);
    CurrentToolbar.MapWindowPoints(nullptr, reinterpret_cast<LPPOINT>(&rc), 2);

    //CurrentToolbar.ClientToScreen(&rc);
    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;
    sub.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, m_hWnd, &excludeArea);
    bHandled = true;
    for (auto bm: bitmaps) {
        DeleteObject(bm);
    }
    return TBDDRET_DEFAULT;
}

LRESULT CUploadSettings::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hwnd = reinterpret_cast<HWND>(wParam);
    int xPos = LOWORD(lParam);
    int yPos = HIWORD(lParam);

    RECT rc;
    POINT pt = {xPos, yPos};

    if(hwnd == Toolbar.m_hWnd || hwnd == FileServerSelectBar.m_hWnd)
    {
        bool ImageServer = (hwnd == Toolbar.m_hWnd);
        CToolBarCtrl& CurrentToolbar = (ImageServer) ? Toolbar: FileServerSelectBar;

        ::SendMessage(CurrentToolbar.m_hWnd,TB_GETRECT, ID_SERVERBUTTON, reinterpret_cast<LPARAM>(&rc));
        CurrentToolbar.ClientToScreen(&rc);
        if(PtInRect(&rc, pt))
        {
            OnServerButtonContextMenu(pt, ImageServer);
            return 0;
        }
        if(!CurrentToolbar.IsButtonHidden(IDC_SELECTFOLDER))
        {
            CurrentToolbar.GetRect(IDC_SELECTFOLDER, &rc);
            CurrentToolbar.ClientToScreen(&rc);
            if(PtInRect(&rc, pt))
            {
                OnFolderButtonContextMenu(pt, ImageServer);
                return 0;
            }
        }
    }
    return 0;
}

void CUploadSettings::OnFolderButtonContextMenu(POINT pt, bool isImageServerToolbar)
{
    ServerProfile & serverProfile = isImageServerToolbar ? getSessionImageServerItem() : getSessionFileServerItem();

    CMenu sub;
    sub.CreatePopupMenu();

    sub.AppendMenu(MFT_STRING, IDC_NEWFOLDER + static_cast<int>(isImageServerToolbar), TR("New folder"));

    if (!serverProfile.folderId().empty()) {
        UINT flags = MFT_STRING;
        if (serverProfile.folderId().empty() || serverProfile.folderId() == CFolderItem::NewFolderMark) {
            flags |= MFS_DISABLED;
        }
        sub.AppendMenu(flags, IDC_COPYFOLDERID + static_cast<int>(isImageServerToolbar), TR("Copy folder's ID"));
    }

    if (!serverProfile.folderUrl().empty()) {
        sub.AppendMenu(MFT_STRING, IDC_OPENINBROWSER + static_cast<int>(isImageServerToolbar), TR("Open in Web Browser"));
    }

    sub.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON, pt.x, pt.y, m_hWnd);
}

LRESULT CUploadSettings::OnNewFolder(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool ImageServer = (wID % 2)!=0;
    ServerProfile & serverProfile = ImageServer ? getSessionImageServerItem() : getSessionFileServerItem();

    auto uploadEngine = std::dynamic_pointer_cast<CAdvancedUploadEngine>(uploadEngineManager_->getUploadEngine(serverProfile));
    if (!uploadEngine) {
        return 0;
    }

    std::vector<std::string> accessTypeList;

    uploadEngine->getAccessTypeList(accessTypeList);
    CFolderItem newFolder;

    BasicSettings* Settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* serverSettings = Settings->getServerSettings(serverProfile);

    if (serverProfile.folderId() == CFolderItem::NewFolderMark) {
        newFolder = serverSettings ? serverSettings->newFolder : CFolderItem();
    }

     CNewFolderDlg dlg(newFolder, true, accessTypeList);
     if(dlg.DoModal(m_hWnd) == IDOK)
     {
         serverProfile.setFolderTitle(newFolder.title);
         serverProfile.setFolderId(CFolderItem::NewFolderMark);
         serverProfile.setFolderUrl("");
         serverProfile.setParentIds(newFolder.parentIds);
         newFolder.setId(CFolderItem::NewFolderMark);

         if (serverSettings) {
             serverSettings->newFolder = newFolder;
         }
        UpdateAllPlaceSelectors();
     }
     return 0;
}

LRESULT CUploadSettings::OnOpenInBrowser(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool ImageServer = (wID % 2)!=0;
    ServerProfile & serverProfile = ImageServer? getSessionImageServerItem() : getSessionFileServerItem();

    CString str = U2W(serverProfile.folderUrl());
    if(!str.IsEmpty()) {
        WinUtils::ShellOpenFileOrUrl(str, m_hWnd);
    }
    return 0;
}

LRESULT CUploadSettings::OnCopyFolderId(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    bool ImageServer = (wID % 2) != 0;
    ServerProfile& serverProfile = ImageServer ? getSessionImageServerItem() : getSessionFileServerItem();
    std::string folderId = serverProfile.folderId();
    if (!folderId.empty() && folderId != CFolderItem::NewFolderMark) {
        CString str = U2W(folderId);

        WinUtils::CopyTextToClipboard(str);
    }
    return 0;
}

void CUploadSettings::OnServerButtonContextMenu(POINT pt, bool isImageServerToolbar)
{
    ServerProfile & serverProfile = isImageServerToolbar? getSessionImageServerItem() : getSessionFileServerItem();
    if ( serverProfile.isNull() ) {
        return;
    }
    const UINT dpi = DPIHelper::GetDpiForWindow(m_hWnd);

    int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);
    CIcon ico;
    ico.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONSETTINGS), iconWidth, iconHeight);

    CMenu sub;
    sub.CreatePopupMenu();
    MENUITEMINFO mi {};
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_ID | MIIM_STRING | MIIM_BITMAP;
    mi.wID = IDC_SERVERPARAMS + (int)isImageServerToolbar;
    CString serverSettingsStr = TR("Server settings...");
    CBitmap bm = iconBitmapUtils_->HIconToBitmapPARGB32(ico, dpi);
    mi.hbmpItem = bm;
    mi.dwTypeData = const_cast<LPWSTR>(serverSettingsStr.GetString());
    mi.cch = serverSettingsStr.GetLength();
    if (!sub.InsertMenuItem(0, true, &mi)) {
        //LOG(WARNING) << "Cannot insert menu item" << std::endl << WinUtils::GetLastErrorAsString();
    }

    mi.fMask = MIIM_ID | MIIM_STRING;
    if(!serverProfile.uploadEngineData()->RegistrationUrl.empty())
    {
        mi.wID = IDC_OPENREGISTERURL + (int)isImageServerToolbar;
        CString goToSignupPageStr = TR("Go to signup page");
        mi.dwTypeData = const_cast<LPWSTR>(goToSignupPageStr.GetString());
        mi.cch = goToSignupPageStr.GetLength();
        sub.InsertMenuItem(1, true, &mi);
    }

    if (!serverProfile.uploadEngineData()->WebsiteUrl.empty()) {
        sub.AppendMenu(MF_STRING, IDC_OPENWEBSITE + (int)isImageServerToolbar, TR("Open the website"));
    }
    sub.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON, pt.x, pt.y, m_hWnd);
}

void CUploadSettings::SetInitialFocus(){
    ::SetFocus(WizardDlg->GetDlgItem(IDC_NEXT));
}

LRESULT CUploadSettings::OnServerParamsClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool ImageServer = (wID % 2)!=0;
    //CToolBarCtrl& CurrentToolbar = (ImageServer) ? Toolbar: FileServerSelectBar;

    ServerProfile& serverProfile = ImageServer ? getSessionImageServerItem() : getSessionFileServerItem();
    const CUploadEngineData *ue = serverProfile.uploadEngineData();
    if (!ue->UsingPlugin && ue->Engine.empty()) {
        GuiTools::LocalizedMessageBox(m_hWnd, TR("This server doesn't have any settings."), APP_NAME, MB_ICONINFORMATION);
        return false;
    }

    CServerParamsDlg dlg(serverProfile, uploadEngineManager_);
    if ( dlg.DoModal(m_hWnd) == IDOK) {
        serverProfile = dlg.serverProfile();
        UpdateAllPlaceSelectors();
    }
    return 0;
}

LRESULT CUploadSettings::OnOpenSignupPage(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool ImageServer = (wID % 2)!=0;
    ServerProfile & serverProfile = ImageServer? getSessionImageServerItem() : getSessionFileServerItem();

    const CUploadEngineData *ue = serverProfile.uploadEngineData();
    if (ue && !ue->RegistrationUrl.empty()) {
        WinUtils::ShellOpenFileOrUrl(U2W(ue->RegistrationUrl), m_hWnd);
    }
    return 0;
}

LRESULT CUploadSettings::OnResizePresetButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    RECT rc;
    ::GetWindowRect(hWndCtl, &rc);
    POINT menuOrigin = {rc.left,rc.bottom};

    CMenu FolderMenu;
    int id = IDC_RESIZEPRESETMENU_FIRST_ID;
    FolderMenu.CreatePopupMenu();

    int prevMenuGroup = 0;
    for ( const auto& preset: resizePresets) {
        // Add menu separator if group number has changed
        if (preset.menuGroup != prevMenuGroup) {
            FolderMenu.AppendMenu(MF_SEPARATOR, static_cast<UINT_PTR>(-1), _T(""));
            prevMenuGroup = preset.menuGroup;
        }
        if (preset.width.empty() && preset.height.empty()) {
            FolderMenu.AppendMenu(MF_STRING, id++, TR("Don't resize"));
            continue;
        }

        std::wstring menuItemTitle;
        if (preset.title.empty()) {
            // Format menu item text like "WxH"
            menuItemTitle  = str(boost::wformat(L"%s\u00D7%s") % preset.width % preset.height);
        } else {
            menuItemTitle = preset.title;
        }

        FolderMenu.AppendMenu(MF_STRING, id++, menuItemTitle.c_str());
    }

    FolderMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, menuOrigin.x, menuOrigin.y, m_hWnd);

   return 0;
}

LRESULT CUploadSettings::OnShorteningUrlServerButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    CServerSelectorControl serverSelectorControl(uploadEngineManager_, false, false);
    serverSelectorControl.setServersMask(CUploadEngineData::TypeUrlShorteningServer);
    serverSelectorControl.setShowImageProcessingParams(false);
    serverSelectorControl.setTitle(TR("URL shortening server"));
    serverSelectorControl.setServerProfile(Settings.urlShorteningServer);
    RECT buttonRect;
    m_ShorteningServerButton.GetClientRect(&buttonRect);
    m_ShorteningServerButton.ClientToScreen(&buttonRect);
    
    serverSelectorControl.setOnChangeCallback([this](auto && PH1) { shorteningUrlServerChanged(std::forward<decltype(PH1)>(PH1)); });
    serverSelectorControl.showPopup(m_hWnd, buttonRect);
    Settings.urlShorteningServer = serverSelectorControl.serverProfile();
    updateUrlShorteningCheckboxLabel();
    return 0;
}

LRESULT CUploadSettings::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	::DestroyWindow(useServerThumbnailsTooltip_);
	useServerThumbnailsTooltip_ = nullptr;
	return 0;
}

LRESULT CUploadSettings::OnResizePresetMenuItemClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   int presetIndex = wID - IDC_RESIZEPRESETMENU_FIRST_ID;
   int presetCount = ARRAY_SIZE(resizePresets);
   if (presetIndex > presetCount - 1) {
       return 0;
   }
   ::SetDlgItemText(m_hWnd, IDC_IMAGEWIDTH, resizePresets[presetIndex].width.c_str());
   ::SetDlgItemText(m_hWnd, IDC_IMAGEHEIGHT, resizePresets[presetIndex].height.c_str());
   return 0;
}

LRESULT CUploadSettings::OnEditProfileClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    SaveCurrentProfile();
    CSettingsDlg dlg(CSettingsDlg::spImages, uploadEngineManager_);
    dlg.DoModal(m_hWnd);
    CurrentProfileName.Empty();
    ShowParams(U2W(settings->DefaultImageUploadParams.ImageProfileName));
    UpdateProfileList();
    ShowParams(U2W(settings->DefaultImageUploadParams.ImageProfileName));
    return 0;
}

 void CUploadSettings::UpdateProfileList()
 {
    SendDlgItemMessage(IDC_PROFILECOMBO, CB_RESETCONTENT);
    std::map<CString, ImageConvertingParams> ::const_iterator it;
    bool found = false;
    for(it = convert_profiles_.begin(); it!=convert_profiles_.end(); ++it)
    {
      GuiTools::AddComboBoxItem(m_hWnd, IDC_PROFILECOMBO, it->first);
      if(it->first == CurrentProfileName) found = true;
    }
    if (!found) {
        GuiTools::AddComboBoxItem(m_hWnd, IDC_PROFILECOMBO, CurrentProfileName);
    }
    SendDlgItemMessage(IDC_PROFILECOMBO, CB_SELECTSTRING, static_cast<WPARAM>(-1),(LPARAM)(LPCTSTR) CurrentProfileName);
 }

void CUploadSettings::selectServer(ServerProfile& sp, int serverIndex) {
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    sp.setServerName(myEngineList->byIndex(serverIndex)->Name);
    std::map<std::string, ServerSettingsStruct>& serverSettings = Settings.ServersSettings[sp.serverName()];
    auto firstAccount = serverSettings.begin();
    if (firstAccount != serverSettings.end()) {
        if (firstAccount->first.empty()) {
            ++firstAccount;
        }
        if (firstAccount != serverSettings.end()) {
            sp.setProfileName(firstAccount->first);
        }
    } else {
        sp.setProfileName("");
    }
    ServerSettingsStruct* ss = Settings.getServerSettings(sp);
    sp.setFolder(ss ? ss->defaultFolder : CFolderItem());
}

void CUploadSettings::updateUrlShorteningCheckboxLabel()
{
    auto settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    CString text;
    CString serverName = Utf8ToWCstring(settings->urlShorteningServer.serverName());
    text.Format(TR("Shorten URL using %s"), serverName.GetString());
    SetDlgItemText(IDC_SHORTENLINKSCHECKBOX, text);
}

void CUploadSettings::shorteningUrlServerChanged(CServerSelectorControl* serverSelectorControl)
{
    auto settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->urlShorteningServer = serverSelectorControl->serverProfile();
    updateUrlShorteningCheckboxLabel();
}

void CUploadSettings::ShowParams(const ImageConvertingParams& params)
{
    m_ProfileChanged = false;
    m_CatchChanges = false;
    if (params.Quality) {
        SetDlgItemInt(IDC_QUALITYEDIT, params.Quality);
    } else {
        SetDlgItemText(IDC_QUALITYEDIT, _T(""));
    }

    SendDlgItemMessage(IDC_FORMATLIST,CB_SETCURSEL, params.Format);
    SendDlgItemMessage(IDC_YOURLOGO,BM_SETCHECK, params.AddLogo);
    SendDlgItemMessage(IDC_YOURTEXT,BM_SETCHECK, params.AddText);
    SetDlgItemText(IDC_IMAGEWIDTH,U2W(params.strNewWidth));
    SetDlgItemText(IDC_IMAGEHEIGHT,U2W(params.strNewHeight));
    SendDlgItemMessage(IDC_SKIPANIMATEDCHECKBOX, BM_SETCHECK, params.SkipAnimated ? BST_CHECKED : BST_UNCHECKED);
    m_ProfileChanged = false;
    m_CatchChanges = true;
}

void CUploadSettings::ShowParams(const CString& profileName) {
    if (convert_profiles_.find(profileName) == convert_profiles_.end()) {
        return;
    }
    auto thumb = getSessionImageServerItem().getImageUploadParams().getThumb();

    SetDlgItemText(IDC_THUMBWIDTH, L"");
    SetDlgItemText(IDC_THUMBHEIGHT, L"");

    if (thumb.Width && (thumb.ResizeMode == ThumbCreatingParams::trByWidth || thumb.ResizeMode == ThumbCreatingParams::trByBoth)){
        SetDlgItemInt(IDC_THUMBWIDTH, thumb.Width);
    }
    if (thumb.Height && (thumb.ResizeMode == ThumbCreatingParams::trByHeight || thumb.ResizeMode == ThumbCreatingParams::trByBoth)) {
        SetDlgItemInt(IDC_THUMBHEIGHT, thumb.Height);
    }

    if (CurrentProfileName == profileName) {
        return;
    }
    CurrentProfileName = profileName;
    CurrentProfileOriginalName = profileName;
    ShowParams(convert_profiles_[profileName]);

    SendDlgItemMessage(IDC_PROFILECOMBO, CB_SELECTSTRING, static_cast<WPARAM>(-1),(LPARAM)(LPCTSTR) profileName);
 }

bool CUploadSettings::SaveParams(ImageConvertingParams& params) {
    params.Quality = GetDlgItemInt(IDC_QUALITYEDIT);
    params.Format = SendDlgItemMessage(IDC_FORMATLIST, CB_GETCURSEL);

    params.strNewWidth = W2U(GuiTools::GetWindowText( GetDlgItem(IDC_IMAGEWIDTH)));

    params.strNewHeight = W2U(GuiTools::GetWindowText( GetDlgItem(IDC_IMAGEHEIGHT)));
    params.SkipAnimated = GuiTools::GetCheck(m_hWnd, IDC_SKIPANIMATEDCHECKBOX);
    return true;
}

void CUploadSettings::ProfileChanged() {
    if (!m_CatchChanges) return;
    if (!m_ProfileChanged) {
        CurrentProfileOriginalName = CurrentProfileName;
        CurrentProfileName.Replace(CString(_T(" ")) + TR("(edited)"), _T(""));
        CurrentProfileName = CurrentProfileName + _T(" ") + TR("(edited)");
        // ::SetWindowText(GetDlgItem(IDC_PROFILECOMBO), CurrentProfileName);
        m_ProfileChanged = true;
        UpdateProfileList();
    }
}

LRESULT CUploadSettings::OnProfileEditedCommand(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ProfileChanged();
   return 0;
}

LRESULT CUploadSettings::OnUserNameMenuItemClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int userNameIndex = wID - IDC_USERNAME_FIRST_ID;
    CString userName = menuOpenedUserNames_[userNameIndex];
    bool ImageServer = menuOpenedIsImageServer_;
    ServerProfile & serverProfile = ImageServer? getSessionImageServerItem() : getSessionFileServerItem();
    serverProfile.setProfileName(WCstringToUtf8(userName));

    BasicSettings* Settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* serverSettings = Settings->getServerSettings(serverProfile);

    serverProfile.setFolder(serverSettings ? serverSettings->defaultFolder : CFolderItem());
    /*if(UserName != ss.authData.Login || ss.authData.DoAuth!=prevAuthEnabled)
    {
        serverProfile.setFolderId("");
        serverProfile.setFolderTitle("");
        serverProfile.setFolderUrl("");
        iuPluginManager.UnloadPlugins();
        m_EngineList->DestroyCachedEngine(m_EngineList->byIndex(nServerIndex)->Name);
    }*/

    UpdateAllPlaceSelectors();
    return 0;
}

LRESULT CUploadSettings::OnAddAccountClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bool isImageServer = (wID % 2)!=0;

    ServerProfile & serverProfile = isImageServer ? getSessionImageServerItem() : getSessionFileServerItem();
    ServerProfile serverProfileCopy = serverProfile;
    serverProfileCopy.setProfileName("");
    CLoginDlg dlg(serverProfileCopy, uploadEngineManager_, true);
    UINT dlgResult = dlg.DoModal(m_hWnd);
    //ServerSettingsStruct & ss = ImageServer ? sessionImageServer_.serverSettings() : sessionFileServer_.serverSettings();
    if (dlgResult  != IDCANCEL) {
        serverProfileCopy.setProfileName(WCstringToUtf8(dlg.accountName()));
        serverProfileCopy.clearFolderInfo();
        serverProfile = serverProfileCopy;
        UpdateAllPlaceSelectors();
    }
    return 0;
}

LRESULT CUploadSettings::OnNoAccountClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bool ImageServer = (wID % 2)!=0;
    ServerProfile & serverProfile = ImageServer ? getSessionImageServerItem() : getSessionFileServerItem();
    serverProfile.setProfileName("");
    serverProfile.clearFolderInfo();
    UpdateAllPlaceSelectors();
    return 0;
}

void CUploadSettings::SaveCurrentProfile()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    CString saveToProfile = CurrentProfileName;
    if (CurrentProfileOriginalName == _T("Default"))
        saveToProfile = CurrentProfileOriginalName;

    if (!SaveParams(convert_profiles_[saveToProfile]))
        return;

   settings->DefaultImageUploadParams.ImageProfileName = W2U(saveToProfile);
}

bool CUploadSettings::OnHide()
{
   SaveCurrentProfile();
   return true;
}

LRESULT CUploadSettings::OnProfileComboSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    CString profile = GuiTools::GetWindowText(GetDlgItem(IDC_PROFILECOMBO));

    ShowParams(profile);
    UpdateProfileList();
   return 0;
}

ServerProfile& CUploadSettings::getSessionImageServerItem() {
    return sessionImageServer_.getByIndex(0);
}

ServerProfile& CUploadSettings::getSessionFileServerItem() {
    return sessionFileServer_.getByIndex(0);
}

LRESULT CUploadSettings::OnChooseMoreImageServersClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    CServerProfileGroupSelectDialog dlg(uploadEngineManager_, sessionImageServer_, CUploadEngineData::TypeImageServer | CUploadEngineData::TypeFileServer);
    if (dlg.DoModal(m_hWnd) == IDOK) {
        sessionImageServer_ = dlg.serverProfileGroup();

        UpdateAllPlaceSelectors();
    }
    return 0;
}

LRESULT CUploadSettings::OnChooseMoreFileServersClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    CServerProfileGroupSelectDialog dlg(uploadEngineManager_, sessionFileServer_, CUploadEngineData::TypeFileServer | CUploadEngineData::TypeVideoServer);
    if (dlg.DoModal(m_hWnd) == IDOK) {
        sessionFileServer_ = dlg.serverProfileGroup();

        UpdateAllPlaceSelectors();
    }
    return 0;
}

LRESULT CUploadSettings::OnOpenWebsite(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    bool ImageServer = (wID % 2) != 0;
    const ServerProfile& serverProfile = ImageServer ? getSessionImageServerItem() : getSessionFileServerItem();

    const CUploadEngineData* ue = serverProfile.uploadEngineData();
    if (ue && !ue->WebsiteUrl.empty()) {
        WinUtils::ShellOpenFileOrUrl(U2WC(ue->WebsiteUrl), m_hWnd);
    }
    return  0;
}

void CUploadSettings::updateMoreServersLink(bool isImageServer) {
    const ServerProfileGroup& serverGroup = isImageServer ? sessionImageServer_ : sessionFileServer_;
    CHyperLink& hyperLink = isImageServer ? moreImageServersLink_ : moreFileServersLink_;

    std::wstring text;
    if (serverGroup.getCount() == 1) {
        text = TR("Choose more servers...");
    } else {
        text = IuCoreUtils::Utf8ToWstring(str(IuStringUtils::FormatNoExcept(
                _nc("upload_settings_wizard_page.more_servers_link", "%d server selected in total...",
                                                      "%d servers selected in total...", serverGroup.getCount()
                ))
            % serverGroup.getCount()
        ));
    }

    hyperLink.SetLabel(text.c_str());
}
