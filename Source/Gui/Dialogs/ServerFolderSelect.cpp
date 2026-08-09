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

#include "ServerFolderSelect.h"

#include <algorithm>

#include "Core/Upload/UploadEngineManager.h"
#include "NewFolderDlg.h"
#include "Core/Upload/AdvancedUploadEngine.h"
#include "Func/WinUtils.h"
#include "Core/Network/NetworkClientFactory.h"
#include "Core/Upload/FolderTask.h"
#include "Core/ServiceLocator.h"
#include "Core/Upload/UploadManager.h"
#include "Core/TaskDispatcher.h"

struct TreeItemData
{
    bool isLoadingTempItem = false;
    bool childrenStartedLoading = false;
    bool childrenLoaded = false;
    CFolderItem folder;
};

namespace {
    constexpr int NORMAL_LOAD_SESSION = 0;
    constexpr int INITIAL_LOAD_SESSION = 1;
}

CServerFolderSelect::CServerFolderSelect(ServerProfile& serverProfile, UploadEngineManager * uploadEngineManager) :serverProfile_(serverProfile)
{
    m_UploadEngine = serverProfile_.uploadEngineData();
    uploadEngineManager_ = uploadEngineManager;
    m_FolderOperationType = FolderOperationType::foGetFolders;
    NetworkClientFactory factory;
    m_NetworkClient = factory.create();
    stopSignal = false;
    isRunning_ = false;
}

CServerFolderSelect::~CServerFolderSelect()
{
}

LRESULT CServerFolderSelect::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    GuiTools::SetWindowPointer(m_hWnd, this);
    m_FolderTree = GetDlgItem(IDC_FOLDERTREE);
    m_FolderTree.SetExtendedStyle(TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
    CenterWindow(GetParent());

    DlgResize_Init();

    HWND hWnd = GetDlgItem(IDC_ANIMATIONSTATIC);
    if (hWnd)
    {
        m_wndAnimation.SubclassWindow(hWnd);
        m_wndAnimation.ShowWindow(SW_HIDE);
    }

    // Internalization
    TRC(IDCANCEL, "Cancel");
    TRC(IDOK, "OK");
    TRC(IDC_NEWFOLDERBUTTON, "Create folder");
    SetWindowText(TR("Folder list"));

    // Get color depth (minimum requirement is 32-bits for alpha blended images).
    //int iBitsPixel = GetDeviceCaps(::GetDC(HWND_DESKTOP), BITSPIXEL);
    int iconWidth = ::GetSystemMetrics(SM_CXSMICON);
    int iconHeight = ::GetSystemMetrics(SM_CYSMICON);
    folderTreeViewImageList.Create(iconWidth, iconHeight, ILC_COLOR32, 0, 6);

    CIcon iconFolder;
    iconFolder.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONFOLDER2), iconWidth, iconHeight);
    folderTreeViewImageList.AddIcon(iconFolder);

    m_FolderTree.SetImageList(folderTreeViewImageList);
    m_FolderMap[""] = nullptr;

    m_FolderOperationType = FolderOperationType::foGetFolders;
    auto engineList = ServiceLocator::instance()->engineList();
    auto uploadScript = std::dynamic_pointer_cast<CAdvancedUploadEngine>(uploadEngineManager_->getUploadEngine(serverProfile_));

    if (!uploadScript)
    {
        SetDlgItemText(IDC_FOLDERLISTLABEL, TR("An error occured while loading script."));
        return 0;
    }
    CString title;
    title.Format(TR("Folder list on server %s for account '%s':"), U2W(engineList->getServerDisplayName(m_UploadEngine)).GetString(),
                 U2W(serverProfile_.profileName()).GetString());
    SetDlgItemText(IDC_FOLDERLISTLABEL, title);

    uploadScript->setNetworkClient(m_NetworkClient.get());
    uploadScript->getAccessTypeList(m_accessTypeList);

    loadInitialTree();

    m_FolderTree.SetFocus();
    return FALSE;
}

LRESULT CServerFolderSelect::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    GuiTools::ClearWindowPointer(m_hWnd);
    return 0;
}

