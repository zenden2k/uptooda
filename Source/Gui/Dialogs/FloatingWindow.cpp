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

#include "FloatingWindow.h"

#include "ResultsWindow.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "LogWindow.h"
#include "Core/ServiceLocator.h"
#include "Core/Utils/CoreTypes.h"
#include "Func/WebUtils.h"
#include "Func/WinUtils.h"
#include "Core/Upload/UrlShorteningTask.h"
#include "Gui/GuiTools.h"
#include "Core/Upload/FileUploadTask.h"
#include "Func/myutils.h"
#include "Gui/Dialogs/WizardDlg.h"
#include "ScreenCapture/MonitorEnumerator.h"
#include "Gui/Dialogs/ImageDownloaderDlg.h"
#include "Core/OutputGenerator/AbstractOutputGenerator.h"
#include "Core/OutputGenerator/OutputGeneratorFactory.h"
#include "Core/OutputGenerator/XmlTemplateGenerator.h"
#include "Func/IuCommonFunctions.h"
#include "Func/Common.h"
#include "Gui/IconBitmapUtils.h"
#include "Gui/Helpers/DPIHelper.h"

namespace {

// {86EE6805-4ADE-444C-9886-F67E949DFBAD}
static const GUID MainTrayIconBaseGUID = { 0x86ee6805, 0x4ade, 0x444c, { 0x98, 0x86, 0xf6, 0x7e, 0x94, 0x9d, 0xfb, 0xad } };

constexpr auto TRAY_MUTEX_NAME = _T("Uptooda_TrayWnd_Mutex");

bool MyInsertMenu(HMENU hMenu, int pos, UINT id, LPCTSTR szTitle, HBITMAP bm = nullptr)
{
    MENUITEMINFO MenuItem {};

    MenuItem.cbSize = sizeof(MenuItem);
    MenuItem.fMask = MIIM_FTYPE | MIIM_ID;
    if (szTitle) {
        MenuItem.fType = MFT_STRING;
        MenuItem.fMask |= MIIM_STRING;
    } else {
        MenuItem.fType = MFT_SEPARATOR;
    }

    if (bm) {
        MenuItem.fMask |= MIIM_BITMAP;
        MenuItem.hbmpItem = bm;
    }
    MenuItem.wID = id;

    MenuItem.dwTypeData = const_cast<LPWSTR>(szTitle);
    MenuItem.cch = lstrlen(szTitle);
    return InsertMenuItem(hMenu, pos, TRUE, &MenuItem) != 0;
}

}

// FloatingWindow
CFloatingWindow::CFloatingWindow(CWizardDlg* wizardDlg, UploadManager* uploadManager,
    UploadEngineManager* uploadEngineManager):
    uploadManager_(uploadManager),
    uploadEngineManager_(uploadEngineManager),
    wizardDlg_(wizardDlg)
{
    m_bFromHotkey = false;
    m_ActiveWindow = nullptr;
    EnableClicks = true;
    hMutex = nullptr;
    m_PrevActiveWindow = nullptr;
    m_bStopCapturingWindows = false;
    WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));
    m_bIsUploading = false;
    iconAnimationCounter_ = 0;
    animationEnabled_ = false;
    m_hTrayIconMenu = nullptr;
    iconBitmapUtils_ = std::make_unique<IconBitmapUtils>();
    const int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
    int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    auto loadSmallIconBitmap = [&](int resourceId) -> HBITMAP {
        CIcon icon;
        icon.LoadIconWithScaleDown(MAKEINTRESOURCE(resourceId), iconWidth, iconHeight);
        if (!icon) {
            return nullptr;
        }
        return iconBitmapUtils_->HIconToBitmapPARGB32(icon, dpi);
    };

    screenshotBitmap_ = loadSmallIconBitmap(IDI_SCREENSHOT);
    screenRecordingBitmap_ = loadSmallIconBitmap(IDI_ICONRECORD);
    startRecordingBitmap_ = loadSmallIconBitmap(IDI_ICONSTARTRECORD);
    regionScreenshotBitmap_ = loadSmallIconBitmap(IDI_ICONREGION);
    addFilesBitmap_ = loadSmallIconBitmap(IDI_ICONADD);
    settingsBitmap_ = loadSmallIconBitmap(IDI_ICONSETTINGS);
    historyBitmap_ = loadSmallIconBitmap(IDI_ICONHISTORY);
}

CFloatingWindow::~CFloatingWindow()
{
    if (hMutex) {
        CloseHandle(hMutex);
    }
    m_hWnd = 0;
}

LRESULT CFloatingWindow::OnClose()
{
    return 0;
}

