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

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include <atlframe.h>
#include "3rdpart/PropertyList.h"
#include "Core/Upload/UploadEngine.h"
#include "Core/Upload/ScriptUploadEngine.h"
#include "Gui/Controls/DialogIndirect.h"
#include "Core/Upload/Parameters/AbstractParameter.h"
#include "Gui/Components/ParameterListAdapter.h"

class ServerProfile;
class UploadEngineManager;

class CServerParamsDlg :
    public CCustomDialogIndirectImpl<CServerParamsDlg>,
    public CDialogResize<CServerParamsDlg>
{
    public:
        CServerParamsDlg(const ServerProfile&  serverProfile, UploadEngineManager * uploadEngineManager, bool focusOnLoginEdit = false);
        ~CServerParamsDlg();
        enum { IDD = IDD_SERVERPARAMSDLG };

        BEGIN_MSG_MAP(CServerParamsDlg)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
            COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
            COMMAND_HANDLER(IDC_DOAUTH, BN_CLICKED, OnClickedDoAuth)
            COMMAND_HANDLER(IDC_BROWSESERVERFOLDERS, BN_CLICKED, OnBrowseServerFolders)
            COMMAND_HANDLER(IDC_LOGINEDIT, EN_CHANGE, OnLoginEditChange)
            NOTIFY_HANDLER(IDC_PARAMLIST, PIN_BROWSE, OnParamListBrowseFile)
            CHAIN_MSG_MAP(CDialogResize<CServerParamsDlg>)
            REFLECT_NOTIFICATIONS()
        END_MSG_MAP()

        BEGIN_DLGRESIZE_MAP(CServerParamsDlg)
            DLGRESIZE_CONTROL(IDC_PARAMLIST, DLSZ_SIZE_X|DLSZ_SIZE_Y)
            DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X| DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X|DLSZ_MOVE_Y)
        END_DLGRESIZE_MAP()
         // Handler prototypes:
         //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
         //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedDoAuth(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnBrowseServerFolders(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnLoginEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnParamListBrowseFile(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        ServerProfile serverProfile() const;
    protected:
        CPropertyListCtrl m_wndParamList;
        ParameterList m_paramNameList;
        const CUploadEngineData *m_ue;
        bool focusOnLoginControl_;
        std::shared_ptr<CAdvancedUploadEngine> m_pluginLoader;
        CString oldLogin_;
        ServerProfile  serverProfile_;
        UploadEngineManager * uploadEngineManager_;
        std::unique_ptr<ParameterListAdapter> parameterListAdapter_;
        CIcon iconFolder_;
        CStatic folderPictureControl_;

        void doAuthChanged();
        void createResources();
};


