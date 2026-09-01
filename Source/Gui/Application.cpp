/*
     Uptooda - program for uploading images/files to the Internet

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

#ifdef NDEBUG
#include <CrashRpt.h>

crash_rpt::CrashRpt g_crashRpt(
    "7b4202e6-8294-4be5-a18d-69c097167b46", // GUID assigned to this application.
    L"Uptooda", // Application name that will be used in message box.
    L"Sergey Svistunov" // Company name that will be used in message box.
);
#endif

#include "atlheaders.h"

#include <boost/filesystem/path.hpp>
#include <boost/locale.hpp>
#include <string_view>

#include "Gui/Dialogs/LogWindow.h"
#include "Gui/Dialogs/WizardDlg.h"
#include "Gui/Dialogs/FloatingWindow.h"
#include "Func/CmdLine.h"
#include "Func/WinUtils.h"
#include "Func/IuCommonFunctions.h"
#include "Core/Logging.h"
#include "Core/Logging/MyLogSink.h"
#include "Core/Upload/ScriptUploadEngine.h"
#include "Core/ServiceLocator.h"
#include "Func/DefaultUploadErrorHandler.h"
#include "Func/DefaultLogger.h"
#include "Func/WtlScriptDialogProvider.h"
#include "Core/AppRuntimeInfo.h"
#include "Func/LangClass.h"
#include "Func/GdiPlusInitializer.h"
#include "Gui/Dialogs/LangSelect.h"
#include "versioninfo.h"
#include "Core/Network/NetworkClientFactory.h"
#include "Core/Network/NetworkDebugger.h"
#include "Core/Scripting/ScriptsManager.h"
#include "Core/Upload/Filters/UserFilter.h"
#include "Core/Upload/Filters/ImageConverterFilter.h"
#include "Core/Upload/Filters/SizeExceedFilter.h"
#include "Core/Upload/Filters/UrlShorteningFilter.h"
#include "Core/Upload/Filters/ImageSearchFilter.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Core/3rdpart/xdgmime/xdgmime.h"
#include "Gui/Helpers/LangHelper.h"
#include "Video/MediaFoundationFrameGrabber.h"
#include "Core/3rdpart/dotenv.h"
#include "3rdpart/wintoastlib.h"

#ifndef NDEBUG
//#include <vld.h>
#endif

CAppModule _Module;

// If oleInitializer is destroyed before the WinToast singleton, this will cause an application crash.
// The linking order in C++ does NOT guarantee the destruction order of static objects across
// different translation units. So we need to call WinToast::instance()->clear() before exit and
// disable 'clear' function call in the WinToast destructor.
// TODO: do not use WinToast singleton at all

class COleInitialize {
public:
    COleInitialize() {
        m_hr = ::OleInitialize(NULL);
    }

    ~COleInitialize() {
        if (SUCCEEDED(m_hr)) {
            ::OleUninitialize();
        }
    }

    operator HRESULT() const { return m_hr; }
    HRESULT GetResult() const { return m_hr; }

private:
    HRESULT m_hr;
};

#ifndef NDEBUG
extern "C" const char* __asan_default_options() {
    return "continue_on_error=1";
}
#endif

COleInitialize oleInitializer;

class Application {
    CMessageLoop theLoop_;
    CLogWindow logWindow_;
    WtlGuiSettings settings_;
    CLang lang_;
    std::shared_ptr<CFloatingWindow> floatWnd_;
    std::unique_ptr<ScriptsManager> scriptsManager_;
    std::shared_ptr<DefaultLogger> logger_;
    std::unique_ptr<MyLogSink> myLogSink_;
    std::unique_ptr<UploadEngineManager> uploadEngineManager_;
    std::unique_ptr<UploadManager> uploadManager_;
    std::unique_ptr<CMyEngineList> engineList_;
    std::unique_ptr<ImageConverterFilter> imageConverterFilter_;
    std::unique_ptr<SizeExceedFilter> sizeExceedFilter_;
    std::shared_ptr<UrlShorteningFilter> urlShorteningFilter_;
    std::shared_ptr<ImageSearchFilter> imageSearchFilter_;
    std::unique_ptr<UserFilter> userFilter_;
    std::shared_ptr<WtlScriptDialogProvider> scriptDialogProvider_;
    std::unique_ptr<TaskDispatcher> taskDispatcher_;
    std::shared_ptr<NetworkDebugger> networkDebugger_;
    MediaFoundationInitializer mediaFoundationInitializer_;
    CString commonTempFolder_, tempFolder_;

public:
    Application() {
        srand(static_cast<unsigned>(time(nullptr)));
        setAppVersion();
        initBasicServices();
    }

    ~Application() {
        CScriptUploadEngine::DestroyScriptEngine();
        //ServiceLocator::instance()->setUploadManager(nullptr);
        logWindow_.DestroyWindow();
        _Module.RemoveMessageLoop();
        // Remove temporary files
        IuCommonFunctions::ClearTempFolder(tempFolder_);
        std::vector<CString> folders;
        WinUtils::GetFolderFileList(folders, commonTempFolder_, "iu_temp_*");
        for (const auto& folder : folders) {
            // Extract process ID from temp folder name
            CString pidStr = folder;
            pidStr.Replace(_T("iu_temp_"), _T(""));
            unsigned long pid = wcstoul(pidStr, 0, 16) ^ 0xa1234568;
            if (pid && !WinUtils::IsProcessRunning(pid))
                IuCommonFunctions::ClearTempFolder(commonTempFolder_ + _T("\\") + folder);
        }

        xdg_mime_set_dirs(nullptr);
        xdg_mime_shutdown();
        // deletes empty temp directory
        RemoveDirectory(commonTempFolder_);
    }

    static void setAppVersion() {
        AppRuntimeInfo::AppVersionInfo appVersion;
        appVersion.FullVersion = IU_APP_VER;
        appVersion.FullVersionClean = IU_APP_VER_CLEAN;
        appVersion.Build = atoi(IU_BUILD_NUMBER);
        appVersion.BuildDate = IU_BUILD_DATE;
        appVersion.CommitHash = IU_COMMIT_HASH;
        appVersion.CommitHashShort = IU_COMMIT_HASH_SHORT;
        appVersion.BranchName = IU_BRANCH_NAME;
        AppRuntimeInfo::instance()->setVersionInfo(appVersion);
    }

    void initBasicServices() {
        ServiceLocator* serviceLocator = ServiceLocator::instance();
        logger_ = std::make_shared<DefaultLogger>();
        myLogSink_ = std::make_unique<MyLogSink>(logger_.get());
        google::AddLogSink(myLogSink_.get());
        serviceLocator->setSettings(&settings_);
        serviceLocator->setNetworkClientFactory(std::make_shared<NetworkClientFactory>());

        _Module.AddMessageLoop(&theLoop_);
        logWindow_.Create(nullptr);
        logWindow_.setLogger(logger_.get());

        serviceLocator->setLogWindow(&logWindow_);
        serviceLocator->setLogger(logger_);
    }

    void initServices() {
        AbstractImage::autoRegisterFactory<void>();
        CString dataFolder = settings_.DataFolder;
        if (dataFolder.Right(1) == "\\") {
            dataFolder.Truncate(dataFolder.GetLength() - 1);
        }
        std::string dir = W2U(dataFolder);

        dotenv::init(dotenv::Preserve, (dir + "/.env").c_str());
        char* cacheDir = _strdup(dir.c_str());
        if (cacheDir) {
            const char* dirs[2]
                = { cacheDir, nullptr };
            xdg_mime_set_dirs(dirs);
            free(cacheDir);
        }

        if (dotenv::getenv("IU_CRASH_TEST") == "1") {
            *((int*)0) = 0;
        }

        ServiceLocator* serviceLocator = ServiceLocator::instance();
        taskDispatcher_ = std::make_unique<TaskDispatcher>(3);
        serviceLocator->setTaskDispatcher(taskDispatcher_.get());
        engineList_ = std::make_unique<CMyEngineList>();
        auto uploadErrorHandler = std::make_shared<DefaultUploadErrorHandler>(logger_, engineList_.get());
        serviceLocator->setUploadErrorHandler(uploadErrorHandler);
        scriptDialogProvider_ = std::make_shared<WtlScriptDialogProvider>();
        serviceLocator->setDialogProvider(scriptDialogProvider_.get());
        serviceLocator->setTranslator(&lang_);
        networkDebugger_ = std::make_shared<NetworkDebugger>();
        serviceLocator->setNetworkDebugger(networkDebugger_);
        scriptsManager_ = std::make_unique<ScriptsManager>(serviceLocator->networkClientFactory());

        serviceLocator->setEngineList(engineList_.get());
        serviceLocator->setMyEngineList(engineList_.get());
        settings_.setEngineList(engineList_.get());
        uploadEngineManager_ = std::make_unique<UploadEngineManager>(engineList_.get(), uploadErrorHandler, serviceLocator->networkClientFactory());
        uploadManager_ = std::make_unique<UploadManager>(uploadEngineManager_.get(), scriptsManager_.get(),
            uploadErrorHandler, serviceLocator->networkClientFactory(), &settings_, settings_.MaxThreads);
        serviceLocator->setUploadManager(uploadManager_.get());

        imageConverterFilter_ = std::make_unique<ImageConverterFilter>();
        sizeExceedFilter_ = std::make_unique<SizeExceedFilter>(engineList_.get(), uploadEngineManager_.get());
        urlShorteningFilter_ = std::make_shared<UrlShorteningFilter>();
        imageSearchFilter_ = std::make_shared<ImageSearchFilter>();
        userFilter_ = std::make_unique<UserFilter>(scriptsManager_.get());

        uploadManager_->addUploadFilter(imageConverterFilter_.get());
        uploadManager_->addUploadFilter(userFilter_.get());
        uploadManager_->addUploadFilter(sizeExceedFilter_.get());
        uploadManager_->addUploadFilter(urlShorteningFilter_.get());
        uploadManager_->addUploadFilter(imageSearchFilter_.get());

        serviceLocator->setUrlShorteningFilter(urlShorteningFilter_);
    }

    int Run(LPTSTR lpstrCmdLine, int nCmdShow)
    {
        for (const auto& CurrentParam : CmdLine) {
            if (CurrentParam.Left(12) == _T("/waitforpid=")) {
                CString pidStr = CurrentParam.Right(CurrentParam.GetLength() - 12);
                DWORD pid = _ttoi(pidStr);
                HANDLE hProcess = OpenProcess(SYNCHRONIZE, false, pid);
                WaitForSingleObject(hProcess, 20000);
                CloseHandle(hProcess);

                // Workaround for version prior to 1.1.7
                if (!CmdLine.IsOption(_T("update")) && !CmdLine.IsOption(L"afterupdate")) {
                    settings_.FindDataFolder();
                    if (!WinUtils::IsDirectory(settings_.DataFolder + "Thumbnails\\") || !WinUtils::IsDirectory(WinUtils::GetAppFolder() + "Docs\\")) {
                        SimpleXml xml;
                        std::string updateFile = WCstringToUtf8(settings_.DataFolder + "Update\\iu_core.xml");
                        if (xml.LoadFromFile(updateFile)) {
                            SimpleXmlNode root = xml.getRoot("UpdateInfo", false);
                            if (!root.IsNull()) {
                                int64_t timestamp = root.AttributeInt64("TimeStamp");
                                if (timestamp >= 0) {
                                    root.SetAttribute("TimeStamp", timestamp - 1);
                                    xml.SaveToFile(updateFile);
                                    CmdLine.AddParam(L"/update");
                                }
                            }
                        }
                    }
                }
            }
            else if (CurrentParam == "/debuglog") {
                FLAGS_logtostderr = false;
                FLAGS_alsologtostderr = true;
                logWindow_.Show();
            }
        }

        // for Windows Vista and later versions
        if (CmdLine.IsOption(_T("integration"))) {
            settings_.LoadSettings("", "", false);
            lang_.LoadLanguage(settings_.Language);
            settings_.ApplyRegSettingsRightNow();
            CScriptUploadEngine::DestroyScriptEngine();
            return 0;
        }

        IuCommonFunctions::CreateTempFolder(commonTempFolder_, tempFolder_);

        AppRuntimeInfo::instance()->setTempDirectory(W2U(tempFolder_));
        std::vector<CString> fileList;
        WinUtils::GetFolderFileList(fileList, WinUtils::GetAppFolder() + _T("\\"), _T("*.old"));
        for (const auto& file : fileList) {
            DeleteFile(WinUtils::GetAppFolder() + file);
        }

        settings_.LoadSettings();

        GdiPlusInitializer gdiPlusInitializer;
        initServices();

        CWizardDlg  dlgMain(logger_, engineList_.get(), uploadEngineManager_.get(), uploadManager_.get(), scriptsManager_.get(), &settings_);
        auto* serviceLocator = ServiceLocator::instance();
        serviceLocator->setProgramWindow(&dlgMain);
        serviceLocator->setTaskRunner(&dlgMain);

        floatWnd_ = std::make_shared<CFloatingWindow>(&dlgMain, uploadManager_.get(), uploadEngineManager_.get());
        dlgMain.setFloatWnd(floatWnd_);
        settings_.setFloatWnd(floatWnd_.get());


        DWORD DlgCreationResult = 0;
        bool ShowMainWindow = true;

        if (CmdLine.IsOption(_T("uninstall"))) {
            settings_.Uninstall();
            return 0;
        }

        bool BecomeTray = false;
        if (settings_.ShowTrayIcon && !CmdLine.IsOption(_T("tray"))) {
            if (!CFloatingWindow::IsRunningFloatingWnd()) {
                BecomeTray = true;
                CmdLine.AddParam(_T("/tray"));
            }
        }

        bool bCreateFloatingWindow = false;

        if (CmdLine.IsOption(_T("tray")) || BecomeTray) {
            if (!CFloatingWindow::IsRunningFloatingWnd()) {
                bCreateFloatingWindow = true;
                ShowMainWindow = BecomeTray;
            }
            else {
                return 0;
            }
        }

        lang_.SetDirectory(WinUtils::GetAppFolder() + "Lang\\");
        bool isFirstRun = settings_.Language.IsEmpty() || FALSE;
        for (size_t i = 0; i < CmdLine.GetCount(); i++) {
            CString CurrentParam = CmdLine[i];
            if (CurrentParam.Left(10) == _T("/language=")) {
                CString shortLanguageName = CurrentParam.Right(CurrentParam.GetLength() - 10);
                auto languageList{ LangHelper::instance()->getLanguageList((WinUtils::GetAppFolder() + "Lang").GetString()) };

                auto it = languageList.find(W2U(shortLanguageName));

                if (it != languageList.end()) {
                    settings_.Language = U2W(it->first);
                }

                /*CString foundName = lang_.getLanguageFileNameForLocale(shortLanguageName);
                if (!foundName.IsEmpty()) {
                    settings_.Language = foundName;
                }*/
            }
        }

        if (isFirstRun) {
            CLangSelect LS;
            if (LS.DoModal(nullptr) == IDCANCEL) {
                //*DlgCreationResult = 1;
                return 0;
            }
            settings_.Language = LS.getLanguage();

            lang_.LoadLanguage(settings_.Language);
            settings_.SaveSettings();
        }
        else {
            std::map<CString, CString> oldLangs = {
            {_T("Arabic"), _T("ar")},
            {_T("Farsi"), _T("fa")},
            {_T("Hrvatski"), _T("hr")},
            {_T("Hungarian"), _T("hu")},
            {_T("Romanian"), _T("ro")},
            {_T("Russian"), _T("ru")},
            {_T("Serbian"), _T("sr")},
            {_T("Swedish"), _T("sv")},
            {_T("Turkish"), _T("tr")},
            {_T("Ukrainian"), _T("uk")},
            };
            auto it = oldLangs.find(settings_.Language);
            if (it != oldLangs.end()) {
                settings_.Language = it->second;
            }
            lang_.LoadLanguage(settings_.Language);
        }

        //AppRuntimeInfo::instance()->setLanguageFile(W2U(lang_.getCurrentLanguageFile()));

        if (lang_.isRTL()) {
            SetProcessDefaultLayout(LAYOUT_RTL);
        }

        dlgMain.setIsFirstRun(isFirstRun);
        if (dlgMain.Create(0, reinterpret_cast<LPARAM>(&DlgCreationResult)) == NULL) {
            ATLTRACE(_T("Main dialog creation failed!  :( sorry\n"));
            dlgMain.m_hWnd = 0;
            return 0;
        }
        if (DlgCreationResult != 0) {
            dlgMain.DestroyWindow();
            dlgMain.m_hWnd = 0;
            return 0;
        }

        if (bCreateFloatingWindow) {
            floatWnd_->CreateTrayIcon();
        }
        if (/* ( CmdLine.GetCount() > 1 && CmdLine.IsOption( _T("quickshot") )*/
            CmdLine.IsOption(_T("mediainfo")) || !ShowMainWindow || !dlgMain.isShowWindowSet()) {
            dlgMain.ShowWindow(SW_HIDE);
        }
        else {
            dlgMain.ShowWindow(nCmdShow);
        }

        int nRet = theLoop_.Run();
        if (dlgMain.m_hWnd) {
            dlgMain.DestroyWindow();
        }

        return nRet;
    }
};

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
    // Create and install global locale
    std::locale::global(boost::locale::generator().generate(""));
    // Make boost.filesystem use it
    boost::filesystem::path::imbue(std::locale());