LRESULT CFloatingWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    using namespace Uptooda::Core::OutputGenerator;
    auto *settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    /*int w = ::GetSystemMetrics(SM_CXSMICON);
    if ( w > 32 ) {
        w = 48;
    } else if ( w > 16 ) {
        w = 32;
    }
    int h = w;*/
    activeIcon_ = GuiTools::LoadSmallIcon(IDI_ICONTRAYACTIVE);
    m_hIconSmall.LoadIconMetric(IDR_MAINFRAME, LIM_SMALL);

    RegisterHotkeys();

    trayIconGuid_ = WinUtils::GenerateFakeUUIDv4(MainTrayIconBaseGUID);
    //LOG(ERROR) << WinUtils::GUIDToString(*trayIconGUID);
    if (!InstallIcon(APP_NAME, m_hIconSmall, NULL, trayIconGuid_ ? &trayIconGuid_.value() : nullptr)) {
        LOG(WARNING) << "Failed to create tray icon!";
    }
    NOTIFYICONDATA nid;
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    nid.hWnd = m_hWnd;
    nid.uVersion = NOTIFYICON_VERSION;
    if (trayIconGuid_) {
        nid.uFlags = NIF_GUID;
        nid.guidItem = *trayIconGuid_;
    }
    Shell_NotifyIcon(NIM_SETVERSION, &nid);

     CString XmlFileName = IuCommonFunctions::GetDataFolder() + _T("templates.xml");
    std::string userTemplateFile = settings->SettingsFolder + "user_templates.xml";
    templateList_ = std::make_unique<XmlTemplateList>();
    try {
        templateList_->loadFromFile(W2U(XmlFileName));

    } catch (const std::exception& e) {
        ServiceLocator::instance()->logger()->write(ILogger::logWarning, _T("Floating Window Module"), U2W(e.what()));
    }

    if (IuCoreUtils::FileExists(userTemplateFile)) {
        try {
            templateList_->loadFromFile(userTemplateFile);
        } catch (const std::exception& e) {
            ServiceLocator::instance()->logger()->write(ILogger::logWarning, _T("Floating Window Module"), U2W(e.what()));
        }
    }
    return 0;
}

LRESULT CFloatingWindow::OnExit(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (!wizardDlg_->IsWindowEnabled()) {
        wizardDlg_->SetActiveWindow();
        wizardDlg_->FlashWindow(true);
        return 0;
    }
    if (canExitApp()) {
        wizardDlg_->CloseWizard();
    }
    return 0;
}

LRESULT CFloatingWindow::OnTrayIcon(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    if (!EnableClicks )
        return 0;
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    if (LOWORD(lParam) == WM_LBUTTONDOWN)
    {
        m_bStopCapturingWindows = true;
    }
    if (LOWORD(lParam) == WM_MOUSEMOVE)
    {
        if (!m_bStopCapturingWindows)
        {
            HWND wnd =  GetForegroundWindow();
            if (wnd != m_hWnd)
                m_PrevActiveWindow = GetForegroundWindow();
        }
    }
    if (LOWORD(lParam) == WM_RBUTTONUP)
    {
        if (m_bIsUploading && Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.RightClickCommandStr).commandId != IDM_CONTEXTMENU)
            return 0;
        SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.RightClickCommandStr).commandId, 0));
    }
    else if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
    {
        EnableClicks = false;
        KillTimer(1);
        SetTimer(2, GetDoubleClickTime());
        if (m_bIsUploading && Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.LeftDoubleClickCommandStr).commandId !=
            IDM_CONTEXTMENU)
            return 0;
        SendMessage(WM_COMMAND,
                    MAKEWPARAM(Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.LeftDoubleClickCommandStr).commandId, 0));
    }
    else if (LOWORD(lParam) == WM_LBUTTONUP)
    {
        m_bStopCapturingWindows = false;
        if (m_bIsUploading && Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.LeftDoubleClickCommandStr).commandId !=
            IDM_CONTEXTMENU)
            return 0;

        if (!Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.LeftDoubleClickCommandStr).commandId)
            SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.LeftClickCommandStr).commandId, 0));
        else
            SetTimer(1, static_cast<UINT>(1.2 * GetDoubleClickTime()));
    }
    else if (LOWORD(lParam) == WM_MBUTTONUP)
    {
        if (m_bIsUploading && Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.MiddleClickCommandStr).commandId != IDM_CONTEXTMENU)
            return 0;

        SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.MiddleClickCommandStr).commandId, 0));
    }
    else if (LOWORD(lParam) == NIN_BALLOONUSERCLICK)
    {
        if (balloonClickFunction_) {
            balloonClickFunction_();
        }
    }
    return 0;
}

LRESULT CFloatingWindow::OnMenuSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (!wizardDlg_->IsWindowEnabled())
    {
        HWND childModalDialog = wizardDlg_->GetLastActivePopup();
        if (childModalDialog && ::IsWindowVisible(childModalDialog))
            SetForegroundWindow(childModalDialog);
        return 0;
    }
    HWND hParent  = wizardDlg_->m_hWnd; // wizardDlg_->IsWindowEnabled()?  : 0;
    CSettingsDlg dlg(CSettingsDlg::spTrayIcon, uploadEngineManager_);
    dlg.DoModal(hParent);
    return 0;
}

LRESULT CFloatingWindow::OnShowHistory(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    wizardDlg_->executeFunc(_T("history"));
    return 0;
}

