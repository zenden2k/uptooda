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

#include "ServerSelectorControl.h"

#include <strsafe.h>

#include "Gui/Dialogs/WizardDlg.h"
#include "Gui/Dialogs/ServerListPopup.h"
#include "Gui/GuiTools.h"
#include "Gui/Dialogs/ServerParamsDlg.h"
#include "Gui/Dialogs/UploadParamsDlg.h"
#include "Func/WinUtils.h"
#include "Gui/IconBitmapUtils.h"
#include "Gui/Dialogs/LoginDlg.h"
#include "Gui/Dialogs/AddFtpServerDialog.h"
#include "Gui/Dialogs/AddDirectoryServerDialog.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/AbstractServerIconCache.h"
#include "Gui/Helpers/DPIHelper.h"
#include "Core/WinServerIconCache.h"

namespace {

constexpr TCHAR MENU_EXIT_NOTIFY[] = _T("MENU_EXIT_NOTIFY"), MENU_EXIT_COMMAND_ID[] = _T("MENU_EXIT_COMMAND_ID");

}

// CServerSelectorControl
CServerSelectorControl::CServerSelectorControl(UploadEngineManager* uploadEngineManager, bool defaultServer, bool isChildWindow, bool showServerIcons)
{
    showDefaultServerItem_ = false;
    serversMask_ = CUploadEngineData::TypeImageServer | CUploadEngineData::TypeFileServer;
    showImageProcessingParams_ = true;
    showParamsLink_ = true;
    defaultServer_ = defaultServer;
    iconBitmapUtils_ = std::make_unique<IconBitmapUtils>();
    uploadEngineManager_ = uploadEngineManager;
    isChildWindow_ = isChildWindow;
    showFileSizeLimits_ = false;
    hMyDlgTemplate_ = nullptr;
    isPopingUp_ = false;
    showEmptyItem_ = false;
    showServerIcons_ = showServerIcons;
    auto serviceLocator = ServiceLocator::instance();
    BasicSettings* settings = serviceLocator->basicSettings();
    iconCache_ = dynamic_cast<WinServerIconCache*>(serviceLocator->serverIconCache());
    profileListChangedConnection_ = settings->onProfileListChanged.connect([this](auto&& settings, auto&& servers) {
        profileListChanged(std::forward<decltype(settings)>(settings),
            std::forward<decltype(servers)>(servers)
        );
    } );
}

CServerSelectorControl::~CServerSelectorControl()
{
    if (hMyDlgTemplate_) {
        GlobalFree(hMyDlgTemplate_);
    }
}

void CServerSelectorControl::TranslateUI() {
}

LRESULT CServerSelectorControl::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TranslateUI();

    accountLink_.SubclassWindow(GetDlgItem(IDC_ACCOUNTINFO));
    accountLink_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER | HLINK_COMMANDBUTTON;
    accountLink_.m_clrLink = GuiTools::GetDefaultHyperlinkColor(accountLink_);
    accountLink_.SetToolTipText(TR("User name"));

    imageProcessingParamsLink_.SubclassWindow(GetDlgItem(IDC_IMAGEPROCESSINGPARAMS));
    imageProcessingParamsLink_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER | HLINK_COMMANDBUTTON;
    imageProcessingParamsLink_.m_clrLink = GuiTools::GetDefaultHyperlinkColor(imageProcessingParamsLink_);
    CString linkLabel = showImageProcessingParams_ ? TR("Image processing settings...") : TR("Settings...");
    imageProcessingParamsLink_.SetLabel(linkLabel);
    imageProcessingParamsLink_.SetToolTipText(linkLabel);
    imageProcessingParamsLink_.ShowWindow(showParamsLink_ ? SW_SHOW : SW_HIDE);

    createSettingsButton();
    setTitle(title_);
    serverGroupboxFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_SERVERGROUPBOX));
    userPictureControl_ = GetDlgItem(IDC_USERICON);
    folderPictureControl_ = GetDlgItem(IDC_FOLDERICON);
    serverButton_ = GetDlgItem(IDC_SERVERBUTTON);
    serverButton_.SetButtonStyle(BS_SPLITBUTTON);

    createResources();
    updateServerButton();
    updateInfoLabel();

    return FALSE;
}

