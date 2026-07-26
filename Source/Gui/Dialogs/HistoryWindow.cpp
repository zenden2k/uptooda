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

#include "HistoryWindow.h"

#include <boost/date_time/gregorian/gregorian.hpp>

#include "Core/i18n/Translator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/HistoryManager.h"
#include "Core/ServiceLocator.h"
#include "ResultsWindow.h"
#include "Core/3rdpart/pcreplusplus.h"
#include "Func/WinUtils.h"
#include "ClearHistoryDlg.h"
#include "Core/Utils/DesktopUtils.h"
#include "Core/ServiceLocator.h"
#include "Core/TaskDispatcher.h"
#include "History/HistoryManagerImpl.h"

namespace {

SYSTEMTIME GregorianDateToSystemTime(const boost::gregorian::date& d) {

    if (d.is_special()) {
        throw std::out_of_range("Cannot handle such date");
    }

    SYSTEMTIME st;

    boost::gregorian::date::ymd_type ymd = d.year_month_day();
    std::memset(&st, 0, sizeof(st));

    st.wYear = ymd.year;
    st.wMonth = ymd.month;
    st.wDay = ymd.day;
    st.wDayOfWeek = d.day_of_week();

    return st;
}

}

// CHistoryWindow
CHistoryWindow::CHistoryWindow(CWizardDlg* wizardDlg) :
    m_treeView(ServiceLocator::instance()->networkClientFactory())
{
    delayedClosing_ = false;
    wizardDlg_ = wizardDlg;
    delayedLoad_ = false;
}

CHistoryWindow::~CHistoryWindow()
{
}

LRESULT CHistoryWindow::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    GuiTools::SetWindowPointer(m_hWnd, this);
    CenterWindow();
    DlgResize_Init();
    dateFromPicker_ = GetDlgItem(IDC_DATEFROMPICKER);
    dateToPicker_ = GetDlgItem(IDC_DATETOPICKER);

    dateFilterCheckbox_ = GetDlgItem(IDC_DATEFROMCHECKBOX);
    dateFilterCheckbox_.SetCheck(BST_CHECKED);

    m_treeView.SubclassWindow(GetDlgItem(IDC_HISTORYTREE));
    using namespace std::placeholders;
    m_treeView.setOnThreadsFinishedCallback(std::bind(&CHistoryWindow::threadsFinished, this));
    m_treeView.setOnThreadsStartedCallback(std::bind(&CHistoryWindow::threadsStarted, this));
    m_treeView.setOnItemDblClickCallback(std::bind(&CHistoryWindow::onItemDblClick, this, _1));
    TRC(IDOK, "Apply");
    TRC(IDC_CLEARFILTERS, "Clear filters");
    TRC(IDC_FILTERSGROUPBOX, "Filters");
    TRC(IDC_DATEFROMCHECKBOX, "Date from:");
    TRC(IDC_DATETOLABEL, "to:");
    TRC(IDC_FILENAMELABEL, "Filename:");
    TRC(IDC_URLLABEL, "URL:");
    TRC(IDCANCEL, "Close");
    TRC(IDC_SESSIONSCOUNTDESCR, "Session total:");
    TRC(IDC_FILESCOUNTDESCR, "Files total:");
    TRC(IDC_UPLOADTRAFFICDESCR, "Total size:");
    SetWindowText(TR("Upload History"));
    TRC(IDC_DOWNLOADTHUMBS, "Retrieve thumbnails from the Internet");
    TRC(IDC_CLEARHISTORYBTN, "Clear History...");

    HWND hWnd = GetDlgItem(IDC_ANIMATIONSTATIC);
    if (hWnd)
    {
        m_wndAnimation.SubclassWindow(hWnd);
        m_wndAnimation.ShowWindow(SW_HIDE);
    }

    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    SendDlgItemMessage(IDC_DOWNLOADTHUMBS, BM_SETCHECK, static_cast<WPARAM>(settings->HistorySettings.EnableDownloading));
    //SelectedMonthChanged();
    initSearchForm();
    dateFromCheckboxChanged();
    applyFilters();
    m_treeView.SetFocus();
    return 0;
}