LRESULT CFloatingWindow::OnCloseTray(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ShowWindow(SW_HIDE);
    wizardDlg_->ShowWindow(SW_SHOW);
    wizardDlg_->SetDlgItemText(IDCANCEL, TR("Exit"));
    if (hMutex) {
        CloseHandle(hMutex);
    }
    RemoveIcon();
    UnRegisterHotkeys();
    DestroyWindow();
    hMutex = nullptr;
    m_hWnd = nullptr;
    return 0;
}

LRESULT CFloatingWindow::OnReloadSettings(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!lParam)
        UnRegisterHotkeys();

    if (!wParam) {
        auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
        settings->LoadSettings();
    }

    if (!lParam)
        RegisterHotkeys();
    return 0;
}

LRESULT CFloatingWindow::OnImportvideo(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("importvideo,1")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnUploadFiles(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("addfiles,1")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnReUploadImages(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("reuploadimages,1"))) {
        wizardDlg_->ShowWindow(SW_SHOW);
    }
    return 0;
}


LRESULT CFloatingWindow::OnUploadImages(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("addimages,1")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnPasteFromWeb(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("downloadimages,1")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnQuickUploadFromClipboard(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    if (m_bIsUploading) {
        return 0;
    }

    if (IsClipboardFormatAvailable(CF_BITMAP) != 0 && OpenClipboard()) {
        auto bmp = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));

        if (bmp) {
            CString filePath;
            SIZE dim;
            GetBitmapDimensionEx(bmp, &dim);
            Gdiplus::Bitmap bm(bmp, nullptr);
            if (bm.GetLastStatus() == Gdiplus::Ok) {
                try {
                    if (ImageUtils::MySaveImage(&bm, _T("clipboard"), filePath, ImageUtils::sifPNG, 100)) {
                        CString fileName = WinUtils::DoExtractFileName(filePath);
                        UploadScreenshot(filePath, fileName);
                    }
                }
                catch (const std::exception& ex) {
                    LOG(ERROR) << "Unable to save image: " << ex.what();
                }
            }
        }
        CloseClipboard();
        return 0;
    }

    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        CString url;
        WinUtils::GetClipboardText(url);
        CString outFileName;
        if (ImageUtils::SaveImageFromClipboardDataUriFormat(url, outFileName)) {
            CString fileName = WinUtils::DoExtractFileName(outFileName);
            UploadScreenshot(outFileName, fileName);
            return true;
        }
        if (!url.IsEmpty() && WebUtils::DoesTextLookLikeUrl(url)) {
            CImageDownloaderDlg dlg(nullptr, url);
            dlg.EmulateModal(m_hWnd);
            const auto& downloadedFilesList = dlg.getDownloadedFiles();
            if (!downloadedFilesList.empty()) {
                CString filePath = downloadedFilesList[0];
                CString fileName = WinUtils::DoExtractFileName(filePath);
                UploadScreenshot(filePath, fileName);
            }
            return true;
        }
    }

    return 0;
}

LRESULT CFloatingWindow::OnScreenshotDlg(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("screenshotdlg,2"));
    return 0;
}

LRESULT CFloatingWindow::OnRegionScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("regionscreenshot_dontshow,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
    return 0;
}

LRESULT CFloatingWindow::OnFullScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("fullscreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
    return 0;
}

LRESULT CFloatingWindow::OnWindowHandleScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("windowhandlescreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
    return 0;
}

