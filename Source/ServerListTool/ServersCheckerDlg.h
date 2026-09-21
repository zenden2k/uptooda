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

#ifndef IU_SERVERLISTTOOL_MAINDLG_H
#define IU_SERVERLISTTOOL_MAINDLG_H

#include <map>

#include "atlheaders.h"
#include "resource.h"
#include "Func/MyEngineList.h"
#include "Core/Utils/SimpleXml.h"
#include "ServerListTool/ServersCheckerModel.h"
#include "ServersChecker.h"
#include "Helpers.h"
#include "ServerCheckerListView.h"
#include "Core/TaskDispatcher.h"
#include "Gui/Controls/ProgressRingControl.h"

//#include "Gui/Dialogs/WizardDlg.h"

class UploadManager;
class UploadEngineManager;
class NetworkClient;
class WtlGuiSettings;

namespace ServersListTool {

class ServersChecker;


class CServersCheckerDlg :
    public CDialogImpl<CServersCheckerDlg>, public CDialogResize<CServersCheckerDlg>, public CWinDataExchange<CServersCheckerDlg> {
public:
    enum { IDD = IDD_SERVERSCHECKERDLG };
    enum {
        ID_COPYDIRECTURL = 13000, ID_COPYTHUMBURL, ID_COPYVIEWURL /*WM_TASKDISPATCHERMSG = WM_USER + 100*/
    };
    CServersCheckerDlg(WtlGuiSettings *settings, UploadEngineManager* uploadEngineManager, UploadManager* uploadManager, CMyEngineList* engineList, std::shared_ptr<INetworkClientFactory> factory);
    BEGIN_MSG_MAP(CServersCheckerDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDC_BUTTONSKIP, OnSkip)
        COMMAND_ID_HANDLER(IDC_BUTTONSKIPALL, OnSkipAll)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDC_ERRORLOGBUTTON, OnErrorLogButtonClicked)
        COMMAND_ID_HANDLER(IDC_TOOLBROWSEBUTTON, OnBrowseButton)
        COMMAND_ID_HANDLER(ID_COPYDIRECTURL, OnCopyDirectUrl)
        COMMAND_ID_HANDLER(ID_COPYTHUMBURL, OnCopyThumbUrl)
        COMMAND_ID_HANDLER(ID_COPYVIEWURL, OnCopyViewUrl)
        COMMAND_HANDLER(IDC_STOPBUTTON, BN_CLICKED, OnBnClickedStopButton)
        CHAIN_MSG_MAP(CDialogResize<CServersCheckerDlg>)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CServersCheckerDlg)
        DLGRESIZE_CONTROL(IDC_TOOLSERVERLIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_BUTTONSKIP, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_BUTTONSKIPALL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_ERRORLOGBUTTON, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_STOPBUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_DDX_MAP(CServersCheckerDlg)
        DDX_CONTROL_HANDLE(IDC_RADIOWITHACCS, withAccountsRadioButton_)
        DDX_CONTROL_HANDLE(IDC_RADIOALWAYSACCS, alwaysWithAccountsRadioButton_)
        DDX_CONTROL_HANDLE(IDC_CHECKIMAGESERVERS, checkImageServersCheckBox_)
        DDX_CONTROL_HANDLE(IDC_CHECKFILESERVERS, checkFileServersCheckBox_)
        DDX_CONTROL_HANDLE(IDC_CHECKURLSHORTENERS, checkUrlShortenersCheckBox_)
        DDX_CONTROL(IDC_TOOLSERVERLIST, listView_)
        DDX_CONTROL(IDC_ANIMATIONSTATIC, loadingAnimation_)
    END_DDX_MAP()

    // Handler prototypes (uncomment arguments if needed):
    //    LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    //    LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    //    LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBrowseButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnErrorLogButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSkipAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCopyDirectUrl(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCopyThumbUrl(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCopyViewUrl(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedStopButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
    int contextMenuItemId;
    CString sourceFileHash_;
    Helpers::MyFileInfo sourceFileInfo_;
    UploadEngineManager* uploadEngineManager_;
    UploadManager* uploadManager_;
    CMyEngineList* engineList_;
    ServersCheckerModel model_;
    CServerCheckerListView listView_;

    SimpleXml xml;
    CIcon icon_, iconSmall_;
    CButton withAccountsRadioButton_, alwaysWithAccountsRadioButton_, checkImageServersCheckBox_,
        checkFileServersCheckBox_, checkUrlShortenersCheckBox_;

    std::unique_ptr<ServersChecker> serversChecker_;
    std::shared_ptr<INetworkClientFactory> networkClientFactory_;
    WtlGuiSettings* settings_;
    CProgressRingControl loadingAnimation_;
    void processFinished();
    void stop();
    bool isRunning() const;

    /**
     * @throws ValidationException
     */
    void validateSettings();
};

}

#endif