LRESULT CServerFolderSelect::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (isRunning_) {
        return false;
    }
    HTREEITEM item = m_FolderTree.GetSelectedItem();

    if (!item)
        return 0;

    auto* tid = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(item));

    if (!tid || tid->isLoadingTempItem) {
        return 0;
    }

    if (!::IsWindowEnabled(GetDlgItem(IDOK)))
        return 0;

    m_SelectedFolder = tid->folder;

    // We save the path to the node (consisting of folder IDs) so that when opening
    // the dialog box we can expand the list and select the desired element.
    m_SelectedFolder.parentIds.clear();
    HTREEITEM parentItem = item;
    while ((parentItem = m_FolderTree.GetParentItem(parentItem))) {
        auto* tid2 = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(parentItem));
        m_SelectedFolder.parentIds.emplace_back(tid2->folder.getId());
    }
    std::reverse(m_SelectedFolder.parentIds.begin(), m_SelectedFolder.parentIds.end());

    EndDialog(wID);

    return 0;
}

LRESULT CServerFolderSelect::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    {
        std::lock_guard<std::mutex> guard(runningScriptMutex_);
        if (isRunning_ && uploadSession_) {
            auto* uploadManager = ServiceLocator::instance()->uploadManager();
            uploadManager->stopSession(uploadSession_.get());
            stopSignal = true;
        } else {
            EndDialog(wID);
        }
    }
    return 0;
}

void CServerFolderSelect::onTaskFinished(UploadTask* task, bool success)
{
    auto* folderTask = dynamic_cast<FolderTask*>(task);
    if (!folderTask) {
        return;
    }

    if (folderTask->operationType() == FolderOperationType::foGetFolders) {
        ServiceLocator::instance()->taskRunner()->runInGuiThread([this, wnd = m_hWnd, success, folderTask]() {
            if (!GuiTools::CheckWindowPointer(wnd, this)) {
                return;
            }
            getListTaskFinished(folderTask, success);
        });
    } else if (folderTask->operationType() == FolderOperationType::foCreateFolder) {
        m_FolderOperationType = FolderOperationType::foGetFolders;
        m_SelectedFolder = folderTask->folder();
        refreshList(folderTask->folder().parentId);
        return;
    } else if (m_FolderOperationType == FolderOperationType::foModifyFolder) {
        // Modifying an existing folder
        m_FolderOperationType = FolderOperationType::foGetFolders;
        m_newFolder.setId(folderTask->folder().getId());
        m_SelectedFolder = m_newFolder;
        refreshList(folderTask->folder().parentId);
        return;
    }

    OnLoadFinished();
}

void CServerFolderSelect::getListTaskFinished(FolderTask* folderTask, bool success) {
    std::string parentFolderId = folderTask->folderList().parentFolder().getId();

    HTREEITEM treeViewItem{};
    try {
        treeViewItem = m_FolderMap[parentFolderId];
    }
    catch (const std::exception& ex) {
        LOG(ERROR) << ex.what();
    }
    auto* tid = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(treeViewItem));
    if (parentFolderId.empty()) {
        m_FolderTree.DeleteAllItems();
        m_FolderMap.clear();
        tid = nullptr;
    } else {
        HTREEITEM item = m_FolderTree.GetChildItem(treeViewItem);
        auto* tid2 = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(item));
        if (tid2 && tid2->isLoadingTempItem) {
            m_FolderTree.DeleteItem(item);
        }
    }
    if (tid) {
        if (tid->childrenLoaded) {
            return;
        }
        tid->childrenLoaded = true;
    }

    if (!success) {
        TRC(IDC_FOLDERLISTLABEL, "Failed to load folder list.");
        return;
    }

    if (folderTask->folderList().GetCount() == 0) {
        //m_FolderTree.Expand(treeViewItem, TVE_COLLAPSERESET);
        // Tell treeview that the node has no children
        TVITEM modifiedItem = {};
        modifiedItem.hItem = treeViewItem;
        modifiedItem.mask = TVIF_CHILDREN;
        modifiedItem.cChildren = 0;
        m_FolderTree.SetItem(&modifiedItem);
    }
    BuildFolderTree(tid, folderTask->folderList().m_folderItems, parentFolderId);

    m_FolderTree.SelectItem(m_FolderMap[m_SelectedFolder.id]);
}

void CServerFolderSelect::OnLoadFinished()
{

}

