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

#include "AddDirectoryServerDialog.h"

#include "Gui/GuiTools.h"
#include "Core/ServerListManager.h"
#include "Core/Settings/WtlGuiSettings.h"
#include <WinSock2.h>
#include <iphlpapi.h>
#define SECURITY_WIN32
#include <security.h>
#include "Core/CommonDefs.h"
#include <Lm.h>
#include "Gui/Components/NewStyleFolderDialog.h"
#include "Gui/Helpers/DPIHelper.h"

CAddDirectoryServerDialog::CAddDirectoryServerDialog(CUploadEngineList* uploadEngineList)
{
    connectionNameEdited = false;
    downloadUrlEdited = false;
    serverNameEdited = false;
    uploadEngineList_ = uploadEngineList;
}

LRESULT CAddDirectoryServerDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetWindowText(TR("Adding directory as server"));
    TRC(IDC_CONNECTIONNAMELABEL, "Server name:");
    TRC(IDC_DIRECTORYLABEL, "Folder:");
    TRC(IDC_DOWNLOADURLLABEL, "URL for downloading:");
    TRC(IDOK, "OK");
    TRC(IDCANCEL, "Cancel");
    TRC(IDC_THEURLOFUPLOADEDLABEL, "URL for downloading will look like:");
    const CString addFileProtocolLabelText = TR("Convert UNC path \"\\\\\" to \"file://\"");

    SetDlgItemText(IDC_ADDFILEPROTOCOL, addFileProtocolLabelText);

    const UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);

    presetButtonIcon_ = GuiTools::CreateDropDownArrowIcon(GetDlgItem(IDC_PRESETSBUTTON), dpi);
    presetButton_ = GetDlgItem(IDC_PRESETSBUTTON);
    presetButton_.SetIcon(presetButtonIcon_);

    ::SetFocus(GetDlgItem(IDC_CONNECTIONNAMEEDIT));
    LoadComputerAddresses();
    GuiTools::ShowDialogItem(m_hWnd, IDC_ADDFILEPROTOCOL, false);

    if (ServiceLocator::instance()->translator()->isRTL()) {
        // Removing WS_EX_RTLREADING style from some controls to look properly when RTL interface language is chosen
        HWND downloadUrlEditHwnd = GetDlgItem(IDC_DOWNLOADURLEDIT);
        LONG styleEx = ::GetWindowLong(downloadUrlEditHwnd, GWL_EXSTYLE);
        ::SetWindowLong(downloadUrlEditHwnd, GWL_EXSTYLE, styleEx & ~WS_EX_RTLREADING);

        HWND directoryEditHwnd = GetDlgItem(IDC_DIRECTORYEDIT);
        styleEx = ::GetWindowLong(directoryEditHwnd, GWL_EXSTYLE);
        ::SetWindowLong(directoryEditHwnd, GWL_EXSTYLE, styleEx & ~WS_EX_RTLREADING);

        HWND exampleUrlLabel = GetDlgItem(IDC_EXAMPLEURLLABEL);
        styleEx = ::GetWindowLong(exampleUrlLabel, GWL_EXSTYLE);
        ::SetWindowLong(exampleUrlLabel, GWL_EXSTYLE, styleEx & ~WS_EX_RTLREADING & ~WS_EX_LAYOUTRTL);
        LONG style = ::GetWindowLong(exampleUrlLabel, GWL_STYLE);
        ::SetWindowLong(exampleUrlLabel, GWL_STYLE, style | SS_RIGHT);
    }

    CenterWindow(GetParent());
    return 0;  // Let the system set the focus
}

LRESULT CAddDirectoryServerDialog::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CString connectionName = GuiTools::GetDlgItemText(m_hWnd, IDC_CONNECTIONNAMEEDIT);
    connectionName.TrimLeft(L" ");
    connectionName.TrimRight(L" ");
    if ( connectionName.IsEmpty() ) {
        LocalizedMessageBox(TR("Connection name cannot be empty"),TR("Error"), MB_ICONERROR);
        return 0;
    }

    CString downloadUrl = GuiTools::GetDlgItemText(m_hWnd, IDC_DOWNLOADURLEDIT);
    downloadUrl.TrimLeft(L" ");
    downloadUrl.TrimRight(L" ");
    if ( downloadUrl.IsEmpty() ) {
        LocalizedMessageBox(TR("Download URL cannot be empty."), TR("Error"), MB_ICONERROR);
        return 0;
    }

    CString directory = GuiTools::GetDlgItemText(m_hWnd, IDC_DIRECTORYEDIT);
    if ( directory.Right(1) != _T("\\") ) {
        directory += _T("\\");
    }

    const bool addFileProtocol = GuiTools::GetCheck(m_hWnd, IDC_ADDFILEPROTOCOL);
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    ServerListManager slm(settings->SettingsFolder + "Servers\\", uploadEngineList_, settings->ServersSettings);
    try {
        createdServerName_ = U2W(slm.addDirectoryAsServer(W2U(connectionName), W2U(directory), W2U(downloadUrl), addFileProtocol));
        EndDialog(wID);
    } catch (const std::exception& ex) {
        CString errorMessage = TR("Could not add server.");
        const CString reason = U2W(ex.what());
        if ( !reason.IsEmpty() ) {
            errorMessage += CString(L"\r\n") + TR("Reason:") + L"\r\n" + reason;
        }
        LocalizedMessageBox(errorMessage, TR("Error"), MB_ICONERROR);
    }
    return 0;
}