LRESULT CHistoryWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    GuiTools::ClearWindowPointer(m_hWnd);
    return 0;
}

BOOL CHistoryWindow::PreTranslateMessage(MSG* pMsg)
{
    return CWindow::IsDialogMessage(pMsg);
}

LRESULT CHistoryWindow::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    delayedClosing_ = true;
    if(!m_treeView.isRunning())
    {
        settings->HistorySettings.EnableDownloading = SendDlgItemMessage(IDC_DOWNLOADTHUMBS, BM_GETCHECK) == BST_CHECKED;
        EndDialog(0);
    }
    else
    {
        ::EnableWindow(GetDlgItem(IDCANCEL), false);
        ::EnableWindow(GetDlgItem(IDOK), false);
        ::EnableWindow(GetDlgItem(IDC_CLEARFILTERS), false);
        m_treeView.EnableWindow(true);
        m_treeView.abortLoadingThreads();
    }
    return 0;
}

void CHistoryWindow::Show()
{
    if (!IsWindowVisible()) {
        ShowWindow(SW_SHOW);
    }
    SetForegroundWindow(m_hWnd);
}

LRESULT CHistoryWindow::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hwnd = reinterpret_cast<HWND>(wParam);
    POINT clientPoint, screenPoint;

    if(hwnd != GetDlgItem(IDC_HISTORYTREE)) return 0;

    TreeItem* item{};
    int itemIndex = -1;
    if(lParam == -1)
    {
        clientPoint.x = 0;
        clientPoint.y = 0;
        itemIndex = m_treeView.GetCurSel();
        if (itemIndex >= 0) {
            CRect rc;
            if (m_treeView.GetItemRect(itemIndex, &rc) != LB_ERR) {
                clientPoint = rc.CenterPoint();
            }
        }

        screenPoint = clientPoint;
        ::ClientToScreen(hwnd, &screenPoint);
        item = m_treeView.selectedItem();
    }
    else
    {
        screenPoint.x = GET_X_LPARAM(lParam);
        screenPoint.y = GET_Y_LPARAM(lParam);

        clientPoint = screenPoint;
        ::ScreenToClient(hwnd, &clientPoint);
        BOOL outside = FALSE;
        itemIndex = m_treeView.ItemFromPoint(clientPoint, outside);
        if (outside) {
            return 0;
        }

        item = m_treeView.GetItem(itemIndex);
    }

    if (!item) {
        return 0;
    }
    bool isSessionItem = item->level()==0;

    const HistoryItem* historyItem = CHistoryTreeControl::getItemData(item);
    CMenu menu;
    menu.CreatePopupMenu();
    if(!isSessionItem)
    {
        menu.AppendMenu(MF_STRING, ID_OPENINBROWSER, TR("Open in Web Browser"));
        menu.SetMenuDefaultItem(ID_OPENINBROWSER, FALSE);
        menu.AppendMenu(MF_STRING, ID_COPYTOCLIPBOARD, TR("Copy link") + CString(_T("\tCtrl+C")));
        menu.AppendMenu(MF_STRING, ID_COPYVIEWLINK, TR("Copy link to view page"));
        menu.EnableMenuItem(ID_COPYVIEWLINK, historyItem->viewUrl.empty() ? MF_DISABLED : MF_ENABLED);

        menu.AppendMenu(MF_STRING, ID_COPYTHUMBLINK, TR("Copy link to thumbnail"));
        menu.EnableMenuItem(ID_COPYTHUMBLINK, historyItem->thumbUrl.empty() ? MF_DISABLED : MF_ENABLED);
    }
    menu.AppendMenu(MF_STRING, ID_VIEWBBCODE, TR("View BBCode/HTML codes"));

    if (!isSessionItem) {
        bool fileExists = !historyItem->localFilePath.empty() &&
            IuCoreUtils::DirectoryExists(IuCoreUtils::ExtractFilePath(historyItem->localFilePath));

        menu.AppendMenu(MF_STRING, ID_OPENFOLDER, TR("Open in folder"));
        menu.EnableMenuItem(ID_OPENFOLDER, fileExists ? MF_ENABLED : MF_DISABLED);

        menu.AppendMenu(MF_STRING, ID_EDITFILEONSERVER, TR("Edit file on server"));
        menu.EnableMenuItem(ID_EDITFILEONSERVER, historyItem->editUrl.empty() ? MF_DISABLED : MF_ENABLED);

        menu.AppendMenu(MF_STRING, ID_DELETEFILEONSERVER, TR("Delete file from server")+CString(_T("\tDelete")));
        menu.EnableMenuItem(ID_DELETEFILEONSERVER, historyItem->deleteUrl.empty() ? MF_DISABLED : MF_ENABLED);
    }

    menu.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON, screenPoint.x, screenPoint.y, m_hWnd);
    return 0;
}

