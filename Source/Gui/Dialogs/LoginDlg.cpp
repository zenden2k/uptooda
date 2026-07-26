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

#include "LoginDlg.h"

#include <boost/format.hpp>

#include "WizardDlg.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/GuiTools.h"
#include "Core/Upload/ScriptUploadEngine.h"
#include "Func/WinUtils.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Core/Network/NetworkClientFactory.h"
#include "Core/ServiceLocator.h"
#include "Core/Upload/AuthTask.h"
#include "Core/Upload/UploadSession.h"
#include "Core/Upload/UploadManager.h"
#include "Core/AbstractServerIconCache.h"
#include "Gui/Helpers/DPIHelper.h"

// CLoginDlg
CLoginDlg::CLoginDlg(ServerProfile& serverProfile, UploadEngineManager* uem, bool createNew) : serverProfile_(serverProfile)
{
    m_UploadEngine = serverProfile.uploadEngineData();
    ignoreExistingAccount_ = false;
    serverSupportsBeforehandAuthorization_ = false;
    serverSupportsLogout_ = false;
    isAuthenticated_ = false;
    uploadEngineManager_ = uem;

    if (!m_UploadEngine->PluginName.empty() || !m_UploadEngine->Engine.empty()) {
        auto plugin = std::dynamic_pointer_cast<CAdvancedUploadEngine>(uploadEngineManager_->getUploadEngine(serverProfile));
        if (plugin) {
            serverSupportsBeforehandAuthorization_ = plugin->supportsBeforehandAuthorization();
            serverSupportsLogout_ = plugin->supportsLogout();
            isAuthenticated_ = plugin->isAuthenticated();
        }
    }
    createNew_ = createNew;
}

LRESULT CLoginDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CenterWindow(GetParent());
    DoDataExchange(FALSE);
    serverImage_.SubclassWindow(GetDlgItem(IDC_SERVERICON));
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);

    auto* myEngineList = dynamic_cast<CMyEngineList*>(ServiceLocator::instance()->engineList());
    if (myEngineList) {
        serverIcon_ = ServiceLocator::instance()->serverIconCache()->getBigIconForServer(m_UploadEngine->Name, dpi);
        if (serverIcon_) {
            int iconWidth = ::GetSystemMetrics(SM_CXICON);
            int iconHeight = ::GetSystemMetrics(SM_CYICON);
            serverImage_.SetWindowPos(0, 0, 0, iconWidth, iconHeight, SWP_NOMOVE | SWP_NOZORDER);
            auto [serverBitmap, data] = ImageUtils::GetIconPixelData(serverIcon_);
            if (serverBitmap && serverBitmap->GetLastStatus() == Gdiplus::Ok) {
                serverImage_.loadImage(0, std::move(serverBitmap), 0, false, GetSysColor(COLOR_BTNFACE), false, true, false);
            }
        }
    }

    BasicSettings* Settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* serverSettings = Settings->getServerSettings(serverProfile_);

    LoginInfo li = serverSettings ? serverSettings->authData : LoginInfo();
    auto* uploadEngineData = serverProfile_.uploadEngineData();
    SetWindowText(TR("Autorization parameters"));
    CString loginLabelText = uploadEngineData->LoginLabel.empty() ? CString(TR("Login:")) : CString(U2W(uploadEngineData->LoginLabel)) + _T(":");
    SetDlgItemText(IDC_LOGINLABEL, loginLabelText);
    CString passwordLabelText = uploadEngineData->PasswordLabel.empty() ? CString(TR("Password:")) : CString(U2W(uploadEngineData->PasswordLabel)) + _T(":");
    SetDlgItemText(IDC_PASSWORDLABEL, passwordLabelText);
    TRC(IDOK, "OK");
    TRC(IDCANCEL, "Cancel");
    TRC(IDC_DELETEACCOUNTLABEL, "Delete account");
    TRC(IDC_LOGOUT, "Logout");

    HWND hWnd = GetDlgItem(IDC_ANIMATIONSTATIC);
    if (hWnd)
    {
        wndAnimation_.SubclassWindow(hWnd);
        wndAnimation_.ShowWindow(SW_HIDE);
    }

    loginButton_ = GetDlgItem(IDC_AUTHORIZEBUTTON);
    loginButton_.SetWindowText(TR("Sign in..."));
    bool showLoginButton = serverSupportsBeforehandAuthorization_ && !isAuthenticated_;
    loginButton_.ShowWindow(showLoginButton ? SW_SHOW : SW_HIDE);

    logoutButton_ = GetDlgItem(IDC_LOGOUT);
    bool showLogoutButton = serverSupportsLogout_ && isAuthenticated_;
    logoutButton_.ShowWindow(showLogoutButton ? SW_SHOW : SW_HIDE);

    if (!m_UploadEngine->RegistrationUrl.empty()) {
        signupLink_.SubclassWindow(GetDlgItem(IDC_SIGNUPLINK));
        signupLink_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;
        signupLink_.m_clrLink = GuiTools::GetDefaultHyperlinkColor(signupLink_);
        std::wstring linkText = str(boost::wformat(TR("Don't have an account? Sign up on %s right now!")) % IuCoreUtils::Utf8ToWstring(myEngineList->getServerDisplayName(m_UploadEngine)));
        signupLink_.SetLabel(linkText.c_str());
        signupLink_.SetHyperLink(U2W(m_UploadEngine->RegistrationUrl));
        signupLink_.ShowWindow(SW_SHOW);
    }

    if (!m_UploadEngine->WebsiteUrl.empty()) {
        websiteLink_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;
        websiteLink_.m_clrLink = GuiTools::GetDefaultHyperlinkColor(websiteLink_);
        std::wstring linkText = TR("Open the website");
        websiteLink_.SetLabel(linkText.c_str());
        websiteLink_.SetHyperLink(U2W(m_UploadEngine->WebsiteUrl));
        websiteLink_.ShowWindow(SW_SHOW);
    }
    accountName_ = Utf8ToWCstring(li.Login);

    SetDlgItemText(IDC_LOGINEDIT, accountName_);
    SetDlgItemText(IDC_PASSWORDEDIT, Utf8ToWCstring(li.Password));
    if (myEngineList) {
        SetDlgItemText(IDC_LOGINFRAME, Utf8ToWCstring(myEngineList->getServerDisplayName(m_UploadEngine)));
    }

    ::EnableWindow(GetDlgItem(IDC_PASSWORDEDIT),m_UploadEngine->NeedPassword);
    ::EnableWindow(GetDlgItem(IDC_PASSWORDLABEL),m_UploadEngine->NeedPassword);

    deleteAccountLabel_.SubclassWindow(GetDlgItem(IDC_DELETEACCOUNTLABEL));
    deleteAccountLabel_.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER | HLINK_COMMANDBUTTON;
