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

#include "MainDlg.h"


#include <ComDef.h>
#include <boost/format.hpp>
#include <shlobj.h>

#include "Core/Utils/StringUtils.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Func/CmdLine.h"
#include "Func/ClipboardUtils.h"
#include "Func/WinUtils.h"
#include "Func/MyUtils.h"
#include "ImageEditor/Gui/ImageEditorWindow.h"
#include "Func/ImageEditorConfigurationProvider.h"
#include "Gui/Components/NewStyleFolderDialog.h"
#include "Gui/Dialogs/WizardDlg.h"
#include "Gui/Dialogs/SearchByImageDlg.h"
#include "Func/IuCommonFunctions.h"
#include "Core/SearchByImage.h"
#include "3rdpart/ShellPidl.h"
#include "Gui/Components/MyFileDialog.h"
#include "Core/WinServerIconCache.h"
#ifdef IU_ENABLE_MEDIAINFO
    #include "MediaInfoDlg.h"
#endif
#include "Gui/Helpers/DPIHelper.h"

CMainDlg::CMainDlg(WinServerIconCache* iconCache):
    m_EditorProcess(nullptr),
    listChanged_(false),
    callbackLastCallTime_(0),
    callbackLastCallType_(false),
    hotkeys_(nullptr),
    iconCache_(iconCache)
{

}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    PageWnd = m_hWnd;
    GuiTools::SetWindowPointer(m_hWnd, this);

    // register object for message filtering
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);

    listChanged_ = false;
    callbackLastCallType_ = false;
    SetTimer(kStatusTimer, 500);
    TRC(IDC_ADDIMAGES, "Add Files");
    TRC(IDC_ADDVIDEO, "Import Video File");
    TRC(IDC_SCREENSHOT, "Screenshot");
    TRC(IDC_PROPERTIES, "Properties");
    TRC(IDC_DELETE, "Remove from list");

    ThumbsView.SubclassWindow(GetDlgItem(IDC_FILELIST));
    ThumbsView.Init(true);
    callbackLastCallTime_ = 0;
    ThumbsView.SetOnItemCountChanged([&](CThumbsView*, bool selected) {
        DWORD curTime = ::GetTickCount();

        listChanged_ = true;
        if (callbackLastCallType_ == selected && curTime - callbackLastCallTime_ < 150) {
            return;
        }
        callbackLastCallTime_ = curTime;
        callbackLastCallType_ = selected;
        UpdateStatusLabel();
    });

    UpdateStatusLabel();

    ACCEL accels[] = {
        { FCONTROL | FSHIFT | FVIRTKEY, static_cast<WORD>(VkKeyScan('c')), MENUITEM_COPYFILEPATH},
        { FALT | FVIRTKEY, VK_RETURN, MENUITEM_PROPERTIES},
        { FVIRTKEY, VK_F2, MENUITEM_RENAME },
    };

    hotkeys_.CreateAcceleratorTable(accels, std::size(accels));

    WaitThreadStop.Create();
    WaitThreadStop.ResetEvent();
    return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    GuiTools::ClearWindowPointer(m_hWnd);
    bHandled = FALSE;
    WaitThreadStop.Close();
    return 0;
}

LRESULT CMainDlg::OnMyDpiChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    ThumbsView.PostMessage(WM_MY_DPICHANGED, wParam, lParam);
    return 0;
}

LRESULT CMainDlg::OnBnClickedAddvideo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    WizardDlg->executeFunc(_T("importvideo"));
    return 0;
}

LRESULT CMainDlg::OnBnClickedAddimages(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    WizardDlg->executeFunc(_T("addimages"));
    return 0;
}

