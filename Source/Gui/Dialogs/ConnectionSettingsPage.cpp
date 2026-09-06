#include "ConnectionSettingsPage.h"

#include "Core/CommonDefs.h"
#include "WizardDlg.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Gui/Components/MyFileDialog.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/Helpers/DPIHelper.h"

void CConnectionSettingsPage::TranslateUI()
{
    TRC(IDC_CONNECTIONSETTINGS, "Connection settings");
    TRC(IDC_USEPROXYSERVER, "Use provided proxy:");
    TRC(IDC_USESYSTEMPROXY, "Use system proxy settings");
    TRC(IDC_NOPROXY, "No proxy (direct connection)");
    TRC(IDC_ADDRESSLABEL, "Address:");
    TRC(IDC_PORTLABEL, "Port:");
    TRC(IDC_SERVERTYPE, "Proxy type:");
    TRC(IDC_NEEDSAUTH, "Authorization on proxy");
    TRC(IDC_LOGINLABEL, "Login:");
    TRC(IDC_PASSWORDLABEL, "Password:");
    TRC(IDC_UPLOADBUFFERLABEL, "Upload Buffer Size:");
    TRC(IDC_UPLOADSPEEDLIMITLABEL, "Upload speed limit per thread:");
}
    
LRESULT CConnectionSettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    TranslateUI();

    BOOL temp;
    DoDataExchange(FALSE);
    int selectedProxyTypeIndex = 0;
    for (size_t i = 0; i < proxyTypes.size(); i++) {
        auto& item = proxyTypes[i];
		if (item.second == Settings.ConnectionSettings.ProxyType) {
            selectedProxyTypeIndex = i;
		}
        int index = serverTypeCombo_.AddString(item.first);
        serverTypeCombo_.SetItemData(index, item.second);
	}
    
    // ---- connection settings -----
    SetDlgItemText(IDC_ADDRESSEDIT, U2W(Settings.ConnectionSettings.ServerAddress));
    SendDlgItemMessage(IDC_NEEDSAUTH, BM_SETCHECK, (WPARAM) Settings.ConnectionSettings.NeedsAuth);
   
    SendDlgItemMessage(IDC_USEPROXYSERVER, BM_SETCHECK, (WPARAM)(Settings.ConnectionSettings.UseProxy == ConnectionSettingsStruct::kUserProxy)?TRUE:FALSE);
    SendDlgItemMessage(IDC_USESYSTEMPROXY, BM_SETCHECK, (WPARAM)(Settings.ConnectionSettings.UseProxy == ConnectionSettingsStruct::kSystemProxy)?TRUE:FALSE);
    SendDlgItemMessage(IDC_NOPROXY, BM_SETCHECK, (WPARAM)(Settings.ConnectionSettings.UseProxy == ConnectionSettingsStruct::kNoProxy)?TRUE:FALSE);

    SIZE radioButtonSize_{};

    // Auto-size useSystemProxy_ radio button, then move openSystemConnectionSettingsButton_
    // to the right of the radio button and center openSystemConnectionSettingsButton_ vertically 
    if (useSystemProxy_.GetIdealSize(&radioButtonSize_)) {
        useSystemProxy_.SetWindowPos(0, 0, 0, radioButtonSize_.cx, radioButtonSize_.cy, SWP_NOZORDER | SWP_NOMOVE);
        CRect useSystemProxyRect;
        if (useSystemProxy_.GetWindowRect(&useSystemProxyRect) && ScreenToClient(useSystemProxyRect)) {
            WORD unitX = LOWORD(GetDialogBaseUnits());
            // Horizontal margin of the button is equal to 3 base dialog units 
            int offsetX = MulDiv(3, unitX, 4);
            CRect buttonRect;
            if (openSystemConnectionSettingsButton_.GetWindowRect(buttonRect) && ScreenToClient(buttonRect)) {
                openSystemConnectionSettingsButton_.SetWindowPos(0, useSystemProxyRect.right + offsetX,
                    buttonRect.top + (buttonRect.Height() - useSystemProxyRect.Height()) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE
                );
            }
        }
    }

    createResources();
    openSystemConnectionSettingsButton_.SetIcon(externalLink_);

    // Creating tooltip control. We will use subclassing of controls to intercept their messages
    // The variant with usage of RelayEvent method is not available because we don't have access to message loop
    // that is running in DialogBox method of CSettingsDlg.
    toolTip_.Create(m_hWnd);
    CString tipText = TR("Open system connection settings");
    CToolInfo tip(TTF_SUBCLASS, openSystemConnectionSettingsButton_, 0, 0, const_cast<LPWSTR>(tipText.GetString()));
    toolTip_.AddTool(tip);

    SetDlgItemText(IDC_PROXYLOGINEDIT, U2W(Settings.ConnectionSettings.ProxyUser));
    SetDlgItemText(IDC_PROXYPASSWORDEDIT, static_cast<CString>(Settings.ConnectionSettings.ProxyPassword));

    if(Settings.ConnectionSettings.ProxyPort) 
        SetDlgItemInt(IDC_PORTEDIT, Settings.ConnectionSettings.ProxyPort);

    serverTypeCombo_.SetCurSel(selectedProxyTypeIndex);

    SendDlgItemMessage(IDC_NEEDSAUTH, BM_SETCHECK, (WPARAM) Settings.ConnectionSettings.NeedsAuth);
    OnClickedUseProxy(BN_CLICKED, IDC_USEPROXYSERVER, 0, temp);
    SetDlgItemText(IDC_SCRIPTFILENAMEEDIT, IuCoreUtils::Utf8ToWstring(Settings.ScriptFileName).c_str());
    SetDlgItemInt(IDC_UPLOADSPEEDLIMITEDIT, Settings.MaxUploadSpeed);
	
    return 1;  // Let the system set the focus
}

