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

#include "UploadDlg.h"

#include <shobjidl.h>
#include <boost/format.hpp>

#include "Gui/Dialogs/LogWindow.h"
#include "Core/Upload/UploadEngine.h"
#include "Gui/GuiTools.h"
#include "Core/Upload/FileQueueUploader.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UploadManager.h"
#include "Func/WinUtils.h"
#include "Gui/Dialogs/WizardDlg.h"
#ifdef IU_ENABLE_MEDIAINFO
#include "Func/MediaInfoHelper.h"
#endif
#include "Func/IuCommonFunctions.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/Upload/ServerProfileGroup.h"
#include "Gui/Models/UploadListModel.h"
#include "Gui/Helpers/DPIHelper.h"

constexpr auto UPLOAD_LIST_VIEW_COLUMN_WIDTH = 170;

// CUploadDlg
CUploadDlg::CUploadDlg(CWizardDlg *dlg, UploadManager* uploadManager) : resultsWindow_(new CResultsWindow(dlg, urlList_, true))
{
    MainDlg = nullptr;
    engineList_ = ServiceLocator::instance()->myEngineList();
    backgroundThreadStarted_ = false;
    isEnableNextButtonTimerRunning_ = false;
    uploadManager_ = uploadManager;
    using namespace std::placeholders;
    resultsWindow_->setOnShortenUrlChanged(std::bind(&CUploadDlg::onShortenUrlChanged, this, _1));
}

LRESULT CUploadDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    GuiTools::SetWindowPointer(m_hWnd, this);
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    uploadProgressBar_ = GetDlgItem(IDC_UPLOADPROGRESS);
    imageViewWindow_.Create(m_hWnd);

    RECT rc;
    ::GetWindowRect(GetDlgItem(IDC_RESULTSPLACEHOLDER), &rc);
    ::MapWindowPoints(0,m_hWnd, reinterpret_cast<POINT*>(&rc), 2);

    uploadListView_.AttachToDlgItem(m_hWnd, IDC_UPLOADTABLE);
    uploadListView_.AddColumn(TR("File"), 1);
    uploadListView_.AddColumn(TR("Status"), 1);
    uploadListView_.AddColumn(TR("Thumbnail"), 2);
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);

    int columnWidth = MulDiv(UPLOAD_LIST_VIEW_COLUMN_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI);
    uploadListView_.SetColumnWidth(0, columnWidth);
    uploadListView_.SetColumnWidth(1, columnWidth);
    uploadListView_.SetColumnWidth(2, columnWidth);

    uploadListView_.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);

    createToolbar();

    resultsWindow_->Create(m_hWnd);
    resultsWindow_->SetWindowPos(nullptr, &rc, SWP_NOZORDER|SWP_NOSIZE);

        // Initializing Windows 7 taskbar related stuff
        //const GUID IID_ITaskbarList3 = { 0xea1afb91,0x9e28,0x4b86,{0x90,0xe9,0x9e,0x9f, 0x8a,0x5e,0xef,0xaf}};
    ptl_.CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL);

    TRC(IDC_COMMONPROGRESS, "Progress:");
    bool IsLastVideo = false;
#ifdef IU_ENABLE_MEDIAINFO
    if (MediaInfoHelper::IsMediaInfoAvailable()) {
        IsLastVideo = !WizardDlg->getLastVideoFile().IsEmpty();
    }
#endif
	resultsWindow_->EnableMediaInfo(IsLastVideo);

    SetDlgItemInt(IDC_THUMBSPERLINE, 4);
    SendDlgItemMessage(IDC_THUMBPERLINESPIN, UDM_SETRANGE, 0, (LPARAM) MAKELONG((short)100, (short)1) );

    commonProgressLabelFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_COMMONPROGRESS));
    commonPercentLabelFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_COMMONPERCENTS));
    PageWnd = m_hWnd;
    using namespace Uptooda::Core::OutputGenerator;
    resultsWindow_->SetPage(static_cast<CodeLang>(Settings.CodeLang));
    resultsWindow_->SetCodeType(Settings.CodeType);
    showUploadProgressTab();
    return FALSE;
}