void CHistoryWindow::FillList(CHistoryReader* mgr) {
    bool enabledDownload = SendDlgItemMessage(IDC_DOWNLOADTHUMBS, BM_GETCHECK) == BST_CHECKED;
    m_treeView.setDownloadingEnabled(enabledDownload);
    size_t nSessionsCount = mgr->getSessionCount();

    m_treeView.SetRedraw(false);
    TreeItem* res = nullptr;
    int nFilesCount = 0;
    int64_t totalFileSize = 0;

    for (size_t i = 0; i < nSessionsCount; i++) {
        CHistorySession* ses = mgr->getSession(i);
        std::string serverName = ses->serverName();
        if (serverName.empty()) serverName = "n/a";

        std::string label = W2U(WinUtils::TimestampToString(ses->timeStamp())) + "\r\n Server: " + serverName +
            " Files: " + std::to_string(ses->entriesCount());
        res = m_treeView.addEntry(ses, Utf8ToWCstring(label));
        size_t nCount = ses->entriesCount();

        for (int j = 0; j < nCount; j++) {
            nFilesCount++;
            if (ses->entry(j).uploadFileSize < 1000000000) {
                totalFileSize += ses->entry(j).uploadFileSize;
            }
            m_treeView.addSubEntry(res, &ses->entry(j), nCount < 4);
        }
        ses->setDeleteItems(false); // treeView will manage history items
        //m_treeView.ExpandItem(res);
    }

    if (res) {
        m_treeView.SetCurSel(m_treeView.GetCount() - 1);
        m_treeView.SetCurSel(-1);
    }

    m_treeView.SetRedraw(true);

    SetDlgItemInt(IDC_FILESCOUNTLABEL, nFilesCount, false);
    SetDlgItemInt(IDC_SESSIONSCOUNTLABEL, nSessionsCount, false);
    SetDlgItemText(IDC_UPLOADTRAFFICLABEL, Utf8ToWCstring(IuCoreUtils::FileSizeToString(totalFileSize)));
}

LRESULT CHistoryWindow::OnHistoryTreeCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    bHandled = true;
    return 0;
}

LRESULT CHistoryWindow::OnOpenInBrowser(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TreeItem* item = m_treeView.selectedItem();
    if(!item) return 0;
    OpenInBrowser(item);
    return 0;
}

LRESULT CHistoryWindow::OnCopyToClipboard(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TreeItem* item = m_treeView.selectedItem();
    if(!item) return 0;
    const HistoryItem* historyItem = CHistoryTreeControl::getItemData(item);
    if (!historyItem) {
        return 0;
    }
    std::string url = historyItem->directUrl.length()?historyItem->directUrl:historyItem->viewUrl;
    WinUtils::CopyTextToClipboard(Utf8ToWCstring(url));
    return 0;
}

LRESULT CHistoryWindow::OnCopyViewLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    TreeItem* item = m_treeView.selectedItem();
    if (!item) {
        return 0;
    }
    const HistoryItem* historyItem = CHistoryTreeControl::getItemData(item);
    if (!historyItem) {
        return 0;
    }

    if (!historyItem->viewUrl.empty()) {
        WinUtils::CopyTextToClipboard(Utf8ToWCstring(historyItem->viewUrl));
    }
    return 0;
}