LRESULT CMainDlg::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    auto settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    HWND hwnd = reinterpret_cast<HWND>(wParam);
    POINT ClientPoint, ScreenPoint;
    if(hwnd != GetDlgItem(IDC_FILELIST)) return 0;

    if(lParam == -1)
    {
        int nCurItem = ThumbsView.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
        if (nCurItem >= 0) {
            CRect rc;
            ThumbsView.GetItemRect(nCurItem, &rc, LVIR_ICON);
            ClientPoint = rc.CenterPoint();
        } else {
            ClientPoint.x = 0;
            ClientPoint.y = 0;
        }
        ScreenPoint = ClientPoint;
        ::ClientToScreen(hwnd, &ScreenPoint);
    }
    else
    {
        ScreenPoint.x = GET_X_LPARAM(lParam);
        ScreenPoint.y = GET_Y_LPARAM(lParam);
        ClientPoint = ScreenPoint;
        ::ScreenToClient(hwnd, &ClientPoint);
    }

    LV_HITTESTINFO hti;
    hti.pt = ClientPoint;
    ThumbsView.HitTest(&hti);

    if(hti.iItem < 0) { // no item selected
        CMenu contextMenu;
        contextMenu.CreatePopupMenu();
        contextMenu.AppendMenu(MF_STRING, MENUITEM_ADDIMAGES, TR("Add images..."));
        contextMenu.AppendMenu(MF_STRING, MENUITEM_ADDFILES, TR("Add files..."));
        contextMenu.AppendMenu(MF_STRING, MENUITEM_ADDFOLDER, TR("Add folder..."));

        CString pasteMenuItemTitle(TR("Paste"));
        pasteMenuItemTitle += _T("\t");
        pasteMenuItemTitle += settings->Hotkeys.getByFunc("paste").localKey.toString();
        contextMenu.AppendMenu(MF_STRING, MENUITEM_PASTE, pasteMenuItemTitle);
        contextMenu.EnableMenuItem(MENUITEM_PASTE, WizardDlg->IsClipboardDataAvailable() ? MF_ENABLED : MF_GRAYED);

        contextMenu.AppendMenu(MF_SEPARATOR);
        contextMenu.AppendMenu(MF_STRING, MENUITEM_DELETEALL, TR("Remove all"));
        contextMenu.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON, ScreenPoint.x, ScreenPoint.y, m_hWnd);
    }
    else
    {
        const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
        auto* engineList = ServiceLocator::instance()->engineList();

        CString singleSelectedItem;
        bool isImage = false;
        if ( ThumbsView.GetSelectedCount() == 1 ) {
            singleSelectedItem = getSelectedFileName();
            isImage = IuCommonFunctions::IsImage(singleSelectedItem);
        }
        CString fileName = FileList[hti.iItem].FileName;
        bool isImageFile = IuCommonFunctions::IsImage(fileName);
        bool isVideoFile = IsVideoFile(fileName);
        bool isAudioFile = IsAudioFile(fileName);

        CMenu contextMenu;
        contextMenu.CreatePopupMenu();
        if (isImageFile && !singleSelectedItem.IsEmpty()) {
            contextMenu.AppendMenu(MF_STRING, MENUITEM_VIEW, TR("View"));
            contextMenu.SetMenuDefaultItem(MENUITEM_VIEW, FALSE);
        }
        contextMenu.AppendMenu(MF_STRING, MENUITEM_OPENINDEFAULTVIEWER, TR("Show in default viewer"));
        if (isVideoFile) {
            contextMenu.SetMenuDefaultItem(MENUITEM_OPENINDEFAULTVIEWER, FALSE);
        }
        contextMenu.AppendMenu(MF_STRING, MENUITEM_OPENWITH, TR("Open with..."));

        if (isImageFile && !singleSelectedItem.IsEmpty()) {
            contextMenu.AppendMenu(MF_STRING, MENUITEM_EDIT, TR("Edit"));
            contextMenu.AppendMenu(MF_STRING, MENUITEM_EDITINEXTERNALEDITOR, TR("Open in external editor"));
            contextMenu.AppendMenu(MF_STRING, MENUITEM_PRINT, TR("Print..."));
        }

        if (isVideoFile) {
            contextMenu.AppendMenu(MF_STRING, MENUITEM_EXTRACTFRAMES, TR("Extract frames"));
        }
#ifdef IU_ENABLE_MEDIAINFO
        if (isVideoFile || isAudioFile) {
            contextMenu.AppendMenu(MF_STRING, MENUITEM_FILEINFO, TR("Information about file"));
        }
#endif
        contextMenu.AppendMenu(MF_STRING, MENUITEM_OPENINFOLDER, TR("Open in folder"));
        contextMenu.AppendMenu(MF_STRING, MENUITEM_SAVEAS, TR("Save as..."));

        CString menuItemTitle = (isImage ? TR("Copy image") : TR("Copy")) + CString(_T("\tCtrl+C"));
        contextMenu.AppendMenu(MF_STRING, MENUITEM_COPYFILETOCLIPBOARD, menuItemTitle);

        menuItemTitle = TR("Copy full path") + CString(_T("\tCtrl+Shift+C"));
        contextMenu.AppendMenu(MF_STRING, MENUITEM_COPYFILEPATH, menuItemTitle);

        if (isImageFile && !singleSelectedItem.IsEmpty()) {
            CMenu subMenu;
            subMenu.CreatePopupMenu();
            subMenu.AppendMenu(MFT_STRING, MENUITEM_COPYFILEASDATAURI, _T("data:URI"));
            subMenu.AppendMenu(MFT_STRING, MENUITEM_COPYFILEASDATAURIHTML, _T("data:URI (HTML)"));

            contextMenu.AppendMenu(0, subMenu.Detach(), TR("Copy &as"));

            CMenu searchSubMenu;
            searchSubMenu.CreatePopupMenu();

            int i = 0;

            for (const auto& engine : *engineList) {
                if (engine->hasType(CUploadEngineData::TypeSearchByImageServer)) {
                    CString itemText = U2W(engine->Name);
                    MENUITEMINFO mi;
                    ZeroMemory(&mi, sizeof(mi));
                    mi.cbSize = sizeof(mi);
                    mi.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
                    mi.fType = MFT_STRING;
                    mi.wID = MENUITEM_SEARCHBYIMG_START + i;
                    mi.dwTypeData = const_cast<LPWSTR>(itemText.GetString());
                    mi.cch = itemText.GetLength();
                    mi.hbmpItem = iconCache_->getIconBitmapForServer(engine->Name, dpi, true);

                    if (mi.hbmpItem) {
                        mi.fMask |= MIIM_BITMAP;
                    }
                    searchSubMenu.InsertMenuItem(i, true, &mi);
                    //searchSubMenu.AppendMenu(MF_STRING, , itemText,);
                    i++;
                }
            }
            contextMenu.AppendMenu(0, searchSubMenu.Detach(), TR("Search by image") );
        }

        contextMenu.AppendMenu(MF_STRING, MENUITEM_DELETE, TR("Remove") + CString("\tDel"));
        contextMenu.AppendMenu(MF_STRING, MENUITEM_PROPERTIES, TR("Properties")+CString(_T("\tAlt+Enter")));

        contextMenu.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON, ScreenPoint.x, ScreenPoint.y, m_hWnd);
    }
    return 0;
}