bool CUploadDlg::startUpload() {
    if (!MainDlg) {
        return false;
    }
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    if (ptl_) {
        ptl_->SetProgressState(GetParent(), TBPF_NORMAL); // initialise Windows 7 taskbar button progress
    }

    SetDlgItemText(IDC_COMMONPROGRESS2, _T(""));
    showUploadProgressTab();
    int n = MainDlg->FileList.GetCount();
    uploadProgressBar_.SetState(PBST_NORMAL);
    uploadProgressBar_.SetPos(0);
    //uploadProgressBar_.SetRange(0, n);
    SetDlgItemText(IDC_COMMONPERCENTS, _T("0%"));

    SetTimer(kProgressTimer, 1000);
    uploadListView_.DeleteAllItems();

    urlList_.clear();

    uploadSession_ = std::make_shared<UploadSession>();

    int rowIndex = 0;
    for (int i = 0; i < n; i++) {
        const auto& item = MainDlg->FileList[i];
        if (item.isSkipped()) {
            continue;
        }
        CString FileName = item.FileName;
        std::string fileNameUtf8 = WCstringToUtf8(FileName);
        std::string displayName = WCstringToUtf8(MainDlg->FileList[i].VirtualFileName);

        bool isImage = IuCommonFunctions::IsImage(FileName);
        bool isVideo = !isImage && IuCoreUtils::GetFileMimeType(fileNameUtf8).find("video/") != std::string::npos;

        ServerProfileGroup& serverProfileGroup = isImage ? sessionImageServer_ : sessionFileServer_;
        //serverProfileGroup.addItem();
        for (const auto& serverProfile: serverProfileGroup.getItems()) {

            auto task = std::make_shared<FileUploadTask>(fileNameUtf8, displayName);
            task->setIndex(/*rowIndex++*/i);
            //task->setFileIndex(i);
            task->setIsImage(isImage);
            task->setIsVideo(isVideo);

            task->setServerProfile(/*isImage ? sessionImageServer_ : sessionFileServer_*/serverProfile);
            task->setUrlShorteningServer(settings->urlShorteningServer);
            using namespace std::placeholders;
            task->addTaskFinishedCallback([this](auto && PH1, auto && PH2) { onTaskFinished(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
            task->addChildTaskAddedCallback([this](auto && PH1) { onChildTaskAdded(std::forward<decltype(PH1)>(PH1)); });

            task->setOnFolderUsedCallback([this](auto && PH1) { OnFolderUsed(std::forward<decltype(PH1)>(PH1)); });
            uploadSession_->addTask(task);
            rowIndex++;
        }
    }
    TotalUploadProgress(0, rowIndex);
    urlList_.resize(rowIndex);
    uploadProgressBar_.SetRange(0, rowIndex);
    uploadSession_->addSessionFinishedCallback([this](auto && PH1) { onSessionFinished(std::forward<decltype(PH1)>(PH1)); });

    uploadListModel_ = std::make_unique<UploadListModel>(uploadSession_);
    uploadListView_.SetModel(uploadListModel_.get());


    uploadManager_->addSession(uploadSession_);
    return true;
}

LRESULT CUploadDlg::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == kEnableNextButtonTimer)
    {
        EnableNext(true);
        isEnableNextButtonTimerRunning_ = false;
        KillTimer(kEnableNextButtonTimer);
    } else if (wParam == kProgressTimer) {
        updateTotalProgress();
    }
    return 0;
}

LRESULT CUploadDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    HWND hwnd = reinterpret_cast<HWND>(wParam);
    POINT ClientPoint, ScreenPoint;
    if (hwnd != GetDlgItem(IDC_UPLOADTABLE)) {
        return 0;
    }

    if (!uploadSession_) {
        return 0;
    }
    int nCurItem = uploadListView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);

    if (lParam == -1) {
        ClientPoint.x = 0;
        ClientPoint.y = 0;
        if (nCurItem >= 0) {
            CRect rc;
            if (uploadListView_.GetItemRect(nCurItem, &rc, LVIR_BOUNDS)) {
                ClientPoint = rc.CenterPoint();
            }
        }
        ScreenPoint = ClientPoint;
        ::ClientToScreen(hwnd, &ScreenPoint);
    } else {
        ScreenPoint.x = GET_X_LPARAM(lParam);
        ScreenPoint.y = GET_Y_LPARAM(lParam);
        ClientPoint = ScreenPoint;
        ::ScreenToClient(hwnd, &ClientPoint);
    }

    bool menuItemEnabled = false;
    bool enableCopyLinkMenuItem = false;
    bool enableCopyViewLinkMenuItem = false;
    bool enableCopyThumbLinkMenuItem = false;
    bool enableCopyDeleteLinkMenuItem = false;
    bool isImage = false;
    if (nCurItem >= 0) {
        UploadListItem* item = uploadListModel_->getDataByIndex(nCurItem);
        if (item) {
            isImage = IuCommonFunctions::IsImage(item->fileName());
            if (!uploadSession_->isRunning()) {
                isImage = IuCommonFunctions::IsImage(item->fileName());
                auto task = uploadSession_->getTask(nCurItem);
                if (task) {
                    auto status = task->status();
                    if (status != UploadTask::StatusFinished && status != UploadTask::StatusRunning) {
                        menuItemEnabled = true;
                    } else if (status == UploadTask::StatusFinished) {
                        std::lock_guard<std::mutex> lk(resultsWindow_->outputMutex());
                        if (nCurItem < urlList_.size()) {
                            auto& obj = urlList_[nCurItem];
                            enableCopyLinkMenuItem = !obj.uploadResult.getDirectUrl().empty()
                                || !obj.uploadResult.getDownloadUrl().empty();
                            enableCopyViewLinkMenuItem = !obj.uploadResult.getDownloadUrl().empty();
                            enableCopyThumbLinkMenuItem = !obj.uploadResult.getThumbUrl().empty();
                            enableCopyDeleteLinkMenuItem = !obj.uploadResult.deleteUrl.empty();
                        }
                    }

                }
            }
        }
    }

    CMenu menu;
    menu.CreatePopupMenu();
    if (isImage) {
        menu.AppendMenu(MF_STRING, ID_VIEWIMAGE, TR("View"));
    }
    menu.AppendMenu(MF_STRING, ID_RETRYUPLOAD, TR("Retry"));
    menu.EnableMenuItem(ID_RETRYUPLOAD, menuItemEnabled ? MF_ENABLED : MF_GRAYED);

    menu.AppendMenu(MF_STRING, ID_SHOWLOGFORTHISFILE, TR("Show log for this file"));
    menu.SetMenuDefaultItem(ID_VIEWIMAGE, FALSE);

    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_COPYLINK, TR("Copy link"));
    menu.EnableMenuItem(ID_COPYLINK, enableCopyLinkMenuItem ? MF_ENABLED : MF_DISABLED);

    menu.AppendMenu(MF_STRING, ID_COPYVIEWLINK, TR("Copy link to view page"));
    menu.EnableMenuItem(ID_COPYVIEWLINK, enableCopyViewLinkMenuItem ? MF_ENABLED : MF_DISABLED);

    menu.AppendMenu(MF_STRING, ID_COPYTHUMBLINK, TR("Copy link to thumbnail"));
    menu.EnableMenuItem(ID_COPYTHUMBLINK, enableCopyThumbLinkMenuItem ? MF_ENABLED : MF_DISABLED);

    menu.AppendMenu(MF_STRING, ID_COPYDELETELINK, TR("Copy delete link"));
    menu.EnableMenuItem(ID_COPYDELETELINK, enableCopyDeleteLinkMenuItem ? MF_ENABLED : MF_DISABLED);

    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, ScreenPoint.x, ScreenPoint.y, m_hWnd);
    return 0;
}