LRESULT CHistoryWindow::OnCopyThumbLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    TreeItem* item = m_treeView.selectedItem();
    if (!item) {
        return 0;
    }
    const HistoryItem* historyItem = CHistoryTreeControl::getItemData(item);
    if (!historyItem) {
        return 0;
    }

    if (!historyItem->thumbUrl.empty()) {
        WinUtils::CopyTextToClipboard(Utf8ToWCstring(historyItem->thumbUrl));
    }
    return 0;
}

Uptooda::Core::OutputGenerator::UploadObject fromHistoryItem(const HistoryItem& historyItem)
{
    Uptooda::Core::OutputGenerator::UploadObject it;
    it.uploadResult.directUrl = historyItem.directUrl;
    it.uploadResult.directUrlShortened = historyItem.directUrlShortened;
    it.uploadResult.thumbUrl = historyItem.thumbUrl;
    it.uploadResult.downloadUrl = historyItem.viewUrl;
    it.uploadResult.downloadUrlShortened = historyItem.viewUrlShortened;
    it.uploadResult.editUrl = historyItem.editUrl;
    it.uploadResult.deleteUrl = historyItem.deleteUrl;
    it.displayFileName = historyItem.displayName;
    it.uploadResult.serverName = historyItem.serverName;
    it.fileIndex = historyItem.sortIndex;
    return it;
}

LRESULT CHistoryWindow::OnViewBBCode(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    using namespace Uptooda::Core::OutputGenerator;
    TreeItem* item = m_treeView.selectedItem();
    if (!item) return 0;
    std::vector<UploadObject> items;

    if (item->level() == 0) {
        const auto* ses = static_cast<CHistorySession*>(item->userData());
        for (size_t i = 0; i < ses->entriesCount(); i++) {
            UploadObject it = fromHistoryItem(ses->entry(i));
            items.push_back(it);
        }
    }
    else {
        const HistoryItem* hit = CHistoryTreeControl::getItemData(item);
        if (!hit) {
            return 0;
        }
        Uptooda::Core::OutputGenerator::UploadObject it = fromHistoryItem(*hit);
        items.push_back(it);
    }
    CResultsWindow rp(wizardDlg_, items, false);
    rp.DoModal(m_hWnd);
    return 0;
}

LRESULT CHistoryWindow::OnOpenFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TreeItem* item = m_treeView.selectedItem();
    if(!item) return 0;
    const HistoryItem* historyItem = CHistoryTreeControl::getItemData(item);
    if (!historyItem) {
        return 0;
    }
    std::string fileName  = historyItem->localFilePath;
    if(fileName.empty()) return 0;
    std::string directory = IuCoreUtils::ExtractFilePath(fileName);
    if(IuCoreUtils::FileExists(fileName))
    {
        WinUtils::ShowFileInFolder(U2W(fileName), m_hWnd);
    }
    else if(IuCoreUtils::DirectoryExists(directory))
    {
        WinUtils::ShellOpenFileOrUrl(U2W(directory), m_hWnd);
    }
    return 0;
}

LRESULT CHistoryWindow::OnEditFileOnServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TreeItem* item = m_treeView.selectedItem();
    if (!item) return 0;
    const HistoryItem* historyItem = CHistoryTreeControl::getItemData(item);
    if (!historyItem) {
        return 0;
    }
    WinUtils::ShellOpenFileOrUrl(U2W(historyItem->editUrl), m_hWnd);
    return 0;
}