LRESULT CMainDlg::OnEnableDropTarget(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
    SendMessage(GetParent(), MYWM_ENABLEDROPTARGET, wParam, lParam);
    return 0;
}

bool CMainDlg::AddToFileList(LPCTSTR FileName, const CString& virtualFileName, bool ensureVisible, Gdiplus::Image* Img,
                             bool selectItem) {
    CFileListItem fl; //internal list item

    if (!FileName) return FALSE;

    //if(!WinUtils::FileExists(FileName)) return FALSE;
    fl.selected = false;

    fl.FileName = FileName;

    if (virtualFileName.IsEmpty())
        fl.VirtualFileName = WinUtils::DoExtractFileName(FileName);
    else
        fl.VirtualFileName = virtualFileName;

    FileList.Add(fl);

    int itemIndex = ThumbsView.AddImage(fl.FileName, fl.VirtualFileName, ensureVisible, Img);

    EnableNext(FileList.GetCount() > 0);
    if (selectItem && itemIndex != -1) {
        ThumbsView.SelectItem(itemIndex);
    }
    listChanged_ = true;
    return TRUE;
}

LRESULT CMainDlg::OnDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ThumbsView.DeleteSelected();
    return 0;
}

LRESULT CMainDlg::OnEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int nCurItem;

    if ((nCurItem=ThumbsView.GetNextItem(-1, LVNI_ALL|LVNI_SELECTED))<0)
        return FALSE;

    LPCTSTR FileName = ThumbsView.GetFileName(nCurItem);
    if(!FileName) return FALSE;
    using namespace ImageEditor;
    ImageEditorConfigurationProvider configProvider;
    ImageEditor::ImageEditorWindow imageEditor(FileName, &configProvider);
    imageEditor.showUploadButton(false);
    imageEditor.showAddToWizardButton(false);

    auto settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    imageEditor.DoModal(WizardDlg->m_hWnd, nullptr, settings->ImageEditorSettings.AllowEditingInFullscreen ? ImageEditorWindow::wdmAuto :ImageEditorWindow::wdmWindowed);

    ThumbsView.OutDateThumb(nCurItem);

    return 0;
}

