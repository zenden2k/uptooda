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

#ifndef IntegrationSettings_H
#define IntegrationSettings_H

#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Gui/Dialogs/settingspage.h"
#include "Core/Upload/ServerProfile.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/Constants.h"

class UploadEngineManager;
class CIntegrationSettings : public CDialogImpl<CIntegrationSettings>,
                             public CWinDataExchange<CIntegrationSettings>,
                             public CSettingsPage
{
    public:
        enum { IDD = IDD_INTEGRATIONSETTINGS };

        CIntegrationSettings(UploadEngineManager *uploadEngineManager);
        bool apply() override;

        struct ListItemData {
            ServerProfile serverProfile;
            CString name;
            bool invalid;
            CString itemId;
            ListItemData() {
                invalid = false;
            }
        };

    protected:
        BEGIN_MSG_MAP(CIntegrationSettings)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
            MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)
            COMMAND_HANDLER(IDC_SHELLINTEGRATION, BN_CLICKED, OnShellIntegrationCheckboxChanged)
            COMMAND_HANDLER(IDC_ADDITEM, BN_CLICKED, OnBnClickedAdditem)
            COMMAND_HANDLER(IDC_DELETEITEM, BN_CLICKED, OnBnClickedDeleteitem)
            COMMAND_HANDLER(IDC_DOWNBUTTON, BN_CLICKED, OnBnClickedDownbutton)
            COMMAND_HANDLER(IDC_UPBUTTON, BN_CLICKED, OnBnClickedUpbutton)
            REFLECT_NOTIFICATIONS()
        END_MSG_MAP()

        BEGIN_DDX_MAP(CIntegrationSettings)
        DDX_CONTROL_HANDLE(IDC_ADDITEM, addItemButton_)
        DDX_CONTROL_HANDLE(IDC_DELETEITEM, deleteItemButton_)
        DDX_CONTROL_HANDLE(IDC_UPBUTTON, upButton_)
        DDX_CONTROL_HANDLE(IDC_DOWNBUTTON, downButton_)
        END_DDX_MAP()
        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnShellIntegrationCheckboxChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnBnClickedAdditem(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnBnClickedDeleteitem(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnBnClickedDownbutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnBnClickedUpbutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

        void ShellIntegrationChanged();
        void createResources();
        ServerProfilesMap serverProfiles_;
        CListBox menuItemsListBox_;
        bool menuItemsChanged_;
        CButton upButton_;
        CButton downButton_;
        CButton addItemButton_;
        CButton deleteItemButton_;
        UploadEngineManager *uploadEngineManager_;
        CIcon iconAdd_, iconDelete_, iconUp_, iconDown_;
        CToolTipCtrl toolTipCtrl_;
};

#endif // IntegrationSettings_H

