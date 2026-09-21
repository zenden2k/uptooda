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

#include "QuickSetupDlg.h"

#include "Core/CommonDefs.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Gui/Dialogs/ServerParamsDlg.h"
#include "Core/ServiceLocator.h"
#include "Func/MyEngineList.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/AbstractServerIconCache.h"
#include "Gui/Helpers/DPIHelper.h"

LRESULT CQuickSetupDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    translateUI();
    SetWindowText(APP_NAME);
    CString titleText;
    titleText.Format(TR("%s - Quick Setup"), APP_NAME);
    SetDlgItemText(IDC_TITLE, titleText );

    CenterWindow();
    hIcon = static_cast<HICON>(::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME),
        IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
    SetIcon(hIcon, TRUE);
    hIconSmall = GuiTools::LoadSmallIcon(IDR_MAINFRAME);
    SetIcon(hIconSmall, FALSE);
    serverComboBox_.Attach( GetDlgItem( IDC_SERVERCOMBOBOX ) );

    /*if (!settings->IsPortable) {
        SendDlgItemMessage( IDC_AUTOSTARTUPCHECKBOX, BM_SETCHECK, BST_CHECKED, 0);
        SendDlgItemMessage( IDC_CAPTUREPRINTSCREENCHECKBOX, BM_SETCHECK, BST_CHECKED, 0);
        SendDlgItemMessage( IDC_EXPLORERINTEGRATION, BM_SETCHECK, BST_CHECKED, 0);
    }*/
    LogoImage.SubclassWindow(GetDlgItem( IDC_STATICLOGO ) );
    LogoImage.SetWindowPos(0, 0,0, 48, 48, SWP_NOMOVE | SWP_NOZORDER );
    LogoImage.loadImage(0, 0, IDR_ICONMAINNEW, false, GetSysColor(COLOR_BTNFACE));

    HFONT font = GetFont();
    LOGFONT alf;

    bool ok = ::GetObject(font, sizeof(LOGFONT), &alf) == sizeof(LOGFONT);

    if(ok)
    {
        alf.lfWeight = FW_BOLD;

        NewFont=CreateFontIndirect(&alf);

        CClientDC dc(m_hWnd);
        alf.lfHeight  =  - MulDiv(11, dc.GetDeviceCaps(LOGPIXELSY), 72);
        NewFont = CreateFontIndirect(&alf);
        SendDlgItemMessage(IDC_TITLE,WM_SETFONT,(WPARAM)(HFONT)NewFont,MAKELPARAM(false, 0));
    }

    comboBoxImageList_.Create(16,16,ILC_COLOR32 | ILC_MASK,0,6);

    int selectedIndex = 0;
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    //CUploadEngineData *uploadEngine = _EngineList->byIndex( Settings.getServerID() );
    std::string selectedServerName = settings->imageServer.getByIndex(0).serverName();
    TCHAR line[40];
    for (int i = 0; i < ARRAY_SIZE(line) - 1; i++) {
        line[i] = '-';
    }
    line[ARRAY_SIZE(line) - 1] = 0;
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < myEngineList->count(); i++) {
            const CUploadEngineData * ue = myEngineList->byIndex(i);
            if ((!ue->hasType(CUploadEngineData::TypeImageServer) && j == 0)|| (!ue->hasType(CUploadEngineData::TypeFileServer) && j == 1)) {
                continue;
            }
            HICON hImageIcon = ServiceLocator::instance()->serverIconCache()->getIconForServer(ue->Name, dpi, true);
            int nImageIndex = -1;
            if (hImageIcon) {
                nImageIndex = comboBoxImageList_.AddIcon(hImageIcon);
            }
            char *serverName = new char[ue->Name.length() + 1];
            lstrcpyA(serverName, ue->Name.c_str());
            int itemIndex = serverComboBox_.AddItem(Utf8ToWCstring(myEngineList->getServerDisplayName(ue)), nImageIndex, nImageIndex, 1, reinterpret_cast<LPARAM>(serverName));
            if (ue->Name == selectedServerName) {
                selectedIndex = itemIndex;
            }
        }
        if (j == 0) {
            serverComboBox_.AddItem(line, -1, -1, 0);
        }
    }
    serverComboBox_.SetImageList( comboBoxImageList_ );
    serverComboBox_.SetCurSel( selectedIndex );

    doAuthCheckboxChanged();

    serverChanged();

    return 1;
}