LRESULT CServerSelectorControl::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (LOWORD(wParam)) {
        activationTime_ = GetTickCount64();
    }
    return 0;
}

LRESULT CServerSelectorControl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CServerSelectorControl::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createSettingsButton();
    createResources();
    updateServerButton();
    updateInfoLabel();
    return 0;
}

void CServerSelectorControl::setTitle(CString title) {
    if (m_hWnd){
        SetDlgItemText(IDC_SERVERGROUPBOX, title);
    }
    title_ = title;
}

CString CServerSelectorControl::getTitle() const {
    return title_;
}

void CServerSelectorControl::setServerProfile(const ServerProfile& serverProfile) {
    serverProfile_ = serverProfile;

    if ( !m_hWnd ) {
        return;
    }

    updateServerButton();
    updateInfoLabel();
}

ServerProfile CServerSelectorControl::serverProfile() const {
    return serverProfile_;
}

LRESULT CServerSelectorControl::OnClickedEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if (serverProfile_.isNull()) {
        return 0;
    }
    CServerParamsDlg serverParamsDlg(serverProfile_, uploadEngineManager_);

    if (serverParamsDlg.DoModal(m_hWnd) == IDOK) {
        serverProfile_ = serverParamsDlg.serverProfile();
        notifyChange();
    }

    updateInfoLabel();
    return 0;
}

void CServerSelectorControl::addAccount()
{
    ServerProfile serverProfileCopy = serverProfile_;
    serverProfileCopy.setProfileName("");
    CLoginDlg dlg(serverProfileCopy, uploadEngineManager_, true);

    if (dlg.DoModal(m_hWnd) != IDCANCEL)
    {
        serverProfileCopy.setProfileName(WCstringToUtf8(dlg.accountName()));
        serverProfileCopy.clearFolderInfo();
        serverProfile_ = serverProfileCopy;
        updateInfoLabel();
        notifyChange();
    }
}

void CServerSelectorControl::serverChanged() {
    const CUploadEngineData * uploadEngineData = nullptr;
    std::string serverName = serverProfile_.serverName();
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    if (!serverName.empty()) {
        CString serverNameW = Utf8ToWCstring( serverName );
        serverProfile_.setServerName(serverName);
        uploadEngineData = myEngineList->byName(serverNameW);
        if (!uploadEngineData) {
            return;
        }
        auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
        auto ssIt = settings->ServersSettings.find(serverName);
        if (ssIt != settings->ServersSettings.end()) {
            std::map<std::string, ServerSettingsStruct>& ss = ssIt->second;
            auto it = ss.begin();
            if (it->first.empty()) {
                ++it;
            }
            if (it != ss.end()) {
                ServerSettingsStruct& s = it->second;
                serverProfile_.setProfileName(s.authData.Login);
                serverProfile_.setFolder(s.defaultFolder);
            } else {
                serverProfile_.setProfileName("");
                serverProfile_.clearFolderInfo();
            }
        }
    } else {
        serverProfile_ = ServerProfile();
    }

    notifyChange();

    updateInfoLabel();
}