LRESULT CMainDlg::OnBnClickedScreenshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    WizardDlg->executeFunc(_T("screenshotdlg"));
    return 0;
}

bool CMainDlg::OnShow()
{
    ShowPrev();
    ShowNext();
    EnablePrev();
    EnableExit();
    EnableNext(FileList.GetCount()>0);
    ThumbsView.SetFocus();
//    ThumbsView.LoadThumbnails();
    return true;
}

LRESULT CMainDlg::OnLvnItemDelete(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& bHandled)
{
    NM_LISTVIEW * pnmv = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);
    if(!pnmv) return 0;

    FileList.RemoveAt(pnmv->iItem);

    EnableNext(FileList.GetCount()>0);
    bHandled = false;
    //UpdateStatusLabel();
    return 0;
}

LRESULT CMainDlg::OnBnClickedDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ThumbsView.DeleteSelected();
    UpdateStatusLabel();
    return 0;
}

bool CMainDlg::OnHide()
{
    //ThumbsView.StopBackgroundThread();
    if(IsRunning())
    {
        WaitThreadStop.SetEvent(); // Sending stop message destinated for child thread
        WaitForThread(3500);
    }
    return false;
}

BOOL CMainDlg::FileProp(){
    CComPtr<IShellFolder> desktop; // namespace root for parsing the path
    HRESULT hr = SHGetDesktopFolder(&desktop);

    if (!SUCCEEDED(hr)) {
        return false;
    }

    int nCurItem = -1;

    std::vector<LPITEMIDLIST> list;
    while ((nCurItem = ThumbsView.GetNextItem(nCurItem, LVNI_ALL | LVNI_SELECTED)) >= 0) {
        LPCTSTR FileName = ThumbsView.GetFileName(nCurItem);
        if (!FileName) {
            continue;
        }

        PIDLIST_RELATIVE newPIdL;
        hr = desktop->ParseDisplayName(m_hWnd, 0, const_cast<LPWSTR>(FileName), 0, &newPIdL, 0);
        if (SUCCEEDED(hr)) {
            list.push_back(newPIdL);
        }
    }

    if (!list.empty()) {
        CShellPidl::MultiFileProperties(m_hWnd, desktop, const_cast<LPCITEMIDLIST*>(&list[0]), list.size());

        for (auto& pidl : list) {
            SHFree(pidl);
        }
    }
    return TRUE;
}

LRESULT CMainDlg::OnBnClickedProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    FileProp();
    return 0;
}

LRESULT CMainDlg::OnEditExternal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int nCurItem;

    if ((nCurItem=ThumbsView.GetNextItem(-1, LVNI_ALL|LVNI_SELECTED))<0)
            return FALSE;

    LPCTSTR FileName = ThumbsView.GetFileName(nCurItem);
    if(!FileName) return FALSE;

    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    CString editorCmd = settings->ImageEditorPath;
    editorCmd.Replace(_T("%1"), FileName);
    CString editorCmdLine = WinUtils::ExpandEnvironmentStrings(editorCmd);

    CCmdLine editorLine(editorCmdLine);
    CString moduleName = editorLine.ModuleName();
    CString params = editorLine.OnlyParams();
    SHELLEXECUTEINFO Sei;
    ZeroMemory(&Sei, sizeof(Sei));
    Sei.cbSize = sizeof(Sei);
    Sei.fMask  = SEE_MASK_NOCLOSEPROCESS;
    Sei.hwnd = m_hWnd;
    Sei.lpVerb = _T("open");

    Sei.lpFile = moduleName;
    Sei.lpParameters = params;
    Sei.nShow = SW_SHOW;

    if (!::ShellExecuteEx(&Sei)) {
        DWORD lastError = GetLastError();
        LOG(ERROR) << "Opening external editor failed." << std::endl
                   << "Reason: " << WinUtils::ErrorCodeToString(lastError);
        return 0;
    }

    if(Sei.hProcess)
    {
        if(IsRunning())
        {
            WaitThreadStop.SetEvent();
            (void)WaitForThread();
        }
        itemIndexThumbToBeUpdated_ = nCurItem;
        m_EditorProcess = Sei.hProcess;
        Release();
        Start();
    }
    return 0;
}