LRESULT CUploadDlg::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createToolbar();
    int dpi = LOWORD(wParam);
    int columnWidth = MulDiv(UPLOAD_LIST_VIEW_COLUMN_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI);
    uploadListView_.SetColumnWidth(0, columnWidth);
    uploadListView_.SetColumnWidth(1, columnWidth);
    uploadListView_.SetColumnWidth(2, columnWidth);
    resultsWindow_->SendMessage(WM_MY_DPICHANGED, wParam, lParam);
    return 0;
}

int CUploadDlg::ThreadTerminated()
{
    WizardDlg->setQuickUploadMarker(false);

    if (ptl_) {
        ptl_->SetProgressState(GetParent(), TBPF_NOPROGRESS);
    }

    KillTimer(kProgressTimer);
    SetNextCaption(TR("Finish >"));
    backgroundThreadStarted_ = false;
    EnablePrev();
    EnableNext();
    EnableExit();
    return 0;
}

bool CUploadDlg::OnShow()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    EnableNext(false);
    ShowNext();
    ShowPrev();
    sessionFileServer_ = WizardDlg->getSessionFileServer();
    sessionImageServer_ = WizardDlg->getSessionImageServer();
    bool IsLastVideo=false;
#ifdef IU_ENABLE_MEDIAINFO
    if ( MediaInfoHelper::IsMediaInfoAvailable()) {
        auto *vg = WizardDlg->getPage<CVideoGrabberPage>(CWizardDlg::wpVideoGrabberPage);

        if(vg && !vg->fileName_.IsEmpty())
            IsLastVideo=true;
    }
