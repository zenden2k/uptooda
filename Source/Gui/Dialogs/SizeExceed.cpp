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

#include "SizeExceed.h"

#include "Func/Common.h"
#include "Core/Upload/UploadEngine.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Core/Upload/FileUploadTask.h"
#include "Func/IuCommonFunctions.h"
#include "Func/MyEngineList.h"
#include "Core/Settings/WtlGuiSettings.h"

// CSizeExceed
CSizeExceed::CSizeExceed(FileUploadTask * fileTask, CUploadEngineList * EngineList, UploadEngineManager* uploadEngineManager):
        m_EngineList(EngineList)
{
    fileTask_ = fileTask;
    m_szFileName = U2W(fileTask->getFileName());
    uploadEngineManager_ = uploadEngineManager;
}

void CSizeExceed::Translate()
{
    TRC(IDC_WHATTODO, "Change image processing settings or choose another server, so your image can be uploaded.");

    TRC(IDOK, "OK");
    TRC(IDC_FORALL, "To all");
    TRC(IDCANCEL, "Ignore");
    SetWindowText(TR("File size exceeding"));
}

LRESULT CSizeExceed::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    RECT rc = {12, 30, 162, 144};
    img.Create(m_hWnd, rc);
    bool isImage = fileTask_->isImage() && IuCommonFunctions::IsImage(m_szFileName);
    if (isImage) {
        img.loadImage(m_szFileName);
    }

    CenterWindow(GetParent());

    RECT serverSelectorRect = GuiTools::GetDialogItemRect(m_hWnd, IDC_SERVERPLACEHOLDER);
    imageServerSelector_ = std::make_unique<CServerSelectorControl>(uploadEngineManager_, true);
    imageServerSelector_->setShowFilesizeLimits(true);
    imageServerSelector_->Create(m_hWnd, serverSelectorRect);
    imageServerSelector_->setTitle(fileTask_->isImage() ? TR("Server for uploading images") : TR("Server for other file types"));
    if (!fileTask_->isImage()) {
        imageServerSelector_->setServersMask(CUploadEngineData::TypeImageServer | CUploadEngineData::TypeFileServer);
    }

    imageServerSelector_->ShowWindow(SW_SHOW);
    imageServerSelector_->SetWindowPos(nullptr, serverSelectorRect.left, serverSelectorRect.top, serverSelectorRect.right - serverSelectorRect.left,
                                        serverSelectorRect.bottom - serverSelectorRect.top, SWP_NOZORDER);
    imageServerSelector_->setServerProfile(settings->imageServer.getByIndex(0));

    //CString serverName = U2W(fileTask_->serverProfile().serverName());

    fileExceedNameLabelFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_FILEEXCEEDNAME));

    int64_t fileSize = IuCoreUtils::GetFileSize(W2U(m_szFileName));
    WCHAR buf2[25];
    WinUtils::NewBytesToString(fileSize, buf2, 25);


    CString params;
    CString onlyFileName = WinUtils::TrimString(WinUtils::DoExtractFileName(m_szFileName), 40);
    if (isImage) {
        params.Format(_T(" %s (%dx%d, %s)"), static_cast<LPCTSTR>(onlyFileName), img.imageWidth(), img.imageHeight(), static_cast<LPCTSTR>(buf2));
    } else {
        params.Format(_T(" %s (%s)"), static_cast<LPCTSTR>(onlyFileName), static_cast<LPCTSTR>(buf2));
    }

    CString name = TR("File") + params;

    SetDlgItemText(IDC_FILEEXCEEDNAME, name);
    WinUtils::NewBytesToString(m_EngineList->byName(fileTask_->serverProfile().serverName())->MaxFileSize, buf2, 25);

    CString message;
    message.Format(TR("File exceeds filesize limit of \"%s\" server (%s)."), static_cast<LPCTSTR>(U2W(fileTask_->serverProfile().serverName())), buf2);
    SetDlgItemText(IDC_FILEEXCEEDSIZE2, message);
    Translate();

    return 1;  // Let the system set the focus
}

LRESULT CSizeExceed::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (!checkAccount()) {
        return 0;
    }
    fileTask_->setServerProfile(imageServerSelector_->serverProfile());
    EndDialog(wID);
    return 0;
}

LRESULT CSizeExceed::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CSizeExceed::OnBnClickedForall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!checkAccount()) {
        return 0;
    }
    fileTask_->setServerProfile(imageServerSelector_->serverProfile());
    EndDialog(IDC_FORALL);
    return 0;
}

bool CSizeExceed::checkAccount() {
    std::string serverName = imageServerSelector_->serverProfile().serverName();
    if (serverName.empty()) {
        LocalizedMessageBox(TR("You have not selected server!"), TR("Error"), MB_ICONERROR);
        return false;
    }
    if (!imageServerSelector_->isAccountChosen()) {
        CString message;
        message.Format(TR("You have not selected account for server \"%s\""), IuCoreUtils::Utf8ToWstring(imageServerSelector_->serverProfile().serverName()).c_str());
        LocalizedMessageBox(message, TR("Error"), MB_ICONERROR);
        return false;
    }
    return true;

}