void CServerSelectorControl::updateInfoLabel() {
    std::string serverName = serverProfile_.serverName();
    currentUserName_.Empty();

    bool showServerParams = (serverName != CMyEngineList::DefaultServer && serverName != CMyEngineList::RandomServer );

    if (!showServerParams) {
        GuiTools::ShowDialogItem(m_hWnd, IDC_FOLDERLABEL, showServerParams );
        GuiTools::ShowDialogItem(m_hWnd, IDC_FOLDERICON, showServerParams );
        GuiTools::ShowDialogItem(m_hWnd, IDC_ACCOUNTINFO, showServerParams );
        GuiTools::ShowDialogItem(m_hWnd, IDC_USERICON, showServerParams );
        settingsButtonToolbar_.ShowWindow(SW_HIDE);
        return;
    }
    //GuiTools::ShowDialogItem(m_hWnd, IDC_ACCOUNTINFO, showServerParams);
//    GuiTools::ShowDialogItem(m_hWnd, IDC_EDIT, showServerParams);
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    const CUploadEngineData* uploadEngineData = myEngineList->byName(Utf8ToWCstring(serverName));
    if ( ! uploadEngineData ) {
        return;
    }

    showServerParams = uploadEngineData->UsingPlugin || uploadEngineData->NeedAuthorization;

    CString accountInfoText;
    BasicSettings* settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* res = settings->getServerSettings(serverProfile_);

    LoginInfo loginInfo = res ? res->authData : LoginInfo();

    if ( loginInfo.Login.empty() || (!loginInfo.DoAuth  && uploadEngineData->NeedAuthorization != 2 ) ) {
        accountInfoText += TR("Account...");
        accountLink_.SetToolTipText(TR("Enter account information"));
    } else {
        accountInfoText += Utf8ToWCstring(loginInfo.Login );
        currentUserName_  =  Utf8ToWCstring(loginInfo.Login );
    }
    CString folderTitle;
    if ( uploadEngineData->SupportsFolders ) {
         folderTitle = Utf8ToWCstring( serverProfile_.folderTitle() );
        //accountInfoText += CString(_T("\r\n")) + TR("Folder/album:") + _T(" ");
        /*if ( folderTitle.IsEmpty() ) {
            folderTitle = TR("not set");
        } */

    }
    SetDlgItemText(IDC_ACCOUNTINFO, accountInfoText);
    bool showAccount = uploadEngineData->NeedAuthorization != 0 && showServerParams;
    GuiTools::ShowDialogItem(m_hWnd, IDC_ACCOUNTINFO, showAccount);
    accountLink_.SetLabel(accountInfoText);
    SetDlgItemText(IDC_FOLDERLABEL, folderTitle);

    GuiTools::ShowDialogItem(m_hWnd, IDC_USERICON, showAccount);

    bool showFolder = !folderTitle.IsEmpty() && showServerParams;

    RECT rect;
    int settingsBtnPlaceHolderId = ( showFolder || showAccount ) ? IDC_SETTINGSBUTTONPLACEHOLDER : IDC_SETTINGSBUTTONPLACEHOLDER2;
    GetDlgItem(settingsBtnPlaceHolderId).GetWindowRect(&rect);
    ::MapWindowPoints(0, m_hWnd, reinterpret_cast<LPPOINT>(&rect), 2);
    settingsButtonToolbar_.SetWindowPos(GetDlgItem(IDC_FOLDERLABEL), rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
    settingsButtonToolbar_.ShowWindow(showServerParams ? SW_SHOW : SW_HIDE);

    GuiTools::ShowDialogItem(m_hWnd, IDC_FOLDERLABEL, showFolder );
    GuiTools::ShowDialogItem(m_hWnd, IDC_FOLDERICON, showFolder );
}

void CServerSelectorControl::setShowDefaultServerItem(bool show) {
    showDefaultServerItem_ = show;
}

void CServerSelectorControl::setServersMask(int mask) {
    serversMask_ = mask;
}

void CServerSelectorControl::setShowFilesizeLimits(bool show) {
    showFileSizeLimits_ = show;
}

void CServerSelectorControl::notifyChange()
{
    ::SendMessage(GetParent(), WM_SERVERSELECTCONTROL_CHANGE, reinterpret_cast<WPARAM>(m_hWnd), 0);
    if (onChangeCallback_)
    {
        onChangeCallback_(this);
    }
}

void CServerSelectorControl::notifyServerListChanged()
{
    ::SendMessage(GetParent(), WM_SERVERSELECTCONTROL_SERVERLIST_CHANGED, reinterpret_cast<WPARAM>(m_hWnd), 0);
}

void CServerSelectorControl::updateServerButton() {
    const UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    auto iconCache = ServiceLocator::instance()->serverIconCache();
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    const CUploadEngineData *ued = myEngineList->byName(serverProfile_.serverName());
    HICON serverIcon = ued ? iconCache->getIconForServer(ued->Name, dpi, true) : nullptr;
    serverButton_.SetWindowText(ued ? U2WC(myEngineList->getServerDisplayName(ued)) : TR("Choose server"));
    serverButton_.SetIcon(serverIcon);
}

bool CServerSelectorControl::isAccountChosen() const
{
    const CUploadEngineData* ued = serverProfile_.uploadEngineData();
    return !serverProfile_.profileName().empty() || (ued && ued->NeedAuthorization != CUploadEngineData::naObligatory);
}

LRESULT CServerSelectorControl::OnAccountClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if (serverProfile_.isNull()) {
        return 0;
    }
    CMenu sub;
    MENUITEMINFO mi;
    ZeroMemory(&mi,sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_TYPE | MIIM_ID;
    mi.fType = MFT_STRING;
    sub.CreatePopupMenu();
    const CUploadEngineData* uploadEngine = serverProfile_.uploadEngineData();

    if (!uploadEngine) {
        return 0;
    }
    const int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    std::map<std::string, ServerSettingsStruct>& serverUsers = settings->ServersSettings[serverProfile_.serverName()];
    HICON userIcon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICONUSER));
    CBitmap bmp = iconBitmapUtils_->HIconToBitmapPARGB32(userIcon, dpi);

    if (!serverUsers.empty() && (serverUsers.size() > 1 || serverUsers.find("") == serverUsers.end())) {
        bool addedSeparator = false;

        int i = 0;
        if (!serverUsers.empty() && !serverProfile_.profileName().empty()) {
            mi.wID = IDC_LOGINMENUITEM;
            CString name = TR("Edit login credentials...");
            mi.dwTypeData = const_cast<LPWSTR>(name.GetString());
            mi.cch = name.GetLength();
            sub.InsertMenuItem(i++, true, &mi);
        }
        else {
            addedSeparator = true;
        }

        menuOpenedUserNames_.clear();

        int command = IDC_USERNAME_FIRST_ID;
        for (auto it = serverUsers.begin(); it != serverUsers.end(); ++it) {
            CString login = Utf8ToWCstring(it->first);
            if (!login.IsEmpty())/*&& it->second.authData.DoAuth**/ {
                if (!addedSeparator) {
                    ZeroMemory(&mi,sizeof(mi));
                    mi.cbSize = sizeof(mi);
                    mi.fMask = MIIM_TYPE | MIIM_ID;
                    mi.wID = 1;
                    mi.fType = MFT_SEPARATOR;

                    sub.InsertMenuItem(i++, true, &mi);
                    addedSeparator = true;
                }
                ZeroMemory(&mi,sizeof(mi));
                mi.cbSize = sizeof(mi);

                mi.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
                mi.fType = MFT_STRING;
                mi.wID = command;

                mi.dwTypeData = const_cast<LPWSTR>(login.GetString());
                mi.cch = login.GetLength();
                mi.hbmpItem = bmp;
                if (mi.hbmpItem) {
                    mi.fMask |= MIIM_BITMAP;
                }
                menuOpenedUserNames_.push_back(login);
                sub.InsertMenuItem(i++, true, &mi);
                command++;
            }

        }
        if (uploadEngine->NeedAuthorization != CUploadEngineData::naObligatory) {
            ZeroMemory(&mi,sizeof(mi));
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
            mi.fType = MFT_STRING;
            mi.wID = IDC_NO_ACCOUNT;

            CString text = TR("<no authentication>");

            mi.dwTypeData = const_cast<LPWSTR>(text.GetString());
            mi.cch = text.GetLength();
            sub.InsertMenuItem(i++, true, &mi);
        }

        ZeroMemory(&mi,sizeof(mi));
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_TYPE | MIIM_ID;
        mi.wID = 1;
        mi.fType = MFT_SEPARATOR;

        sub.InsertMenuItem(i++, true, &mi);

        ZeroMemory(&mi,sizeof(mi));
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
        mi.fType = MFT_STRING;
        mi.wID = IDC_ADD_ACCOUNT ;

        CString text = TR("Add account...");

        mi.dwTypeData = const_cast<LPWSTR>(text.GetString());
        mi.cch = text.GetLength();

        sub.InsertMenuItem(i++, true, &mi);
        sub.SetMenuDefaultItem(0,TRUE);
    }
    else {
        addAccount();
        return 0;
    }

    RECT rc = {0,0,0,0};
    ::GetClientRect(GetDlgItem(IDC_ACCOUNTINFO), &rc);
    ::ClientToScreen(GetDlgItem(IDC_ACCOUNTINFO), reinterpret_cast<LPPOINT>(&rc));
    ::ClientToScreen(GetDlgItem(IDC_ACCOUNTINFO), 1 + reinterpret_cast<LPPOINT>(&rc));

    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;
    sub.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, m_hWnd, &excludeArea);
    updateInfoLabel();
    return 0;
}