LRESULT CHistoryWindow::OnDeleteFileOnServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TreeItem* item = m_treeView.selectedItem();
    if (!item) return 0;
    const HistoryItem* historyItem = CHistoryTreeControl::getItemData(item);
    if (!historyItem) {
        return 0;
    }
    if (historyItem->deleteUrl.empty()) {
        return 0;
    }
    std::string message = str(
        boost::format(_("Are you sure you want to delete the file '%s' from the server?"))
        % historyItem->displayName
    );
    if (LocalizedMessageBox(U2W(message), TR("Deleting the file from the server"), MB_ICONQUESTION|MB_YESNO) == IDYES) {
        DesktopUtils::ShellOpenUrl(historyItem->deleteUrl);
    }

    return 0;
}

void CHistoryWindow::LoadHistory()
{
    //m_delayedFileName = fileName;
    if(!m_treeView.isRunning())
    {
        m_treeView.ResetContent();

        auto manager = dynamic_cast<CHistoryManager*>(ServiceLocator::instance()->historyManager());
        assert(manager);
        m_historyReader = std::make_unique<CHistoryReader>(manager);

        SYSTEMTIME dateFrom, dateTo;
        time_t timeFrom = 0, timeTo = 0;
        if (dateFilterCheckbox_.GetCheck() == BST_CHECKED) {
            if (dateFromPicker_.GetSystemTime(&dateFrom) == GDT_VALID) {
                dateFrom.wHour = 0;
                dateFrom.wMinute = 0;
                dateFrom.wSecond = 0;
                dateFrom.wMilliseconds = 0;
                timeFrom = WinUtils::SystemTimeToTime(dateFrom);
            }
            if (dateToPicker_.GetSystemTime(&dateTo) == GDT_VALID) {
                dateTo.wHour = 23;
                dateTo.wMinute = 59;
                dateTo.wSecond = 59;
                dateTo.wMilliseconds = 999;
                timeTo = WinUtils::SystemTimeToTime(dateTo);
            }
        }
        CString fileName = GuiTools::GetDlgItemText(m_hWnd, IDC_FILENAMEEDIT);
        CString url = GuiTools::GetDlgItemText(m_hWnd, IDC_URLEDIT);
        if (m_historyReader->loadFromDB(timeFrom, timeTo, W2U(fileName), W2U(url))) {
            FillList(m_historyReader.get());
        }

        delayedLoad_ = false;
    }
    else
    {
        delayedLoad_ = true;
        ::EnableWindow(GetDlgItem(IDOK), false);
        ::EnableWindow(GetDlgItem(IDCANCEL), false);
        ::EnableWindow(GetDlgItem(IDC_CLEARHISTORYBTN), false);
        ::EnableWindow(GetDlgItem(IDC_CLEARFILTERS), false);
        m_treeView.EnableWindow(false);
        m_treeView.abortLoadingThreads();
    }
}

void CHistoryWindow::OpenInBrowser(const TreeItem* item) const {
    const HistoryItem* historyItem = CHistoryTreeControl::getItemData(item);
    if (!historyItem) {
        return;
    }
    std::string url = historyItem->directUrl.length() ? historyItem->directUrl : historyItem->viewUrl;
    WinUtils::ShellOpenFileOrUrl(U2W(url), m_hWnd);
}

void CHistoryWindow::threadsFinished()
{
    ServiceLocator::instance()->taskRunner()->runInGuiThread([wnd = m_hWnd, this] {
        if (!GuiTools::CheckWindowPointer(wnd, this)) {
            return;
        }
        m_wndAnimation.ShowWindow(SW_HIDE);

        if (delayedLoad_) {
            SendMessage(WM_MY_OPENHISTORYFILE);
        }
        else if(delayedClosing_)
        {
            EndDialog(0);
            return;
        }
        m_treeView.EnableWindow(true);
        ::EnableWindow(GetDlgItem(IDCANCEL), true);
        ::EnableWindow(GetDlgItem(IDOK), true);
        ::EnableWindow(GetDlgItem(IDC_CLEARFILTERS), true);
        ::EnableWindow(GetDlgItem(IDC_CLEARHISTORYBTN), true);
    },
    true);
}

void CHistoryWindow::threadsStarted()
{
    m_wndAnimation.ShowWindow(SW_SHOW);
}

