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
#ifndef VIDEOGRABBERPARAMS_H
#define VIDEOGRABBERPARAMS_H


#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "settingspage.h"
#include <atlcrack.h>
#include "3rdpart/ColorButton.h"
// CVideoGrabberParams

class CVideoGrabberSettingsPage :
    public CDialogImpl<CVideoGrabberSettingsPage>,
    public CSettingsPage,
    public CSettingsPageTrait<CVideoGrabberSettingsPage>
{
public:
    CVideoGrabberSettingsPage();
    enum { IDD = IDD_VIDEOGRABBERPARARAMS};

    BEGIN_MSG_MAP(CVideoGrabberParams)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER_EX(IDC_MEDIAINFOFONT, OnMediaInfoFontClicked)
        COMMAND_HANDLER(IDC_MEDIAINFOONIMAGE,BN_CLICKED, OnShowMediaInfoTextBnClicked)
        COMMAND_HANDLER(IDC_VIDEOSNAPSHOTSFOLDERBUTTON, BN_CLICKED, OnVideoSnapshotsFolderButtonClicked);
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    bool apply() override;
    LRESULT OnMediaInfoFontClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnShowMediaInfoTextBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnVideoSnapshotsFolderButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
private:
    LOGFONT m_Font;
    CColorButton textColorButton_;
};



#endif // VIDEOGRABBERPARAMS_H
