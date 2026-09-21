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
#ifndef IU_GUI_DIALOGS_LOGOSETTINGS_H
#define IU_GUI_DIALOGS_LOGOSETTINGS_H

#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Gui/Controls/myimage.h"
#include "SettingsPage.h"
#include "3rdpart/ColorButton.h"
#include "Core/Images/ImageConverter.h"
#include "Gui/Constants.h"

constexpr int IDC_NEWPROFILE = 10001;
constexpr int IDC_SAVEPROFILE = 10002;
constexpr int IDC_DELETEPROFILE = 10003;

class CLogoSettings :
    public CDialogImpl<CLogoSettings>, public CSettingsPage
{
    public:
        CLogoSettings();
        enum { IDD = IDD_LOGOSETTINGS };

        BEGIN_MSG_MAP(CLogoSettings)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)
            COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
            COMMAND_HANDLER(IDC_LOGOBROWSE, BN_CLICKED, OnBnClickedLogobrowse)
            COMMAND_HANDLER(IDC_SELECTFONT, BN_CLICKED, OnBnClickedSelectfont)
            COMMAND_HANDLER(IDC_THUMBFONT, BN_CLICKED, OnBnClickedThumbfont)
            COMMAND_HANDLER_EX(IDC_YOURLOGO, BN_CLICKED, OnYourLogoCheckboxClicked)
            COMMAND_HANDLER_EX(IDC_YOURTEXT, BN_CLICKED, OnAddTextChecboxClicked)
            COMMAND_ID_HANDLER_EX(IDC_NEWPROFILE, OnNewProfile)
            COMMAND_ID_HANDLER_EX(IDC_SAVEPROFILE, OnSaveProfile)
            COMMAND_ID_HANDLER_EX(IDC_DELETEPROFILE, OnDeleteProfile)
            COMMAND_HANDLER_EX(IDC_PROFILECOMBO, CBN_SELCHANGE, OnProfileComboSelChange)
            NOTIFY_HANDLER( IDC_SELECTCOLOR, CPN_SELCHANGE, OnProfileEditedNotification)
            NOTIFY_HANDLER( IDC_STROKECOLOR, CPN_SELCHANGE, OnProfileEditedNotification)
            COMMAND_HANDLER(IDC_FORMATLIST, CBN_SELCHANGE, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_QUALITYEDIT, EN_CHANGE, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_IMAGEWIDTH, EN_CHANGE, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_IMAGEHEIGHT, EN_CHANGE, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_RESIZEMODECOMBO, CBN_SELCHANGE, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_SMARTCONVERTING, BN_CLICKED, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_LOGOPOSITION, CBN_SELCHANGE, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_EDITYOURTEXT, EN_CHANGE, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_TEXTPOSITION, CBN_SELCHANGE, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_LOGOEDIT, EN_CHANGE, OnProfileEditedCommand)
            COMMAND_HANDLER(IDC_SKIPANIMATEDCHECKBOX, BN_CLICKED, OnProfileEditedCommand)
            REFLECT_NOTIFICATIONS()
        END_MSG_MAP()
protected:
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    CMyImage img;
    LRESULT OnProfileEditedCommand(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnProfileEditedNotification(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnBnClickedLogobrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedSelectfont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedThumbfont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    CColorButton FrameColor;
    CColorButton Color1, Color2, ThumbTextColor, TextColor, StrokeColor;
    //LogoParams *params;
    LOGFONT lf, ThumbFont;
    CString CurrentProfileOriginalName;
    bool apply() override;
    void ShowParams(const ImageConvertingParams& params);
    void ShowParams(const CString& profileName);
    bool SaveParams(ImageConvertingParams& params);
    std::map<CString, ImageConvertingParams> convert_profiles_;
    CImageListManaged profileEditToolbarImagelist_;
    LRESULT OnYourLogoCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnAddTextChecboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    CToolBarCtrl m_ProfileEditToolbar;
    void UpdateProfileList();
    LRESULT OnSaveProfile(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnNewProfile(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnDeleteProfile(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    CString CurrentProfileName;
    LRESULT OnProfileComboSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    void ProfileChanged();
    void createProfileToolbar();
    bool m_CatchChanges;
    bool m_ProfileChanged;
    CComboBox profileCombobox_;
public:
    void TranslateUI();
};

#endif


