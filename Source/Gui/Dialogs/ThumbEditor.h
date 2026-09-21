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

#ifndef THUMBEDITOR_H
#define THUMBEDITOR_H

#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include <atlframe.h>
#include "3rdpart/ColorButton.h"
#include "Gui/Controls/DialogIndirect.h"

class Thumbnail;
// CThumbEditor

class CThumbEditor :
    public CCustomDialogIndirectImpl<CThumbEditor>
{
public:
    explicit CThumbEditor(Thumbnail *thumb);

    enum { IDD = IDD_THUMBEDITOR };

    BEGIN_MSG_MAP(CThumbEditor)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
          COMMAND_HANDLER(IDC_ADDFILESIZE, BN_CLICKED, OnShowTextCheckboxClicked)
          COMMAND_HANDLER(IDC_DRAWFRAME, BN_CLICKED, OnDrawFrameCheckboxClicked)
          COMMAND_HANDLER(IDC_THUMBFONT, BN_CLICKED, OnFontSelect);
            REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    protected:
         // Handler prototypes:
         //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
         //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
         //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnShowTextCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnDrawFrameCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnFontSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

        void LoadParams();
        void SaveParams();
        CColorButton FrameColor;
        LOGFONT ThumbFont;
        Thumbnail * thumb_;
        CColorButton Color1,Color2,ThumbTextColor,TextColor, StrokeColor;
        void ShowTextCheckboxChanged();
        void DrawFrameCheckboxChanged();
};



#endif // THUMBEDITOR_H
