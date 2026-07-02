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
#ifndef LOGINDLG_H
#define LOGINDLG_H

#pragma once
#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Core/Upload/UploadEngine.h"
#include "3rdpart/ColorButton.h"
#include "Gui/Controls/DialogIndirect.h"

class CUploadParamsDlg : public CCustomDialogIndirectImpl<CUploadParamsDlg>
{
    public:
        CUploadParamsDlg(ServerProfile& serverProfile, bool showImageProcessingParams = true, bool defaultServer = false);

        enum { IDD = IDD_UPLOADPARAMSDLG };
        ImageUploadParams imageUploadParams() const;
    protected:
        BEGIN_MSG_MAP(CUploadParamsDlg)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
            COMMAND_HANDLER(IDC_CREATETHUMBNAILS, BN_CLICKED, OnClickedCreateThumbnailsCheckbox)
            COMMAND_HANDLER(IDC_DEFAULTSETTINGSCHECKBOX, BN_CLICKED, OnClickedDefaultSettingsCheckbox)
            COMMAND_HANDLER(IDC_DEFAULTTHUMBSETTINGSCHECKBOX, BN_CLICKED, OnClickedDefaultThumbSettingsCheckbox)
            COMMAND_HANDLER(IDC_THUMBTEXTCHECKBOX, BN_CLICKED, OnClickedThumbTextCheckbox)
            COMMAND_HANDLER(IDC_USESERVERTHUMBNAILS, BN_CLICKED, OnClickedUseServerThumbnailsCheckbox)
            COMMAND_HANDLER(IDC_PROCESSIMAGESCHECKBOX, BN_CLICKED, OnClickedProcessImagesCheckbox)
            REFLECT_NOTIFICATIONS()
        END_MSG_MAP()
        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedCreateThumbnailsCheckbox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedProcessImagesCheckbox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedDefaultSettingsCheckbox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedDefaultThumbSettingsCheckbox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedThumbTextCheckbox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedUseServerThumbnailsCheckbox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

        void createThumbnailsCheckboxChanged();
        void processImagesChanged();
        void defaultSettingsCheckboxChanged();
        void defaultThumbSettingsCheckboxChanged();
        void thumbTextCheckboxChanged();
        void useServerThumbnailsChanged();
        CUploadEngineData *m_UploadEngine;
        CColorButton ThumbBackground_;
        ImageUploadParams params_;
        ServerProfile&  serverProfile_;
        CComboBox profileCombo_, thumbTemplateCombo_;
        bool defaultServer_;
        bool showImageProcessingParams_;
};

#endif // LOGINDLG_H