void CServerFolderSelect::NewFolder(const CFolderItem& parentFolder)
{
    CFolderItem newFolder;
    CNewFolderDlg dlg(newFolder, true, m_accessTypeList);
    if (dlg.DoModal(m_hWnd) == IDOK)
    {
        m_newFolder = newFolder;
        m_newFolder.parentId = parentFolder.getId();
        m_FolderOperationType = FolderOperationType::foCreateFolder;
        if (!isRunning_){
            auto task = std::make_shared<FolderTask>(FolderOperationType::foCreateFolder);
            task->setServerProfile(serverProfile_);
            task->setFolder(m_newFolder);
            using namespace std::placeholders;
            task->addTaskFinishedCallback([this](auto && PH1, auto && PH2) { onTaskFinished(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
            isRunning_ = true;
            UploadManager* uploadManager = ServiceLocator::instance()->uploadManager();
            currentTask_ = task;
            uploadSession_ = std::make_shared<UploadSession>();
            uploadSession_->addSessionFinishedCallback([this](auto && PH1) { onSessionFinished(std::forward<decltype(PH1)>(PH1)); });
            uploadSession_->addTask(task);
            ++sessionsRunning_;
            BlockWindow(true);
            uploadManager->addSession(uploadSession_);
        }
    }
}

LRESULT CServerFolderSelect::OnBnClickedNewfolderbutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    NewFolder({});
    return 0;
}

LRESULT CServerFolderSelect::OnFolderlistLbnDblclk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    /*HTREEITEM item = m_FolderTree.GetSelectedItem();
    if (item) {
        SendDlgItemMessage(IDOK, BM_CLICK);
    }*/
    return 0;
}

LRESULT CServerFolderSelect::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT screenPoint{};
    HWND hwnd = reinterpret_cast<HWND>(wParam);
    if (hwnd != m_FolderTree.m_hWnd) {
        return 0;
    }

    HTREEITEM selectedItem{};
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);

    if (xPos == -1 && yPos == -1) {
        // If the context menu is generated from the keyboard, the application should display
        // the context menu at the location of the current selection rather than at (xPos, yPos).
        CRect rc;
        HTREEITEM firstSelectedItem = m_FolderTree.GetNextSelectedItem(nullptr);
        if (firstSelectedItem) {
            if (m_FolderTree.GetItemRect(firstSelectedItem, &rc, FALSE) ) {
                m_FolderTree.MapWindowPoints(nullptr, &rc);
                screenPoint = { rc.left, rc.bottom };
                selectedItem = firstSelectedItem;
            }
        }
    } else {
        screenPoint.x = xPos;
        screenPoint.y = yPos;
        POINT clientPoint = screenPoint;
        ::ScreenToClient(hwnd, &clientPoint);

        UINT flags{};
        HTREEITEM testItem = m_FolderTree.HitTest(clientPoint, &flags);
        if (testItem && flags & TVHT_ONITEM) {
            selectedItem = testItem;
        }
    }

    if (!selectedItem) {
        return 0;
    }

    m_FolderTree.SelectItem(selectedItem);

    auto* tid = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(selectedItem));
    bool showViewInBrowserItem = false;
    BOOL copyFolderIdFlag = MFS_DISABLED;
    if (tid) {
        const CFolderItem& folder = tid->folder;
        if (!folder.getViewUrl().empty()) {
            showViewInBrowserItem = true;
        }
        if (!folder.getId().empty() && folder.getId() != CFolderItem::NewFolderMark) {
            copyFolderIdFlag = MFS_ENABLED;
        }
    }

    CMenu sub;
    MENUITEMINFO mi {};
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_TYPE | MIIM_ID;
    mi.fType = MFT_STRING;
    sub.CreatePopupMenu();

    mi.wID = ID_EDITFOLDER;
    CString editStr = TR("Edit");
    mi.dwTypeData = const_cast<LPWSTR>(editStr.GetString());
    mi.cch = editStr.GetLength();
    sub.InsertMenuItem(1, true, &mi);

    if (showViewInBrowserItem) {
        mi.wID = ID_OPENINBROWSER;
        CString openInBrowserStr = TR("Open in Web Browser");
        mi.dwTypeData = const_cast<LPWSTR>(openInBrowserStr.GetString());
        mi.cch = openInBrowserStr.GetLength();
        sub.InsertMenuItem(0, true, &mi);
    }

    mi.wID = ID_CREATENESTEDFOLDER;
    CString createNestedFolderStr = TR("Create nested folder");
    mi.dwTypeData = const_cast<LPWSTR>(createNestedFolderStr.GetString());
    mi.cch = createNestedFolderStr.GetLength();
    sub.InsertMenuItem(2, true, &mi);

    sub.AppendMenu(MFT_STRING | copyFolderIdFlag, ID_COPYFOLDERID, TR("Copy folder's ID"));

    sub.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, screenPoint.x, screenPoint.y, m_hWnd);
    return 0;
}