#endif
    resultsWindow_->InitUpload();
    resultsWindow_->EnableMediaInfo(IsLastVideo);
    CancelByUser = false;

    isEnableNextButtonTimerRunning_ = true;
    SetTimer(kEnableNextButtonTimer, 1000);
    MainDlg = WizardDlg->getPage<CMainDlg>(CWizardDlg::wpMainPage);
    //Toolbar.CheckButton(IDC_USETEMPLATE,Settings.UseTxtTemplate);
    urlList_.clear();
    resultsWindow_->Clear();
    resultsWindow_->setShortenUrls(sessionImageServer_.getByIndex(0).shortenLinks());

    int code = resultsWindow_->GetCodeType();
    int newcode = code;
    bool Thumbs = sessionImageServer_.getByIndex(0).getImageUploadParams().CreateThumbs!=0;

    if(Thumbs)
    {
        if(code<4 && code>1)
            newcode = 0;
    }
    else
    {
        if(code<2)
            newcode=2;
    }
    resultsWindow_->SetCodeType(newcode);
    resultsWindow_->SetPage(static_cast<CResultsPanel::TabPage>(settings->CodeLang));

    //::SetFocus(GetDlgItem(IDC_CODEEDIT));
    alreadyShortened_ = false;
    backgroundThreadStarted();
    startUpload();
    return true;
}

bool CUploadDlg::OnNext() {
    if (uploadSession_->isRunning()) {
        uploadListView_.SetRedraw(FALSE);
        uploadManager_->stopSession(uploadSession_.get());
        uploadListView_.SetRedraw(TRUE);
        CancelByUser = true;
    }
    else {
        MainDlg->ThumbsView.MyDeleteAllItems();
        EnableExit();
        return true;
    }
    return false;
}

bool CUploadDlg::OnHide()
{
    urlList_.clear();
    resultsWindow_->Clear();
    uploadManager_->removeSession(uploadSession_);
    uploadSession_.reset();
    uploadListView_.SetModel(nullptr);
    uploadListModel_.reset();

    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->UseTxtTemplate = (SendDlgItemMessage(IDC_USETEMPLATE, BM_GETCHECK) == BST_CHECKED);
    settings->CodeType = resultsWindow_->GetCodeType();
    settings->CodeLang = resultsWindow_->GetPage();
    return true;
}

void CUploadDlg::GenerateOutput(bool immediately)
{
    resultsWindow_->UpdateOutput(immediately);
}