#if defined(_WIN32) && !defined(NDEBUG)
    // These global strings in GLOG are initially reserved with a small
    // amount of storage space (16 bytes). Resizing the string larger than its
    // initial size, after the _CrtMemCheckpoint call, can be reported as
    // a memory leak.
    // So for 'debug builds', where memory leak checking is performed,
    // reserve a large enough space so the string will not be resized later.
    // For these variables, _MAX_PATH should be fine.
    FLAGS_log_dir.reserve(_MAX_PATH);  // comment out this line to trigger false memory leak
    FLAGS_log_link.reserve(_MAX_PATH);

    // Enable memory dump from within VS.

#endif
    FLAGS_logtostderr = true;

    constexpr std::wstring_view LOG_DIR_OPTION = L"/log_dir=";
    for (const auto& argument : CmdLine) {
        const std::wstring_view argumentView(argument.GetString(), argument.GetLength());
        if (argumentView.compare(0, LOG_DIR_OPTION.size(), LOG_DIR_OPTION) == 0) {
            FLAGS_log_dir = IuCoreUtils::WstringToSystemLocale(std::wstring(argumentView.substr(LOG_DIR_OPTION.size())));
            FLAGS_logtostderr = false;
            break;
        }
    }

    google::InitGoogleLogging(WCstringToUtf8(WinUtils::GetAppFileName()).c_str());
    google::SetLogDestination(google::GLOG_WARNING, "");
    google::SetLogDestination(google::GLOG_ERROR, "");
    google::SetLogDestination(google::GLOG_FATAL, "");

    LOG(INFO) << "Application started";
    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc( nullptr, 0, 0, 0L );

    AtlInitCommonControls( ICC_BAR_CLASSES | ICC_USEREX_CLASSES  );    // add flags to support other controls

    HRESULT hRes = _Module.Init( nullptr, hInstance );
    int nRet;
    ATLASSERT( SUCCEEDED( hRes ) );
    {
        Application app;
        nRet = app.Run(lpstrCmdLine, nCmdShow);
    }
    _Module.Term();

    //google::RemoveLogSink(&logSink);
    google::ShutdownGoogleLogging();
    return nRet;
}