LRESULT CMainDlg::OnImageView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ThumbsView.ViewSelectedImage();
    return 0;
}

LRESULT CMainDlg::OnMenuItemPaste(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    WizardDlg->executeFunc(_T("paste"));
    return 0;
}

LRESULT CMainDlg::OnAddFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    WizardDlg->executeFunc(_T("addfolder"));
    return 0;
}

LRESULT CMainDlg::OnBnClickedDeleteAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ThumbsView.MyDeleteAllItems();
    UpdateStatusLabel();
    return 0;
}

DWORD CMainDlg::Run()
{
    HANDLE Events[2];
    Events[0] = m_EditorProcess;
    Events[1] = WaitThreadStop.m_hEvent;
    WaitForMultipleObjects(2, Events, FALSE, INFINITE);
    ServiceLocator::instance()->taskRunner()->runInGuiThread([wnd = m_hWnd, this] {
        if (!GuiTools::CheckWindowPointer(wnd, this) || itemIndexThumbToBeUpdated_ < 0) {
            return;
        }
        ThumbsView.OutDateThumb(itemIndexThumbToBeUpdated_);
        itemIndexThumbToBeUpdated_ = -1;
    }, true);

    WaitThreadStop.ResetEvent();
    return 0;
}

// Executing Windows Explorer; file will be highlighted
LRESULT CMainDlg::OnOpenInFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int nCurItem = -1;
    PIDLIST_ABSOLUTE folderPidl = nullptr;

    CComPtr<IShellFolder> desktop; // namespace root for parsing the path
    HRESULT hr = SHGetDesktopFolder(&desktop);

    if (!SUCCEEDED(hr)) {
        return 0;
    }

    std::vector<LPCITEMIDLIST> list;
    while ((nCurItem = ThumbsView.GetNextItem(nCurItem, LVNI_ALL | LVNI_SELECTED)) >= 0) {
        LPCTSTR FileName = ThumbsView.GetFileName(nCurItem);
        if (!FileName) {
            continue;
        }

        PIDLIST_RELATIVE newPIdL;
        hr = desktop->ParseDisplayName(m_hWnd, nullptr, const_cast<LPWSTR>(FileName), nullptr, &newPIdL, nullptr);
        if (SUCCEEDED(hr)) {
            LPCITEMIDLIST relativePidl = nullptr;
            CComPtr<IShellFolder> folder;

            hr = SHBindToParent(newPIdL, IID_IShellFolder, reinterpret_cast<void**>(&folder), &relativePidl);
            if (SUCCEEDED(hr)) {
                list.push_back(relativePidl);
                if (!folderPidl) {
                    CComQIPtr<IPersistFolder2> persistFolder(folder);
                    if (persistFolder) {
                        persistFolder->GetCurFolder(&folderPidl);
                    }
                }
            }
        }
    }
    if (!list.empty() && folderPidl != nullptr) {
        // Will be opened the parent folder of the first file.
        // Files laying in another folders will be ignored.
        hr = SHOpenFolderAndSelectItems(folderPidl, list.size(), &list[0], 0);
        if (!SUCCEEDED(hr)) {
            _com_error err(hr);
            LOG(ERROR) << "Unable to open folder in shell, error code=" << hr << std::endl << err.ErrorMessage();
        }

        // SHBindToParent does not allocate a new PIDL;
        // it simply receives a pointer through this parameter.
        // Therefore, we are not responsible for freeing this resource.
        /*for (auto& pidl : list) {
            SHFree(const_cast<LPITEMIDLIST>(pidl));
        }*/
    }

    return 0;
}