LRESULT CAddDirectoryServerDialog::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CAddDirectoryServerDialog::OnConnectionNameEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if ( GetFocus() == hWndCtl ) {
        connectionNameEdited = !GuiTools::GetDlgItemText(m_hWnd, IDC_CONNECTIONNAMEEDIT).IsEmpty();
    }

    return 0;
}

LRESULT CAddDirectoryServerDialog::OnPresetMenuItemClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int presetIndex = wID - IDC_PRESETMENU_FIRST_ID;
    if ( presetIndex < 0 || presetIndex >= static_cast<int>(addresses_.size()) ) {
        return 0;
    }
    SetDlgItemText(IDC_DOWNLOADURLEDIT, addresses_[presetIndex]);
    GenerateExampleUrl();
    return 0;
}

LRESULT CAddDirectoryServerDialog::OnPresetSharedFolderMenuItemClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int presetIndex = wID - IDC_PRESETMENU_SHARED_FOLDER_FIRST_ID;
    if ( presetIndex < 0 || presetIndex >= static_cast<int>(sharedFolders_.size()) ) {
        return 0;
    }
    SetDlgItemText(IDC_DOWNLOADURLEDIT, sharedFolders_[presetIndex]);
    GenerateExampleUrl();
    return 0;
}

LRESULT CAddDirectoryServerDialog::OnDirectoryEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    CString directory =  GuiTools::GetDlgItemText(m_hWnd, IDC_DIRECTORYEDIT);
    if ( !connectionNameEdited ) {
        CString serverName;
        serverName.Format(TR("Folder %s"), WinUtils::TrimString(directory, 30).GetString());
        SetDlgItemText(IDC_CONNECTIONNAMEEDIT, serverName);
    }
    GenerateDownloadLink();

    PSHARE_INFO_502 BufPtr,p;
    NET_API_STATUS res;
    DWORD er=0,tr=0,resume=0;
    sharedFolders_.clear();

    //
    do // begin do
    {
        LPTSTR lpszServer = nullptr;
        res = NetShareEnum (lpszServer, 502, (LPBYTE *) &BufPtr, MAX_PREFERRED_LENGTH, &er, &tr, &resume);
        //
        // If the call succeeds,
        //
        if(res == ERROR_SUCCESS || res == ERROR_MORE_DATA)
        {
            p=BufPtr;
            //
            // Loop through the entries;
            // print retrieved data.
            //
            for(size_t i=1;i<=er;i++)
            {
                //LPCTSTR str = (LPCTSTR)p->shi502_remark;
                if( ! (p->shi502_type & STYPE_SPECIAL) ) {
                    CString path = p->shi502_path+CString(L"\\");
                    if (directory ==  p->shi502_path) {
                            sharedFolders_.emplace_back(L"\\\\" + computerName_ + "\\" + p->shi502_netname+"\\");
                    } else if ( directory.Find(path) == 0  ) {
                        sharedFolders_.push_back(L"\\\\" + computerName_ + "\\" + p->shi502_netname+"\\" + directory.Mid(path.GetLength())+CString(L"\\"));
                    }
                }
                p++;
            }
            //
            // Free the allocated buffer.
            //
            NetApiBufferFree(BufPtr);
        }
        else
            printf("Error: %ld\n",static_cast<long>(res));
    }
    // Continue to call NetShareEnum while
    // there are more entries.
    //
    while (res==ERROR_MORE_DATA); // end do

    return 0;
}

LRESULT CAddDirectoryServerDialog::OnDownloadUrlEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if (GetFocus() == hWndCtl) {
        downloadUrlEdited = !GuiTools::GetDlgItemText(m_hWnd, IDC_DOWNLOADURLEDIT).IsEmpty();
        GenerateExampleUrl();
    }
    CString downloadUrl = GuiTools::GetDlgItemText(m_hWnd, IDC_DOWNLOADURLEDIT);
    GuiTools::ShowDialogItem(m_hWnd, IDC_ADDFILEPROTOCOL, downloadUrl.Left(2) == _T("\\\\") );
    return 0;
}

CString CAddDirectoryServerDialog::createdServerName() const
{
    return createdServerName_;
}

void CAddDirectoryServerDialog::GenerateDownloadLink()
{
    if ( !downloadUrlEdited ) {
        CString serverName;
        CString generatedDownloadUrl;

        if ( !serverName.IsEmpty() ) {
            SetDlgItemText(IDC_DOWNLOADURLEDIT, generatedDownloadUrl);
        }
    }
    GenerateExampleUrl();
}

