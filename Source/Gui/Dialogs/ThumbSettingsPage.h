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
#ifndef IU_GUI_DIALOGS_THUMBSETTINGSPAGE_H
#define IU_GUI_DIALOGS_THUMBSETTINGSPAGE_H

#pragma once

#include <string>
#include <memory>

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Gui/Controls/MyImage.h"
#include "SettingsPage.h"
#include "3rdpart/ColorButton.h"
#include "Core/Images/ImageConverter.h"
#include "Gui/Constants.h"

// CThumbSettingsPage

class CThumbSettingsPage : public CDialogImpl<CThumbSettingsPage>,
                           public CSettingsPage
{
public:
    CThumbSettingsPage();

    enum { IDD = IDD_THUMBSETTINGSPAGE };

    BEGIN_MSG_MAP(CThumbSettingsPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)
        COMMAND_HANDLER(IDC_NEWTHUMBNAIL, BN_CLICKED, OnBnClickedNewThumbnail)
        COMMAND_HANDLER(IDC_THUMBSCOMBO, CBN_SELCHANGE, OnThumbComboChanged)
        COMMAND_HANDLER(IDC_EDITTHUMBNAILPRESET, BN_CLICKED, OnEditThumbnailPreset)
        COMMAND_HANDLER(IDC_THUMBTEXTCHECKBOX, BN_CLICKED,OnThumbTextCheckboxClick )
        COMMAND_HANDLER(IDC_THUMBTEXT, EN_CHANGE, OnThumbTextChange)
        COMMAND_HANDLER_EX(IDC_HEIGHTCHECKBOX, BN_CLICKED, OnWidthEditChange)
        COMMAND_HANDLER_EX(IDC_WIDTHCHECKBOX, BN_CLICKED, OnWidthEditChange)
        COMMAND_HANDLER_EX(IDC_HEIGHTEDIT, EN_CHANGE, OnWidthEditChange)
        COMMAND_HANDLER_EX(IDC_WIDTHEDIT, EN_CHANGE, OnWidthEditChange)
        COMMAND_HANDLER_EX(IDC_THUMBMACROSES, BN_CLICKED, OnThumbMacrosButtonClicked)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        bool apply() override;
    protected:
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnThumbTextChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        CMyImage img;
        std::string getSelectedThumbnailFileName() const;
        std::string getSelectedThumbnailName() const;
        LRESULT OnEditThumbnailPreset(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnThumbTextCheckboxClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        void ThumbTextCheckboxChange();
        LRESULT OnThumbComboChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnBnClickedNewThumbnail(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnThumbMacrosButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);

        void showSelectedThumbnailPreview();
        bool CreateNewThumbnail();
        void createResources();
        LRESULT OnWidthEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);
        std::unique_ptr<Gdiplus::Bitmap> createSampleImage(int width, int height);
        ThumbCreatingParams params_;
        std::map<std::string, std::unique_ptr<Thumbnail>> thumb_cache_;
        bool m_CatchFormChanges;
        CColorButton ThumbBackground;
        CComboBox thumbsCombo_;
        CButton thumbTextMacrosButton_;
        CIcon iconInfo_;
        CEdit thumbTextEdit_;
        CToolTipCtrl tooltipControl_;
};

#endif // IU_GUI_DIALOGS_THUMBSETTINGSPAGE_H

