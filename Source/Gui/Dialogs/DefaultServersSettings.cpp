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

#include "DefaultServersSettings.h"

#include "Func/Common.h"
#include "Gui/GuiTools.h"
#include "Gui/Controls/ServerSelectorControl.h"
#include "Gui/Controls/MultiServerSelectorControl.h"
#include "WizardDlg.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/WtlGuiSettings.h"

// CDefaultServersSettings
CDefaultServersSettings::CDefaultServersSettings(UploadEngineManager* uploadEngineManager)
{
    uploadEngineManager_ = uploadEngineManager;
}

LRESULT CDefaultServersSettings::OnServerListChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CDefaultServersSettings::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    TRC(IDC_REMEMBERIMAGESERVERSETTINGS, "Remember image server's settings in wizard");
    TRC(IDC_REMEMBERFILESERVERSETTINGS, "Remember file server's settings in wizard");
    RECT serverSelectorRect = GuiTools::GetDialogItemRect(m_hWnd, IDC_IMAGESERVERPLACEHOLDER);
    imageServerSelector_ = std::make_unique<CMultiServerSelectorControl>(uploadEngineManager_, true);
    if ( !imageServerSelector_->Create(m_hWnd, serverSelectorRect) ) {
        return 0;
    }
    imageServerSelector_->setTitle(TR("Default server for uploading images"));
    imageServerSelector_->ShowWindow( SW_SHOW );
    imageServerSelector_->SetWindowPos(GetDlgItem(IDC_IMAGESERVERPLACEHOLDER), serverSelectorRect.left, serverSelectorRect.top, serverSelectorRect.right - serverSelectorRect.left, serverSelectorRect.bottom - serverSelectorRect.top, 0);
    imageServerSelector_->setServerProfileGroup(Settings.imageServer);

    serverSelectorRect = GuiTools::GetDialogItemRect( m_hWnd, IDC_FILESERVERPLACEHOLDER);

    fileServerSelector_ = std::make_unique<CMultiServerSelectorControl>(uploadEngineManager_);
    fileServerSelector_->setServersMask(CUploadEngineData::TypeFileServer | CUploadEngineData::TypeVideoServer);
    //fileServerSelector_->setShowImageProcessingParams(false);
    fileServerSelector_->Create(m_hWnd, serverSelectorRect);
    fileServerSelector_->ShowWindow( SW_SHOW );
    fileServerSelector_->SetWindowPos(GetDlgItem(IDC_FILESERVERPLACEHOLDER), serverSelectorRect.left, serverSelectorRect.top, serverSelectorRect.right - serverSelectorRect.left, serverSelectorRect.bottom - serverSelectorRect.top, 0);
    fileServerSelector_->setServerProfileGroup(Settings.fileServer);
    fileServerSelector_->setTitle(TR("Default server for other file types"));

    serverSelectorRect = GuiTools::GetDialogItemRect( m_hWnd, IDC_TRAYSERVERPLACEHOLDER);
    trayServerSelector_ = std::make_unique<CServerSelectorControl>(uploadEngineManager_);
    //trayServerSelector_->setShowDefaultServerItem(true);
    trayServerSelector_->Create(m_hWnd, serverSelectorRect);
    trayServerSelector_->ShowWindow( SW_SHOW );
    trayServerSelector_->SetWindowPos(GetDlgItem(IDC_TRAYSERVERPLACEHOLDER), serverSelectorRect.left, serverSelectorRect.top, serverSelectorRect.right - serverSelectorRect.left, serverSelectorRect.bottom - serverSelectorRect.top, 0);

    trayServerSelector_->setServerProfile(Settings.quickScreenshotServer.getByIndex(0));
    trayServerSelector_->setTitle(TR("Server for quick screenshot uploading"));

    serverSelectorRect = GuiTools::GetDialogItemRect( m_hWnd, IDC_CONTEXTMENUSERVERPLACEHOLDER);

    contextMenuServerSelector_  = std::make_unique<CMultiServerSelectorControl>(uploadEngineManager_);
    //contextMenuServerSelector_->setShowDefaultServerItem(true);
    contextMenuServerSelector_->Create(m_hWnd, serverSelectorRect);
    contextMenuServerSelector_->ShowWindow( SW_SHOW );
    contextMenuServerSelector_->SetWindowPos(GetDlgItem(IDC_CONTEXTMENUSERVERPLACEHOLDER), serverSelectorRect.left, serverSelectorRect.top, serverSelectorRect.right - serverSelectorRect.left, serverSelectorRect.bottom - serverSelectorRect.top, 0);

    contextMenuServerSelector_->setServerProfileGroup(Settings.contextMenuServer);
    contextMenuServerSelector_->setTitle(TR("Server for uploading from Explorer's context menu"));


    // Intermediate server for storing temporary images
    serverSelectorRect = GuiTools::GetDialogItemRect(m_hWnd, IDC_TEMPORARYSERVERPLACEHOLDER);
    temporaryServerSelector_ = std::make_unique<CServerSelectorControl>(uploadEngineManager_);
    //trayServerSelector_->setShowDefaultServerItem(true);
    temporaryServerSelector_->Create(m_hWnd, serverSelectorRect);
    temporaryServerSelector_->ShowWindow(SW_SHOW);
    temporaryServerSelector_->SetWindowPos(GetDlgItem(IDC_TEMPORARYSERVERPLACEHOLDER), serverSelectorRect.left, serverSelectorRect.top, serverSelectorRect.right - serverSelectorRect.left, serverSelectorRect.bottom - serverSelectorRect.top, 0);

    temporaryServerSelector_->setServerProfile(Settings.temporaryServer);
    temporaryServerSelector_->setTitle(TR("Server for temporary images"));

    serverSelectorRect = GuiTools::GetDialogItemRect( m_hWnd, IDC_URLSHORTENERPLACEHOLDER);

    urlShortenerServerSelector_ = std::make_unique<CServerSelectorControl>(uploadEngineManager_);
    urlShortenerServerSelector_->setServersMask(CUploadEngineData::TypeUrlShorteningServer);
    urlShortenerServerSelector_->setShowImageProcessingParams(false);
    urlShortenerServerSelector_->setShowParamsLink(false);
    urlShortenerServerSelector_->Create(m_hWnd, serverSelectorRect);
    urlShortenerServerSelector_->ShowWindow( SW_SHOW );
    urlShortenerServerSelector_->SetWindowPos(GetDlgItem(IDC_URLSHORTENERPLACEHOLDER), serverSelectorRect.left, serverSelectorRect.top, serverSelectorRect.right - serverSelectorRect.left, serverSelectorRect.bottom - serverSelectorRect.top, 0);
    urlShortenerServerSelector_->setServerProfile(Settings.urlShorteningServer);
    urlShortenerServerSelector_->setTitle(TR("URL shortening server"));


    GuiTools::SetCheck(m_hWnd, IDC_REMEMBERIMAGESERVERSETTINGS, Settings.RememberImageServer);
    GuiTools::SetCheck(m_hWnd, IDC_REMEMBERFILESERVERSETTINGS, Settings.RememberFileServer);

    return 1;  // Let the system set the focus
}

