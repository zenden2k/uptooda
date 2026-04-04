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
#ifndef WIZARDDLG_H
#define WIZARDDLG_H


#pragma once

#include <optional>

#include <boost/signals2.hpp>

#include "atlheaders.h"
#include "Func/MyEngineList.h"
#include "HotkeySettings.h"
#include "ScreenCapture/ScreenCaptureWin.h"
#include "resource.h"       // main symbols
#include "Gui/Dialogs/UpdateDlg.h"
#include "Core/ProgramWindow.h"
#include "Core/TaskDispatcher.h"
#include "FolderAddDlg.h"
#include "Core/Upload/ServerProfileGroup.h"
#include "Gui/HwndScopedWrapper.h"
#include "Gui/CommonDefines.h"
#include "Gui/Controls/DialogIndirect.h"
#include "Gui/Components/DragndropOverlay.h"
#include "Gui/WizardCommon.h"
#include "Gui/Dialogs/RegionSelect.h"
#include "Gui/Dialogs/ScreenRecordingDlg.h"

class WinToastHandler;
class ScreenRecorderWindow;
class CHistoryManager;

constexpr int ID_PASTE = 9888;
constexpr int ID_HOTKEY_BASE = 10000;
constexpr int WM_MY_ADDIMAGE = WM_USER + 222;
constexpr int WM_MY_SHOWPAGE = WM_USER + 223;
constexpr int WM_MY_EXIT = WM_USER + 224;
constexpr int WM_TASKDISPATCHERMSG = WM_USER + 225;

// Forward declarations
class CFolderAdd;
class CWizardPage;
class CWizardDlg;
class CUpdateDlg;
class CStatusDlg;
class DefaultLogger;
class UploadManager;
class UploadEngineManager;
class ScriptsManager;
class Win7JumpList;
class WtlGuiSettings;
class CFloatingWindow;
class WinServerIconCache;
#ifdef IU_ENABLE_NETWORK_DEBUGGER
class CNetworkDebugDlg;
class IconBitmapUtils;

#endif

class CWizardDlg :
    public CCustomDialogIndirectImpl<CWizardDlg>, public CUpdateUI<CWizardDlg>,
        public CMessageFilter, public CIdleHandler, public IDropTarget, public CRegionSelectCallback,
        public CUpdateDlg::CUpdateDlgCallback,
        public IProgramWindow,
        public ITaskRunner
{
public:
    enum { IDD = IDD_WIZARDDLG };
    enum { IDM_OPENSCREENSHOTS_FOLDER = 9889, IDM_OPENSERVERSCHECKER, IDM_NETWORKDEBUGGER, IDM_VIEWHISTORY };
    enum { kNewFilesTimer = 1 };
    static constexpr WPARAM kWmMyExitParam = 5;

    enum WizardPageId { wpWelcomePage = 0, wpVideoGrabberPage = 1, wpMainPage = 2, wpUploadSettingsPage = 3, wpUploadPage = 4 };

    struct AddImageStruct
    {
        CString RealFileName, VirtualFileName;
        bool show;
    };

    CWizardDlg(std::shared_ptr<DefaultLogger> logger, CMyEngineList* enginelist,
        UploadEngineManager* uploadEngineManager, UploadManager* uploadManager,
        ScriptsManager* scriptsManager, WtlGuiSettings* settings);
    ~CWizardDlg() override;

    void setFloatWnd(std::shared_ptr<CFloatingWindow> floatWnd);

    BOOL PreTranslateMessage(MSG* pMsg) override;
    BOOL OnIdle() override;

    BEGIN_UPDATE_UI_MAP(CWizardDlg)
    END_UPDATE_UI_MAP()

    BEGIN_MSG_MAP(CWizardDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ENABLE, OnEnable)
        //MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)
        MESSAGE_HANDLER(WM_MY_ADDIMAGE, OnAddImages)
        MESSAGE_HANDLER(WM_MY_SHOWPAGE, OnWmShowPage)
        MESSAGE_HANDLER(WM_MY_EXIT, OnWmMyExit)
        MESSAGE_HANDLER(WM_TASKDISPATCHERMSG, OnTaskDispatcherMsg)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(MYWM_ENABLEDROPTARGET, OnEnableDropTarget)
        MESSAGE_HANDLER(WM_APPCOMMAND, OnAppCommand)
        MESSAGE_HANDLER(WM_QUERYENDSESSION, OnQueryEndSession)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDPICHanged)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
        COMMAND_HANDLER(IDC_UPDATESLABEL, BN_CLICKED, OnUpdateClicked)
        COMMAND_HANDLER(IDM_OPENSCREENSHOTS_FOLDER, BN_CLICKED, OnOpenScreenshotFolderClicked)
