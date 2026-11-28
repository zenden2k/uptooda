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

#include "ServerParamsDlg.h"

#include <boost/format.hpp>
#include <utility>

#include "Core/AbstractServerIconCache.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/TaskDispatcher.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Core/Upload/UploadManager.h"
#include "Core/i18n/Translator.h"
#include "Gui/Dialogs/ServerFolderSelect.h"
#include "Gui/GuiTools.h"
#include "Gui/Helpers/DPIHelper.h"

CServerParamsDlg::CServerParamsDlg(const ServerProfile& serverProfile, UploadEngineManager* uploadEngineManager,
                                   Mode mode) :
    serverProfile_(serverProfile),
    originalProfileName_(mode == Mode::ADD_ACCOUNT ? std::string { } : serverProfile.profileName()),
    m_ue(serverProfile.uploadEngineData()), uploadEngineManager_(uploadEngineManager), mode_(mode) {
    if (mode_ == Mode::ADD_ACCOUNT) {
        serverProfile_.setProfileName({ });
        serverProfile_.clearFolderInfo();
    }
}

LRESULT CServerParamsDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (!m_ue) {
        EndDialog(IDABORT);
        return 0;
    }

    CenterWindow(GetParent());
    TRC(IDCANCEL, "Cancel");
    TRC(IDOK, "OK");
    TRC(IDC_DOAUTH, "Authorize");
    TRC(IDC_FOLDERLABEL, "Folder/album:");
    TRC(IDC_BROWSESERVERFOLDERS, "Select...");
    TRC(IDC_PARAMETERSLABEL, "Parameters:");
    TRC(IDC_AUTHORIZEBUTTON, "Sign in...");
    TRC(IDC_LOGOUT, "Logout");
    TRC(IDC_DELETEACCOUNTLABEL, "Delete account");

    CString loginLabel = m_ue->LoginLabel.empty() ? CString(TR("Login:")) : CString(U2WC(m_ue->LoginLabel)) + L":";
    CString passwordLabel
        = m_ue->PasswordLabel.empty() ? CString(TR("Password:")) : CString(U2WC(m_ue->PasswordLabel)) + L":";
    SetDlgItemText(IDC_LOGINLABEL, loginLabel);
    SetDlgItemText(IDC_PASSWORDLABEL, passwordLabel);

    const auto serverDisplayName = Utf8ToWCstring(CUploadEngineListBase::getServerDisplayName(m_ue));
    CString windowTitle;
    windowTitle.Format(TR("%s server settings"), serverDisplayName.GetString());
    SetWindowText(windowTitle);
    SetDlgItemText(IDC_LOGINFRAME, serverDisplayName);

    deleteAccountLink_.SubclassWindow(GetDlgItem(IDC_DELETEACCOUNTLABEL));
    deleteAccountLink_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER | HLINK_COMMANDBUTTON;
    deleteAccountLink_.m_clrLink = GuiTools::GetDefaultHyperlinkColor(deleteAccountLink_);
    deleteAccountLink_.SetToolTipText(TR("Remove this account from the list (not from the remote server)"));
    deleteAccountLink_.ShowWindow(originalProfileName_.empty() ? SW_HIDE : SW_SHOW);

    if (!m_ue->WebsiteUrl.empty()) {
        websiteLink_.SubclassWindow(GetDlgItem(IDC_WEBSITELINK));
        websiteLink_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;
        websiteLink_.m_clrLink = GuiTools::GetDefaultHyperlinkColor(websiteLink_);
        websiteLink_.SetLabel(TR("Open the website"));
        websiteLink_.SetHyperLink(U2W(m_ue->WebsiteUrl));
        websiteLink_.ShowWindow(SW_SHOW);
    }
    if (!m_ue->RegistrationUrl.empty()) {
        signupLink_.SubclassWindow(GetDlgItem(IDC_SIGNUPLINK));
        signupLink_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;
        signupLink_.m_clrLink = GuiTools::GetDefaultHyperlinkColor(signupLink_);
        const auto linkText = str(boost::wformat(TR("Don't have an account? Sign up on %s right now!"))
                                  % serverDisplayName.GetString());
        signupLink_.SetLabel(linkText.c_str());
        signupLink_.SetHyperLink(U2W(m_ue->RegistrationUrl));
        signupLink_.ShowWindow(SW_SHOW);
    }

    animation_.SubclassWindow(GetDlgItem(IDC_ANIMATIONSTATIC));
    createResources();

    for (const auto id : { IDC_BROWSESERVERFOLDERS, IDC_FOLDERLABEL, IDC_FOLDERNAMELABEL, IDC_FOLDERICON }) {
        GuiTools::ShowDialogItem(m_hWnd, id, m_ue->SupportsFolders);
    }
    const bool canAuthorize = m_ue->NeedAuthorization != CUploadEngineData::naNotAvailable;
    for (const auto id : { IDC_LOGINLABEL, IDC_LOGINEDIT, IDC_PASSWORDLABEL, IDC_PASSWORDEDIT }) {
        GuiTools::ShowDialogItem(m_hWnd, id, canAuthorize);
    }
    GuiTools::ShowDialogItem(m_hWnd, IDC_DOAUTH, m_ue->NeedAuthorization == CUploadEngineData::naAvailable);

    m_wndParamList.SubclassWindow(GetDlgItem(IDC_PARAMLIST));
    m_wndParamList.SetExtendedListStyle(PLS_EX_SHOWSELALWAYS | PLS_EX_SINGLECLICKEDIT);
    parameterListAdapter_ = std::make_unique<ParameterListAdapter>(&m_paramNameList, &m_wndParamList);

    if (m_ue->UsingPlugin || !m_ue->Engine.empty()) {
        auto engine
            = std::dynamic_pointer_cast<CAdvancedUploadEngine>(uploadEngineManager_->getUploadEngine(serverProfile_));
        if (engine) {
            engine->getServerParamList(m_paramNameList);
            supportsSignIn_ = engine->supportsBeforehandAuthorization();
            supportsLogout_ = engine->supportsLogout();
            isAuthenticated_ = mode_ != Mode::ADD_ACCOUNT && engine->isAuthenticated();
        }
    }

    auto* settings = ServiceLocator::instance()->basicSettings()->getServerSettings(serverProfile_);
    auto initialSettings = settings && mode_ != Mode::ADD_ACCOUNT ? *settings : ServerSettingsStruct { };
    parameterListAdapter_->updateControl(&initialSettings);
    const auto& initialLogin
        = initialSettings.authData.Login.empty() ? originalProfileName_ : initialSettings.authData.Login;
    SetDlgItemText(IDC_LOGINEDIT, Utf8ToWCstring(initialLogin));
    SetDlgItemText(IDC_PASSWORDEDIT, Utf8ToWCstring(initialSettings.authData.Password));
    GuiTools::SetCheck(m_hWnd, IDC_DOAUTH, initialSettings.authData.DoAuth || mode_ != Mode::SETTINGS);
    updateFolderLabel();
    updateControls();
    DlgResize_Init();

    if (mode_ != Mode::SETTINGS && authorizationEnabled()) {
        ::SetFocus(GetDlgItem(IDC_LOGINEDIT));
        SendDlgItemMessage(IDC_LOGINEDIT, EM_SETSEL, 0, -1);
        return FALSE;
    }
    return TRUE;
}

