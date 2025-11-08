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

#include "ServerListPopup.h"

#include <strsafe.h>
#include <dwmapi.h>

#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Gui/IconBitmapUtils.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/AbstractServerIconCache.h"
#include "Gui/Helpers/DPIHelper.h"
#include "Gui/Models/ServerListModel.h"
#include "AddFtpServerDialog.h"
#include "AddDirectoryServerDialog.h"
#include "Func/MyEngineList.h"
#include "Func/Common.h"

namespace {

constexpr TCHAR MENU_EXIT_NOTIFY[] = _T("MENU_EXIT_NOTIFY"), MENU_EXIT_COMMAND_ID[] = _T("MENU_EXIT_COMMAND_ID");

}

// CServerListPopup
CServerListPopup::CServerListPopup(CMyEngineList* engineList, WinServerIconCache* serverIconCache, int serverMask, int selectedServerType, int serverIndex, bool isChildWindow)
    : engineList_(engineList)
    , serverListModel_(std::make_unique<ServerListModel>(engineList))
    , listView_(serverListModel_.get(), serverIconCache)
    , serversMask_(serverMask)
    , selectedServerType_(selectedServerType)
    , serverIndex_(serverIndex)
{
    auto ued = engineList->byIndex(serverIndex);
    if (ued && ((selectedServerType & ued->TypeMask) == 0)) {
        // Fix current server type to ensure that current server is visible
        selectedServerType_ = ued->TypeMask & serverMask;
    }
    iconBitmapUtils_ = std::make_unique<IconBitmapUtils>();
    isChildWindow_ = isChildWindow;
    hMyDlgTemplate_ = nullptr;
    isPopingUp_ = false;
}

CServerListPopup::~CServerListPopup()
{
    if (hMyDlgTemplate_) {
        GlobalFree(hMyDlgTemplate_);
    }
}

void CServerListPopup::TranslateUI() {
    SetWindowText(TR("Choose server"));
    TRCC(IDC_ALLTYPESRADIO, "serverlist.servertype", "All");
    TRCC(IDC_IMAGERADIO, "serverlist.servertype", "Image");
    TRCC(IDC_FILERADIO, "serverlist.servertype", "File");
    TRCC(IDC_VIDEORADIO, "serverlist.servertype", "Video");
    TRC(IDOK, "OK");
    TRC(IDC_ADDBUTTON, "Options");
}

LRESULT CServerListPopup::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
    DwmSetWindowAttribute(m_hWnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));

    DlgResize_Init(true, true, 0); // resizable dialog without "griper"
    DoDataExchange(FALSE);
   
    TranslateUI();
    optionsButton_.SetButtonStyle(BS_SPLITBUTTON);

    listView_.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    // Creating tooltip control. We will use subclassing of controls to intercept their messages
    toolTip_.Create(m_hWnd);
    CString tipText = TR("Help");
    CToolInfo tip(TTF_SUBCLASS, helpButton_, 0, 0, const_cast<LPWSTR>(tipText.GetString()));
    toolTip_.AddTool(tip);

    if (selectedServerType_ == CUploadEngineData::TypeImageServer) {
        imageTypeRadioButton_.SetCheck(BST_CHECKED);
    } else if (selectedServerType_ == CUploadEngineData::TypeFileServer) {
        fileTypeRadioButton_.SetCheck(BST_CHECKED);
    } else if (selectedServerType_ == CUploadEngineData::TypeVideoServer) {
        videoTypeRadioButton_.SetCheck(BST_CHECKED);
    } else {
        allTypesRadioButton_.SetCheck(BST_CHECKED);
    }

    imageTypeRadioButton_.EnableWindow(serversMask_ & CUploadEngineData::TypeImageServer);
    fileTypeRadioButton_.EnableWindow(serversMask_ & CUploadEngineData::TypeFileServer);
    videoTypeRadioButton_.EnableWindow(serversMask_ & CUploadEngineData::TypeVideoServer);

    createResources();


    /*ACCEL accels[] = {
        { FVIRTKEY, VK_F1, IDC_HELPBUTTON },
    };
    
    hotkeys_.CreateAcceleratorTable(accels, std::size(accels));*/

    updateServerList();

    applyFilter(false);

    CUploadEngineData* ued = engineList_->byIndex(serverIndex_);

    if (ued) {
        selectServerByName(U2W(ued->Name));
    }
    
    listView_.SetFocus();

    return FALSE;
}