LRESULT CServerSelectorControl::OnAddAccountClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    addAccount();
    return 0;
}

LRESULT CServerSelectorControl::OnLoginMenuItemClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BasicSettings* settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* serverSettings = settings->getServerSettings(serverProfile_);
    std::string UserName = serverSettings ? serverSettings->authData.Login: std::string();
    ServerProfile copy = serverProfile_;
    CLoginDlg dlg(copy, uploadEngineManager_);
    if (dlg.DoModal(m_hWnd) != IDCANCEL) {
        copy.setProfileName(WCstringToUtf8(dlg.accountName()));
        if(Utf8ToWCstring(UserName) != dlg.accountName())
        {
            copy.setFolderId("");
            copy.setFolderTitle("");
            copy.setFolderUrl("");
            serverProfile_.setParentIds({});
            serverProfile_ = copy;
            //iuPluginManager.UnloadPlugins();
            //_EngineList->DestroyCachedEngine(WCstringToUtf8(serverProfile_.serverName()), WCstringToUtf8(serverProfile_.profileName()));
        }
        notifyChange();
        updateInfoLabel();
    }
    return 0;
}

LRESULT CServerSelectorControl::OnNoAccountClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    serverProfile_.setProfileName("");
    serverProfile_.clearFolderInfo();
    updateInfoLabel();
    notifyChange();
    return 0;
}

