
#ifndef IU_GUI_DIALOGS_CONNECTIONSETTINGSPAGE_H
#define IU_GUI_DIALOGS_CONNECTIONSETTINGSPAGE_H


#pragma once

#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "settingspage.h"
// CConnectionSettingsPage

class CConnectionSettingsPage : 
    public CDialogImpl<CConnectionSettingsPage>,
    public CSettingsPage,
    public CWinDataExchange<CConnectionSettingsPage>
{
public:
    CConnectionSettingsPage() = default;

    enum { IDD = IDD_CONNECTIONSETTINGSPAGE };

    BEGIN_MSG_MAP(CConnectionSettingsPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDpiChanged)
        COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
        COMMAND_HANDLER(IDC_USEPROXYSERVER, BN_CLICKED, OnClickedUseProxy)
        COMMAND_HANDLER(IDC_NEEDSAUTH, BN_CLICKED, OnClickedUseProxyAuth)
        COMMAND_HANDLER(IDC_NOPROXY, BN_CLICKED, OnClickedNoProxy)
        COMMAND_HANDLER(IDC_USESYSTEMPROXY, BN_CLICKED, OnClickedUseSystemProxy)
        COMMAND_HANDLER(IDC_OPENSYSTEMCONNECTION, BN_CLICKED, OnOpenSystemConnectionSettingsClicked)
    END_MSG_MAP()
        
    BEGIN_DDX_MAP(CConnectionSettingsPage)
        DDX_CONTROL_HANDLE(IDC_SERVERTYPECOMBO, serverTypeCombo_)
        DDX_CONTROL_HANDLE(IDC_OPENSYSTEMCONNECTION, openSystemConnectionSettingsButton_)
        DDX_CONTROL_HANDLE(IDC_USESYSTEMPROXY, useSystemProxy_)
        DDX_CONTROL_HANDLE(IDC_ADDRESSEDIT, addressEdit_)
    END_DDX_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedUseProxy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnClickedUseProxyAuth(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnClickedNoProxy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedUseSystemProxy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOpenSystemConnectionSettingsClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    bool apply() override;
    void TranslateUI();
protected:
    CComboBox serverTypeCombo_;
    CButton openSystemConnectionSettingsButton_, useSystemProxy_;
    CIcon externalLink_;
    CToolTipCtrl toolTip_;
    CEdit addressEdit_;
    std::vector<std::pair<CString, int>> proxyTypes = {
        { _T("HTTP"), 0 },
        { _T("HTTPS"), 5 },
        { _T("SOCKS4"), 1 },
        { _T("SOCKS4A"), 2 },
        { _T("SOCKS5"), 3 },
        { _T("SOCKS5(DNS)"), 4 },
    };

    void proxyRadioChanged();
    void createResources();
};

#endif // IU_GUI_DIALOGS_CONNECTIONSETTINGSPAGE_H


