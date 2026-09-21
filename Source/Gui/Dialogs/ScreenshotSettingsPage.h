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
#ifndef SCREENSHOTSETTINGSPAGE_H
#define SCREENSHOTSETTINGSPAGE_H

#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include <atlcrack.h>
#include "SettingsPage.h"

class CScreenshotSettingsPage :    public CDialogImpl<CScreenshotSettingsPage>,
                                     public CSettingsPage
{
    public:
        CScreenshotSettingsPage() = default;
        enum { IDD = IDD_SCREENSHOTSETTINGSPAGE};

    protected:
        BEGIN_MSG_MAP(CScreenshotSettingsPage)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)
            COMMAND_HANDLER(IDC_SCREENSHOTSFOLDERSELECT, BN_CLICKED, OnScreenshotsFolderSelect)
            COMMAND_HANDLER(IDC_SCREENSHOTMACROSBUTTON, BN_CLICKED, OnMacrosButtonClicked)
        END_MSG_MAP()
        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnMacrosButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnScreenshotsFolderSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

        bool apply() override;
        void createResources();
        CEdit filepathTemplateEdit_;
        CButton filepathMacrosButton_;
        CToolTipCtrl tooltipControl_;
        CIcon iconInfo_;
};


#endif // SCREENSHOTSETTINGSPAGE_H