#ifdef IU_ENABLE_SERVERS_CHECKER
        COMMAND_HANDLER(IDM_OPENSERVERSCHECKER, BN_CLICKED, OnServersCheckerClicked)
#endif
#ifdef IU_ENABLE_NETWORK_DEBUGGER
        COMMAND_HANDLER(IDM_NETWORKDEBUGGER, BN_CLICKED, OnNetworkDebuggerClicked)
#endif

        COMMAND_HANDLER(ID_PASTE, 1, OnPaste)
        COMMAND_RANGE_HANDLER(ID_HOTKEY_BASE, ID_HOTKEY_BASE +100, OnLocalHotkey);
        COMMAND_HANDLER_EX(IDC_PREV, BN_CLICKED, OnPrevBnClicked)
        COMMAND_HANDLER_EX(IDC_NEXT, BN_CLICKED, OnNextBnClicked)
        COMMAND_HANDLER(IDC_ABOUT, BN_CLICKED, OnBnClickedAbout)
        COMMAND_HANDLER(IDC_HELPBUTTON, BN_CLICKED, OnBnClickedHelpbutton)
        NOTIFY_HANDLER(IDC_HELPBUTTON, BCN_DROPDOWN, OnBnDropdownHelpButton)
        COMMAND_ID_HANDLER(IDC_DOCUMENTATION, OnDocumentation)
        COMMAND_ID_HANDLER(IDC_SHOWLOG, OnShowLog)
        COMMAND_ID_HANDLER(IDC_SETTINGS, OnShowSettings)
        COMMAND_ID_HANDLER(IDM_VIEWHISTORY, OnShowHistory)
        REFLECT_NOTIFICATIONS()

    END_MSG_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnAddImages(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWmShowPage(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnTaskDispatcherMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnEnable(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDPICHanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnUpdateClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnLocalHotkey(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWmMyExit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDocumentation(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnShowLog(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOpenScreenshotFolderClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
#ifdef IU_ENABLE_SERVERS_CHECKER
    LRESULT OnServersCheckerClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
#endif
    LRESULT OnEnableDropTarget(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBnClickedHelpbutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnAppCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnQueryEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#ifdef IU_ENABLE_NETWORK_DEBUGGER
    LRESULT OnNetworkDebuggerClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
#endif
    LRESULT OnBnDropdownHelpButton(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnShowSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnShowHistory(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    void CloseDialog(int nVal);
    bool CreatePage(WizardPageId PageID);
    void setSessionImageServer(const ServerProfileGroup& server);
    void setSessionFileServer(const ServerProfileGroup& server);
    ServerProfileGroup getSessionImageServer() const;
    ServerProfileGroup getSessionFileServer() const;

    // IProgramWindow methods
    void setServersChanged(bool changed) override;
    bool serversChanged() const;
    WindowHandle getHandle() override;
    WindowNativeHandle getNativeHandle() override;

    void ShowUpdateMessage(const CString& msg) override;

    void runInGuiThread(TaskRunnerTask task, bool async) override;

    template<class T> T* getPage(WizardPageId id) {
        if (id < 0 || id >= ARRAY_SIZE(Pages)) {
            return nullptr;
        }
        return dynamic_cast<T*>(Pages[id]);
    }
    void settingsChanged(BasicSettings* settings);
    bool pasteFromClipboard();
    bool ShowPage(WizardPageId idPage, int prev = -1, int next = -1);
    bool AddImage(const CString &FileName, const CString &VirtualFileName, bool Show=true);
    bool AddImageAsync(const CString &FileName, const CString &VirtualFileName, bool show);
    LRESULT OnPrevBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    LRESULT OnNextBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
    HBITMAP GenHeadBitmap(WizardPageId PageID) const;
    void PasteBitmap(HBITMAP);
    LRESULT OnBnClickedAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    void Exit();
    void ProcessDroppedFiles(HDROP hDrop);
    //CSavingOptions SavingOptions;
    bool LoadUploadEngines(const CString &filename, CString &Error);
    bool ParseCmdLine();
    bool CommonScreenshot(ScreenCapture::CaptureMode mode);
    // functions
    bool funcAddImages(bool AnyFiles = false);
    bool funcImportVideo();
    bool funcScreenshotDlg();
    bool funcScreenRecordingDlg();
    bool funcScreenRecording();
    bool funcRegionScreenshot(bool ShowAfter = true);
    bool funcFullScreenshot();
    bool funcWindowHandleScreenshot();
    bool funcTopWindowScreenshot();
    bool funcFreeformScreenshot();
    bool funcWindowScreenshot(bool Delay = false);
    bool funcLastRegionScreenshot();
    bool funcAddFolder();
    //bool funcPaste();
    bool funcSettings();
    bool funcHistory();
#ifdef IU_ENABLE_MEDIAINFO
    bool funcMediaInfo();
#endif
    bool funcAddFiles();
    bool funcDownloadImages();
    bool funcReuploadImages();
    bool funcShortenUrl();
    bool funcOpenScreenshotFolder();
    bool funcFromClipboard(bool fromCmdLine = false);
    bool funcExit(bool force = false);
    // end of functions

    bool executeFunc(CString funcName, bool fromCmdLine = false);
    void executeFuncLater(CString funcName);
    bool importVideoFile(const CString& fileName, int prevPage = 0);
    bool queryDropFiledescriptors(IDataObject* pDataObj, bool* enableOverlay = nullptr);
    bool HandleDropFiledescriptors(IDataObject *pDataObj);
    bool HandleDropHDROP(IDataObject *pDataObj);
    bool HandleDropBitmap(IDataObject *pDataObj);
    void setIsFirstRun(bool isFirstRun);

    bool CanShowWindow() override;
    void UpdateAvailabilityChanged(bool Available) override;
    bool startScreenRecording(const ScreenRecordingRuntimeParams& params, bool forceShowWizardAfter = false);

    //    IUnknown methods
    STDMETHODIMP_(ULONG) AddRef() override;

    STDMETHODIMP_(ULONG) Release() override;
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    //    IDropTarget methods
    STDMETHODIMP DragEnter(
        /* [unique][in] */ IDataObject *pDataObj,
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD *pdwEffect) override;
    STDMETHODIMP DragOver(
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD *pdwEffect) override;

    STDMETHODIMP DragLeave(void) override;
    STDMETHODIMP Drop(
        /* [unique][in] */ IDataObject *pDataObj,
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD *pdwEffect) override;

    void AddFolder(LPCTSTR szFolder, bool SubDirs = false);

    //CRegionSelectCallback
    void OnScreenshotFinished(int Result) override;
    void OnScreenshotSaving(LPTSTR FileName, Gdiplus::Bitmap* Bm) override;
    void CloseWizard(bool force = false);
    bool RegisterLocalHotkeys();
    bool UnRegisterLocalHotkeys();
    void CreateUpdateDlg();
    bool IsClipboardDataAvailable();
    void showLogWindowForFileName(CString fileName);
    bool hasLastScreenshotRegion() const;
    void setLastScreenshotRegion(std::shared_ptr<ScreenCapture::CScreenshotRegion> region, HMONITOR monitor);
    void addLastRegionAvailabilityChangeCallback(std::function<void(bool)> cb);
    bool getQuickUploadMarker() const;
    void setQuickUploadMarker(bool val);
    CString getLastVideoFile() const;
    void setLastVideoFile(CString fileName);
    bool isShowWindowSet() const;
    void beginAddFiles();
    void endAddFiles();
    void showScreenshotCopiedToClipboardMessage(std::shared_ptr<Gdiplus::Bitmap> resultBitmap, CString imageFilePath);
    bool checkFileFormats(const ServerProfileGroup& imageServer, const ServerProfileGroup& fileServer, ImageUploadParams defaultImageUploadParams);
    void showHelpButtonMenu(HWND control);
    bool isScreenRecorderRunning() const;
    void stopScreenRecording();
    bool trayIconEnabled() const;
    bool canExitApp() const;

protected:
    bool acceptsDragnDrop() const;
    void createIcons();
    CIcon windowIcon_;
    CIcon smallWindowIcon_;
    CHotkeyList m_hotkeys;
    CFolderAdd FolderAdd;

    UploadManager* uploadManager_;
    UploadEngineManager* uploadEngineManager_;
    ScriptsManager* scriptsManager_;
	std::unique_ptr<Win7JumpList> win7JumpList_;
    HwndScopedWrapper aboutButtonToolTip_;
    CString funcToExecuteLater_;
    CButton helpButton_;
    CIcon helpButtonIcon_;
    CStatic headBitmap_;
    long m_lRef = 0;
    bool QuickUploadMarker;
    CString LastVideoFile;
    bool m_bShowAfter;
    bool isFirstRun_;
    WtlGuiSettings& Settings;

    std::map<CString, std::unique_ptr<CLogWindow>> logWindowsByFileName_;
    std::vector<AddImageStruct> newImages_;
    std::mutex newImagesMutex_;
    std::shared_ptr<ScreenCapture::CScreenshotRegion> lastScreenshotRegion_;
    HMONITOR lastScreenshotMonitor_;
    std::vector<std::function<void(bool)>> lastRegionAvailabilityChangeCallbacks_;
    std::shared_ptr<DefaultLogger> logger_;
    std::shared_ptr<CStatusDlg> statusDlg_;
    std::vector<TaskRunnerTask> scheduledTasks_;
    std::mutex scheduledTasksMutex_;
    std::shared_ptr<CFloatingWindow> floatWnd_;
    DWORD mainThreadId_;
    std::unique_ptr<CUpdateDlg> updateDlg;
    CAccelerator localHotkeys_;
    ScreenshotInitiator screenshotInitiator_;
    bool m_bShowWindow;
    bool m_bHandleCmdLineFunc;
    ServerProfileGroup sessionImageServer_, sessionFileServer_;
    bool serversChanged_;
    int CurPage;
    int PrevPage, NextPage;
    bool DragndropEnabled;
    CStringList m_Paths;
    CWizardPage* Pages[5];
    CString m_bCurrentFunc;
    CMyEngineList* enginelist_;
    std::unique_ptr<WinServerIconCache> serverIconCache_;
    boost::signals2::connection settingsChangedConnection_;
    boost::signals2::signal<void(bool)> onRepeatScreenRecordingAvailabilityChanged_;
    CDragndropOverlay dragndropOverlay_;
    bool enableDragndropOverlay_ = false;
    std::optional<CDragndropOverlay::ItemId> dragndropOverlaySelectedItem_;
#ifdef IU_ENABLE_NETWORK_DEBUGGER
    std::unique_ptr<CNetworkDebugDlg> networkDebugDlg_;
#endif
    ScreenRecordingRuntimeParams screenRecordingParams_;
    boost::weak_ptr<ScreenRecorderWindow> screenRecorderWindow_;
    std::unique_ptr<CHistoryManager> historyManager_;
    CBitmap settingsBitmap_, historyBitmap_, helpBitmap_;
    std::unique_ptr<IconBitmapUtils> iconBitmapUtils_;
};



#endif // WIZARDDLG_H