LRESULT CQuickSetupDlg::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int count = serverComboBox_.GetCount();
    for ( int i = 0; i < count ; i++ ) {
        char* serverName = reinterpret_cast<char*>(serverComboBox_.GetItemData(i));
        delete[] serverName;
    }
    serverComboBox_.ResetContent();
    return 0;
}

LRESULT CQuickSetupDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    defer d([&] { // Run at function exit
        ::EnableWindow(GetDlgItem(IDOK), TRUE);
    });

    ::EnableWindow(GetDlgItem(IDOK), FALSE);
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    int serverComboElementIndex = serverComboBox_.GetCurSel();
    if ( serverComboElementIndex > 0 ) {
        std::string serverNameA = reinterpret_cast<char*>(serverComboBox_.GetItemData(serverComboElementIndex));
        const CUploadEngineData * uploadEngineData = myEngineList->byName(serverNameA);
        Settings.imageServer.getByIndex(0).setServerName(uploadEngineData->Name) ;
        bool needAuth = GuiTools::GetCheck( m_hWnd, IDC_DOAUTHCHECKBOX );
        if ( needAuth ) {
            CString login = GuiTools::GetDlgItemText( m_hWnd, IDC_LOGINEDIT );
            Settings.imageServer.getByIndex(0).setProfileName(WCstringToUtf8(login));
            CString password = GuiTools::GetDlgItemText( m_hWnd, IDC_PASSWORDEDIT );
            if ( login.IsEmpty() ) {
                LocalizedMessageBox(TR("Enter your account information"), APP_NAME, MB_ICONEXCLAMATION);
                return 0;
            }
            LoginInfo& loginInfo = Settings.ServersSettings[W2U(Settings.getServerName())][W2U(login)].authData;
            loginInfo.DoAuth = true;
            loginInfo.Login = WCstringToUtf8( login );
            loginInfo.Password = WCstringToUtf8( password );
        }
        Settings.quickScreenshotServer = Settings.imageServer;
        Settings.contextMenuServer = Settings.imageServer;
    } else {

    }

    GuiTools::GetCheck(m_hWnd, IDC_AUTOSTARTUPCHECKBOX, Settings.AutoStartup);
    Settings.AutoStartup_changed = true;


    Settings.ExplorerContextMenu = GuiTools::GetCheck(m_hWnd, IDC_EXPLORERINTEGRATION);
    Settings.ExplorerContextMenu_changed = Settings.ExplorerContextMenu;

    bool capturePrintScreen = GuiTools::GetCheck( m_hWnd, IDC_CAPTUREPRINTSCREENCHECKBOX );
    if ( capturePrintScreen ) {
        Settings.Hotkeys.getByFunc( _T("regionscreenshot") ).globalKey.DeSerialize("44"); // PrintScreen
        Settings.Hotkeys.getByFunc( _T("windowscreenshot") ).globalKey.DeSerialize("65580"); // Alt + PrintScreen
    } else {
        Settings.Hotkeys.getByFunc( _T("regionscreenshot") ).Clear();
        Settings.Hotkeys.getByFunc( _T("windowscreenshot") ).Clear();
    }

    if ( Settings.AutoStartup || capturePrintScreen ) {
        Settings.ShowTrayIcon        = true;
        Settings.ShowTrayIcon_changed = true;
    }
    Settings.SaveSettings();

    EndDialog(wID);
    return 0;
}

LRESULT CQuickSetupDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    EndDialog(wID);
    return 0;
}

LRESULT CQuickSetupDlg::OnClickedDoAuthCheckbox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    doAuthCheckboxChanged();
    return 0;
}