LRESULT CHistoryWindow::OnDownloadThumbsCheckboxChecked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->HistorySettings.EnableDownloading = SendDlgItemMessage(IDC_DOWNLOADTHUMBS, BM_GETCHECK) == BST_CHECKED;
    m_treeView.setDownloadingEnabled(settings->HistorySettings.EnableDownloading);
    return 0;
}

LRESULT CHistoryWindow::OnWmOpenHistoryFile(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LoadHistory();
    return 0;
}

LRESULT CHistoryWindow::OnBnClickedClearHistoryBtn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CClearHistoryDlg dlg;
    if (dlg.DoModal(m_hWnd) == IDOK) {
        if (ServiceLocator::instance()->historyManager()->clearHistory(dlg.GetValue())) {
            SetDlgItemInt(IDC_FILESCOUNTLABEL, 0, false);
            SetDlgItemInt(IDC_SESSIONSCOUNTLABEL, 0, false);
            SetDlgItemText(IDC_UPLOADTRAFFICLABEL, L"0");
            applyFilters();
            LocalizedMessageBox(TR("History has been cleared successfully."), APP_NAME, MB_ICONINFORMATION);
        }
    }
    return 0;
}

void CHistoryWindow::onItemDblClick(TreeItem* item) {
    bool isSessionItem = item->level() == 0;
    if (!isSessionItem) {
        OpenInBrowser(item);
    }
}

void CHistoryWindow::applyFilters() {
    LoadHistory();
}

LRESULT CHistoryWindow::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    applyFilters();
    return 0;
}

LRESULT CHistoryWindow::OnDateFromCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    dateFromCheckboxChanged();
    return 0;
}

void CHistoryWindow::dateFromCheckboxChanged() {
    BOOL isChecked = dateFilterCheckbox_.GetCheck() == BST_CHECKED;
    dateFromPicker_.EnableWindow(isChecked);
    dateToPicker_.EnableWindow(isChecked);
}

LRESULT CHistoryWindow::OnClearFilters(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    dateFilterCheckbox_.SetCheck(BST_UNCHECKED);

    initSearchForm();
    LoadHistory();
    return 0;
}

LRESULT CHistoryWindow::OnHistoryTreeVkeyToItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bool isCtrlPressed = (GetKeyState(VK_CONTROL) & 0x80) != 0;
    bool isShiftPressed = (GetKeyState(VK_SHIFT) & 0x80) != 0;
    bool isMenuPressed = (GetKeyState(VK_MENU) & 0x80) != 0;
    WORD vKey = LOWORD(wParam);
    if (vKey == VK_DELETE && !isCtrlPressed && !isShiftPressed && !isMenuPressed) {
        SendMessage(WM_COMMAND, MAKEWPARAM(ID_DELETEFILEONSERVER, BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
    }
    else if (vKey == _T('C') && isCtrlPressed && !isShiftPressed && !isMenuPressed) {
        SendMessage(WM_COMMAND, MAKEWPARAM(ID_COPYTOCLIPBOARD, BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
    }
    else if (vKey == VK_RETURN && !isCtrlPressed && !isShiftPressed && !isMenuPressed) {
        SendMessage(WM_COMMAND, MAKEWPARAM(ID_OPENINBROWSER, BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
    }
    return -1;
}

void CHistoryWindow::initSearchForm() {
    using namespace boost::gregorian;
    date today = day_clock::local_day();
    date start = today - days(30);

    try {
        SYSTEMTIME dateFrom = GregorianDateToSystemTime(start);
        SYSTEMTIME dateTo = GregorianDateToSystemTime(today);

        dateFromPicker_.SetSystemTime(GDT_VALID, &dateFrom);
        dateToPicker_.SetSystemTime(GDT_VALID, &dateTo);
    } catch (const std::exception& ex) {
        LOG(ERROR) << ex.what();
    }
    SetDlgItemText(IDC_FILENAMEEDIT, _T(""));
    SetDlgItemText(IDC_URLEDIT, _T(""));

    dateFromCheckboxChanged();
}