LRESULT CServerFolderSelect::OnEditFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HTREEITEM item = m_FolderTree.GetSelectedItem();

    if (!item)
        return 0;
    auto* tid = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(item));

    CFolderItem& folder = tid->folder;

    CNewFolderDlg dlg(folder, false, m_accessTypeList);
    if (dlg.DoModal(m_hWnd) == IDOK)
    {
        m_FolderOperationType = FolderOperationType::foModifyFolder;  // Editing an existing folder
        m_newFolder = folder;
        if (!isRunning_)
        {
            auto task = std::make_shared<FolderTask>(FolderOperationType::foModifyFolder);
            task->setServerProfile(serverProfile_);
            task->setFolder(folder);
            using namespace std::placeholders;
            task->addTaskFinishedCallback([this](auto && PH1, auto && PH2) { onTaskFinished(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
            isRunning_ = true;
            UploadManager* uploadManager = ServiceLocator::instance()->uploadManager();
            currentTask_ = task;
            uploadSession_ = std::make_shared<UploadSession>();
            uploadSession_->addSessionFinishedCallback([this](auto && PH1) { onSessionFinished(std::forward<decltype(PH1)>(PH1)); });
            uploadSession_->addTask(task);
            ++sessionsRunning_;
            BlockWindow(true);
            uploadManager->addSession(uploadSession_);
        }
    }
    return 0;
}

void CServerFolderSelect::BlockWindow(bool Block)
{
    m_wndAnimation.ShowWindow(Block ? SW_SHOW : SW_HIDE);
    ::EnableWindow(GetDlgItem(IDOK), !Block);
    //::EnableWindow(GetDlgItem(IDCANCEL), !Block);
    ::EnableWindow(GetDlgItem(IDC_NEWFOLDERBUTTON), !Block);
    m_FolderTree.EnableWindow(!Block);
    m_FolderTree.SetFocus();
}

LRESULT CServerFolderSelect::OnOpenInBrowser(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HTREEITEM item = m_FolderTree.GetSelectedItem();

    if (!item)
        return 0;
    auto* tid = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(item));

    const CFolderItem& folder = tid->folder;

    CString str = U2W(folder.viewUrl);
    if (!str.IsEmpty())
    {
        WinUtils::ShellOpenFileOrUrl(str, m_hWnd);
    }
    return 0;
}

LRESULT CServerFolderSelect::OnCopyFolderId(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    HTREEITEM item = m_FolderTree.GetSelectedItem();

    if (!item)
        return 0;
    auto* tid = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(item));

    if (!tid) {
        return 0;
    }
    const CFolderItem& folder = tid->folder;

    if (!folder.getId().empty() && folder.getId() != CFolderItem::NewFolderMark) {
        CString str = U2W(folder.getId());

        WinUtils::CopyTextToClipboard(str);
    }
    return 0;
}

LRESULT CServerFolderSelect::OnCreateNestedFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HTREEITEM item = m_FolderTree.GetSelectedItem();

    if (!item)
        return 0;
    auto* tid = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(item));
    if (!tid) {
        return 0;
    }

    NewFolder(tid->folder);
    return 0;
}