//    deleteAccountLabel_.SetLabel(deleteAccountLabelText);
    deleteAccountLabel_.m_clrLink = GuiTools::GetDefaultHyperlinkColor(deleteAccountLabel_);
    deleteAccountLabel_.SetToolTipText(TR("Remove this account from the list (not from the remote server)"));

    deleteAccountLabel_.ShowWindow((createNew_ || accountName_.IsEmpty()) ? SW_HIDE : SW_SHOW);

    ::SetFocus(GetDlgItem(IDC_LOGINEDIT));
    return 0;
}

LRESULT CLoginDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    Accept();
    return 0;
}

LRESULT CLoginDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (currentTask_) {
        currentTask_->stop();
	} else {
        EndDialog(wID);
	}

    return 0;
}

LRESULT CLoginDlg::OnDeleteAccountClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    std::wstring text = str(IuStringUtils::FormatWideNoExcept(TR("Are you sure you want to delete this account from %s's internal list?")) % APP_NAME);
    if (LocalizedMessageBox(text.c_str(), APP_NAME, MB_ICONQUESTION | MB_YESNO) != IDYES) {
        return 0;
    }
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    uploadEngineManager_->unloadUploadEngines(serverProfile_.serverName(), serverProfile_.profileName());
    settings->deleteProfile(serverProfile_.serverName(), W2U(accountName_));

    accountName_.Empty();
    EndDialog(ID_DELETEACCOUNT);
    return 0;
}

LRESULT CLoginDlg::OnDoLoginClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    startAuthentication(AuthActionType::Login);
    return 0;
}

LRESULT CLoginDlg::OnLogoutClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    startAuthentication(AuthActionType::Logout);
    return 0;
}

LRESULT CLoginDlg::OnLoginEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CString text = GuiTools::GetWindowText(hWndCtl);
    loginButton_.ShowWindow((serverSupportsBeforehandAuthorization_ && !isAuthenticated_) && !text.IsEmpty() ? SW_SHOW : SW_HIDE);
    return 0;
}

CString CLoginDlg::accountName() const
{
    return accountName_;
}

void CLoginDlg::startAuthentication(AuthActionType actionType)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    if (!m_UploadEngine->PluginName.empty() ) {

        LoginInfo li;
        CString login = GuiTools::GetDlgItemText(m_hWnd, IDC_LOGINEDIT);
        li.Login = WCstringToUtf8(login);
        std::string serverNameA = serverProfile_.serverName();
        if ( !ignoreExistingAccount_ && createNew_ && settings->ServersSettings[serverNameA].find(li.Login ) != settings->ServersSettings[serverNameA].end() ) {
            LocalizedMessageBox(TR("Account with such name already exists."),TR("Error"), MB_ICONERROR);
            OnProcessFinished();
            return;
        }

        ignoreExistingAccount_ = true;

        if ( li.Login != W2U(accountName_) ) {
            serverProfile_.clearFolderInfo();
        }

        accountName_ = login;
        serverProfile_.setProfileName(WCstringToUtf8(login));
        li.Password = W2U(GuiTools::GetDlgItemText(m_hWnd, IDC_PASSWORDEDIT));
        li.DoAuth = true;

        ServerSettingsStruct* serverSettings = settings->getServerSettings(serverProfile_, true);

        if (serverSettings) {
            serverSettings->authData = li;
        } else {
            LOG(WARNING) << "No server settings for name=" << serverProfile_.serverName() << " login=" << serverProfile_.profileName();
        }
        using namespace std::placeholders;
        auto authTask = std::make_shared<AuthTask>(actionType);
        authTask->setServerProfile(serverProfile_);
        authTask->addTaskFinishedCallback(std::bind(&CLoginDlg::authTaskFinishedCallback, this, _1, _2));
        auto* uploadManager = ServiceLocator::instance()->uploadManager();
        enableControls(false);
        currentTask_ = authTask;
        uploadManager->addSingleTask(authTask);
    }
}