LRESULT CServerListPopup::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CServerListPopup::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createResources();
    listView_.SendMessage(WM_MY_DPICHANGED, wParam);
    return 0;
}

void CServerListPopup::setTitle(CString title) {

}

CString CServerListPopup::getTitle() const {
    return {};
}

void CServerListPopup::setServerProfile(const ServerProfile& serverProfile) {
    serverProfile_ = serverProfile;
}

ServerProfile CServerListPopup::serverProfile() const {
    return serverProfile_;
}

void CServerListPopup::serverChanged() {

}

void CServerListPopup::setServersMask(int mask) {
    serversMask_ = mask;
}


void CServerListPopup::notifyChange()
{
    if (onChangeCallback_)
    {
        onChangeCallback_(this);
    }
}

void CServerListPopup::notifyServerListChanged()
{
}

void CServerListPopup::updateServerList()
{
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    //auto iconCache = ServiceLocator::instance()->serverIconCache();
}

LRESULT CServerListPopup::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return MA_NOACTIVATE;
}

void CServerListPopup::createResources() {
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    const int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    if (helpButtonIcon_) {
        helpButtonIcon_.DestroyIcon();
    }

    helpButtonIcon_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICON_HELP_DROPDOWN), iconWidth, iconHeight);
    helpButton_.SetIcon(helpButtonIcon_);

    searchIconCtrl_.SetWindowPos(0, 0, 0, iconWidth, iconHeight, SWP_NOMOVE | SWP_NOZORDER);
    searchIconCtrl_.loadImage(0, 0, IDB_ICONSEARCHSMALL, false, GetSysColor(COLOR_BTNFACE));
}

void CServerListPopup::setOnChangeCallback(std::function<void(CServerListPopup*)> cb) {
    onChangeCallback_ = std::move(cb);
}

int CServerListPopup::serverIndex() const {
    return serverIndex_;
}