void CQuickSetupDlg::doAuthCheckboxChanged() {
    bool isDoAuthChecked = SendDlgItemMessage(IDC_DOAUTHCHECKBOX, BM_GETCHECK) == BST_CHECKED;
    GuiTools::EnableNextN( GetDlgItem( IDC_DOAUTHCHECKBOX), 4, isDoAuthChecked);
}

void CQuickSetupDlg::translateUI() {
    TRC(IDOK, "Continue");
    TRC(IDC_LOGINLABEL, "Login:");
    TRC(IDC_PASSWORDLABEL, "Password:");
    TRC(IDC_SERVERLABEL, "Choose server for uploading images");
    TRC(IDC_AUTOSTARTUPCHECKBOX, "Launch program on Windows startup");
    TRC(IDC_CAPTUREPRINTSCREENCHECKBOX, "Intercept PrintScreen and Alt+PrintScreen hotkeys");
    TRC(IDC_EXPLORERINTEGRATION, "Add an item to the context menu of Windows Explorer");
}

void CQuickSetupDlg::showAuthorizationControls(bool show) {
    int cmdShow = show ? SW_SHOW : SW_HIDE;
    ::ShowWindow( GetDlgItem( IDC_DOAUTHCHECKBOX), cmdShow);
    ::ShowWindow( GetDlgItem( IDC_LOGINLABEL), cmdShow);
    ::ShowWindow( GetDlgItem( IDC_LOGINEDIT), cmdShow);
    ::ShowWindow( GetDlgItem( IDC_PASSWORDLABEL), cmdShow);
    ::ShowWindow( GetDlgItem( IDC_PASSWORDEDIT), cmdShow);
}

LRESULT CQuickSetupDlg::OnServerComboSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    serverChanged();
    return 0;
}

void  CQuickSetupDlg::serverChanged() {
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    int serverComboElementIndex = serverComboBox_.GetCurSel();
    if ( serverComboElementIndex >= 0 ) {
        const char* serverName = reinterpret_cast<char*>(serverComboBox_.GetItemData(serverComboElementIndex));
        if (!serverName) { // If the selected item is separator
            if (serverComboElementIndex != 0) {
                serverComboBox_.SetCurSel(0);
            }
            return;
        }
        std::string serverNameA = serverName;
        const CUploadEngineData* uploadEngineData = myEngineList->byName(serverNameA);
        if ( !uploadEngineData ) {
            return ;
        }
        bool authorizationAvailable = uploadEngineData->NeedAuthorization != 0;
        showAuthorizationControls( authorizationAvailable );
        bool forceAuthorization = uploadEngineData->NeedAuthorization == 2;
        CString doAuthCheckboxText = forceAuthorization ? TR("Authorize") : TR("I have an account on this server"); //+ (forceAuthorization? _T("") : TR(" (optional)"));
        SetDlgItemText( IDC_DOAUTHCHECKBOX, doAuthCheckboxText );
        ::EnableWindow( GetDlgItem( IDC_DOAUTHCHECKBOX), !forceAuthorization);
        SendDlgItemMessage( IDC_DOAUTHCHECKBOX, BM_SETCHECK, forceAuthorization? BST_CHECKED : BST_UNCHECKED );
        CString loginLabelText = uploadEngineData->LoginLabel.empty()? CString(TR("Login:")) : CString(Utf8ToWCstring( uploadEngineData->LoginLabel )) + _T(":");
        SetDlgItemText( IDC_LOGINLABEL, loginLabelText );
        CString passwordLabelText = uploadEngineData->PasswordLabel.empty() ? CString(TR("Password:")) : CString(Utf8ToWCstring(uploadEngineData->PasswordLabel)) + _T(":");
        SetDlgItemText(IDC_PASSWORDLABEL, passwordLabelText);

        doAuthCheckboxChanged();
        bool isDoAuthChecked = SendDlgItemMessage(IDC_DOAUTHCHECKBOX, BM_GETCHECK) == BST_CHECKED;
        ::EnableWindow( GetDlgItem( IDC_PASSWORDEDIT),isDoAuthChecked && uploadEngineData->NeedPassword);
    } else {
        showAuthorizationControls(false);
    }
}

