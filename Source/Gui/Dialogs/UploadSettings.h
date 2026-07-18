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

#include <memory>
#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "atlctrlx.h"
#include "Gui/WizardCommon.h"
#include "Gui/Controls/ServerSelectorControl.h"
#include "Func/Common.h"

constexpr unsigned int IDC_SELECTFOLDER = 4050;
constexpr unsigned int ID_SERVERBUTTON = 4000;
constexpr unsigned int IDC_IMAGETOOLBAR = 4010;
constexpr unsigned int IDC_FILETOOLBAR = 4011;

constexpr unsigned int IDC_NEWFOLDER = 4012;

constexpr unsigned int IDC_OPENINBROWSER = 4014;

constexpr unsigned int IDC_SERVERPARAMS = 4016;
constexpr unsigned int IDC_OPENREGISTERURL = 4018;
constexpr unsigned int IDC_LOGINTOOLBUTTON = 4020;
constexpr unsigned int IDC_COPYFOLDERID = 4022;
constexpr unsigned int IDC_OPENWEBSITE = 4024;

constexpr unsigned int IDC_TOOLBARSEPARATOR1 = 4002;
constexpr unsigned int IDC_TOOLBARSEPARATOR2 = 4003;

constexpr unsigned int IDC_IMAGESERVER_FIRST_ID = 14000;
constexpr unsigned int IDC_IMAGESERVER_LAST_ID = 15000;

constexpr unsigned int IDC_FILESERVER_FIRST_ID = 16000;
constexpr unsigned int IDC_FILESERVER_LAST_ID = 17000;

constexpr unsigned int IDC_RESIZEPRESETMENU_FIRST_ID = 18000;
constexpr unsigned int IDC_RESIZEPRESETMENU_LAST_ID = 18100;

class CMyEngineList;
class IconBitmapUtils;
class CServerSelectorControl;
class WinServerIconCache;

