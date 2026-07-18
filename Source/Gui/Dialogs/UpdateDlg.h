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

#ifndef UPDATEDLG_H
#define UPDATEDLG_H

#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Func/UpdatePackage.h"
#include "3rdpart/thread.h"
#include "Gui/Controls/ResultsListView.h"
#include "Gui/Controls/DialogIndirect.h"

class CUpdateDlg :
    public CCustomDialogIndirectImpl<CUpdateDlg>,
    public CDialogResize<CUpdateDlg>,
    public CThreadImpl<CUpdateDlg>,
    public CUpdateStatusCallback
{
    public:
        CUpdateDlg();
        ~CUpdateDlg();
        enum { IDD = IDD_UPDATEDLG };
        enum { kTimer = 2};
        class CUpdateDlgCallback
        {
            public:
                virtual ~CUpdateDlgCallback() = default;
                virtual bool CanShowWindow() = 0;
                virtual void UpdateAvailabilityChanged(bool Available) = 0;
                virtual void ShowUpdateMessage(const CString& msg) = 0;
        };
    protected:
        BEGIN_MSG_MAP(CUpdateDlg)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
            MESSAGE_HANDLER(WM_TIMER, OnTimer)
            COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
            COMMAND_HANDLER(IDC_DOWNLOADBUTTON, BN_CLICKED, OnDownloadButtonClicked)
            CHAIN_MSG_MAP(CDialogResize<CUpdateDlg>)
        END_MSG_MAP()

        BEGIN_DLGRESIZE_MAP(CUpdateDlg)
            DLGRESIZE_CONTROL(IDC_UPDATELISTVIEW, DLSZ_SIZE_X|DLSZ_SIZE_Y)
            DLGRESIZE_CONTROL(IDC_UPDATEINFO,  DLSZ_SIZE_X | DLSZ_SIZE_Y)
            DLGRESIZE_CONTROL(IDC_MANUALUPDATEINFO,  DLSZ_SIZE_X )
            DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X| DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X|DLSZ_MOVE_Y)
        END_DLGRESIZE_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDownloadButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    void setUpdateCallback(CUpdateDlgCallback* callback);
    DWORD Run();

    void CheckUpdates();
    void DoUpdates();
    bool ShowModal(HWND parent, bool forceCheck = false);
    void Abort();
    void updateStatus(int packageIndex, const CString& status) override;

    CResultsListView m_listView;
    bool m_bUpdateFinished;
    CUpdateDlgCallback* m_UpdateCallback;
    int m_bClose;
    bool m_InteractiveUpdate;
    CEvent m_UpdateEvent;
    CUpdateManager m_UpdateManager;
    bool stop;
    bool m_Checked;
    bool m_Modal;
};


#endif // UPDATEDLG_H