void CMainDlg::openInDefaultProgram(CString fileName) {
    if (!fileName.IsEmpty()) {
        SHELLEXECUTEINFO TempInfo = {};
        TCHAR filePath[1024];
        WinUtils::ExtractFilePath(fileName, filePath, ARRAY_SIZE(filePath));
        TempInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
        TempInfo.hwnd = m_hWnd;
        TempInfo.lpVerb = _T("open");
        TempInfo.lpFile = fileName;
        TempInfo.lpDirectory = filePath;
        TempInfo.nShow = SW_NORMAL;

        if (!::ShellExecuteEx(&TempInfo)) {
            DWORD lastError = GetLastError();
            if (lastError != ERROR_CANCELLED) { // The operation was canceled by the user.
                LOG(ERROR) << "Opening default application failed." << std::endl << "Reason: " << WinUtils::ErrorCodeToString(lastError);
            }

        }
    }
}

LRESULT CMainDlg::OnOpenInDefaultViewer(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CString fileName = getSelectedFileName();
    openInDefaultProgram(fileName);
    return 0;
}

LRESULT CMainDlg::OnOpenWith(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CString fileName = getSelectedFileName();
    if (!fileName.IsEmpty()) {
        OPENASINFO info = {};
        info.pcszFile = fileName;
        info.oaifInFlags = OAIF_HIDE_REGISTRATION | OAIF_EXEC;
        info.pcszClass = nullptr;
        HRESULT hr = SHOpenWithDialog(m_hWnd, &info);
        if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            _com_error err(hr);

            LOG(ERROR) << "SHOpenWithDialog failed. 0x" << std::hex << hr <<  std::endl << err.ErrorMessage();
        }
    }

    return 0;
}

LRESULT CMainDlg::OnAddFiles(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    WizardDlg->executeFunc(_T("addfiles"));
    return 0;
}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg) {
    if (hotkeys_ && WinUtils::TranslateAcceleratorForWindow(m_hWnd, hotkeys_, pMsg)) {
        return TRUE;
    }
    return FALSE;
}

CString CMainDlg::getSelectedFileName() {
    int nCurItem;

    if ((nCurItem = ThumbsView.GetNextItem(-1, LVNI_ALL|LVNI_SELECTED))<0)
        return {};

    LPCTSTR FileName = ThumbsView.GetFileName(nCurItem);
    if ( !FileName ) {
        return CString();
    }

    return FileName;
}

int CMainDlg::getSelectedFiles(std::vector<CString>& selectedFiles) {
    int nCurItem = -1;

    while ((nCurItem = ThumbsView.GetNextItem(nCurItem, LVNI_ALL | LVNI_SELECTED)) >= 0) {
        LPCTSTR FileName = ThumbsView.GetFileName(nCurItem);
        if (!FileName) {
            continue;
        }
        selectedFiles.emplace_back(FileName);

    }
    return selectedFiles.size();
}

LRESULT CMainDlg::OnCopyFileToClipboard(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    int nCurItem = -1;

    std::vector<CString> selectedFiles;
    while ((nCurItem = ThumbsView.GetNextItem(nCurItem, LVNI_ALL|LVNI_SELECTED)) >= 0 ) {
        LPCTSTR FileName = ThumbsView.GetFileName(nCurItem);
        if ( !FileName ) {
            continue;
        }
        selectedFiles.emplace_back(FileName);

    }
    if ( selectedFiles.empty() ) {
        return FALSE;
    }

    if ( selectedFiles.size() == 1) {
        // If just one file is selected, copy it in two different formats
        ClipboardUtils::CopyFileAndImageToClipboard(selectedFiles[0], m_hWnd);
    } else {
        ClipboardUtils::CopyFilesToClipboard(selectedFiles, m_hWnd);
    }

    return 0;
}