LRESULT CServerSelectorControl::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return MA_NOACTIVATE;
}

void CServerSelectorControl::createSettingsButton() {
    const UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);
    CIcon ico;
    ico.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONSETTINGS), iconWidth, iconHeight);
    RECT profileRect;
    ::GetWindowRect(GetDlgItem(IDC_SETTINGSBUTTONPLACEHOLDER), &profileRect);
    ScreenToClient(&profileRect);


    if (!settingsButtonToolbar_) {
        settingsButtonToolbar_.Create(m_hWnd, profileRect, _T(""), WS_CHILD | WS_VISIBLE | WS_CHILD | WS_TABSTOP | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_NORESIZE | CCS_RIGHT | /*CCS_BOTTOM |CCS_ADJUSTABLE|*/ TBSTYLE_TOOLTIPS | CCS_NODIVIDER | TBSTYLE_AUTOSIZE);
        settingsButtonToolbar_.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
        settingsButtonToolbar_.SetButtonStructSize();
        //settingsButtonToolbar_.SetButtonSize(iconWidth + 1, iconWidth + 1);
    }
    GuiTools::DeleteAllToolbarButtons(settingsButtonToolbar_);

    if (settingsButtonImageList_) {
        settingsButtonToolbar_.SetImageList(nullptr);
        settingsButtonImageList_.Destroy();
    }
    settingsButtonImageList_.Create(iconWidth, iconWidth, ILC_COLOR32, 0, 6);
    settingsButtonImageList_.AddIcon(ico);
    settingsButtonToolbar_.SetImageList(settingsButtonImageList_);
    settingsButtonToolbar_.AddButton(IDC_EDIT, TBSTYLE_BUTTON |BTNS_AUTOSIZE, TBSTATE_ENABLED, 0,TR("Server and authentication settings"), 0);
}

