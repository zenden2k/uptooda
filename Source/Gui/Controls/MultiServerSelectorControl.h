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

#ifndef IU_GUI_CONTROLS_MULTISERVERSELECTORCONTROL_H
#define IU_GUI_CONTROLS_MULTISERVERSELECTORCONTROL_H


#pragma once

#include <functional>
#include <memory>
#include <boost/signals2.hpp>

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Gui/Dialogs/SettingsPage.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Upload/ServerProfileGroup.h"
#include "Gui/Controls/DialogIndirect.h"


class UploadEngineManager;

class CMultiServerSelectorControl :
    public CDialogImpl<CMultiServerSelectorControl>
{
public:
    explicit CMultiServerSelectorControl(UploadEngineManager* uploadEngineManager, bool defaultServer = false, bool isChildWindow = true);

    enum {
        IDD = IDD_MULTISERVERSELECTORCONTROL
    };

    BEGIN_MSG_MAP(CMultiServerSelectorControl)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDC_CHOOSESERVERS, BN_CLICKED, OnClickedChoose)
    END_MSG_MAP()

    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnClickedChoose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    void TranslateUI();
    void setTitle(CString title);
    CString getTitle() const;
    void setServerProfileGroup(const ServerProfileGroup& serverProfile);
    void setServersMask(int mask);

    const ServerProfileGroup& serverProfileGroup() const;
private:
    ServerProfileGroup serverProfileGroup_;
    int serversMask_;
    void serverChanged();
    UploadEngineManager* uploadEngineManager_;
    CString title_;
    CFont serverGroupboxFont_;
    std::function<void(CMultiServerSelectorControl*)> onChangeCallback_;
    boost::signals2::scoped_connection profileListChangedConnection_;

    void updateInfoLabel();
    void profileListChanged(BasicSettings* settings, const std::vector<std::string>& affectedServers);
};

#endif