int CServerListPopup::showPopup(HWND parent, const RECT& anchorRect) {
    // Code from \Program Files\Microsoft SDKs\Windows\v7.1\Samples\winui\shell\legacysamples\fakemenu\fakemenu.cpp
    isChildWindow_ = false;
    if (Create(parent) == NULL) {
        ATLTRACE(_T("CServerListPopup dialog creation failed!  :( sorry\n"));
        return 0;
    }

    CRect windowRect;
    GetWindowRect(windowRect);
    int popupWidth = windowRect.Width();
    int popupHeight = windowRect.Height();
    
    int x = anchorRect.left;
    int y = anchorRect.bottom + 2;

    HMONITOR hMonitor = MonitorFromRect(windowRect, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { 0 };
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(hMonitor, &mi)) {
        // Check monitor work area boundaries
        RECT workArea = mi.rcWork;
        // Adjust horizontal position
        if (x + popupWidth > workArea.right) {
            // Doesn't fit on the right - shift left
            x = anchorRect.right - popupWidth;
            // If still doesn't fit, align to screen right edge
            if (x < workArea.left) {
                x = workArea.right - popupWidth;
            }
        }
        // Check that window doesn't go beyond left boundary
        if (x < workArea.left) {
            x = workArea.left;
        }
        // Adjust vertical position
        if (y + popupHeight > workArea.bottom) {
            // Doesn't fit below - show above the button
            y = anchorRect.top - popupHeight;
            // If doesn't fit above either
            if (y < workArea.top) {
                // Place next to button on the right
                if (anchorRect.right + popupWidth <= workArea.right) {
                    x = anchorRect.right;
                    y = anchorRect.top;
                }
                // Or on the left
                else if (anchorRect.left - popupWidth >= workArea.left) {
                    x = anchorRect.left - popupWidth;
                    y = anchorRect.top;
                }
                // As last resort - center in work area
                else {
                    x = workArea.left + (workArea.right - workArea.left - popupWidth) / 2;
                    y = workArea.top + (workArea.bottom - workArea.top - popupHeight) / 2;
                }
            }
        }
        // Final boundary check
        if (x < workArea.left) {
            x = workArea.left;
        }
        if (y < workArea.top) {
            y = workArea.top;
        }
        if (x + popupWidth > workArea.right) {
            x = workArea.right - popupWidth;
        }
        if (y + popupHeight > workArea.bottom) {
            y = workArea.bottom - popupHeight;
        }
    }

    SetWindowPos(0, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
    ULONGLONG startTime = GetTickCount64(); 
    ShowWindow(SW_SHOW);

    //BOOL bMenuDestroyed(FALSE);
    HWND hwndOwner = GetWindow(GW_OWNER);
    HWND hwndPopup = m_hWnd;
    // We want to receive all mouse messages, but since only the active
    // window can capture the mouse, we have to set the capture to our
    // owner window, and then steal the mouse messages out from under it.
    //::SetCapture(hwndOwner);

    // Go into a message loop that filters all the messages it receives
    // and route the interesting ones to the color picker window.
    MSG msg;
    POINT pt {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // If our owner stopped being the active window (e.g. the user
        // Alt+Tab'd to another window in the meantime), then stop.
        HWND hwndActive = GetActiveWindow();
        if (hwndActive != hwndPopup && hwndActive != hwndOwner && !::IsChild(hwndActive, hwndOwner) /*||
            GetCapture() != hwndOwner*/)
        {
            break;
        }


        bool isChildMessage = ::IsChild(hwndPopup, msg.hwnd)!=FALSE;
        bool breakLoop = false;
        // At this point, we get to snoop at all input messages before
        // they get dispatched.  This allows us to route all input to our
        // popup window even if really belongs to somebody else.

        // All mouse messages are remunged and directed at our popup
        // menu. If the mouse message arrives as client coordinates, then
        // we have to convert it from the client coordinates of the original
        // target to the client coordinates of the new target.
        switch (msg.message)
        {
            // These mouse messages arrive in client coordinates, so in
            // addition to stealing the message, we also need to convert the
            // coordinates.

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            if ((msg.hwnd == hwndOwner || ::IsChild(hwndOwner, msg.hwnd)) && GetTickCount64() - startTime > 400) {
                breakLoop = true;
               
            }
            break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
            if ((msg.hwnd == hwndOwner || ::IsChild(hwndOwner, msg.hwnd)))
            {
                breakLoop = true;
                break;
            }
        case WM_MOUSEMOVE:
            if (!isChildMessage) {
                pt.x = (short)LOWORD(msg.lParam);
                pt.y = (short)HIWORD(msg.lParam);
                ::MapWindowPoints(msg.hwnd, hwndPopup, &pt, 1);
                msg.lParam = MAKELPARAM(pt.x, pt.y);
                msg.hwnd = hwndPopup;
            }
            break;

            // These mouse messages arrive in screen coordinates, so we just
            // need to steal the message.

        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
        case WM_NCMBUTTONDBLCLK:
        case WM_SETCURSOR:
            if (msg.hwnd == hwndOwner || ::IsChild(hwndOwner, msg.hwnd))
            {
                breakLoop = true;
                break;
            }
        case WM_NCMOUSEMOVE:
            if (!isChildMessage) {
                msg.hwnd = hwndPopup;
            }
            break;

            // We need to steal all keyboard messages, too.

        case WM_CHAR:
            if (msg.hwnd == listView_) {
                msg.hwnd = queryEditControl_;
                queryEditControl_.SetFocus();
            }
            break;
            
        case WM_KEYDOWN:
        case WM_KEYUP:
            if (msg.hwnd == queryEditControl_ && (msg.wParam == VK_UP || msg.wParam == VK_DOWN)) {
                msg.hwnd = listView_;
                listView_.SetFocus();
                break;
            }

        case WM_DEADCHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
            if (!isChildMessage)
            {
                msg.hwnd = hwndPopup;
            }
            if (msg.wParam == VK_ESCAPE && (msg.hwnd == hwndPopup || ::IsChild(hwndPopup, msg.hwnd)))
            {
                breakLoop = true;
                break;
            }
            break;
        }
        if (breakLoop)
        {
            break;
        }
        if (!::IsDialogMessage(hwndPopup, &msg) && (!hotkeys_ || !WinUtils::TranslateAcceleratorForWindow(hwndPopup, hotkeys_, &msg)))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }


        // If our owner stopped being the active window (e.g. the user
        // Alt+Tab'd to another window in the meantime), then stop.
        hwndActive = GetActiveWindow();
        if (hwndActive != hwndPopup && hwndActive != hwndOwner && !::IsChild(hwndActive, hwndOwner)/* ||
            GetCapture() != hwndOwner*/)
        {
            break;
        }
    }

    // Clean up the capture we created.
    //ReleaseCapture();

    isPopingUp_ = false;
    // If we got a WM_QUIT message, then re-post it so the caller's message
    // loop will see it.
    /*if (msg.message == WM_QUIT)
    {
        PostQuitMessage((int)msg.wParam);
    }*/


    //if (!bMenuDestroyed)
    DestroyWindow();

    return ret_;
}