void CServerSelectorControl::createResources() {
    const UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    const int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    if (iconUser_) {
        iconUser_.DestroyIcon();
    }
    iconUser_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONUSER), iconWidth, iconHeight);

    userPictureControl_.SetIcon(iconUser_);
    userPictureControl_.SetWindowPos(0, 0, 0, iconWidth, iconHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    if (iconFolder_) {
        iconFolder_.DestroyIcon();
    }
    iconFolder_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONFOLDER2), iconWidth, iconHeight);

    folderPictureControl_.SetIcon(iconFolder_);
    folderPictureControl_.SetWindowPos(0, 0, 0, iconWidth, iconHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void CServerSelectorControl::profileListChanged(BasicSettings* settings, const std::vector<std::string>& affectedServers) {
    if (!serverProfile_.profileName().empty()) {
        ServerSettingsStruct* serverSettings = settings->getServerSettings(serverProfile_);
        if (!serverSettings) {
            serverProfile_.setProfileName({});
            serverProfile_.setParentIds({});
            serverProfile_.clearFolderInfo();
        }
    }

    updateInfoLabel();
}

void CServerSelectorControl::setShowImageProcessingParams(bool show) {
    showImageProcessingParams_ = show;
}

void CServerSelectorControl::setShowParamsLink(bool show) {
    showParamsLink_ = show;
}

void CServerSelectorControl::setOnChangeCallback(std::function<void(CServerSelectorControl*)> cb) {
    onChangeCallback_ = std::move(cb);
}

LRESULT CServerSelectorControl::OnImageProcessingParamsClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    CUploadParamsDlg dlg(serverProfile_, showImageProcessingParams_, defaultServer_);
    if (dlg.DoModal(m_hWnd) == IDOK) {
        serverProfile_.setImageUploadParams(dlg.imageUploadParams());
    }
    return 0;
}

LRESULT CServerSelectorControl::OnUserNameMenuItemClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int userNameIndex = wID - IDC_USERNAME_FIRST_ID;
    CString userName = menuOpenedUserNames_[userNameIndex];
    serverProfile_.setProfileName(WCstringToUtf8(userName));

    BasicSettings* Settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* serverSettings = Settings->getServerSettings(serverProfile_);

    serverProfile_.setFolder(serverSettings ? serverSettings->defaultFolder : CFolderItem {});
    notifyChange();
    updateInfoLabel();
    return 0;
}