LRESULT CServerParamsDlg::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (m_ue) {
        createResources();
    }
    return 0;
}

void CServerParamsDlg::createResources() {
    const UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    CStatic folderPicture(GetDlgItem(IDC_FOLDERICON));
    folderPicture.SetIcon(nullptr);
    if (iconFolder_) {
        iconFolder_.DestroyIcon();
    }
    const int width = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int height = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);
    iconFolder_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONFOLDER2), width, height);
    folderPicture.SetIcon(iconFolder_);
    folderPicture.SetWindowPos(nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    CStatic serverPicture(GetDlgItem(IDC_SERVERICON));
    serverPicture.SetIcon(nullptr);
    if (serverIcon_) {
        serverIcon_.DestroyIcon();
    }
    if (auto* iconCache = ServiceLocator::instance()->serverIconCache()) {
        serverIcon_.Attach(iconCache->getBigIconForServer(m_ue->Name, dpi));
        serverPicture.SetIcon(serverIcon_);
    }
}

bool CServerParamsDlg::authorizationEnabled() const {
    return m_ue->NeedAuthorization == CUploadEngineData::naObligatory
        || (m_ue->NeedAuthorization == CUploadEngineData::naAvailable && GuiTools::GetCheck(m_hWnd, IDC_DOAUTH));
}