void CAddDirectoryServerDialog::GenerateExampleUrl()
{
    CString downloadUrl = GuiTools::GetDlgItemText(m_hWnd, IDC_DOWNLOADURLEDIT);

    bool addFileProtocol = GuiTools::GetCheck(m_hWnd, IDC_ADDFILEPROTOCOL);

    // Should be the same converting as in directory.nut script
    if ( addFileProtocol && downloadUrl.Left(2) == _T("\\\\") ) {
        downloadUrl = downloadUrl.Mid(2);
        downloadUrl.Replace(L"\\", L"/");
        downloadUrl = L"file://" + downloadUrl;
    }

    SetDlgItemText(IDC_EXAMPLEURLLABEL, downloadUrl + "example.png");
}

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))

#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
bool CAddDirectoryServerDialog::LoadComputerAddresses()
{
    /* Declare and initialize variables */

    DWORD dwRetVal = 0;

    unsigned int i = 0;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

    // default to unspecified address family (both)
    ULONG family = AF_UNSPEC;

    LPVOID lpMsgBuf = NULL;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    /*PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
    PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;*/
    //IP_ADAPTER_PREFIX *pPrefix = NULL;

    family = AF_INET;

    outBufLen = WORKING_BUFFER_SIZE;

    do {

        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(MALLOC(outBufLen));
        if (pAddresses == NULL) {
            printf
                ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
        }

        dwRetVal =
            GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

    if (dwRetVal == NO_ERROR) {
        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {

            pUnicast = pCurrAddresses->FirstUnicastAddress;
            if (pUnicast != NULL && pCurrAddresses->OperStatus == IfOperStatusUp && pCurrAddresses->IfType !=
                IF_TYPE_SOFTWARE_LOOPBACK) {
                for (i = 0; pUnicast != NULL; i++) {
                    sockaddr_in* addr = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                    char* ip = inet_ntoa(addr->sin_addr);
                    addresses_.push_back(CString(L"http://") + ip + L"/");
                    pUnicast = pUnicast->Next;
                }
            }

            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        printf("Call to GetAdaptersAddresses failed with error: %d\n", (int)dwRetVal);
        if (dwRetVal == ERROR_NO_DATA) {
        }
            //printf("\tNo addresses were found for the requested parameters\n");
        else {

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                              FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              // Default language
                              (LPTSTR)&lpMsgBuf, 0, NULL)) {
                //printf("\tError: %s", reinterpret_cast<>lpMsgBuf);
                LocalFree(lpMsgBuf);
                if (pAddresses)
                    FREE(pAddresses);
                pAddresses = nullptr;
            }
        }
    }

    if (pAddresses) {
        FREE(pAddresses);
    }

    TCHAR computerName[1024] = L"";
    DWORD size = ARRAY_SIZE(computerName);
    if (GetComputerName(computerName, &size)) {
        computerName_ = computerName;
        addresses_.push_back(CString(L"\\\\") + computerName + L"\\");
    }
    return true;
}

LRESULT CAddDirectoryServerDialog::OnBnClickedBrowsebutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CString path = GuiTools::GetWindowText(GetDlgItem(IDC_DIRECTORYEDIT));
    CNewStyleFolderDialog fd(m_hWnd, path, TR("Select folder"));

    if(fd.DoModal(m_hWnd) == IDOK)
    {
        path = fd.GetFolderPath();
        CString lastChar = path.Right(1);
        if (lastChar != _T("\\") && lastChar != _T("/")) {
            path += _T("\\");
        }
        SetDlgItemText(IDC_DIRECTORYEDIT, path);
        return true;
    }
    return 0;
}

LRESULT CAddDirectoryServerDialog::OnPresetButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    RECT rc;
    ::GetWindowRect(hWndCtl, &rc );
    POINT menuOrigin = {rc.left,rc.bottom};

    CMenu popupMenu;
    popupMenu.CreatePopupMenu();
    int id =  IDC_PRESETMENU_SHARED_FOLDER_FIRST_ID;

    for( size_t i =0; i < sharedFolders_.size(); i++ ) {
        // Adding Unicode Left-To-Right marks to text to be rendered correctly if RTL language is choosen
        CString itemTitle = L"\u200E" + sharedFolders_[i];
        if (ServiceLocator::instance()->translator()->isRTL()) {
            itemTitle.Replace(L"\\", L"\\\u200E");
            itemTitle.Replace(L"/", L"/\u200E");
        }
        popupMenu.AppendMenu(MF_STRING, id++, itemTitle);

    }
    id =  IDC_PRESETMENU_FIRST_ID;
    for (size_t i = 0; i < addresses_.size(); i++) {
        CString itemTitle = L"\u200E" +  addresses_[i];
        if (ServiceLocator::instance()->translator()->isRTL()) {
            itemTitle.Replace(L"\\", L"\\\u200E");
            itemTitle.Replace(L"/", L"/\u200E");
        }
        popupMenu.AppendMenu(MF_STRING, id++, itemTitle);
    }

    popupMenu.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON, menuOrigin.x, menuOrigin.y, m_hWnd);

    return 0;
}

LRESULT CAddDirectoryServerDialog::OnBnClickedAddfileprotocol(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    GenerateExampleUrl();
    return 0;
}