void CUploadDlg::TotalUploadProgress(int CurPos, int Total, int FileProgress)
{
    uploadProgressBar_.SetPos(CurPos);

    if(ptl_) {
        int NewCurrent = CurPos * 100 + FileProgress;
        int NewTotal = Total * 100;
        ptl_->SetProgressValue(WizardDlg->m_hWnd, NewCurrent, NewTotal);
    }

    progressCurrent = CurPos;
    progressTotal = Total;
    CString res;
    res.Format(TR("Links (%d)"), CurPos);

    if (Total) {
        CString buffer;
        buffer.Format(_T("%d %%"), static_cast<int>(100 * (static_cast<double>(CurPos) / Total)));
        SetDlgItemText(IDC_COMMONPERCENTS, buffer);
    }
    toolbar_.SetButtonInfo(IDC_UPLOADRESULTSTAB, TBIF_TEXT, 0, 0, res,0, 0, 0, 0);
}

void CUploadDlg::OnFolderUsed(UploadTask* task) {
    ServiceLocator::instance()->taskRunner()->runInGuiThread([wnd = this->m_hWnd, profile = task->serverProfile(), this] {
        if (::IsWindow(wnd)) {
            resultsWindow_->AddServer(profile);
        }
    }, true);
}

void CUploadDlg::onShortenUrlChanged(bool shortenUrl) {
    if ( !alreadyShortened_ && shortenUrl ) {
        GenerateOutput();

        uploadManager_->shortenLinksInSession(uploadSession_, ServiceLocator::instance()->urlShorteningFilter().get());
    } else {
        GenerateOutput(true);
    }
}

void CUploadDlg::createToolbar() {
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    const DWORD rtlStyle = ServiceLocator::instance()->translator()->isRTL() ? ILC_MIRROR | ILC_PERITEMMIRROR : 0;
    const int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    auto loadToolbarIcon = [&](int resourceId) -> int {
        CIcon icon;
        icon.LoadIconWithScaleDown(MAKEINTRESOURCE(resourceId), iconWidth, iconHeight);
        return toolbarImageList_.AddIcon(icon);
    };

    if (toolbarImageList_) {
        toolbar_.SetImageList(nullptr);
        toolbarImageList_.Destroy();
    }
    //if (GuiTools::Is32BPP()) {
        toolbarImageList_.Create(iconWidth, iconHeight, ILC_COLOR32 | rtlStyle, 0, 6);
    //}
    /*else {
        toolbarImageList_.Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK | rtlStyle, 0, 6);
    }*/

    RECT placeholderRect = GuiTools::GetDialogItemRect(m_hWnd, IDC_TOOLBARPLACEHOLDER);
    RECT rc = { 0, 0, 100, 24 };
    GetClientRect(&rc);
    rc.top = placeholderRect.top;
    rc.bottom = rc.top + MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI);
    rc.left = MulDiv(6, dpi, USER_DEFAULT_SCREEN_DPI);
    rc.right -= MulDiv(6, dpi, USER_DEFAULT_SCREEN_DPI);
    if (!toolbar_) {
        toolbar_.Create(m_hWnd, rc, _T(""), WS_CHILD | WS_CHILD | WS_TABSTOP | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_NORESIZE /*|*/ | CCS_BOTTOM | /*CCS_ADJUSTABLE|*/ CCS_NODIVIDER | TBSTYLE_AUTOSIZE);
        toolbar_.SetButtonStructSize();
        //toolbar_.SetButtonSize(30, 18);
    }
    int buttonCount = toolbar_.GetButtonCount();

    for (int i = buttonCount - 1; i >= 0; i--) {
        toolbar_.DeleteButton(i);
    }

    toolbar_.SetImageList(toolbarImageList_);

    toolbar_.AddButton(IDC_UPLOADPROCESSTAB, BTNS_CHECK | BTNS_AUTOSIZE, TBSTATE_ENABLED | (currentTab_ == IDC_UPLOADPROCESSTAB ? TBSTATE_PRESSED : 0), loadToolbarIcon(IDI_ICONUPLOAD), TR("Upload progress"), 0);
    toolbar_.AddButton(IDC_UPLOADRESULTSTAB, BTNS_CHECK | BTNS_AUTOSIZE, TBSTATE_ENABLED | (currentTab_ == IDC_UPLOADRESULTSTAB ? TBSTATE_PRESSED : 0), loadToolbarIcon(IDI_ICONINFO), TR("Links"), 0);
    toolbar_.AddButton(IDC_VIEWLOG, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, loadToolbarIcon(IDI_ICONLOG), TR("Log"), 0);

    toolbar_.AutoSize();
    toolbar_.SetWindowLong(GWL_ID, IDC_RESULTSTOOLBAR);
    toolbar_.SetWindowPos(GetDlgItem(IDC_PROGRESSGROUPBOX), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SIZE toolbarSize;

    if (toolbar_.GetMaxSize(&toolbarSize)) {
        toolbar_.SetWindowPos(nullptr, 0, 0, rc.right - rc.left, toolbarSize.cy, SWP_NOZORDER|SWP_NOMOVE);
    }

    toolbar_.ShowWindow(SW_SHOW);
}