LRESULT CMainDlg::OnSaveAs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    int nCurItem = -1;

    std::vector<CString> selectedFiles;
    while ((nCurItem = ThumbsView.GetNextItem(nCurItem, LVNI_ALL|LVNI_SELECTED)) >= 0 ) {
        LPCTSTR FileName = ThumbsView.GetFileName(nCurItem);
        if ( !FileName ) {
            continue;
        }
        selectedFiles.emplace_back(FileName );
    }
    if ( selectedFiles.empty() ) {
        return FALSE;
    }

    if ( selectedFiles.size() == 1 ) {
        CString FileName = selectedFiles[0];
        CString fileExt = WinUtils::GetFileExt(FileName);
        IMyFileDialog::FileFilterArray filters = {
            { TR("Files") + CString(" *.") + fileExt, CString(_T("*.")) + fileExt },
            { TR("All files"), _T("*.*") }
        };

        auto dlg = MyFileDialogFactory::createFileDialog(m_hWnd, CString(), CString(), filters, false, false);
        dlg->setFileName(WinUtils::GetOnlyFileName(FileName));
        fileExt.MakeLower();
        dlg->setDefaultExtension(fileExt);
        dlg->setFileTypeIndex(1);

        if (dlg->DoModal(m_hWnd) != IDOK) {
            return 0;
        }

        if (!CopyFile(FileName, dlg->getFile(), false)) {
            GuiTools::LocalizedMessageBox(m_hWnd, TR("Cannot copy file: ") + WinUtils::GetLastErrorAsString(), APP_NAME, MB_ICONERROR);
        }
    } else {
        CNewStyleFolderDialog dlg(m_hWnd, CString(), CString());
        dlg.SetOkButtonLabel(TR("Save"));
        if (dlg.DoModal(m_hWnd) == IDOK) {
            CString newPath = dlg.GetFolderPath();
            if (!newPath.IsEmpty()) {
                size_t fileCount = selectedFiles.size();
                for (size_t i = 0; i < fileCount; i++) {
                    if (!CopyFile(selectedFiles[i], newPath + _T("\\") + WinUtils::DoExtractFileName(selectedFiles[i]), false)) {
                        LOG(ERROR) << TR("Cannot copy file ")<< selectedFiles[i] << "\r\n" << WinUtils::GetLastErrorAsString();
                    }
                }
            }
        }
    }

    return 0;
}

LRESULT CMainDlg::OnCopyFileAsDataUri(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CString fileName = getSelectedFileName();
    if (!fileName.IsEmpty()) {
        if (!ImageUtils::CopyFileToClipboardInDataUriFormat(fileName, 0, 85, false)) {
            GuiTools::LocalizedMessageBox(m_hWnd, _T("Failed to copy file to clipboard"), APP_NAME, MB_ICONERROR);
        }
    }
    return 0;
}

LRESULT CMainDlg::OnCopyFileAsDataUriHtml(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CString fileName = getSelectedFileName();
    if (!fileName.IsEmpty()) {
        if (!ImageUtils::CopyFileToClipboardInDataUriFormat(fileName, 0, 85, true)) {
            GuiTools::LocalizedMessageBox(m_hWnd, _T("Failed to copy file to clipboard"), APP_NAME, MB_ICONERROR);
        }
    }
    return 0;
}

LRESULT CMainDlg::OnCopyFilePath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int nCurItem = -1;
    CString result;
    while ((nCurItem = ThumbsView.GetNextItem(nCurItem, LVNI_ALL | LVNI_SELECTED)) >= 0) {
        LPCTSTR FileName = ThumbsView.GetFileName(nCurItem);
        if (!FileName) {
            continue;
        }
        if (!result.IsEmpty()) {
            result += _T("\r\n");
        }
        result += FileName;

    }

    if (!result.IsEmpty()) {
        if (!WinUtils::CopyTextToClipboard(result)) {
            GuiTools::LocalizedMessageBox(m_hWnd, _T("Failed to copy file to clipboard"), APP_NAME, MB_ICONERROR);
		}
	}
	return 0;
}

LRESULT CMainDlg::OnSearchByImage(WORD, WORD wID, HWND, BOOL&) {
    int serverIndex = wID - MENUITEM_SEARCHBYIMG_START;
    int i = 0;
    auto* engineList = ServiceLocator::instance()->engineList();
    for (const auto& engine : *engineList) {
        if (engine->hasType(CUploadEngineData::TypeSearchByImageServer)) {
            if (i == serverIndex) {
                CString fileName = getSelectedFileName();
                if (!fileName.IsEmpty()) {
                    auto uploadManager = ServiceLocator::instance()->uploadManager();
                    ServerProfile searchProfile(engine->Name);
                    CSearchByImageDlg dlg(uploadManager, searchProfile, fileName);
                    dlg.DoModal(m_hWnd);
                }
                break;
            }
            i++;
        }
    }

    return 0;
}


