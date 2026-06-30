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

#include <functional>
#include <memory>

#include <boost/signals2.hpp>

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Core/Settings/BasicSettings.h"
#include "Gui/Controls/DialogIndirect.h"
#include "Gui/Constants.h"
#include "Gui/Controls/ServerListView.h"
#include "Core/Upload/UploadEngine.h"
#include "Gui/Controls/MyImage.h"

// CServerListPopup
class IconBitmapUtils;
class CMyEngineList;
class ServerListModel;
class WinServerIconCache;
class WtlGuiSettings;

class CServerListPopup :
    public CDialogIndirectImpl<CServerListPopup>,
    public CWinDataExchange<CServerListPopup>,
    public CDialogResize<CServerListPopup>
{
public:
    explicit CServerListPopup(CMyEngineList* engineList, WinServerIconCache* serverIconCache, int serverMask, int selectedServerType = CUploadEngineListBase::ALL_SERVERS, int serverIndex = -1, bool isChildWindow = false);
    virtual ~CServerListPopup();

    enum { IDD = IDD_SERVERLISTPOPUP };
    inline static constexpr auto IDM_ADD_FTP_SERVER = 10000;
    inline static constexpr auto IDM_ADD_DIRECTORY_AS_SERVER = 10001;
    inline static constexpr auto IDM_OPEN_SERVERS_FOLDER = 10002;
    inline static constexpr auto IDM_VIEW_MODE_REPORT = 10003;
    inline static constexpr auto IDM_VIEW_MODE_ICONS = 10004;

    BEGIN_MSG_MAP(CServerListPopup)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ENABLE, OnEnable)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_HANDLER(IDC_ALLTYPESRADIO, BN_CLICKED, OnServerTypeChanged)
        COMMAND_HANDLER(IDC_IMAGERADIO, BN_CLICKED, OnServerTypeChanged)
        COMMAND_HANDLER(IDC_FILERADIO, BN_CLICKED, OnServerTypeChanged)
        COMMAND_HANDLER(IDC_VIDEORADIO, BN_CLICKED, OnServerTypeChanged)
        COMMAND_HANDLER(IDC_SEARCHQUERYEDIT, EN_CHANGE, OnSearchQueryEditChanged)
        COMMAND_HANDLER(IDC_ADDBUTTON, BN_CLICKED, OnBnClickedAddServerButton)
        COMMAND_ID_HANDLER_EX(IDM_ADD_FTP_SERVER, OnAddFtpServer)
        COMMAND_ID_HANDLER_EX(IDM_ADD_DIRECTORY_AS_SERVER, OnAddDirectoryAsServer)
        COMMAND_ID_HANDLER_EX(IDM_OPEN_SERVERS_FOLDER, OnOpenServersFolder)
        COMMAND_ID_HANDLER_EX(IDM_VIEW_MODE_REPORT, OnViewModeReport)
        COMMAND_ID_HANDLER_EX(IDM_VIEW_MODE_ICONS, OnViewModeIcons)
        COMMAND_ID_HANDLER_EX(IDC_HELPBUTTON, OnHelpButton)
        NOTIFY_HANDLER(IDC_ADDBUTTON, BCN_DROPDOWN, OnBnDropdownAddServerButton)
        NOTIFY_HANDLER(IDC_SERVERLISTCONTROL, NM_DBLCLK, OnListViewDblClick)
        CHAIN_MSG_MAP(CDialogResize<CServerListPopup>)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CServerListPopup)
        DLGRESIZE_CONTROL(IDC_SERVERLISTCONTROL, DLSZ_SIZE_X |  DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_SEARCHQUERYEDIT, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_SEARCHSTATIC, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_HELPBUTTON, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_ADDBUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_DDX_MAP(CServerListPopup)
        DDX_CONTROL(IDC_SERVERLISTCONTROL, listView_)
        DDX_CONTROL_HANDLE(IDC_ALLTYPESRADIO, allTypesRadioButton_)
        DDX_CONTROL_HANDLE(IDC_IMAGERADIO, imageTypeRadioButton_)
        DDX_CONTROL_HANDLE(IDC_FILERADIO, fileTypeRadioButton_)
        DDX_CONTROL_HANDLE(IDC_VIDEORADIO, videoTypeRadioButton_)
        DDX_CONTROL_HANDLE(IDC_SEARCHQUERYEDIT, queryEditControl_)
        DDX_CONTROL_HANDLE(IDC_ADDBUTTON, optionsButton_)
        DDX_CONTROL_HANDLE(IDC_HELPBUTTON, helpButton_)
        DDX_CONTROL(IDC_SEARCHSTATIC, searchIconCtrl_)
    END_DDX_MAP()

    DLGTEMPLATE* GetTemplate();
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEnable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnListViewDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnServerTypeChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSearchQueryEditChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBnClickedAddServerButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBnDropdownAddServerButton(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnAddFtpServer(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnAddDirectoryAsServer(WORD wNotifyCode, WORD wID, HWND hWndCtl); 
    LRESULT OnHelpButton(WORD wNotifyCode, WORD wID, HWND hWndCtl); 
    LRESULT OnOpenServersFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnViewModeReport(WORD wNotifyCode, WORD wID, HWND hWndCtl); 
    LRESULT OnViewModeIcons(WORD wNotifyCode, WORD wID, HWND hWndCtl); 
    void TranslateUI();
    void setTitle(CString title);
    CString getTitle() const;

    void setServerProfile(const ServerProfile& serverProfile);

    void setServersMask(int mask);

    void notifyChange();
    void notifyServerListChanged();
    void updateServerList();
    int showPopup(HWND parent, const RECT& anchorRect);
    bool exitPopup(int nCommandId);

    ServerProfile serverProfile() const;
    void setOnChangeCallback(std::function<void(CServerListPopup*)> cb);

    int serverIndex() const;

private:
    ServerProfile serverProfile_;
    std::unique_ptr<IconBitmapUtils> iconBitmapUtils_;
    CMyEngineList* engineList_;
    WtlGuiSettings* settings_;
    bool isPopingUp_;
    bool isChildWindow_;
    HGLOBAL hMyDlgTemplate_;
    std::function<void(CServerListPopup*)> onChangeCallback_;
    std::unique_ptr<ServerListModel> serverListModel_;
    CServerListView listView_;
    CButton allTypesRadioButton_, imageTypeRadioButton_, fileTypeRadioButton_, videoTypeRadioButton_;
    CEdit queryEditControl_;
    CButton optionsButton_, helpButton_;
    CIcon helpButtonIcon_;
    CToolTipCtrl toolTip_;
    CMyImage searchIconCtrl_;

    int serversMask_, serverIndex_, selectedServerType_;
    int ret_ = 0;
    CAccelerator hotkeys_;
    void serverChanged();
    void createResources();
    void applyFilter(bool selectItem = true);
    void clearFilter();
    void selectServerByName(const CString& name);
    void showAddServerButtonMenu(HWND control);
    void openDocumentation();
};