void CUploadDlg::updateTotalProgress() {
    if (!uploadSession_) {
        return;
    }
    int n = uploadSession_->taskCount();
    int totalPercent = 0;
    for (int i = 0; i < n; i++) {
        std::shared_ptr<UploadTask> task = uploadSession_->getTask(i);
        if (task->isRunning()) {
            int percent = 0;
            UploadProgress* progress = task->progress();
            if (progress->totalUpload) {
                percent = static_cast<int>(100 * static_cast<float>(progress->uploaded) / progress->totalUpload);
                totalPercent += percent;
            }
        }
    }
    TotalUploadProgress(uploadSession_->finishedTaskCount(UploadTask::StatusFinished), uploadSession_->taskCount(), totalPercent);
}

LRESULT CUploadDlg::OnBnClickedViewLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ServiceLocator::instance()->logWindow()->Show();
    return 0;
}

LRESULT CUploadDlg::OnUploadTableDoubleClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto* lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
    int row = lpnmitem->iItem;
    viewImage(row);
    return 0;
}

void CUploadDlg::viewImage(int itemIndex) {
    if (itemIndex >= 0) {
        UploadListItem* item = uploadListModel_->getDataByIndex(itemIndex);
        if (item && IuCommonFunctions::IsImage(item->fileName())) {
            CImageViewItem imgViewItem;
            imgViewItem.index = 0;
            imgViewItem.fileName = item->fileName();
            HWND wizardDlg = WizardDlg->m_hWnd;
            imageViewWindow_.ViewImage(imgViewItem, wizardDlg);
        }
    }
}

void CUploadDlg::SetInitialFocus() {
    resultsWindow_->SetFocus();
}

LRESULT CUploadDlg::OnRetryUpload(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int nCurItem = uploadListView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
    if (nCurItem >= 0) {
        auto task = uploadSession_->getTask(nCurItem);
        if (task) {
            UploadTask::Status status = task->status();
            if (status != UploadTask::StatusFinished && status != UploadTask::StatusRunning) {
                if (!uploadManager_->IsRunning()) {
                    SetDlgItemText(IDC_COMMONPROGRESS2, _T(""));
                    backgroundThreadStarted();
                    SetTimer(kProgressTimer, 1000);
                    uploadManager_->retrySession(uploadSession_);
                }
            }
        }
    }

    return 0;
}

LRESULT CUploadDlg::OnViewImage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int nCurItem = uploadListView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
    if (nCurItem >= 0) {
        viewImage(nCurItem);
    }
    return 0;
}

void CUploadDlg::showUploadResultsTab() {
    if (currentTab_ == IDC_UPLOADRESULTSTAB) {
        return;
    }
    toolbar_.SetButtonInfo(IDC_UPLOADPROCESSTAB, TBIF_STATE, 0, TBSTATE_ENABLED, 0, 0, 0, 0, 0);
    toolbar_.SetButtonInfo(IDC_UPLOADRESULTSTAB, TBIF_STATE, 0, TBSTATE_ENABLED | TBSTATE_PRESSED, 0, 0, 0, 0, 0);

    currentTab_ = IDC_UPLOADRESULTSTAB;
    uploadListView_.ShowWindow(SW_HIDE);
    resultsWindow_->ShowWindow(SW_SHOW);
}