LRESULT CDefaultServersSettings::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    CMultiServerSelectorControl* multiSelectors[] = {
        imageServerSelector_.get(),
        fileServerSelector_.get(),
        contextMenuServerSelector_.get(),
    };

    CServerSelectorControl* selectors[] = {
        trayServerSelector_.get(),
        urlShortenerServerSelector_.get(),
        temporaryServerSelector_.get()
    };

    for (auto multiSelector : multiSelectors) {
        multiSelector->SendMessage(WM_MY_DPICHANGED, wParam);
    }

    for (auto selector : selectors) {
        selector->SendMessage(WM_MY_DPICHANGED, wParam);
    }
    return 0;
}

bool CDefaultServersSettings::apply()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    CMultiServerSelectorControl* multiControls[] = { fileServerSelector_.get(), imageServerSelector_.get(), contextMenuServerSelector_.get() };
    for (auto* control : multiControls) {
        if (control->serverProfileGroup().isEmpty()) {
            CString message;
            message.Format(TR("You have not selected \"%s\""), control->getTitle().GetString());
            GuiTools::LocalizedMessageBox(m_hWnd, message, TR("Error"), MB_ICONERROR);
            return false;
        }
    }

    CServerSelectorControl* controls[] = { trayServerSelector_.get(), urlShortenerServerSelector_.get(), temporaryServerSelector_.get() };

    for(auto & control : controls) {
        if (control->serverProfile().serverName().empty()) {
            CString message;
            message.Format(TR("You have not selected \"%s\""), control->getTitle().GetString());
            GuiTools::LocalizedMessageBox(m_hWnd, message, TR("Error"), MB_ICONERROR);
            return false;
        }
        if (!control->isAccountChosen()) {
            CString message;
            message.Format(TR("You have not selected account for server \"%s\""), U2WC(control->serverProfile().serverName()));
            GuiTools::LocalizedMessageBox(m_hWnd, message, TR("Error"), MB_ICONERROR);
            return false;
        }
    }

    settings->fileServer = fileServerSelector_->serverProfileGroup();
    settings->imageServer = imageServerSelector_->serverProfileGroup();
    settings->quickScreenshotServer = trayServerSelector_->serverProfile();
    settings->contextMenuServer = contextMenuServerSelector_->serverProfileGroup();
    settings->urlShorteningServer = urlShortenerServerSelector_->serverProfile();
    settings->temporaryServer = temporaryServerSelector_->serverProfile();
    settings->RememberImageServer = GuiTools::GetCheck(m_hWnd, IDC_REMEMBERIMAGESERVERSETTINGS);
    settings->RememberFileServer = GuiTools::GetCheck(m_hWnd, IDC_REMEMBERFILESERVERSETTINGS);
    ServiceLocator::instance()->programWindow()->setServersChanged(true);
    return true;
}