LRESULT CMainDlg::OnExtractFramesFromSelectedFile(WORD, WORD, HWND, BOOL&) {
	CString fileName = getSelectedFileName();
	if (!fileName.IsEmpty()) {
		WizardDlg->importVideoFile(fileName, 2);
	}
	return 0;
}

LRESULT CMainDlg::OnFileInfo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
#ifdef IU_ENABLE_MEDIAINFO
    CString fileName = getSelectedFileName();
    if (!fileName.IsEmpty()) {
        CMediaInfoDlg dlg;
        dlg.ShowInfo(m_hWnd, fileName);
    }
#endif
    return 0;
}

LRESULT CMainDlg::OnTimer(UINT, WPARAM wParam, LPARAM, BOOL&) {
    if (wParam == kStatusTimer && listChanged_) {
        UpdateStatusLabel();
    }
    return 0;
}

LRESULT CMainDlg::OnListViewEndLabelEdit(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    auto* di = reinterpret_cast<NMLVDISPINFO*>(pnmh);
    if (di->item.pszText) {
        CString forbiddenCharacters { _T("\\/:*?\"<>|") };
        CString fileName { di->item.pszText };
        if (fileName.IsEmpty()) {
            GuiTools::LocalizedMessageBox(m_hWnd, TR("The file name cannot be empty."), APP_NAME, MB_ICONERROR);
            return 0;
        }
        if (fileName.FindOneOf(forbiddenCharacters) != -1) {
            GuiTools::LocalizedMessageBox(m_hWnd, TR("The file name contains forbidden characters."), APP_NAME, MB_ICONERROR);
            return 0;
        }
        if (di->item.iItem >= 0 && di->item.iItem < FileList.GetCount()) {
            FileList[di->item.iItem].VirtualFileName = di->item.pszText;
            return TRUE;
        }
    }
    return 0;
}

LRESULT CMainDlg::OnListViewBeginLabelEdit(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    auto* di = reinterpret_cast<NMLVDISPINFO*>(pnmh);
    if (di->item.iItem >= 0 && di->item.iItem < FileList.GetCount()) {
        // Select only file name without extension in the edit control
        CEdit editControl = ThumbsView.GetEditControl();
        CString fileName;
        editControl.GetWindowText(fileName);
        int endPos = fileName.ReverseFind(_T('.'));

        editControl.SetSel(0, endPos);
    }

    return 0;
}

void CMainDlg::UpdateStatusLabel() {
    int selectedItemsCount = ThumbsView.GetSelectedCount();
    int totalCount = ThumbsView.GetItemCount();
    CString statusText;

    try {
        if (selectedItemsCount) {
            std::string first = str(IuStringUtils::FormatNoExcept(boost::locale::ngettext("%d file selected", "%d files selected", selectedItemsCount)) % selectedItemsCount);
            std::string second = str(IuStringUtils::FormatNoExcept(boost::locale::ngettext("%d file total", "%d files total", totalCount)) % totalCount);

            statusText = U2W(str(boost::format("%1%/%2%") % first % second));
        }
        else {
            statusText = U2W(str(IuStringUtils::FormatNoExcept(boost::locale::ngettext("%d file", "%d files", totalCount)) % totalCount));
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << ex.what();
    }
    SetDlgItemText(IDC_STATUSLABEL, statusText);
    listChanged_ = false;
}

LRESULT CMainDlg::OnPrintImages(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    std::vector<CString> selectedFiles;
    if (getSelectedFiles(selectedFiles)) {
        CString fileName = selectedFiles[0];
        bool isImageFile = IuCommonFunctions::IsImage(fileName);
        if (isImageFile) {
            WinUtils::DisplaySystemPrintDialogForImage(selectedFiles, m_hWnd);
        }
    }
    return 0;
}

LRESULT CMainDlg::OnRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    int itemIndex = ThumbsView.GetNextItem(-1, LVNI_ALL | LVNI_FOCUSED | LVNI_SELECTED);
    if (itemIndex < 0) {
        return 0;
    }
    ThumbsView.EditLabel(itemIndex);
    return 0;
}
