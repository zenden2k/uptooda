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

#ifndef IU_GUI_DIALOGS_HISTORYWINDOW_H
#define IU_GUI_DIALOGS_HISTORYWINDOW_H
#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Gui/Controls/HistoryTreeControl.h"
#include "Gui/Controls/DialogIndirect.h"
#include "Gui/Controls/ProgressRingControl.h"

class CWizardDlg;
class CHistoryReader;

// CHistoryWindow

constexpr int ID_OPENINBROWSER = 13000;
constexpr int ID_COPYTOCLIPBOARD = ID_OPENINBROWSER + 1;
constexpr int ID_VIEWBBCODE = ID_OPENINBROWSER + 2;
constexpr int ID_OPENFOLDER = ID_OPENINBROWSER + 3;
constexpr int ID_EDITFILEONSERVER = ID_OPENINBROWSER + 4;
constexpr int ID_DELETEFILEONSERVER = ID_OPENINBROWSER + 5;
constexpr int ID_COPYVIEWLINK = ID_OPENINBROWSER + 6;
constexpr int ID_COPYTHUMBLINK = ID_OPENINBROWSER + 7;
constexpr int WM_MY_OPENHISTORYFILE = WM_USER + 101;


class CHistoryWindow : public CCustomDialogIndirectImpl<CHistoryWindow>,
    public CDialogResize <CHistoryWindow>,
    public CWinDataExchange <CHistoryWindow>,
    public CMessageFilter
{
    public:
        CHistoryWindow(CWizardDlg* wizardDlg);

        enum { IDD = IDD_HISTORYWINDOW };
        BOOL PreTranslateMessage(MSG* pMsg) override;

        BEGIN_MSG_MAP(CHistoryWindow)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
            MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
            MESSAGE_HANDLER(WM_MY_OPENHISTORYFILE, OnWmOpenHistoryFile)
            MESSAGE_HANDLER(WM_VKEYTOITEM, OnHistoryTreeVkeyToItem)
            COMMAND_ID_HANDLER(IDOK, OnOk)
            COMMAND_ID_HANDLER(ID_OPENINBROWSER, OnOpenInBrowser)
            COMMAND_ID_HANDLER(ID_COPYTOCLIPBOARD, OnCopyToClipboard)
            COMMAND_ID_HANDLER(ID_COPYVIEWLINK, OnCopyViewLink)
            COMMAND_ID_HANDLER(ID_COPYTHUMBLINK, OnCopyThumbLink)
            COMMAND_ID_HANDLER(ID_VIEWBBCODE, OnViewBBCode)
            COMMAND_ID_HANDLER(ID_OPENFOLDER, OnOpenFolder)
            COMMAND_ID_HANDLER(ID_EDITFILEONSERVER, OnEditFileOnServer)
            COMMAND_ID_HANDLER(ID_DELETEFILEONSERVER, OnDeleteFileOnServer)
            COMMAND_HANDLER(IDC_DOWNLOADTHUMBS, BN_CLICKED, OnDownloadThumbsCheckboxChecked)
            COMMAND_HANDLER(IDC_CLEARHISTORYBTN, BN_CLICKED, OnBnClickedClearHistoryBtn)
            COMMAND_HANDLER(IDC_DATEFROMCHECKBOX, BN_CLICKED, OnDateFromCheckboxClicked)
            COMMAND_HANDLER(IDC_CLEARFILTERS, BN_CLICKED, OnClearFilters)

            CHAIN_MSG_MAP(CDialogResize<CHistoryWindow>)
            REFLECT_NOTIFICATIONS()
        END_MSG_MAP()

        BEGIN_DLGRESIZE_MAP(CHistoryWindow)
            DLGRESIZE_CONTROL(IDC_HISTORYTREE, DLSZ_SIZE_X | DLSZ_SIZE_Y)
            DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_FILESCOUNTLABEL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_FILESCOUNTDESCR, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_SESSIONSCOUNTLABEL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_SESSIONSCOUNTDESCR, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_UPLOADTRAFFICDESCR, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_UPLOADTRAFFICLABEL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_FILTERSGROUPBOX, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_DATEFROMCHECKBOX, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_DATEFROMPICKER, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_DATETOLABEL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_DATETOPICKER, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_FILENAMELABEL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_FILENAMEEDIT, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_URLLABEL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_URLEDIT, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_CLEARFILTERS, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_CLEARHISTORYBTN, DLSZ_MOVE_Y)
        END_DLGRESIZE_MAP()

        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnDownloadThumbsCheckboxChecked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnWmOpenHistoryFile(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnDateFromCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClearFilters(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnHistoryTreeVkeyToItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnHistoryTreeCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);

        // Context menu callbacks
        LRESULT OnOpenInBrowser(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnCopyToClipboard(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnCopyViewLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnCopyThumbLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnViewBBCode(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnOpenFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnEditFileOnServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnDeleteFileOnServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnBnClickedClearHistoryBtn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

        void Show();
        void FillList(CHistoryReader* mgr);
        CHistoryTreeControl m_treeView;
        std::unique_ptr<CHistoryReader> m_historyReader;

        void threadsStarted();
        void threadsFinished();
        void onItemDblClick(TreeItem* item);
        bool delayedLoad_;
        std::vector<CString> m_HistoryFiles;
        bool delayedClosing_;
        CString historyFolder;
        CWizardDlg* wizardDlg_;
        void LoadHistory();
        void OpenInBrowser(const TreeItem* item) const;
        void applyFilters();
        void dateFromCheckboxChanged();
        void initSearchForm();
        CProgressRingControl m_wndAnimation;
        CDateTimePickerCtrl dateFromPicker_, dateToPicker_;
        CButton dateFilterCheckbox_;
};

#endif
