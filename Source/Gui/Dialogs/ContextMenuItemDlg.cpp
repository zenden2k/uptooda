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

#include "ContextMenuItemDlg.h"

#include <boost/format.hpp>

#include "Gui/GuiTools.h"
#include "Gui/Controls/ServerSelectorControl.h"
#include "Func/WinUtils.h"
#include "Core/Settings/WtlGuiSettings.h"


CContextMenuItemDlg::CContextMenuItemDlg(UploadEngineManager * uploadEngineManager)
{
    titleEdited_ = false;
    uploadEngineManager_ = uploadEngineManager;
}

CContextMenuItemDlg::~CContextMenuItemDlg()
{

}

LRESULT CContextMenuItemDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    CenterWindow(GetParent());

    RECT serverSelectorRect = GuiTools::GetDialogItemRect( m_hWnd, IDC_IMAGESERVERPLACEHOLDER);
    imageServerSelector_ = std::make_unique<CServerSelectorControl>(uploadEngineManager_, false);
    imageServerSelector_->Create(m_hWnd, serverSelectorRect);
    imageServerSelector_->setTitle(TR("Choose server"));
    imageServerSelector_->ShowWindow( SW_SHOW );
    imageServerSelector_->SetWindowPos( nullptr, serverSelectorRect.left, serverSelectorRect.top, serverSelectorRect.right-serverSelectorRect.left, serverSelectorRect.bottom - serverSelectorRect.top , SWP_NOZORDER);
    imageServerSelector_->setServerProfile(settings->imageServer.getByIndex(0));

    SetWindowText(TR("Add Menu Item"));
    TRC(IDC_MENUITEMLABEL, "Name:");
    TRC(IDOK, "OK");
    TRC(IDCANCEL, "Cancel");
    generateTitle();

    ::SetFocus(GetDlgItem(IDC_MENUITEMTITLEEDIT));
    return 0;
}

LRESULT CContextMenuItemDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    title_ = GuiTools::GetDlgItemText(m_hWnd, IDC_MENUITEMTITLEEDIT);
    if ( title_.IsEmpty() ) {
        LocalizedMessageBox(TR("Name cannot be empty"), TR("Error"), MB_ICONERROR);
        return 0;
    }
    serverProfile_ = imageServerSelector_->serverProfile();
    if ( serverProfile_.isNull() ) {
        LocalizedMessageBox(TR("You have not selected server"), TR("Error"), MB_ICONERROR);
        return 0;
    }

    if ( !imageServerSelector_->isAccountChosen() ) {
        std::wstring serverNameW = IuCoreUtils::Utf8ToWstring(imageServerSelector_->serverProfile().serverName());
        std::wstring message = str(boost::wformat(TR("You have not selected account for server \"%s\"")) % serverNameW);
        LocalizedMessageBox(message.c_str(), TR("Error"), MB_ICONERROR);
        return 0;
    }

    EndDialog(wID);
    return 0;
}

LRESULT CContextMenuItemDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CContextMenuItemDlg::OnServerSelectControlChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    generateTitle();
    return 0;
}

LRESULT CContextMenuItemDlg::OnMenuItemTitleEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if ( GetFocus() == hWndCtl ) {
        titleEdited_ = !GuiTools::GetWindowText(hWndCtl).IsEmpty();
    }
    return 0;
}

ServerProfile CContextMenuItemDlg::serverProfile() const
{
    return serverProfile_;
}

CString CContextMenuItemDlg::menuItemTitle() const
{
    return title_;
}

void CContextMenuItemDlg::generateTitle()
{
    if ( !titleEdited_ ) {
        ServerProfile sp = imageServerSelector_->serverProfile();
        auto engineList = ServiceLocator::instance()->engineList();
        const CUploadEngineData* ued = engineList->byName(sp.serverName());
        std::wstring serverName = IuCoreUtils::Utf8ToWstring(engineList->getServerDisplayName(ued));
        std::wstring title = str(boost::wformat(TR("Upload to %s")) % serverName);

        std::wstring additional;
        if ( !sp.profileName().empty()) {
            additional += IuCoreUtils::Utf8ToWstring(sp.profileName());
        }
        if ( !sp.folderId().empty() && !sp.folderTitle().empty() ) {
            if ( !additional.empty() ) {
                additional += _T(", ");
            }
            std::wstring folderTitleW = IuCoreUtils::Utf8ToWstring(sp.folderTitle());
            std::wstring temp = str(boost::wformat(TR("folder \"%s\"")) % folderTitleW);
            additional+= temp;
        }
        if ( !additional.empty() ) {
            title += L" (" + additional + L")";
        }

        SetDlgItemText(IDC_MENUITEMTITLEEDIT, title.c_str());
    }
}