void CUploadDlg::showUploadProgressTab() {
    if (currentTab_ == IDC_UPLOADPROCESSTAB) {
        return;
    }
    currentTab_ = IDC_UPLOADPROCESSTAB;
    toolbar_.SetButtonInfo(IDC_UPLOADRESULTSTAB, TBIF_STATE, 0, TBSTATE_ENABLED, 0,
        0, 0, 0, 0);
    toolbar_.SetButtonInfo(IDC_UPLOADPROCESSTAB, TBIF_STATE, 0, TBSTATE_ENABLED | TBSTATE_PRESSED, 0, 0, 0, 0, 0);

    uploadListView_.ShowWindow(SW_SHOW);
    resultsWindow_->ShowWindow(SW_HIDE);
}

// This callback is being executed in worker thread
void CUploadDlg::onSessionFinished(UploadSession* session) {
    int successFileCount = session->finishedTaskCount(UploadTask::StatusFinished);
    int failedFileCount = session->finishedTaskCount(UploadTask::StatusFailure);
    int totalFileCount = session->taskCount();

    ServiceLocator::instance()->taskRunner()->runInGuiThread([wnd = m_hWnd, successFileCount, failedFileCount, totalFileCount, this] {
        if (!::IsWindow(wnd)) {
            return;
        }
        auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
        KillTimer(kEnableNextButtonTimer);
        CString progressLabelText;
        if (successFileCount == totalFileCount) {
            progressLabelText = TR("All files have been successfully uploaded.");
        } else {
            if (CancelByUser) {
                progressLabelText = TR("File uploading was cancelled by user.");
            } else if (failedFileCount) {
                progressLabelText.Format(TR("Errors: %d"), failedFileCount);
                progressLabelText = CString(TR("Uploading has been finished.")) + _T(" ") + progressLabelText;
            }
        }

        SetDlgItemText(IDC_COMMONPROGRESS2, progressLabelText);
        resultsWindow_->UpdateOutput(true);
        updateTotalProgress();
        if (failedFileCount * 100 / totalFileCount >= 50) {
            uploadProgressBar_.SetState(PBST_ERROR);
        } else if (failedFileCount) {
            uploadProgressBar_.SetState(PBST_PAUSED);
        }
        ThreadTerminated();
        if (successFileCount == totalFileCount) {
            if (settings->AutoCopyToClipboard) {
                resultsWindow_->copyResultsToClipboard();
            }
            showUploadResultsTab();
        }
    }, true);
}

// This callback is being executed in worker thread
void CUploadDlg::onTaskFinished(UploadTask* task, bool ok)
{
    auto* fileTask = dynamic_cast<FileUploadTask*>(task);
    //auto* taskDispatcher = ServiceLocator::instance()->taskRunner();

    if (fileTask && fileTask->role() == UploadTask::DefaultRole /* && ok*/) {
        auto* fps = static_cast<UploadListItem*>(task->userData());
        if (!fps)
        {
            return;
        }
        Uptooda::Core::OutputGenerator::UploadObject item;
        item.fillFromUploadResult(task->uploadResult(), task);

        item.fileIndex = task->index();
        {
            std::lock_guard<std::mutex> lk(resultsWindow_->outputMutex());
            urlList_[fps->tableRow] = std::move(item);
        }

    }
    if (task->role() == UploadTask::UrlShorteningRole && ok) {
        UploadTask* parentTask = task->parentTask();
        auto* fps = static_cast<UploadListItem*>(parentTask->userData());
        if (!fps) {
            return;
        }
        {
            std::lock_guard<std::mutex> lk(resultsWindow_->outputMutex());
            auto& row = urlList_[fps->tableRow];
            row.uploadResult.directUrlShortened = parentTask->uploadResult()->getDirectUrlShortened();
            row.uploadResult.downloadUrlShortened = parentTask->uploadResult()->getDownloadUrlShortened();
        }
    }

    // No need to use runInGuiThread() here because update of the output is postponed and triggers by timer
    GenerateOutput();
}

