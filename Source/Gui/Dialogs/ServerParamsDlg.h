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

#pragma once

#include <optional>

#include "atlheaders.h"

#include "3rdpart/PropertyList.h"
#include "Core/Upload/AuthTask.h"
#include "Core/Upload/Parameters/AbstractParameter.h"
#include "Core/Upload/UploadEngine.h"
#include "Gui/Components/ParameterListAdapter.h"
#include "Gui/Controls/DialogIndirect.h"
#include "Gui/Controls/ProgressRingControl.h"
#include "resource.h"

class UploadEngineManager;

class CServerParamsDlg : public CCustomDialogIndirectImpl<CServerParamsDlg>, public CDialogResize<CServerParamsDlg> {
public:
    enum class Mode { SETTINGS, EDIT_ACCOUNT, ADD_ACCOUNT };
    enum { IDD = IDD_SERVERPARAMSDLG, ID_DELETEACCOUNT = 30 };

    CServerParamsDlg(const ServerProfile& serverProfile, UploadEngineManager* uploadEngineManager,
                     Mode mode = Mode::SETTINGS);

    BEGIN_MSG_MAP(CServerParamsDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
    COMMAND_ID_HANDLER(IDOK, OnClickedOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnClickedCancel)
    COMMAND_ID_HANDLER(IDC_DOAUTH, OnClickedDoAuth)
    COMMAND_ID_HANDLER(IDC_BROWSESERVERFOLDERS, OnBrowseServerFolders)
    COMMAND_ID_HANDLER(IDC_DELETEACCOUNTLABEL, onDeleteAccountClicked)
    COMMAND_ID_HANDLER(IDC_AUTHORIZEBUTTON, onSignInClicked)
    COMMAND_ID_HANDLER(IDC_LOGOUT, onLogoutClicked)
    COMMAND_HANDLER(IDC_LOGINEDIT, EN_CHANGE, OnLoginEditChange)
    NOTIFY_HANDLER(IDC_PARAMLIST, PIN_BROWSE, OnParamListBrowseFile)
    CHAIN_MSG_MAP(CDialogResize<CServerParamsDlg>)
    REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CServerParamsDlg)
    DLGRESIZE_CONTROL(IDC_LOGINFRAME, DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDC_LOGINEDIT, DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDC_PASSWORDEDIT, DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDC_AUTHORIZEBUTTON, DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDC_FOLDERNAMELABEL, DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDC_BROWSESERVERFOLDERS, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_PARAMLIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    DLGRESIZE_CONTROL(IDC_DELETEACCOUNTLABEL, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_ANIMATIONSTATIC, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    ServerProfile serverProfile() const;

private:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedDoAuth(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBrowseServerFolders(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnLoginEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnParamListBrowseFile(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT onDeleteAccountClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT onSignInClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT onLogoutClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    void createResources();
    void updateControls();
    void updateFolderLabel();
    bool authorizationEnabled() const;
    LoginInfo readLogin() const;
    bool validateLogin(const LoginInfo& login);
    void accept();
    void commitSettings();
    ServerSettingsStruct* applySettings(const LoginInfo& login);
    void rollbackSettings();
    void startAuthentication(AuthActionType action);
    void authenticationFinished(AuthActionType action, bool ok);

    ServerProfile serverProfile_;
    const std::string originalProfileName_;
    const CUploadEngineData* m_ue;
    UploadEngineManager* uploadEngineManager_;
    Mode mode_;
    CPropertyListCtrl m_wndParamList;
    ParameterList m_paramNameList;
    std::unique_ptr<ParameterListAdapter> parameterListAdapter_;
    CIcon iconFolder_;
    CIcon serverIcon_;
    CHyperLink deleteAccountLink_;
    CHyperLink signupLink_;
    CHyperLink websiteLink_;
    CProgressRingControl animation_;
    bool supportsSignIn_ = false;
    bool supportsLogout_ = false;
    bool isAuthenticated_ = false;
    std::shared_ptr<AuthTask> currentTask_;
    std::optional<ServerProfile> pendingProfile_;
    std::optional<ServerSettingsStruct> previousSettings_;
};
