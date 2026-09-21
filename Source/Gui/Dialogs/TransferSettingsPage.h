
#ifndef IU_GUI_DIALOGS_TRANSFERSETTINGSPAGE_H
#define IU_GUI_DIALOGS_TRANSFERSETTINGSPAGE_H


#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "settingspage.h"
// CTransferSettingsPage

class CTransferSettingsPage : 
    public CDialogImpl<CTransferSettingsPage>,
    public CSettingsPage,
    public CWinDataExchange<CTransferSettingsPage>,
    public CSettingsPageTrait<CTransferSettingsPage>
{
public:
    CTransferSettingsPage() = default;
    enum { IDD = IDD_TRANSFERSETTINGSPAGE };

    BEGIN_MSG_MAP(CTransferSettingsPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
        COMMAND_HANDLER(IDC_BROWSESCRIPTBUTTON, BN_CLICKED, OnBnClickedBrowseScriptButton)
        COMMAND_HANDLER(IDC_EXECUTESCRIPTCHECKBOX, BN_CLICKED, OnExecuteScriptCheckboxClicked)
    END_MSG_MAP()
        
    BEGIN_DDX_MAP(CTransferSettingsPage)
        DDX_CONTROL_HANDLE(IDC_JUSTURLRADIO, justURLRadioButton_)
        DDX_CONTROL_HANDLE(IDC_USELASTCODETYPERADIO, useLastCodeTypeRadioButton_)
    END_DDX_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnExecuteScriptCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    bool apply() override;
    void TranslateUI();
    LRESULT OnBnClickedBrowseScriptButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    void executeScriptCheckboxChanged();
protected:
    CToolTipCtrl toolTip_;
    CButton justURLRadioButton_, useLastCodeTypeRadioButton_;
};

#endif // IU_GUI_DIALOGS_TRANSFERSETTINGSPAGE_H