void CLoginDlg::OnProcessFinished()
{
    ServiceLocator::instance()->taskRunner()->runInGuiThread([this]{
        enableControls(true);
    });
}

void CLoginDlg::Accept()
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    LoginInfo li;
    CString loginEditText = GuiTools::GetDlgItemText(m_hWnd, IDC_LOGINEDIT);
    std::string oldAccountName = W2U(accountName_);

    li.Login = W2U(loginEditText);

    if ( li.Login.empty() ) {
        auto* uploadEngineData = serverProfile_.uploadEngineData();
        std::string loginLabelText = (uploadEngineData && !uploadEngineData->LoginLabel.empty()) ? uploadEngineData->LoginLabel : _("Login");
        std::string msg = str(IuStringUtils::FormatNoExcept(_("The field '%s' cannot be empty.")) % loginLabelText);
        LocalizedMessageBox(U2WC(msg),TR("Error"), MB_ICONERROR);
        return;
    }
    std::string serverName = serverProfile_.serverName();
    if ( !ignoreExistingAccount_ &&  createNew_ && Settings.ServersSettings[serverName].find(li.Login ) != Settings.ServersSettings[serverName].end() ) {
        LocalizedMessageBox(TR("Account with such name already exists."),TR("Error"), MB_ICONERROR);
        return;
    }

    if ( li.Login != oldAccountName) {
        serverProfile_.clearFolderInfo();

        if (!accountName_.IsEmpty()) {
            // If user has changed account's name, delete account with old name
            uploadEngineManager_->unloadUploadEngines(serverProfile_.serverName(), serverProfile_.profileName());
            Settings.deleteProfile(serverName, oldAccountName);
        }
    }

    accountName_ = loginEditText;
    serverProfile_.setProfileName(W2U(loginEditText));

    CString passwordEditText = GuiTools::GetDlgItemText(m_hWnd, IDC_PASSWORDEDIT);

    li.Password = W2U(passwordEditText);
    li.DoAuth = true;
    uploadEngineManager_->resetAuthorization(serverProfile_);

    ServerSettingsStruct* serverSettings = Settings.getServerSettings(serverProfile_, true);
    if (serverSettings) {
        serverSettings->authData = li;
    } else {
        LOG(WARNING) << "No server settings for name=" << serverProfile_.serverName() << " login=" << serverProfile_.profileName();
    }
    EndDialog(IDOK);
}

void CLoginDlg::enableControls(bool enable)
{
    wndAnimation_.ShowWindow(!enable ? SW_SHOW : SW_HIDE);
    GuiTools::EnableDialogItem(m_hWnd, IDC_LOGINEDIT, enable);
    GuiTools::EnableDialogItem(m_hWnd, IDC_PASSWORDEDIT, enable&&m_UploadEngine->NeedPassword);
    GuiTools::EnableDialogItem(m_hWnd, IDOK, enable);
    //GuiTools::EnableDialogItem(m_hWnd, IDCANCEL, enable);
    loginButton_.EnableWindow(enable);
    deleteAccountLabel_.EnableWindow(enable);
}


void CLoginDlg::authTaskFinishedCallback(UploadTask* task, bool ok) {
    auto* authTask = dynamic_cast<AuthTask*>(task);
    if (!authTask) {
        return;
    }
    if (authTask->authActionType() == AuthActionType::Login) {
        if (ok) {
            OnProcessFinished();
            ServiceLocator::instance()->taskRunner()->runInGuiThread([this] {
                LocalizedMessageBox(TR("Authenticated successfully."));
                Accept();
            });
        }
        else {
            loginButton_.SetWindowText(TR("Could not authenticate. Please try again."));
            OnProcessFinished();
        }
    } else if (authTask->authActionType() == AuthActionType::Logout) {
        OnProcessFinished();
        if (ok) {
            ServiceLocator::instance()->taskRunner()->runInGuiThread([this, ok] {
                LocalizedMessageBox(ok ? TR("Logout successfully.") : TR("Failed to logout."));
                logoutButton_.ShowWindow(SW_HIDE);
                loginButton_.ShowWindow(serverSupportsBeforehandAuthorization_ ? SW_SHOW : SW_HIDE);
            });
        }

    }
    currentTask_.reset();
}