bool CServerListPopup::exitPopup(int nCommandId)
{
    ret_ = nCommandId;
    PostQuitMessage(0);
    //BOOL bRet = SetProp(m_hWnd, MENU_EXIT_NOTIFY, (HANDLE)1);
    //SetProp(m_hWnd, MENU_EXIT_COMMAND_ID, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(nCommandId)));
    return true;
}

DLGTEMPLATE* CServerListPopup::GetTemplate()
{
    HINSTANCE hInst = GetModuleHandle(nullptr);
    HRSRC res = FindResource(hInst, MAKEINTRESOURCE(IDD), RT_DIALOG);
    assert(res);
    DLGTEMPLATE* dit = reinterpret_cast<DLGTEMPLATE*>(LockResource(LoadResource(hInst, res)));

    assert(dit);
    unsigned long sizeDlg = ::SizeofResource(hInst, res);
    hMyDlgTemplate_ = ::GlobalAlloc(GPTR, sizeDlg);
    assert(hMyDlgTemplate_);
    auto pMyDlgTemplate = reinterpret_cast<ATL::_DialogSplitHelper::DLGTEMPLATEEX*>(::GlobalLock(hMyDlgTemplate_));
    if (!pMyDlgTemplate) {
        return {};
    }
    ::memcpy(pMyDlgTemplate, dit, sizeDlg);

    if (isChildWindow_)
    {
        //pMyDlgTemplate->style = pMyDlgTemplate->style & ~ WS_POPUP;
        pMyDlgTemplate->style = pMyDlgTemplate->style | WS_CHILD;
    }
    else
    {
        pMyDlgTemplate->style -= WS_CHILD;
        pMyDlgTemplate->style -= DS_CONTROL;
        pMyDlgTemplate->exStyle |= WS_EX_NOACTIVATE |
            WS_EX_TOOLWINDOW |      // So it doesn't show up in taskbar
            WS_EX_DLGMODALFRAME |   // Get the edges right
            WS_EX_WINDOWEDGE /*|
            WS_EX_TOPMOST*/; // with this style window is overlapping modal dialogs (server params dialog, add ftp server, etc...)

        pMyDlgTemplate->style = pMyDlgTemplate->style | WS_POPUP | WS_BORDER/* | WS_CAPTION*/ ;
    }
    if (ServiceLocator::instance()->translator()->isRTL()) {
        pMyDlgTemplate->exStyle |= WS_EX_LAYOUTRTL | WS_EX_RTLREADING;
    }
    return reinterpret_cast<DLGTEMPLATE*>(pMyDlgTemplate);
}