void CUploadDlg::onChildTaskAdded(UploadTask* child)
{
    using namespace std::placeholders;
    child->addTaskFinishedCallback([this](auto && PH1, auto && PH2) { onTaskFinished(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    auto* dispatcher = ServiceLocator::instance()->taskRunner();
    if (!backgroundThreadStarted_) {
        dispatcher->runInGuiThread([wnd = this->m_hWnd, this] {
            if (GuiTools::CheckWindowPointer(wnd, this)) {
                backgroundThreadStarted();
            }
        }, true);
    }

}

void CUploadDlg::backgroundThreadStarted()
{
    std::lock_guard<std::mutex> lock(backgroundThreadStartedMutex_);
    if (backgroundThreadStarted_)
    {
        return;
    }
    backgroundThreadStarted_ = true;
    EnablePrev(false);
    if (!isEnableNextButtonTimerRunning_)
    {
        EnableNext();
    }

    EnableExit(false);
    SetNextCaption(TR("Stop"));
}

LRESULT CUploadDlg::OnUploadProcessButtonClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    showUploadProgressTab();
    return 0;
}

LRESULT CUploadDlg::OnUploadResultsButtonClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    showUploadResultsTab();
    return 0;
}

LRESULT CUploadDlg::OnShowLogForThisFile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int nCurItem = uploadListView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
    if (nCurItem >= 0) {
        UploadListItem* item = uploadListModel_->getDataByIndex(nCurItem);
        WizardDlg->showLogWindowForFileName(item->fileName());
    }
    return 0;
}

LRESULT CUploadDlg::OnCopyLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int nCurItem = uploadListView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
    if (nCurItem >= 0) {
        std::lock_guard<std::mutex> lk(resultsWindow_->outputMutex());
        if (nCurItem < urlList_.size()) {
            auto& obj = urlList_[nCurItem];

            std::string url = !obj.uploadResult.directUrl.empty() ? obj.uploadResult.directUrl : obj.uploadResult.downloadUrl;
            WinUtils::CopyTextToClipboard(Utf8ToWCstring(url));
        }
    }
    return 0;
}

LRESULT CUploadDlg::OnCopyViewLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int nCurItem = uploadListView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
    if (nCurItem >= 0) {
        std::lock_guard<std::mutex> lk(resultsWindow_->outputMutex());
        if (nCurItem < urlList_.size()) {
            auto& obj = urlList_[nCurItem];

            if (!obj.uploadResult.getDownloadUrl().empty()) {
                WinUtils::CopyTextToClipboard(Utf8ToWCstring(obj.uploadResult.getDownloadUrl()));
            }
        }
    }
    return 0;
}

LRESULT CUploadDlg::OnCopyThumbLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int nCurItem = uploadListView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
    if (nCurItem >= 0) {
        std::lock_guard<std::mutex> lk(resultsWindow_->outputMutex());
        if (nCurItem < urlList_.size()) {
            auto& obj = urlList_[nCurItem];

            if (!obj.uploadResult.getThumbUrl().empty()) {
                WinUtils::CopyTextToClipboard(Utf8ToWCstring(obj.uploadResult.getThumbUrl()));
            }
        }
    }
    return 0;
}

LRESULT CUploadDlg::OnCopyDeleteLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int nCurItem = uploadListView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
    if (nCurItem >= 0) {
        std::lock_guard<std::mutex> lk(resultsWindow_->outputMutex());
        if (nCurItem < urlList_.size()) {
            auto& obj = urlList_[nCurItem];

            if (!obj.uploadResult.deleteUrl.empty()) {
                WinUtils::CopyTextToClipboard(Utf8ToWCstring(obj.uploadResult.deleteUrl));
            }
        }
    }
    return 0;
}
