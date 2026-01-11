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

#ifndef IU_GUI_CONTROLS_SERVERSELECTORCONTROL_H
#define IU_GUI_CONTROLS_SERVERSELECTORCONTROL_H


#pragma once

#include <functional>
#include <memory>

#include <boost/signals2.hpp>

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Gui/Dialogs/SettingsPage.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Upload/ServerProfileGroup.h"
#include "Gui/Controls/DialogIndirect.h"
#include "Gui/Constants.h"
#include "Core/Upload/UploadEngine.h"

// CServerSelectorControl
class IconBitmapUtils;
class UploadEngineManager;
class WinServerIconCache;

constexpr int WM_SERVERSELECTCONTROL_CHANGE = WM_USER + 156;
constexpr int WM_SERVERSELECTCONTROL_SERVERLIST_CHANGED = WM_USER + 157;

class CServerSelectorControl :
    public CDialogIndirectImpl<CServerSelectorControl>
{
public:
    explicit CServerSelectorControl(UploadEngineManager* uploadEngineManager, bool defaultServer = false, bool isChildWindow = true, bool showServerIcons = true);
    virtual ~CServerSelectorControl();
    enum { IDD = IDD_SERVERSELECTORCONTROL, IDC_LOGINMENUITEM = 4020, IDC_USERNAME_FIRST_ID = 20000, IDC_USERNAME_LAST_ID = 21000,
        IDC_ADD_ACCOUNT= 21001, IDC_NO_ACCOUNT = 21003
    };

    BEGIN_MSG_MAP(CServerSelectorControl)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ENABLE, OnEnable)
        MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)
        COMMAND_ID_HANDLER(IDC_EDIT, OnClickedEdit)
        COMMAND_ID_HANDLER(IDC_IMAGEPROCESSINGPARAMS, OnImageProcessingParamsClicked)
        COMMAND_ID_HANDLER(IDC_ACCOUNTINFO, OnAccountClick)
        COMMAND_ID_HANDLER(IDC_ADD_ACCOUNT, OnAddAccountClick)
        COMMAND_ID_HANDLER(IDC_LOGINMENUITEM, OnLoginMenuItemClicked)
        COMMAND_ID_HANDLER(IDC_NO_ACCOUNT, OnNoAccountClicked)
        COMMAND_RANGE_HANDLER(IDC_USERNAME_FIRST_ID, IDC_USERNAME_LAST_ID, OnUserNameMenuItemClick);
        COMMAND_HANDLER(IDC_SERVERBUTTON, BN_CLICKED, OnBnClickedServerButton)
        NOTIFY_HANDLER(IDC_SERVERBUTTON, BCN_DROPDOWN, OnBnDropdownServerButton)
    END_MSG_MAP()

    DLGTEMPLATE* GetTemplate();
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClickedEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAccountClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAddAccountClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnLoginMenuItemClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNoAccountClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnImageProcessingParamsClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnUserNameMenuItemClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnEnable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBnClickedServerButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBnDropdownServerButton(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    void TranslateUI();
    void setTitle(CString title);
    CString getTitle() const;
    void setServerProfile(const ServerProfile& serverProfile);
    void setShowDefaultServerItem(bool show);
    void setServersMask(int mask);
    void setShowFilesizeLimits(bool show);
    void notifyChange();
    void notifyServerListChanged();
    void updateServerButton();
    bool isAccountChosen() const;
    int showPopup(HWND parent, const RECT& anchorRect, bool topMost = false);
    bool exitPopup(int nCommandId);

    ServerProfile serverProfile() const;
    void setShowImageProcessingParams(bool show);
    void setShowParamsLink(bool show);
    void setOnChangeCallback(std::function<void(CServerSelectorControl*)> cb);
    void setShowEmptyItem(bool show);

private:
    CButton serverButton_;
    CHyperLink imageProcessingParamsLink_;
    CHyperLink accountLink_;
    CImageListManaged comboBoxImageList_,settingsButtonImageList_;
    CToolBarCtrl settingsButtonToolbar_;
    CStatic userPictureControl_, folderPictureControl_;
    ServerProfile serverProfile_;
    bool showDefaultServerItem_;
    bool showImageProcessingParams_;
    bool showParamsLink_;
  
    CString currentUserName_;
    int serversMask_;
    bool defaultServer_;
    std::vector<CString> menuOpenedUserNames_;
    std::unique_ptr<IconBitmapUtils> iconBitmapUtils_;
    UploadEngineManager* uploadEngineManager_;
    bool isPopingUp_;
    bool isChildWindow_;
    bool showFileSizeLimits_;
    CString title_;
    CFont serverGroupboxFont_;
    HGLOBAL hMyDlgTemplate_;
    std::function<void(CServerSelectorControl*)> onChangeCallback_;
    bool showEmptyItem_;
    bool showServerIcons_;
    boost::signals2::scoped_connection profileListChangedConnection_;
    CIcon iconUser_, iconFolder_;
    WinServerIconCache* iconCache_;
    ULONGLONG activationTime_ = 0;

    void profileListChanged(BasicSettings* settings, const std::vector<std::string>& affectedServers);
    void addAccount();
    void serverChanged();
    void updateInfoLabel();
    void createSettingsButton();
    void createResources();
    void showServerButtonPopup();
};

#endif