LRESULT CServerListPopup::OnEnable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (!isChildWindow_) {
        // Disable wizard window when a modal window is shown
        ::EnableWindow(GetParent(), wParam);
    }

    return 0;
}

LRESULT CServerListPopup::OnListViewDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto pnmia = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);

    int nItem = pnmia->iItem;

    if (nItem >= 0) {
        const auto& data = serverListModel_->getDataByIndex(nItem);
        serverIndex_ = data.uedIndex;
        exitPopup(IDOK);
    }
    return 0;
}

LRESULT CServerListPopup::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int nSelected = listView_.GetNextItem(-1, LVNI_SELECTED);
    if (nSelected >= 0) {
        const auto& data = serverListModel_->getDataByIndex(nSelected);
        serverIndex_ = data.uedIndex;
        exitPopup(IDOK);
    }
    return 0;
}

LRESULT CServerListPopup::OnServerTypeChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    applyFilter();

    return 0;
}

LRESULT CServerListPopup::OnSearchQueryEditChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    applyFilter();

    return 0;
}

LRESULT CServerListPopup::OnBnClickedAddServerButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    showAddServerButtonMenu(hWndCtl);
    return 0;
}

LRESULT CServerListPopup::OnBnDropdownAddServerButton(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    showAddServerButtonMenu(GetDlgItem(IDC_ADDBUTTON));
    return 0;
}

void CServerListPopup::applyFilter(bool selectItem) {
    int mask = 0;

    if (imageTypeRadioButton_.GetCheck() == BST_CHECKED) {
        mask |= CUploadEngineData::TypeImageServer;
    } else if (fileTypeRadioButton_.GetCheck() == BST_CHECKED) {
        mask |= CUploadEngineData::TypeFileServer;
    } else if (videoTypeRadioButton_.GetCheck() == BST_CHECKED) {
        mask |= CUploadEngineData::TypeVideoServer;
    } else {
        mask = serversMask_;
    }

    ServerFilter filter;
    CString query;
    queryEditControl_.GetWindowText(query);

    filter.query = W2U(query);
    filter.typeMask = mask;
    serverListModel_->applyFilter(filter);
    if (selectItem) {
        listView_.SelectItem(0);
    }
}

void CServerListPopup::clearFilter() {
    queryEditControl_.SetWindowText(_T(""));
    allTypesRadioButton_.SetCheck(BST_CHECKED);
    imageTypeRadioButton_.SetCheck(BST_UNCHECKED);
    fileTypeRadioButton_.SetCheck(BST_UNCHECKED);
    videoTypeRadioButton_.SetCheck(BST_UNCHECKED);
}

void CServerListPopup::selectServerByName(const CString& name) {
    const std::string serverName = W2U(name);
    size_t count = serverListModel_->getCount();
    for (size_t i = 0; i < count; ++i) {
        if (serverListModel_->getDataByIndex(i).ued->Name == serverName) {
            listView_.SelectItem(i);
            return;
        }
    }
}

void CServerListPopup::showAddServerButtonMenu(HWND control) {
    RECT rc;
    ::GetWindowRect(control, &rc);
    POINT menuOrigin = { rc.left, rc.bottom };

    CMenu popupMenu;
    popupMenu.CreatePopupMenu();
    std::wstring itemTitle = str(IuStringUtils::FormatWideNoExcept(TR("Add %s server...")) % L"FTP/SFTP/WebDAV");
    popupMenu.AppendMenu(MF_STRING, IDM_ADD_FTP_SERVER, itemTitle.c_str());
    popupMenu.AppendMenu(MF_STRING, IDM_ADD_DIRECTORY_AS_SERVER, TR("Add folder as new server..."));
    popupMenu.AppendMenu(MF_STRING, IDM_OPEN_SERVERS_FOLDER, TR("Open servers folder"));

    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;
    popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON, menuOrigin.x, menuOrigin.y, m_hWnd, &excludeArea);
}

void CServerListPopup::openDocumentation() {
    OpenDocumentation(m_hWnd, _T("usage"), "serverlist");
}