LRESULT CFloatingWindow::OnLastRegionScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    wizardDlg_->executeFunc(_T("lastregionscreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));

    return 0;
}

LRESULT CFloatingWindow::OnFreeformScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("freeformscreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
    return 0;
}

LRESULT CFloatingWindow::OnWindowScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    wizardDlg_->executeFunc(_T("topwindowscreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
    return 0;
}

LRESULT CFloatingWindow::OnShortenUrlClipboard(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    if (lastUrlShorteningTask_ && lastUrlShorteningTask_->isRunning()) {
        return false;
    }

    CString url;
    WinUtils::GetClipboardText(url);
    if ( !url.IsEmpty() && !WebUtils::DoesTextLookLikeUrl(url) ) {
        return false;
    }
    using namespace std::placeholders;
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    lastUrlShorteningTask_ = std::make_shared<UrlShorteningTask>(W2U(url));
    lastUrlShorteningTask_->setServerProfile(settings->urlShorteningServer);
    lastUrlShorteningTask_->addTaskFinishedCallback(std::bind(&CFloatingWindow::OnFileFinished, this, _1, _2));
    currentUploadSession_ = std::make_shared<UploadSession>();
    currentUploadSession_->addTask(lastUrlShorteningTask_);
    currentUploadSession_->addSessionFinishedCallback(std::bind(&CFloatingWindow::onUploadSessionFinished, this, _1));
    m_bIsUploading = true;
    uploadManager_->addSession(currentUploadSession_);

    CString msg;
    msg.Format(TR("Shortening URL \"%s\" using %s"), static_cast<LPCTSTR>(url),
        static_cast<LPCTSTR>(Utf8ToWstring(settings->urlShorteningServer.serverName()).c_str()));

    // Do not show the first baloon in Windows 10+ so the second baloon will appear immediately
    if (!IsWindows10OrGreater()) {
        ShowBaloonTip(msg, APP_NAME, 6000);
    }
    setStatusText(msg);
    startIconAnimation();
    return 0;
}

LRESULT CFloatingWindow::OnShowLastUploadResults(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    showLastUploadedCode();
    return 0;
}

LRESULT CFloatingWindow::OnActiveWindowScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (m_PrevActiveWindow)
        SetForegroundWindow(m_PrevActiveWindow);
    wizardDlg_->executeFunc(_T("windowscreenshot_delayed,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));

    return 0;
}

LRESULT CFloatingWindow::OnAddFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("addfolder")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnShortenUrl(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    if (wizardDlg_->executeFunc(_T("shortenurl")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}


LRESULT CFloatingWindow::OnShowAppWindow(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->IsWindowEnabled())
    {
        wizardDlg_->ShowWindow(SW_SHOWNORMAL);
        if (wizardDlg_->IsWindowVisible())
        {
            // SetForegroundWindow(m_hWnd);
            wizardDlg_->SetActiveWindow();
            SetForegroundWindow(wizardDlg_->m_hWnd);
        }
    }
    else
    {
        // Activating some child modal dialog if exists one
        HWND childModalDialog = wizardDlg_->GetLastActivePopup();
        if (childModalDialog && ::IsWindowVisible(childModalDialog))
            SetForegroundWindow(childModalDialog);
    }

    return 0;
}

LRESULT CFloatingWindow::OnOpenScreenshotsFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    wizardDlg_->executeFunc(_T("open_screenshot_folder"));
    return 0;
}

LRESULT CFloatingWindow::OnContextMenu(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    if (!IsWindowEnabled()) {
        return 0;
    }

    CMenu TrayMenu;
    TrayMenu.CreatePopupMenu();
    int i = 0;
    if (wizardDlg_->isScreenRecorderRunning()) {
        MyInsertMenu(TrayMenu, i++, IDM_SCREENRECORDINGSTOP, HotkeyToString("screenrecording_stop", TR("Finish Screen Recording")));
    } else if (m_bIsUploading) {
        MyInsertMenu(TrayMenu, i++, IDM_STOPUPLOAD, TR("Stop uploading"));
    } else {
        MyInsertMenu(TrayMenu, i++, IDM_UPLOADFILES, HotkeyToString("addimages", TR("Upload files") + CString(_T("..."))), addFilesBitmap_);
        MyInsertMenu(TrayMenu, i++, IDM_ADDFOLDERS, HotkeyToString("addfolder", TR("Upload folder") + CString(_T("..."))));
        MyInsertMenu(TrayMenu, i++, 0, 0);
        bool isBitmapInClipboard = false;
        bool urlInClipboard = false;
        //if (OpenClipboard())
        {
            isBitmapInClipboard = IsClipboardFormatAvailable(CF_BITMAP) != 0;
            if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                CString url;
                urlInClipboard = WinUtils::GetClipboardText(url) && WebUtils::DoesTextLookLikeUrl(url);
                if (url.Left(5) == _T("data:")) {
                    isBitmapInClipboard = true;
                }
            }
            //CloseClipboard();
        }
        if (isBitmapInClipboard) {
            MyInsertMenu(TrayMenu, i++, IDM_PASTEFROMCLIPBOARD, HotkeyToString("paste", TR("Paste")));
        }
        if (isBitmapInClipboard || urlInClipboard) {
            MyInsertMenu(TrayMenu, i++, IDM_QUICKUPLOADFROMCLIPBOARD, HotkeyToString("uploadfromclipboard", TR("Quick upload image from clipboard")));
            MyInsertMenu(TrayMenu, i++, 0, 0);
        }
        MyInsertMenu(TrayMenu, i++, IDM_IMPORTVIDEO, HotkeyToString("importvideo", TR("Import Video File")));
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_SCREENSHOTDLG, HotkeyToString("screenshotdlg", TR("Screenshot") + CString(_T("..."))), screenshotBitmap_);
        MyInsertMenu(TrayMenu, i++, IDM_REGIONSCREENSHOT, HotkeyToString("regionscreenshot", TR("Capture Selected Area")), regionScreenshotBitmap_);
        if (wizardDlg_->hasLastScreenshotRegion()) {
            MyInsertMenu(TrayMenu, i++, IDM_LASTREGIONSCREENSHOT, HotkeyToString("lastregionscreenshot", TR("Capture Last Region")));
        }
        MyInsertMenu(TrayMenu, i++, IDM_FULLSCREENSHOT, HotkeyToString("fullscreenshot", TR("Capture the Entire Screen")));
        MyInsertMenu(TrayMenu, i++, IDM_ACTIVEWINDOWSCREENSHOT, HotkeyToString("windowscreenshot", TR("Capture the Active Window")));
        MyInsertMenu(TrayMenu, i++, IDM_TOPWINDOWSCREENSHOT, HotkeyToString("topwindowscreenshot", TR("Capture Selected Window")));

        MyInsertMenu(TrayMenu, i++, IDM_WINDOWHANDLESCREENSHOT, HotkeyToString("windowhandlescreenshot", TR("Capture Selected Object")));
        MyInsertMenu(TrayMenu, i++, IDM_FREEFORMSCREENSHOT, HotkeyToString("freeformscreenshot", TR("Freehand Capture")));

        MonitorEnumerator enumerator;

        if (enumerator.enumDisplayMonitors(0, 0) && enumerator.getCount() > 1) {
            CMenu MonitorsSubMenu;
            MonitorsSubMenu.CreatePopupMenu();
            MonitorsSubMenu.InsertMenu(0, MFT_STRING | MFT_RADIOCHECK | (Settings.ScreenshotSettings.MonitorMode == ScreenCapture::kAllMonitors ? MFS_CHECKED : 0),
                IDM_MONITOR_ALLMONITORS, TR("All monitors"));
            MonitorsSubMenu.InsertMenu(1, MFT_STRING | MFT_RADIOCHECK | (Settings.ScreenshotSettings.MonitorMode == ScreenCapture::kCurrentMonitor ? MFS_CHECKED : 0),
                IDM_MONITOR_CURRENTMONITOR, TR("Current monitor"));
            int j = 0;
            for (const MonitorEnumerator::MonitorInfo& monitor : enumerator) {
                CString itemTitle;
                itemTitle.Format(_T("%s %d (%dx%d)"), TR("Monitor"), j + 1, monitor.rect.Width(), monitor.rect.Height());
                bool isSelected = Settings.ScreenshotSettings.MonitorMode == ScreenCapture::kSelectedMonitor + j;
                MonitorsSubMenu.InsertMenu(j + 2, MFT_STRING | MFT_RADIOCHECK | (isSelected ? MFS_CHECKED : 0),
                    IDM_MONITOR_SELECTEDMONITOR_FIRST + j, itemTitle);
                j++;
                if (j >= IDM_MONITOR_SELECTEDMONITOR_LAST - IDM_MONITOR_SELECTEDMONITOR_FIRST) {
                    break;
                }
            }
            MENUITEMINFO monitorMenuItem {};
            monitorMenuItem.cbSize = sizeof(monitorMenuItem);
            monitorMenuItem.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
            monitorMenuItem.fType = MFT_STRING;
            monitorMenuItem.hSubMenu = MonitorsSubMenu;
            monitorMenuItem.wID = 10001;
            CString title = TR("Choose monitor");
            monitorMenuItem.dwTypeData = const_cast<LPWSTR>(title.GetString());
            monitorMenuItem.cch = title.GetLength();
            TrayMenu.InsertMenuItem(i++, true, &monitorMenuItem);
            MonitorsSubMenu.Detach();
        }

        CMenu SubMenu;
        SubMenu.CreatePopupMenu();
        SubMenu.InsertMenu(
            0, MFT_STRING | MFT_RADIOCHECK |
            (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_OPENINEDITOR ? MFS_CHECKED : 0),
            IDM_SCREENTSHOTACTION_OPENINEDITOR, TR("Open in the editor"));
        SubMenu.InsertMenu(
           0, MFT_STRING | MFT_RADIOCHECK |
           (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_UPLOAD ? MFS_CHECKED : 0),
           IDM_SCREENTSHOTACTION_UPLOAD, TR("Upload to Web"));
        SubMenu.InsertMenu(
           1, MFT_STRING | MFT_RADIOCHECK |
           (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_CLIPBOARD ? MFS_CHECKED : 0),
           IDM_SCREENTSHOTACTION_TOCLIPBOARD, TR("Copy to Clipboard"));
        SubMenu.InsertMenu(
           2, MFT_STRING | MFT_RADIOCHECK |
           (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_SHOWWIZARD ? MFS_CHECKED : 0),
           IDM_SCREENTSHOTACTION_SHOWWIZARD, TR("Open in Wizard"));
        SubMenu.InsertMenu(
           3, MFT_STRING | MFT_RADIOCHECK |
           (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_ADDTOWIZARD ? MFS_CHECKED : 0),
           IDM_SCREENTSHOTACTION_ADDTOWIZARD, TR("Add to Wizard"));

        MENUITEMINFO mi {};
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
        mi.fType = MFT_STRING;
        mi.hSubMenu = SubMenu;
        mi.wID = 10000;
        CString title = TR("Screenshot Action");
        mi.cch = title.GetLength();
        mi.dwTypeData = const_cast<LPWSTR>(title.GetString());
        TrayMenu.InsertMenuItem(i++, true, &mi);

        SubMenu.Detach();
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_SCREENRECORDINGDIALOG, HotkeyToString("screenrecordingdlg", TR("Screen Recording...")), screenRecordingBitmap_);
        MyInsertMenu(TrayMenu, i++, IDM_SCREENRECORDINGSTART, HotkeyToString("screenrecording", TR("Start Recording")), startRecordingBitmap_);

        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_SHORTENURL, HotkeyToString("shortenurl",TR("Shorten a Link")));
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_SHOWAPPWINDOW, HotkeyToString("showmainwindow", TR("Show program window")));
        MyInsertMenu(TrayMenu, i++, IDM_OPENSCREENSHOTSFOLDER, HotkeyToString("open_screenshot_folder", TR("Open screenshots folder")));
        if (lastUploadedItem_) {
            MyInsertMenu(TrayMenu, i++, IDM_SHOWLASTUPLOADRESULTS, TR("Show results of last upload"));
        }
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_SETTINGS, HotkeyToString("settings", TR("Settings") + CString(_T("..."))), settingsBitmap_);
        MyInsertMenu(TrayMenu, i++, IDM_HISTORY, HotkeyToString("history", TR("History")), historyBitmap_);
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_EXIT, TR("Exit"));
        TrayMenu.EnableMenuItem(IDM_EXIT, MF_BYCOMMAND | (canExitApp() ? MF_ENABLED : MF_DISABLED));
        if (Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.LeftDoubleClickCommandStr).commandId)
        {
            SetMenuDefaultItem(TrayMenu, Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.LeftDoubleClickCommandStr).commandId,
                               FALSE);
        }
    }

    m_hTrayIconMenu = TrayMenu;
    CMenuHandle oPopup(m_hTrayIconMenu);
    PrepareMenu(oPopup);
    CPoint pos;
    GetCursorPos(&pos);
    SetForegroundWindow(m_hWnd);
    oPopup.TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, m_hWnd);
    // BUGFIX: See "PRB: Menus for Notification Icons Don't Work Correctly"
    PostMessage(WM_NULL);
    return 0;
}