class CUploadSettings :
    public CDialogImpl<CUploadSettings>,
    public CWizardPage
{
    public:
        explicit CUploadSettings(CMyEngineList * EngineList, UploadEngineManager * uploadEngineManager, WinServerIconCache* iconCache);
        ~CUploadSettings() override;
        enum { IDD = IDD_UPLOADSETTINGS };

        enum{
            IDC_USERNAME_FIRST_ID = 20000,
            IDC_USERNAME_LAST_ID = 21000,
            IDC_ADD_ACCOUNT = 21001,
            IDC_ADD_ACCOUNT_FROM_FILE_SERVER = 21002,
            IDC_NO_ACCOUNT = 21003,
            IDC_NO_ACCOUNT_FROM_FILE_SERVER = 21004
        };

    BEGIN_MSG_MAP(CUploadSettings)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)

        COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
        COMMAND_HANDLER(IDC_KEEPASIS, BN_CLICKED, OnBnClickedKeepasis)
        COMMAND_HANDLER(IDC_CREATETHUMBNAILS, BN_CLICKED, OnBnClickedCreatethumbnails)
        COMMAND_HANDLER(IDC_USESERVERTHUMBNAILS, BN_CLICKED, OnBnClickedUseServerThumbnails)
        COMMAND_HANDLER(IDC_LOGINTOOLBUTTON, BN_CLICKED, OnBnClickedLogin)
        COMMAND_HANDLER(IDC_LOGINTOOLBUTTON+1, BN_CLICKED, OnBnClickedLogin)
        COMMAND_HANDLER(IDC_SELECTFOLDER, BN_CLICKED, OnBnClickedSelectFolder)
        COMMAND_HANDLER(ID_SERVERBUTTON, BN_CLICKED, OnBnClickedSelectServer)
        COMMAND_HANDLER(IDC_NEWFOLDER, BN_CLICKED, OnNewFolder)
        COMMAND_HANDLER(IDC_NEWFOLDER+1, BN_CLICKED, OnNewFolder)
        COMMAND_HANDLER(IDC_OPENINBROWSER, BN_CLICKED, OnOpenInBrowser)
        COMMAND_HANDLER(IDC_OPENINBROWSER+1, BN_CLICKED, OnOpenInBrowser)
        COMMAND_HANDLER(IDC_COPYFOLDERID, BN_CLICKED, OnCopyFolderId)
        COMMAND_HANDLER(IDC_COPYFOLDERID + 1, BN_CLICKED, OnCopyFolderId)
        COMMAND_HANDLER(IDC_OPENWEBSITE, BN_CLICKED, OnOpenWebsite)
        COMMAND_HANDLER(IDC_OPENWEBSITE + 1, BN_CLICKED, OnOpenWebsite)
        COMMAND_HANDLER(IDC_OPENREGISTERURL, BN_CLICKED, OnOpenSignupPage)
        COMMAND_HANDLER(IDC_OPENREGISTERURL+1, BN_CLICKED, OnOpenSignupPage)
        COMMAND_HANDLER(IDC_SERVERPARAMS, BN_CLICKED, OnServerParamsClicked)
        COMMAND_HANDLER(IDC_SERVERPARAMS+1, BN_CLICKED, OnServerParamsClicked)
        COMMAND_HANDLER(IDC_ADD_ACCOUNT, BN_CLICKED, OnAddAccountClicked)
        COMMAND_HANDLER(IDC_ADD_ACCOUNT_FROM_FILE_SERVER, BN_CLICKED, OnAddAccountClicked)
        COMMAND_HANDLER(IDC_NO_ACCOUNT, BN_CLICKED, OnNoAccountClicked)
        COMMAND_HANDLER(IDC_CHOOSEMOREIMAGESERVERSLABEL, BN_CLICKED, OnChooseMoreImageServersClicked);
        COMMAND_HANDLER(IDC_CHOOSEMOREFILESERVERS, BN_CLICKED, OnChooseMoreFileServersClicked);

        COMMAND_HANDLER(IDC_NO_ACCOUNT_FROM_FILE_SERVER, BN_CLICKED, OnNoAccountClicked)
        NOTIFY_HANDLER(IDC_IMAGETOOLBAR, TBN_DROPDOWN, OnServerDropDown)
        NOTIFY_HANDLER(IDC_FILETOOLBAR, TBN_DROPDOWN, OnServerDropDown)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);

        COMMAND_HANDLER(IDC_FORMATLIST, CBN_SELCHANGE, OnProfileEditedCommand)
        COMMAND_HANDLER(IDC_QUALITYEDIT, EN_CHANGE, OnProfileEditedCommand)
        COMMAND_HANDLER(IDC_IMAGEWIDTH, EN_CHANGE, OnProfileEditedCommand)
        COMMAND_HANDLER(IDC_IMAGEHEIGHT, EN_CHANGE, OnProfileEditedCommand)
        COMMAND_HANDLER(IDC_SKIPANIMATEDCHECKBOX, BN_CLICKED, OnProfileEditedCommand)

        COMMAND_RANGE_HANDLER(IDC_IMAGESERVER_FIRST_ID, IDC_IMAGESERVER_LAST_ID, OnImageServerSelect)
        COMMAND_RANGE_HANDLER(IDC_FILESERVER_FIRST_ID, IDC_FILESERVER_LAST_ID, OnFileServerSelect)
        COMMAND_RANGE_HANDLER(IDC_RESIZEPRESETMENU_FIRST_ID, IDC_RESIZEPRESETMENU_LAST_ID, OnResizePresetMenuItemClick)
        COMMAND_RANGE_HANDLER(IDC_USERNAME_FIRST_ID, IDC_USERNAME_LAST_ID, OnUserNameMenuItemClick)

        COMMAND_HANDLER_EX(IDC_RESIZEPRESETSBUTTON, BN_CLICKED, OnResizePresetButtonClicked)
        COMMAND_HANDLER_EX(IDC_SHORTENINGURLSERVERBUTTON, BN_CLICKED, OnShorteningUrlServerButtonClicked)

        COMMAND_ID_HANDLER_EX(IDC_EDITPROFILE, OnEditProfileClicked)
        COMMAND_HANDLER_EX(IDC_PROFILECOMBO, CBN_SELCHANGE, OnProfileComboSelChange)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBnClickedKeepasis(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedSelectFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedSelectServer(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBnClickedCreatethumbnails(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedUseServerThumbnails(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedLogin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnImageServerSelect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFileServerSelect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnNewFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnOpenInBrowser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCopyFolderId(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnServerParamsClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnServerDropDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnOpenSignupPage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnResizePresetMenuItemClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnProfileEditedCommand(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnUserNameMenuItemClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAddAccountClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNoAccountClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnResizePresetButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnShorteningUrlServerButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnChooseMoreImageServersClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnChooseMoreFileServersClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOpenWebsite(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //int m_nImageServer, m_nFileServer;
    void ShowParams();
    CToolBarCtrl Toolbar;
    CToolBarCtrl FileServerSelectBar;
    bool OnNext() override;
    bool OnShow() override;
    bool OnHide() override;
    void UpdateAllPlaceSelectors();
    void UpdatePlaceSelector(bool ImageServer);
    void UpdateToolbarIcons();
    void initToolbars();
    CImageListManaged m_PlaceSelectorImageList, m_profileEditToolbarImageList;

    int nImageIndex;
    int nFileIndex;
    void OnFolderButtonContextMenu(POINT pt, bool isImageServerToolbar);
    void OnServerButtonContextMenu(POINT pt, bool isImageServerToolbar);
    void SetInitialFocus() override;

protected:
    CMyEngineList * m_EngineList;
    std::unique_ptr<IconBitmapUtils> iconBitmapUtils_;
    void TranslateUI();
    CButton m_ResizePresetIconButton;
    CButton m_ShorteningServerButton;
    CToolBarCtrl m_ProfileEditToolbar;
    //CPercentEdit m_ThumbSizeEdit;
    void UpdateProfileList();
    ServerProfileGroup sessionImageServer_, sessionFileServer_;
    ServerProfile& getSessionImageServerItem();
    ServerProfile& getSessionFileServerItem();
    void updateMoreServersLink(bool isImageServer);
    bool menuOpenedIsImageServer_;
    std::vector<CString> menuOpenedUserNames_;
    void selectServer(ServerProfile& sp, int serverIndex);
    void updateUrlShorteningCheckboxLabel();
    void shorteningUrlServerChanged(CServerSelectorControl* selectorControl);
    void settingsChanged(BasicSettings* settings);
    void updateButtonIcons();
    HWND useServerThumbnailsTooltip_;
    UploadEngineManager * uploadEngineManager_;
    CIcon iconEdit_, iconDropdown_;
    boost::signals2::connection settingsChangedConnection_;
    CHyperLink moreImageServersLink_, moreFileServersLink_;
    WinServerIconCache* iconCache_;
public:
    LRESULT OnEditProfileClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    std::map<CString, ImageConvertingParams>& convert_profiles_;
    void ShowParams(const ImageConvertingParams& params);
    void ShowParams(const CString& profileName);
    bool SaveParams(ImageConvertingParams& params);
    CString CurrentProfileName;
    void ProfileChanged();
    bool m_CatchChanges;
    bool m_ProfileChanged;
    void SaveCurrentProfile();
    CString CurrentProfileOriginalName;
    LRESULT OnProfileComboSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);
};