void CServerFolderSelect::BuildFolderTree(TreeItemData* treeItemData, const std::vector<CFolderItem>& list, const std::string& parentFolderId)
{
    for (size_t i = 0; i < list.size(); i++)
    {
        const CFolderItem& cur = list[i];
        if (cur.parentId == parentFolderId)
        {
            if (treeItemData) {
                treeItemData->childrenLoaded = true;
            }
            CString title = Utf8ToWCstring(cur.title);
            /*if (cur.itemCount != -1)
                title += _T(" (") + WinUtils::IntToStr(cur.itemCount) + _T(")");*/
            TVINSERTSTRUCT tvis = {};
            tvis.hParent = m_FolderMap[cur.parentId];
            tvis.hInsertAfter = TVI_SORT;
            tvis.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;
            tvis.item.pszText = const_cast<LPTSTR>(title.GetString());
            tvis.item.iImage = 0;
            tvis.item.iSelectedImage = 0;
            tvis.item.state = 0;
            tvis.item.stateMask = 0;
            tvis.item.lParam = 0;
            if (cur.itemCount == CFolderItem::icNoChildren) {
                tvis.item.cChildren = 0;
            } else if (cur.itemCount == CFolderItem::icUnknown || cur.itemCount > 0) {
                tvis.item.cChildren = 1;
            }
            HTREEITEM res = m_FolderTree.InsertItem(&tvis);
            if (!res) {
                LOG(ERROR) << "Cannot insert tree node '" << title << "'";
                continue;
            }
            TreeItemData* tid = new TreeItemData;
            tid->folder = cur;
            m_FolderTree.SetItemData(res, reinterpret_cast<DWORD_PTR>(tid));

            m_FolderMap[cur.id] = res;
            if (!cur.id.empty()) {
                BuildFolderTree(tid, list, cur.id);
            }
        }
    }
}


