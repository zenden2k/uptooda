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
#ifndef IU_GUI_DIALOGS_UPLOADDLG_H
#define IU_GUI_DIALOGS_UPLOADDLG_H


#pragma once

#include <ShObjidl.h>
#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Core/Upload/Uploader.h"
#include "Gui/Dialogs/MainDlg.h"
#include "Gui/Dialogs/ResultsWindow.h"
#include "Core/Upload/FileQueueUploader.h"
#include "Core/Upload/ServerProfileGroup.h"
#include "Gui/Controls/ResultsListView.h"
#include "Func/MyEngineList.h"

class UploadManager;

class CUploadDlg : public CDialogImpl<CUploadDlg>,
                         public CWizardPage
{
    public:
        CUploadDlg(CWizardDlg *dlg, UploadManager* uploadManager);

        enum { IDD = IDD_UPLOADDLG };
        enum {
            IDC_UPLOADPROCESSTAB = 14000, IDC_UPLOADRESULTSTAB, ID_RETRYUPLOAD, ID_VIEWIMAGE, ID_SHOWLOGFORTHISFILE,
            ID_COPYLINK, ID_COPYVIEWLINK, ID_COPYTHUMBLINK, ID_COPYDELETELINK,
            kEnableNextButtonTimer = 5,
            kProgressTimer = 6,
        };

         BEGIN_MSG_MAP(CUploadDlg)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_TIMER, OnTimer)
            MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)
            COMMAND_HANDLER(IDC_UPLOADPROCESSTAB, BN_CLICKED, OnUploadProcessButtonClick)
            COMMAND_HANDLER(IDC_UPLOADRESULTSTAB, BN_CLICKED, OnUploadResultsButtonClick)
            COMMAND_HANDLER(IDC_VIEWLOG, BN_CLICKED, OnBnClickedViewLog)
            COMMAND_ID_HANDLER(ID_RETRYUPLOAD, OnRetryUpload)
            COMMAND_ID_HANDLER(ID_VIEWIMAGE, OnViewImage)
            COMMAND_ID_HANDLER(ID_SHOWLOGFORTHISFILE, OnShowLogForThisFile)
            COMMAND_ID_HANDLER(ID_COPYLINK, OnCopyLink)
            COMMAND_ID_HANDLER(ID_COPYVIEWLINK, OnCopyViewLink)
            COMMAND_ID_HANDLER(ID_COPYTHUMBLINK, OnCopyThumbLink)
            COMMAND_ID_HANDLER(ID_COPYDELETELINK, OnCopyDeleteLink)
            NOTIFY_HANDLER(IDC_UPLOADTABLE, NM_DBLCLK, OnUploadTableDoubleClick)
            MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
            REFLECT_NOTIFICATIONS()
        END_MSG_MAP()

         // Handler prototypes:
         //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
         //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

        bool startUpload();
        CMainDlg *MainDlg;
        std::unique_ptr<CResultsWindow> resultsWindow_;
        int ThreadTerminated();
        std::vector<Uptooda::Core::OutputGenerator::UploadObject> urlList_;
        bool OnShow() override;
        bool OnNext() override;
        bool OnHide() override;
        bool CancelByUser;
        void GenerateOutput(bool immediately = false);
        void TotalUploadProgress(int CurPos, int Total,int FileProgress=0);
        int progressCurrent, progressTotal;
        CMyEngineList *engineList_;
        // Is called when upload engine is uploading to remote folder(album)
        // Then this album will appear in dropdown list "Options"
        void OnFolderUsed(UploadTask* task);
        void onShortenUrlChanged(bool shortenUrl);
    protected:
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnUploadProcessButtonClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnUploadResultsButtonClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnBnClickedViewLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnUploadTableDoubleClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnRetryUpload(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnViewImage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnShowLogForThisFile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnCopyLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnCopyViewLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnCopyThumbLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnCopyDeleteLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

        void showUploadResultsTab();
        void showUploadProgressTab();
        void onSessionFinished(UploadSession* session);
        void onTaskFinished(UploadTask* task, bool ok);
        void onChildTaskAdded(UploadTask* child);
        void backgroundThreadStarted();
        void createToolbar();
        void updateTotalProgress();
        void viewImage(int itemIndex);
        void SetInitialFocus() override;
        int currentTab_;
        CResultsListView uploadListView_;
        bool isEnableNextButtonTimerRunning_;
        std::shared_ptr<UploadSession> uploadSession_;
        std::unique_ptr<UploadListModel> uploadListModel_;
        bool alreadyShortened_;
        ServerProfileGroup sessionImageServer_, sessionFileServer_;
        bool backgroundThreadStarted_;
        std::mutex backgroundThreadStartedMutex_;

        CComPtr<ITaskbarList3> ptl_;

        UploadManager* uploadManager_;
        CToolBarCtrl toolbar_;
        CFont commonProgressLabelFont_, commonPercentLabelFont_;
        CImageListManaged toolbarImageList_;
        CProgressBarCtrl uploadProgressBar_;
        CImageViewWindow imageViewWindow_;
};



#endif // IU_GUI_DIALOGS_UPLOADDLG_H