LRESULT CConnectionSettingsPage::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createResources();
    openSystemConnectionSettingsButton_.SetIcon(externalLink_);
    return 0;
}

LRESULT CConnectionSettingsPage::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CConnectionSettingsPage::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CConnectionSettingsPage::OnClickedUseProxy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
    proxyRadioChanged();
    return 0;
}
    
LRESULT CConnectionSettingsPage::OnClickedUseProxyAuth(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool Checked = SendDlgItemMessage(wID, BM_GETCHECK)!=0;
    GuiTools::EnableNextN(GetDlgItem(wID), 4, Checked);
    return 0;
}

LRESULT CConnectionSettingsPage::OnClickedNoProxy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    proxyRadioChanged();
    return 0;
}

LRESULT CConnectionSettingsPage::OnClickedUseSystemProxy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    proxyRadioChanged();
    return 0;
}

bool CConnectionSettingsPage::apply()
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    
    DoDataExchange(TRUE);
    if (SendDlgItemMessage(IDC_USEPROXYSERVER, BM_GETCHECK) != 0) {
        Settings.ConnectionSettings.UseProxy = ConnectionSettingsStruct::kUserProxy;
    } else if (SendDlgItemMessage(IDC_USESYSTEMPROXY, BM_GETCHECK) != 0) {
        Settings.ConnectionSettings.UseProxy = ConnectionSettingsStruct::kSystemProxy;
    } else {
        Settings.ConnectionSettings.UseProxy = ConnectionSettingsStruct::kNoProxy;
    }
    
    Settings.ConnectionSettings.NeedsAuth = SendDlgItemMessage(IDC_NEEDSAUTH, BM_GETCHECK)!=0;

    CString proxyAddress;
    TCHAR Buffer[128];
    addressEdit_.GetWindowText(proxyAddress);

    Settings.ConnectionSettings.ServerAddress = W2U(proxyAddress);
    Settings.ConnectionSettings.ProxyPort = GetDlgItemInt(IDC_PORTEDIT);

    if (Settings.ConnectionSettings.UseProxy == ConnectionSettingsStruct::kUserProxy && proxyAddress.IsEmpty() /* || Settings.ConnectionSettings.ProxyPort == 0*/) {
        Settings.ConnectionSettings.UseProxy = ConnectionSettingsStruct::kNoProxy;
    }
    
    GetDlgItemText(IDC_PROXYLOGINEDIT, Buffer, 128);
    Settings.ConnectionSettings.ProxyUser = W2U(Buffer);
    GetDlgItemText(IDC_PROXYPASSWORDEDIT, Buffer, 128);
    Settings.ConnectionSettings.ProxyPassword = Buffer;
    Settings.ConnectionSettings.ProxyType = serverTypeCombo_.GetItemData(serverTypeCombo_.GetCurSel());

	int maxUploadSpeed = GetDlgItemInt(IDC_UPLOADSPEEDLIMITEDIT);
	if (maxUploadSpeed < 0) {
        maxUploadSpeed = 0;
	}
    Settings.MaxUploadSpeed = maxUploadSpeed;

    return true;
}

void CConnectionSettingsPage::proxyRadioChanged() {
    bool checked = SendDlgItemMessage(IDC_USEPROXYSERVER, BM_GETCHECK) != 0;
    bool useSystemProxyChecked = useSystemProxy_.GetCheck() == BST_CHECKED;
    openSystemConnectionSettingsButton_.EnableWindow(useSystemProxyChecked);
    GuiTools::EnableNextN(GetDlgItem(IDC_USEPROXYSERVER), checked ? 8 : 11, checked);

    BOOL bHandled = false;
    if (checked) {
        OnClickedUseProxyAuth(BN_CLICKED, IDC_NEEDSAUTH, nullptr, bHandled);
    }
}

void CConnectionSettingsPage::createResources() {
    const UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);
    if (externalLink_) {
        externalLink_.DestroyIcon();
    }
    externalLink_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONEXTERNALLINK), iconWidth, iconHeight);
}


LRESULT CConnectionSettingsPage::OnOpenSystemConnectionSettingsClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if (IsWindows10OrGreater() && WinUtils::ShellOpenFileOrUrl(_T("ms-settings:network-proxy"), m_hWnd)) {
        return 0;
    }
    HINSTANCE hinst = ShellExecute(0, _T("open"), _T("rundll32.exe"), _T("inetcpl.cpl,LaunchConnectionDialog"), NULL, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(hinst) <= 32) {
        LOG(ERROR) << "ShellExecute failed. Error code=" << reinterpret_cast<INT_PTR>(hinst);
    }
    return 0;
}