void CServerParamsDlg::updateControls() {
    const bool busy = currentTask_ != nullptr;
    const bool authorize = authorizationEnabled();
    const auto login = GuiTools::GetDlgItemText(m_hWnd, IDC_LOGINEDIT);
    const bool authenticated = isAuthenticated_ && !originalProfileName_.empty() && W2U(login) == originalProfileName_;

    GuiTools::EnableDialogItem(m_hWnd, IDC_DOAUTH, !busy);
    GuiTools::EnableDialogItem(m_hWnd, IDC_LOGINEDIT, !busy && authorize);
    GuiTools::EnableDialogItem(m_hWnd, IDC_PASSWORDEDIT, !busy && authorize && m_ue->NeedPassword);
    GuiTools::EnableDialogItem(m_hWnd, IDC_PASSWORDLABEL, !busy && authorize && m_ue->NeedPassword);
    GuiTools::EnableDialogItem(m_hWnd, IDC_BROWSESERVERFOLDERS,
                               !busy && m_ue->SupportsFolders && (!authorize || !login.IsEmpty()));
    for (const auto id : { IDC_AUTHORIZEBUTTON, IDC_LOGOUT, IDC_DELETEACCOUNTLABEL, IDC_PARAMLIST, IDOK }) {
        GuiTools::EnableDialogItem(m_hWnd, id, !busy);
    }
    GuiTools::ShowDialogItem(m_hWnd, IDC_AUTHORIZEBUTTON,
                             authorize && supportsSignIn_ && !authenticated && !login.IsEmpty());
    GuiTools::ShowDialogItem(m_hWnd, IDC_LOGOUT, authorize && supportsLogout_ && authenticated);
    animation_.ShowWindow(busy ? SW_SHOW : SW_HIDE);
}

void CServerParamsDlg::updateFolderLabel() {
    const auto title = Utf8ToWCstring(serverProfile_.folderTitle());
    SetDlgItemText(IDC_FOLDERNAMELABEL, title.IsEmpty() ? CString(L"<") + TR("not selected") + L">" : title);
}

LoginInfo CServerParamsDlg::readLogin() const {
    LoginInfo login;
    login.DoAuth = authorizationEnabled();
    if (login.DoAuth) {
        login.Login = W2U(GuiTools::GetDlgItemText(m_hWnd, IDC_LOGINEDIT));
        login.Password = W2U(GuiTools::GetDlgItemText(m_hWnd, IDC_PASSWORDEDIT));
    } else if (m_ue->NeedAuthorization == CUploadEngineData::naAvailable) {
        login.Login = originalProfileName_;
        if (!login.Login.empty()) {
            if (W2U(GuiTools::GetDlgItemText(m_hWnd, IDC_LOGINEDIT)) == login.Login) {
                login.Password = W2U(GuiTools::GetDlgItemText(m_hWnd, IDC_PASSWORDEDIT));
            } else {
                auto profile = serverProfile_;
                profile.setProfileName(login.Login);
                const auto* settings = ServiceLocator::instance()->basicSettings()->getServerSettings(profile);
                if (settings) {
                    login.Password = settings->authData.Password;
                }
            }
        }
    }
    return login;
}