LRESULT CServerListPopup::OnAddFtpServer(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    CAddFtpServerDialog dlg(engineList_);
    if (dlg.DoModal(m_hWnd) == IDOK) {
        serverListModel_->updateEngineList();
        clearFilter();
        applyFilter(false);
        selectServerByName(dlg.createdServerName());
        listView_.SetFocus();
    }
    return 0;
}

LRESULT CServerListPopup::OnAddDirectoryAsServer(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    CAddDirectoryServerDialog dlg(engineList_);
    if (dlg.DoModal(m_hWnd) == IDOK) {
        serverListModel_->updateEngineList();
        clearFilter();
        applyFilter(false);
        selectServerByName(dlg.createdServerName());
        listView_.SetFocus();
    }
    return 0;
}

LRESULT CServerListPopup::OnHelpButton(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    openDocumentation();
    return 0;
}

LRESULT CServerListPopup::OnOpenServersFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    const std::wstring folder = IuCoreUtils::Utf8ToWstring(settings->SettingsFolder + "Servers\\");
    try {
        WinUtils::ShellOpenFileOrUrl(folder.c_str(), m_hWnd, {}, true);
    } catch (const Win32Exception& ex) {
        const std::wstring msg = str(
            IuStringUtils::FormatWideNoExcept(TR("Cannot open folder '%1%'.\n%2%"))
            % folder
            % ex.getMessage().GetString());
        GuiTools::LocalizedMessageBox(m_hWnd, msg.c_str(), TR("Error"), MB_ICONERROR);
    }
    return 0;
}

LRESULT CServerListPopup::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    auto settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    HWND hwnd = reinterpret_cast<HWND>(wParam);
    POINT ClientPoint, ScreenPoint;
    if (hwnd != listView_) {
        return 0;
    }

    if (lParam == -1) {
        int nCurItem = listView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
        if (nCurItem >= 0) {
            CRect rc;
            listView_.GetItemRect(nCurItem, &rc, LVIR_ICON);
            ClientPoint = rc.CenterPoint();
        } else {
            ClientPoint.x = 0;
            ClientPoint.y = 0;
        }
        ScreenPoint = ClientPoint;
        ::ClientToScreen(hwnd, &ScreenPoint);
    } else {
        ScreenPoint.x = GET_X_LPARAM(lParam);
        ScreenPoint.y = GET_Y_LPARAM(lParam);
        ClientPoint = ScreenPoint;
        ::ScreenToClient(hwnd, &ClientPoint);
    }

    LV_HITTESTINFO hti;
    hti.pt = ClientPoint;
    listView_.HitTest(&hti);

    if (hti.iItem >= 0) {
        constexpr auto ID_OPENREGISTERURL = 11000;
        constexpr auto ID_OPENWEBSITE = 11001;

        const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
        auto* engineList = ServiceLocator::instance()->engineList();
        const ServerData& data = serverListModel_->getDataByIndex(hti.iItem);
        if (!data.ued) {
            return 0;
        }
        CMenu contextMenu;
        contextMenu.CreatePopupMenu();

        contextMenu.AppendMenu(MF_STRING | (data.ued->WebsiteUrl.empty() ? MF_DISABLED : MF_ENABLED), ID_OPENWEBSITE, TR("Open the website"));
        contextMenu.AppendMenu(MF_STRING | (data.ued->RegistrationUrl.empty() ? MF_DISABLED : MF_ENABLED), ID_OPENREGISTERURL, TR("Go to signup page"));

        BOOL res = contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD, ScreenPoint.x, ScreenPoint.y, m_hWnd);
        switch (res) {
        case ID_OPENWEBSITE:
            WinUtils::ShellOpenFileOrUrl(U2WC(data.ued->WebsiteUrl), m_hWnd);
            break;
        case ID_OPENREGISTERURL:
            WinUtils::ShellOpenFileOrUrl(U2WC(data.ued->RegistrationUrl), m_hWnd);
            break;
        }
    }
    return 0;
}

LRESULT CServerListPopup::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    openDocumentation();
    return 0;
}