LRESULT CFloatingWindow::OnTimer(UINT id)
{
    if (id == 1)
    {
        KillTimer(1);
        WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
        SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys.getByFunc(Settings.TrayIconSettings.LeftClickCommandStr).commandId, 0));
    } else if (id == 2) {
        EnableClicks = true;
        KillTimer(id);
    } else if (id == kStatusTimer) {
        removeStatusText();
        KillTimer(id);
    } else if (id == kIconAnimationTimer && animationEnabled_) {
        UpdateIcon(iconAnimationCounter_++ == 0 ? activeIcon_ : m_hIconSmall );
        iconAnimationCounter_ = iconAnimationCounter_ % 2;
    }

    return 0;
}

void CFloatingWindow::CreateTrayIcon()
{
    BOOL bFound = FALSE;
    hMutex = ::CreateMutex(NULL, TRUE, TRAY_MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        bFound = TRUE;
    if (hMutex)
        ::ReleaseMutex(hMutex);

    if (!bFound)
    {
        CRect r(100, 100, 400, 400);
        Create(0, r, _T("ImageUploader_TrayWnd"), WS_OVERLAPPED | WS_POPUP | WS_CAPTION );
        ShowWindow(SW_HIDE);
    }
}

BOOL CFloatingWindow::IsRunningFloatingWnd() {
    HANDLE hMutex = NULL;
    BOOL bFound = FALSE;
    hMutex = ::CreateMutex(NULL, TRUE, TRAY_MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        bFound = TRUE;
    if (hMutex)
    {
        ::ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return bFound;
}

void CFloatingWindow::RegisterHotkeys()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    m_hotkeys = settings->Hotkeys;

    for (size_t i = 0; i < m_hotkeys.size(); i++)
    {
        const auto& hotkey = m_hotkeys[i];
        if (hotkey.groupId == HOTKEY_GROUP_APP && hotkey.globalKey.keyCode)
        {
            if (!RegisterHotKey(m_hWnd, i, hotkey.globalKey.keyModifier, hotkey.globalKey.keyCode))
            {
                CString msg;
                msg.Format(TR("Cannot register global hotkey:\n%s.\n Maybe it is being used by another process."),
                    static_cast<LPCTSTR>(hotkey.globalKey.toString()));
                ServiceLocator::instance()->logger()->write(ILogger::logWarning, _T("Hotkeys"), msg);
            }
        }
    }
}

LRESULT CFloatingWindow::OnHotKey(int HotKeyID, UINT flags, UINT vk)
{
    if (HotKeyID < 0 || HotKeyID >= m_hotkeys.size())
        return 0;
    if (m_bIsUploading)
        return 0;

    if (m_hotkeys[HotKeyID].func == _T("windowscreenshot"))
    {
        wizardDlg_->executeFunc(_T("windowscreenshot,1"));
    }
    else
    {
        m_bFromHotkey = true;
        if (m_hotkeys[HotKeyID].setForegroundWindow) {
            SetActiveWindow();
            SetForegroundWindow(m_hWnd);
        }
        SendMessage(WM_COMMAND, MAKEWPARAM(m_hotkeys[HotKeyID].commandId, 0));
        m_bFromHotkey = false;
    }
    return 0;
}

void CFloatingWindow::UnRegisterHotkeys()
{
    for (size_t i = 0; i < m_hotkeys.size(); i++) {
        const auto& hotkey = m_hotkeys[i];
        if (hotkey.groupId == HOTKEY_GROUP_APP && hotkey.globalKey.keyCode) {
            UnregisterHotKey(m_hWnd, i);
        }
    }
    m_hotkeys.clear();
}

LRESULT CFloatingWindow::OnPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("paste")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnMediaInfo(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("mediainfo")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnTaskbarCreated(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    InstallIcon(APP_NAME, m_hIconSmall, 0, trayIconGuid_ ? &trayIconGuid_.value() : nullptr);
    return 0;
}

LRESULT CFloatingWindow::OnMonitorAllMonitors(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    WtlGuiSettings* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->ScreenshotSettings.MonitorMode = ScreenCapture::kAllMonitors;
    return 0;
}

LRESULT CFloatingWindow::OnMonitorCurrentMonitor(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    WtlGuiSettings* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->ScreenshotSettings.MonitorMode = ScreenCapture::kCurrentMonitor;
    return 0;
}

LRESULT CFloatingWindow::OnMonitorSelectedMonitor(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int monitorIndex = wID - IDM_MONITOR_SELECTEDMONITOR_FIRST;
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    Settings.ScreenshotSettings.MonitorMode = ScreenCapture::kSelectedMonitor + monitorIndex;
    return 0;
}

LRESULT CFloatingWindow::OnScreenRecordingDialog(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    wizardDlg_->executeFunc(_T("screenrecordingdlg"));
    return 0;
}

LRESULT CFloatingWindow::OnScreenRecordingStart(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    wizardDlg_->executeFunc(_T("screenrecording"));
    return 0;
}

LRESULT CFloatingWindow::OnScreenRecordingStop(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    wizardDlg_->stopScreenRecording();
    return 0;
}

LRESULT CFloatingWindow::OnScreenshotActionChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    Settings.TrayIconSettings.TrayScreenshotAction = wID - IDM_SCREENTSHOTACTION_UPLOAD;
    Settings.SaveSettings();
    return 0;
}

void CFloatingWindow::UploadScreenshot(const CString& realName, const CString& displayName)
{
    using namespace std::placeholders;
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    auto task = std::make_shared<FileUploadTask>(W2U(realName), W2U(displayName));
    task->setIsImage(true);
    task->setServerProfile(Settings.quickScreenshotServer.getByIndex(0));
    task->addTaskFinishedCallback(std::bind(&CFloatingWindow::OnFileFinished, this, _1, _2));
    task->setUrlShorteningServer(Settings.urlShorteningServer);

    currentUploadSession_ = std::make_shared<UploadSession>();
    currentUploadSession_->addTask(task);
    currentUploadSession_->addSessionFinishedCallback(std::bind(&CFloatingWindow::onUploadSessionFinished, this, std::placeholders::_1));
    m_bIsUploading = true;
    uploadManager_->addSession(currentUploadSession_);

    CString msg;
    CString onlyFileName = WinUtils::GetOnlyFileName(displayName);
    msg.Format(TR("File \"%s\" is beeing uploaded to server %s.."), static_cast<LPCTSTR>(onlyFileName),
        static_cast<LPCTSTR>(Utf8ToWstring(Settings.quickScreenshotServer.getByIndex(0).serverName()).c_str()));

    // Do not show the first baloon in Windows 10+ so the second baloon will appear immediately
    if (!IsWindows10OrGreater()) {
        ShowBaloonTip(msg, TR("Uploading screenshot"), 6000);
    }

    setStatusText(msg);
    startIconAnimation();
}

void CFloatingWindow::setStatusText(const CString& text, int timeoutMs) {
    statusText_ = text;
    if (statusText_.IsEmpty()) {
        SetTooltipText(CString(APP_NAME));
    } else {
        SetTooltipText(CString(APP_NAME) + _T("\r\n") + statusText_);
    }
    if (timeoutMs) {
        SetTimer(kStatusTimer, timeoutMs);
    } else {
        KillTimer(kStatusTimer);
    }
}

void CFloatingWindow::removeStatusText() {
    setStatusText(CString());
}

void CFloatingWindow::ShowBaloonTip(const CString& text, const CString& title, unsigned timeout, const std::function<void()>& onClick) {
    balloonClickFunction_ = onClick;
    CTrayIconImpl<CFloatingWindow>::ShowBaloonTip(text, title, timeout);
}

void CFloatingWindow::startIconAnimation() {
    animationEnabled_ = true;
    iconAnimationCounter_ = 0;
    SetTimer(kIconAnimationTimer, 400);
}

void CFloatingWindow::stopIconAnimation() {
    // KillTimer does not remove the WM_TIMER from message queue, so we need to use a flag
    animationEnabled_ = false;
    KillTimer(kIconAnimationTimer);
    UpdateIcon(m_hIconSmall);
}

void CFloatingWindow::showLastUploadedCode() {
    using namespace Uptooda::Core::OutputGenerator;
    auto *settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    std::vector<UploadObject> items;

    if (lastUploadedItem_) {
        items.push_back(*lastUploadedItem_);
        CResultsWindow resultsWindow_(wizardDlg_, items, false);
        resultsWindow_.DoModal(m_hWnd);
    }
}

void CFloatingWindow::OnFileFinished(UploadTask* task, bool ok)
{
    if (task->type() == UploadTask::TypeUrl ) {
        if ( ok ) {
            CString url = Utf8ToWCstring(task->uploadResult()->directUrl);
            WinUtils::CopyTextToClipboard(url);
            CString text = WinUtils::TrimString(url, 70) + CString("\r\n")
                + TR("(the link has been copied to the clipboard)");
            ShowBaloonTip(text, TR("Short URL"), 17000);
            setStatusText(text, kStatusHideTimeout);
        } else {
            CString statusText = TR("Unable to shorten the link...");
            ShowBaloonTip(TR("View log for details."), statusText, 17000, [&] {ServiceLocator::instance()->logWindow()->Show(); });
            setStatusText(statusText, kStatusHideTimeout);
        }
    } else {
        if (ok) {
            CString url;
            UploadResult* uploadResult = task->uploadResult();
            bool usedDirectLink = true;
            WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
            if ((Settings.UseDirectLinks || uploadResult->downloadUrl.empty()) && !uploadResult->directUrl.empty()) {
                url = Utf8ToWstring(!uploadResult->directUrlShortened.empty() ? uploadResult->directUrlShortened : uploadResult->directUrl).c_str();
            } else if ((!Settings.UseDirectLinks || uploadResult->directUrl.empty()) && !uploadResult->downloadUrl.empty()) {
                url = Utf8ToWstring(!uploadResult->downloadUrlShortened.empty() ? uploadResult->downloadUrlShortened : uploadResult->downloadUrl).c_str();
                usedDirectLink = false;
            }

            ShowImageUploadedMessage(task, url);

        } else {
            CString statusText = TR("Could not upload screenshot :(");
            ShowBaloonTip(TR("View log for details."), statusText, 17000, [&] {ServiceLocator::instance()->logWindow()->Show(); });
            setStatusText(statusText, kStatusHideTimeout);
        }

    }
    stopIconAnimation();
}

LRESULT CFloatingWindow::OnStopUpload(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (m_bIsUploading && currentUploadSession_) {
        currentUploadSession_->stop();
    }

    return 0;
}

void CFloatingWindow::ShowImageUploadedMessage(UploadTask* task, const CString& url)
{
    using namespace Uptooda::Core::OutputGenerator;
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    auto obj = std::make_unique<UploadObject>();
    obj->fillFromUploadResult(task->uploadResult(), task);

    CString code;
    CString message;
    if (settings->TrayResult == WtlGuiSettings::trJustURL) {
        message = TR("(the link has been copied to the clipboard)");
        code = url;
    } else if (settings->TrayResult == WtlGuiSettings::trLastCodeType) {
        GeneratorID generatorId = static_cast<GeneratorID>(settings->CodeLang);
        //CodeLang lang = clBBCode;
        CodeType codeType = static_cast<CodeType>(settings->CodeType);
        OutputGeneratorFactory factory;
        std::vector<UploadObject> objects { *obj };
        auto generator = factory.createOutputGenerator(generatorId, codeType);
        generator->setPreferDirectLinks(settings->UseDirectLinks);
        generator->setItemsPerLine(settings->ThumbsPerLine);
        generator->setGroupByFile(settings->GroupByFilename);
        generator->setShortenUrl(task->serverProfile().shortenLinks());

        if (generatorId == gidXmlTemplate) {
            int templateIndex = settings->CodeType - 4;
            auto xmlTemplateGenerator = dynamic_cast<XmlTemplateGenerator*>(generator.get());
            if (xmlTemplateGenerator) {
                xmlTemplateGenerator->setTemplateIndex(templateIndex);
            }
        }

        code = U2W(generator->generate(objects, settings->UseTxtTemplate));
        message = TR("(the code has been copied to the clipboard)");
    }
    WinUtils::CopyTextToClipboard(code);
    CString trimmedCode = WinUtils::TrimString(code, 70);

    ShowBaloonTip(trimmedCode + CString("\r\n")
            + message + CString("\r\n") + TR("Click on this message to view details..."),
        TR("Screenshot was uploaded"), 17000, [this] {showLastUploadedCode(); });
    CString statusText = TR("Screenshot was uploaded") + CString(_T("\r\n")) + trimmedCode;
    setStatusText(statusText, kStatusHideTimeout);
    lastUploadedItem_ = std::move(obj);
}

void CFloatingWindow::ShowScreenshotCopiedToClipboardMessage(CString message) {
    ShowBaloonTip(message, APP_NAME, 17000);
    setStatusText(message, kStatusHideTimeout);
}

bool CFloatingWindow::canExitApp() const {
    return !m_bIsUploading && wizardDlg_->canExitApp();
}

void  CFloatingWindow::onUploadSessionFinished(UploadSession* session) {
    m_bIsUploading = false;
}