int CServerSelectorControl::showPopup(HWND parent, const RECT& anchorRect, bool topMost) {
    // Code from \Program Files\Microsoft SDKs\Windows\v7.1\Samples\winui\shell\legacysamples\fakemenu\fakemenu.cpp
    isChildWindow_ = false;
    if (Create(parent) == NULL) {
        ATLTRACE(_T("CServerSelectorControl dialog creation failed!  :( sorry\n"));
        return 0;
    }
    int nRet(-1);
    CRect windowRect;
    GetWindowRect(windowRect);
    int popupWidth = windowRect.Width();
    int popupHeight = windowRect.Height();
    
    int x = anchorRect.left;
    int y = anchorRect.bottom + 2;

    HMONITOR hMonitor = MonitorFromRect(windowRect, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { 0 };
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(hMonitor, &mi)) {
        // Check monitor work area boundaries
        RECT workArea = mi.rcWork;
        // Adjust horizontal position
        if (x + popupWidth > workArea.right) {
            // Doesn't fit on the right - shift left
            x = anchorRect.right - popupWidth;
            // If still doesn't fit, align to screen right edge
            if (x < workArea.left) {
                x = workArea.right - popupWidth;
            }
        }
        // Check that window doesn't go beyond left boundary
        if (x < workArea.left) {
            x = workArea.left;
        }
        // Adjust vertical position
        if (y + popupHeight > workArea.bottom) {
            // Doesn't fit below - show above the button
            y = anchorRect.top - popupHeight;
            // If doesn't fit above either
            if (y < workArea.top) {
                // Place next to button on the right
                if (anchorRect.right + popupWidth <= workArea.right) {
                    x = anchorRect.right;
                    y = anchorRect.top;
                }
                // Or on the left
                else if (anchorRect.left - popupWidth >= workArea.left) {
                    x = anchorRect.left - popupWidth;
                    y = anchorRect.top;
                }
                // As last resort - center in work area
                else {
                    x = workArea.left + (workArea.right - workArea.left - popupWidth) / 2;
                    y = workArea.top + (workArea.bottom - workArea.top - popupHeight) / 2;
                }
            }
        }
        // Final boundary check
        if (x < workArea.left) {
            x = workArea.left;
        }
        if (y < workArea.top) {
            y = workArea.top;
        }
        if (x + popupWidth > workArea.right) {
            x = workArea.right - popupWidth;
        }
        if (y + popupHeight > workArea.bottom) {
            y = workArea.bottom - popupHeight;
        }
    }

    SetWindowPos(topMost ? HWND_TOPMOST : 0, x, y, 0, 0, SWP_NOACTIVATE | (topMost ? 0 : SWP_NOZORDER) | SWP_NOSIZE);
    ShowWindow(topMost ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE);

    //BOOL bMenuDestroyed(FALSE);
    HWND hwndOwner = GetWindow(GW_OWNER);
    HWND hwndPopup = m_hWnd;
    // We want to receive all mouse messages, but since only the active
    // window can capture the mouse, we have to set the capture to our
    // owner window, and then steal the mouse messages out from under it.
    //::SetCapture(hwndOwner);

    // Go into a message loop that filters all the messages it receives
    // and route the interesting ones to the color picker window.
    MSG msg;
    POINT pt {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // If our owner stopped being the active window (e.g. the user
        // Alt+Tab'd to another window in the meantime), then stop.
        HWND hwndActive = GetActiveWindow();
        if (hwndActive != hwndPopup && hwndActive != hwndOwner && !::IsChild(hwndActive, hwndOwner) /*||
            GetCapture() != hwndOwner*/)
        {
            break;
        }


        bool isChildMessage = ::IsChild(hwndPopup, msg.hwnd)!=FALSE;
        if (!isChildMessage)
        {
            TCHAR className[MAX_PATH];
            if (::GetClassName(msg.hwnd, className, MAX_PATH) != 0)
            {
                isChildMessage = lstrcmp(className, _T("ComboLBox"))==0; // Style of ComboboxEx popup list box
            }
        }
        bool breakLoop = false;
        // At this point, we get to snoop at all input messages before
        // they get dispatched.  This allows us to route all input to our
        // popup window even if really belongs to somebody else.

        // All mouse messages are remunged and directed at our popup
        // menu. If the mouse message arrives as client coordinates, then
        // we have to convert it from the client coordinates of the original
        // target to the client coordinates of the new target.
        switch (msg.message)
        {
            // These mouse messages arrive in client coordinates, so in
            // addition to stealing the message, we also need to convert the
            // coordinates.

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
            if ((msg.hwnd == hwndOwner || ::IsChild(hwndOwner, msg.hwnd)) && (GetTickCount64() - activationTime_ > 400))
            {
                breakLoop = true;
            }
            break;
        case WM_MOUSEMOVE:
            if (!isChildMessage) {
                pt.x = (short)LOWORD(msg.lParam);
                pt.y = (short)HIWORD(msg.lParam);
                ::MapWindowPoints(msg.hwnd, hwndPopup, &pt, 1);
                msg.lParam = MAKELPARAM(pt.x, pt.y);
                msg.hwnd = hwndPopup;
            }
            break;

            // These mouse messages arrive in screen coordinates, so we just
            // need to steal the message.

        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
        case WM_NCMBUTTONDBLCLK:
        case WM_SETCURSOR:
            if (msg.hwnd == hwndOwner || ::IsChild(hwndOwner, msg.hwnd))
            {
                breakLoop = true;
                break;
            }
        case WM_NCMOUSEMOVE:
            if (!isChildMessage) {
                msg.hwnd = hwndPopup;
            }
            break;

            // We need to steal all keyboard messages, too.
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
            if (!isChildMessage)
            {
                msg.hwnd = hwndPopup;
            }
            if (msg.wParam == VK_ESCAPE && (msg.hwnd == hwndPopup || ::IsChild(hwndPopup, msg.hwnd)))
            {
                breakLoop = true;
                break;
            }
            break;
        }
        if (breakLoop)
        {
            break;
        }
        if (!::IsDialogMessage(hwndPopup, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }


        // If our owner stopped being the active window (e.g. the user
        // Alt+Tab'd to another window in the meantime), then stop.
        hwndActive = GetActiveWindow();
        if (hwndActive != hwndPopup && hwndActive != hwndOwner && !::IsChild(hwndActive, hwndOwner)/* ||
            GetCapture() != hwndOwner*/)
        {
            break;
        }
    }

    // Clean up the capture we created.
    //ReleaseCapture();

    isPopingUp_ = false;
    // If we got a WM_QUIT message, then re-post it so the caller's message
    // loop will see it.
    if (msg.message == WM_QUIT)
    {
        PostQuitMessage((int)msg.wParam);
    }


    //if (!bMenuDestroyed)
    DestroyWindow();

    return nRet;
}

bool CServerSelectorControl::exitPopup(int nCommandId)
{
    BOOL bRet = SetProp(m_hWnd, MENU_EXIT_NOTIFY, (HANDLE)1);
    SetProp(m_hWnd, MENU_EXIT_COMMAND_ID, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(nCommandId)));
    return bRet !=FALSE;
}