bool CServerParamsDlg::validateLogin(const LoginInfo& login) {
    if ((login.DoAuth || mode_ == Mode::ADD_ACCOUNT) && login.Login.empty()) {
        const auto label = m_ue->LoginLabel.empty() ? _("Login") : m_ue->LoginLabel;
        const auto message = str(IuStringUtils::FormatNoExcept(_("The field '%s' cannot be empty.")) % label);
        LocalizedMessageBox(U2WC(message), TR("Error"), MB_ICONERROR);
        return false;
    }

    if (login.DoAuth && login.Login != originalProfileName_
        && (!pendingProfile_ || pendingProfile_->profileName() != login.Login)) {
        auto profile = serverProfile_;
        profile.setProfileName(login.Login);
        if (ServiceLocator::instance()->basicSettings()->getServerSettings(profile)) {
            LocalizedMessageBox(TR("Account with such name already exists."), TR("Error"), MB_ICONERROR);
            return false;
        }
    }
    return true;
}

ServerSettingsStruct* CServerParamsDlg::applySettings(const LoginInfo& login) {
    if (pendingProfile_ && pendingProfile_->profileName() != login.Login) {
        rollbackSettings();
    }
    if (serverProfile_.profileName() != login.Login) {
        serverProfile_.setProfileName(login.Login);
    }
    auto* settings = ServiceLocator::instance()->basicSettings();
    auto* serverSettings = settings->getServerSettings(serverProfile_);
    if (!pendingProfile_) {
        // Folder browsing and authentication read the account settings through the upload engine.
        if (serverSettings) {
            previousSettings_ = *serverSettings;
        } else {
            serverSettings = settings->getServerSettings(serverProfile_, true);
        }
        pendingProfile_ = serverProfile_;
    }
    if (serverSettings->authData.DoAuth != login.DoAuth || serverSettings->authData.Login != login.Login
        || serverSettings->authData.Password != login.Password) {
        serverSettings->authData = login;
        uploadEngineManager_->resetAuthorization(serverProfile_);
    }
    parameterListAdapter_->saveValues(serverSettings);
    return serverSettings;
}

void CServerParamsDlg::rollbackSettings() {
    if (!pendingProfile_) {
        return;
    }
    auto* settings = ServiceLocator::instance()->basicSettings();
    if (previousSettings_) {
        *settings->getServerSettings(*pendingProfile_, true) = std::move(*previousSettings_);
    } else {
        uploadEngineManager_->unloadUploadEngines(pendingProfile_->serverName(), pendingProfile_->profileName());
        settings->deleteProfile(pendingProfile_->serverName(), pendingProfile_->profileName());
    }
    uploadEngineManager_->resetAuthorization(*pendingProfile_);
    previousSettings_.reset();
    pendingProfile_.reset();
}

void CServerParamsDlg::accept() {
    const auto login = readLogin();
    if (currentTask_ || !validateLogin(login)) {
        return;
    }
    applySettings(login);
    commitSettings();
}

void CServerParamsDlg::commitSettings() {
    previousSettings_.reset();
    pendingProfile_.reset();
    if (!originalProfileName_.empty() && serverProfile_.profileName() != originalProfileName_) {
        uploadEngineManager_->unloadUploadEngines(serverProfile_.serverName(), originalProfileName_);
        ServiceLocator::instance()->basicSettings()->deleteProfile(serverProfile_.serverName(), originalProfileName_);
    }
    EndDialog(IDOK);
}

LRESULT CServerParamsDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    accept();
    return 0;
}

LRESULT CServerParamsDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if (auto task = currentTask_) {
        task->stop();
    } else {
        rollbackSettings();
        EndDialog(IDCANCEL);
    }
    return 0;
}

LRESULT CServerParamsDlg::OnClickedDoAuth(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    serverProfile_.clearFolderInfo();
    updateFolderLabel();
    updateControls();
    return 0;
}

LRESULT CServerParamsDlg::OnLoginEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if (::GetFocus() == hWndCtl) {
        serverProfile_.clearFolderInfo();
        updateFolderLabel();
        TRC(IDC_AUTHORIZEBUTTON, "Sign in...");
        updateControls();
    }
    return 0;
}