void CServerFolderSelect::refreshList(const std::string& parentFolderId) {
    HTREEITEM treeItem = m_FolderMap[parentFolderId];
    if (treeItem) {
        m_FolderTree.Expand(treeItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
        auto* tid = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(treeItem));
        tid->childrenLoaded = false;
        tid->childrenStartedLoading = false;
    } else {
        m_FolderTree.DeleteAllItems();
        m_FolderMap.clear();
    }
    auto task = std::make_shared<FolderTask>(FolderOperationType::foGetFolders);
    task->setServerProfile(serverProfile_);
    CFolderItem parentFolder;
    parentFolder.setId(parentFolderId);
    task->folderList().setParentFolder(parentFolder);
    currentTask_ = task;
    using namespace std::placeholders;
    task->addTaskFinishedCallback([this](auto && PH1, auto && PH2) { onTaskFinished(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });

    isRunning_ = true;
    uploadSession_ = std::make_shared<UploadSession>();
    uploadSession_->addSessionFinishedCallback([this](auto && PH1) { onSessionFinished(std::forward<decltype(PH1)>(PH1)); });
    uploadSession_->addTask(task);
    BlockWindow(true);
    UploadManager* uploadManager = ServiceLocator::instance()->uploadManager();
    uploadManager->addSession(uploadSession_);
}

LRESULT CServerFolderSelect::OnFolderTreeItemExpanding(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    if (isRunning_) {
        return 0;
    }
    auto pnmtv = reinterpret_cast<LPNMTREEVIEWW>(pnmh);
    if (pnmtv->action == TVE_EXPAND) {
        auto* tid = reinterpret_cast<TreeItemData*>(m_FolderTree.GetItemData(pnmtv->itemNew.hItem));
        if (tid && (tid->childrenLoaded || tid->childrenStartedLoading)) {
            return 0;
        }
        TVINSERTSTRUCT tvis = {};
        tvis.hParent = pnmtv->itemNew.hItem;
        tvis.hInsertAfter = TVI_SORT;
        tvis.item.mask =  TVIF_TEXT | TVIF_IMAGE;
        CString text = TR("Loading...");
        tvis.item.pszText = const_cast<LPTSTR>(text.GetString());
        tvis.item.iImage = -1;
        tvis.item.iSelectedImage = -1;
        tvis.item.state = 0;
        tvis.item.stateMask = 0;
        tvis.item.lParam = 0;
        tvis.item.cChildren = 1;
        HTREEITEM res = m_FolderTree.InsertItem(&tvis);
        auto* newData = new TreeItemData;
        newData->isLoadingTempItem = true;
        m_FolderTree.SetItemData(res, reinterpret_cast<DWORD_PTR>(newData));

        if (isRunning_) {
            return 0;
        }
        auto result = std::find_if(
            m_FolderMap.begin(),
            m_FolderMap.end(),
            [&](const auto& mo) {return mo.second == pnmtv->itemNew.hItem; });

        if (result != m_FolderMap.end()) {
            std::string foundkey = result->first;
            CFolderItem parent;
            parent.setId(foundkey);
            m_SelectedFolder = parent;
            auto task = std::make_shared<FolderTask>(FolderOperationType::foGetFolders);
            task->folderList().setParentFolder(parent);
            task->setServerProfile(serverProfile_);
            //task->setFolder(parent);
            currentTask_ = task;
            using namespace std::placeholders;
            task->addTaskFinishedCallback([this](auto && PH1, auto && PH2) { onTaskFinished(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });

            isRunning_ = true;
            uploadSession_ = std::make_shared<UploadSession>();
            uploadSession_->addTask(task);
            uploadSession_->addSessionFinishedCallback([this](auto && PH1) { onSessionFinished(std::forward<decltype(PH1)>(PH1)); });
            tid->childrenStartedLoading = true;
            BlockWindow(true);
            UploadManager * uploadManager = ServiceLocator::instance()->uploadManager();
            ++sessionsRunning_;
            uploadManager->addSession(uploadSession_);
        }
    }

    return 0;
}

LRESULT CServerFolderSelect::OnFolderTreeDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto pnmtv = reinterpret_cast<LPNMTREEVIEW>(pnmh);
    if (pnmtv->itemOld.lParam) {
        auto* p = reinterpret_cast<TreeItemData*>(pnmtv->itemOld.lParam);
        delete p;
    }
    return 0;
}

void CServerFolderSelect::onSessionFinished(UploadSession* session) {
    if (session->userData() == reinterpret_cast<void*>(static_cast<INT_PTR>(INITIAL_LOAD_SESSION))) {
        //m_SelectedFolder.id =
        for (int i = 0; i < session->taskCount(); i++) {
            auto task = session->getTask(i);
            if (task->uploadSuccess()) {
                auto* folderTask = dynamic_cast<FolderTask*>(task.get());
                if (folderTask) {
                    ServiceLocator::instance()->taskRunner()->runInGuiThread([this, wnd = m_hWnd, folderTask]() {
                        if (!GuiTools::CheckWindowPointer(wnd, this)) {
                            return;
                        }
                        getListTaskFinished(folderTask, true);
                    });
                }
            } else {
                break;
            }
        }
    }

    if (--sessionsRunning_  < 1) {
        stopSignal = false;
        isRunning_ = false;
        sessionsRunning_ = 0;
        ServiceLocator::instance()->taskRunner()->runInGuiThread([this, wnd = m_hWnd] {
            if (!GuiTools::CheckWindowPointer(wnd, this)) {
                return;
            }
            BlockWindow(false);
        });
    }
}

void CServerFolderSelect::onInitialLoadTaskFinished(UploadTask* task, bool success) {

}

void CServerFolderSelect::loadInitialTree() {
    if (isRunning_) {
        return;
    }
    using namespace std::placeholders;
    CFolderItem folder = m_SelectedFolder;

    uploadSession_ = std::make_shared<UploadSession>();
    uploadSession_->addSessionFinishedCallback([this](auto && PH1) { onSessionFinished(std::forward<decltype(PH1)>(PH1)); });
    std::vector<std::string> parentIds = folder.parentIds;
    parentIds.insert(parentIds.begin(), "");

    for (const auto& folderId: parentIds) {
        CFolderItem cur;
        cur.setId(folderId);
        auto task = std::make_shared<FolderTask>(FolderOperationType::foGetFolders);
        task->folderList().setParentFolder(cur);
        task->setServerProfile(serverProfile_);
        task->addTaskFinishedCallback([this](auto && PH1, auto && PH2) { onInitialLoadTaskFinished(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
        uploadSession_->addTask(task);
    }

    if (!uploadSession_->taskCount()) {
        return;
    }
    uploadSession_->setUserData(reinterpret_cast<void*>(static_cast<INT_PTR>(INITIAL_LOAD_SESSION)));
    isRunning_ = true;
    ++sessionsRunning_;
    BlockWindow(true);
    UploadManager* uploadManager = ServiceLocator::instance()->uploadManager();
    uploadManager->addSession(uploadSession_);
}