DLGTEMPLATE* CServerSelectorControl::GetTemplate()
{
    HINSTANCE hInst = GetModuleHandle(0);
    HRSRC res = FindResource(hInst, MAKEINTRESOURCE(IDD), RT_DIALOG);
    DLGTEMPLATE* dit = reinterpret_cast<DLGTEMPLATE*>(LockResource(LoadResource(hInst, res)));

    unsigned long sizeDlg = ::SizeofResource(hInst, res);
    hMyDlgTemplate_ = ::GlobalAlloc(GPTR, sizeDlg);
    auto pMyDlgTemplate = reinterpret_cast<ATL::_DialogSplitHelper::DLGTEMPLATEEX*>(::GlobalLock(hMyDlgTemplate_));
    ::memcpy(pMyDlgTemplate, dit, sizeDlg);

    if (isChildWindow_)
    {
        //pMyDlgTemplate->style = pMyDlgTemplate->style & ~ WS_POPUP;
        pMyDlgTemplate->style = pMyDlgTemplate->style | WS_CHILD;
    }
    else
    {
        pMyDlgTemplate->style -= WS_CHILD;
        pMyDlgTemplate->style -= DS_CONTROL;
        pMyDlgTemplate->exStyle |= WS_EX_NOACTIVATE |
            WS_EX_TOOLWINDOW |      // So it doesn't show up in taskbar
            WS_EX_DLGMODALFRAME |   // Get the edges right
            WS_EX_WINDOWEDGE /*|
            WS_EX_TOPMOST*/; // with this style window is overlapping modal dialogs (server params dialog, add ftp server, etc...)

        pMyDlgTemplate->style = pMyDlgTemplate->style | WS_POPUP | WS_BORDER/* | WS_CAPTION*/ ;
    }
    if (ServiceLocator::instance()->translator()->isRTL()) {
        pMyDlgTemplate->exStyle |= WS_EX_LAYOUTRTL | WS_EX_RTLREADING;
    }
    return reinterpret_cast<DLGTEMPLATE*>(pMyDlgTemplate);
}

void CServerSelectorControl::setShowEmptyItem(bool show) {
    showEmptyItem_ = show;
}

LRESULT CServerSelectorControl::OnEnable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (!isChildWindow_) {
        // Disable wizard window when a modal window is shown
        ::EnableWindow(GetParent(), wParam);
    }

    return 0;
}

LRESULT CServerSelectorControl::OnBnClickedServerButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    showServerButtonPopup();
    return 0;
}

LRESULT CServerSelectorControl::OnBnDropdownServerButton(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    showServerButtonPopup();
    return 0;
}

void CServerSelectorControl::showServerButtonPopup() {
    RECT buttonRect;
    serverButton_.GetWindowRect(&buttonRect);

    CString serverName = U2W(serverProfile_.serverName());
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    int serverIndex = myEngineList->getUploadEngineIndex(serverName);
    
    CServerListPopup serverListPopup(myEngineList, iconCache_, serversMask_, CUploadEngineListBase::ALL_SERVERS, serverIndex);

    if (serverListPopup.showPopup(m_hWnd, buttonRect) == IDOK) {
        int newServerIndex = serverListPopup.serverIndex();
        if (newServerIndex != -1) {
            const CUploadEngineData * ued = myEngineList->byIndex(newServerIndex);
            if (ued) {
                serverProfile_.setServerName(ued->Name);
            }
            updateServerButton();
            serverChanged();
        }
    };
}