LRESULT CServerParamsDlg::OnBrowseServerFolders(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    const auto login = readLogin();
    if (currentTask_ || !validateLogin(login)) {
        return 0;
    }
    auto* settings = applySettings(login);
    CServerFolderSelect folderSelectDlg(serverProfile_, uploadEngineManager_);
    folderSelectDlg.m_SelectedFolder = serverProfile_.folder();

    if (folderSelectDlg.DoModal(m_hWnd) == IDOK) {
        const auto& folder = folderSelectDlg.m_SelectedFolder;
        if (folder.id.empty()) {
            serverProfile_.clearFolderInfo();
        } else {
            serverProfile_.setFolder(folder);
        }
        updateFolderLabel();
    }

    auto engine
        = std::dynamic_pointer_cast<CAdvancedUploadEngine>(uploadEngineManager_->getUploadEngine(serverProfile_));
    if (engine) {
        m_wndParamList.ResetContent();
        m_paramNameList.clear();
        engine->getServerParamList(m_paramNameList);
        parameterListAdapter_->updateControl(settings);
        isAuthenticated_ = serverProfile_.profileName() == originalProfileName_ && engine->isAuthenticated();
        updateControls();
    }
    return 0;
}

LRESULT CServerParamsDlg::onDeleteAccountClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if (currentTask_ || originalProfileName_.empty()) {
        return 0;
    }
    const auto text = str(
        IuStringUtils::FormatWideNoExcept(TR("Are you sure you want to delete this account from %s's internal list?"))
        % APP_NAME);
    if (LocalizedMessageBox(text.c_str(), APP_NAME, MB_ICONQUESTION | MB_YESNO) == IDYES) {
        rollbackSettings();
        uploadEngineManager_->unloadUploadEngines(serverProfile_.serverName(), originalProfileName_);
        ServiceLocator::instance()->basicSettings()->deleteProfile(serverProfile_.serverName(), originalProfileName_);
        serverProfile_.setProfileName({ });
        serverProfile_.clearFolderInfo();
        EndDialog(ID_DELETEACCOUNT);
    }
    return 0;
}

LRESULT CServerParamsDlg::onSignInClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    startAuthentication(AuthActionType::Login);
    return 0;
}

LRESULT CServerParamsDlg::onLogoutClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    startAuthentication(AuthActionType::Logout);
    return 0;
}

void CServerParamsDlg::startAuthentication(AuthActionType action) {
    if (currentTask_) {
        return;
    }
    auto taskProfile = serverProfile_;
    if (action == AuthActionType::Login) {
        const auto login = readLogin();
        if (!login.DoAuth || !validateLogin(login)) {
            return;
        }
        applySettings(login);
        taskProfile = serverProfile_;
    } else {
        rollbackSettings();
        taskProfile.setProfileName(originalProfileName_);
    }

    currentTask_ = std::make_shared<AuthTask>(action);
    currentTask_->setServerProfile(taskProfile);
    currentTask_->addTaskFinishedCallback([this, action](UploadTask*, bool ok) {
        ServiceLocator::instance()->taskRunner()->runInGuiThread(
            [this, action, ok] { authenticationFinished(action, ok); }, true);
    });
    updateControls();
    ServiceLocator::instance()->uploadManager()->addSingleTask(currentTask_);
}

void CServerParamsDlg::authenticationFinished(AuthActionType action, bool ok) {
    const auto task = std::exchange(currentTask_, { });
    const bool stopped = task->isStopped() || task->stopSignal();
    if (action == AuthActionType::Login) {
        if (ok && !stopped) {
            LocalizedMessageBox(TR("Authenticated successfully."));
            commitSettings();
            return;
        }
        rollbackSettings();
        if (!stopped) {
            TRC(IDC_AUTHORIZEBUTTON, "Could not authenticate. Please try again.");
        }
    } else if (!stopped) {
        if (ok) {
            isAuthenticated_ = false;
            uploadEngineManager_->resetAuthorization(task->serverProfile());
            TRC(IDC_AUTHORIZEBUTTON, "Sign in...");
        }
        LocalizedMessageBox(ok ? TR("Logout successfully.") : TR("Failed to logout."));
    }
    updateControls();
}

LRESULT CServerParamsDlg::OnParamListBrowseFile(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto nmh = reinterpret_cast<NMPROPERTYITEM*>(pnmh);
    parameterListAdapter_->browseFileDialog(nmh->prop);
    return 0;
}

ServerProfile CServerParamsDlg::serverProfile() const {
    return serverProfile_;
}
