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

#include "WtlGuiSettings.h"

#include <cassert>

#include <Shlobj.h>

#include "Core/SettingsManager.h"
#include "Func/MyUtils.h"
#include "Func/CmdLine.h"
#include "3rdpart/Registry.h"
#include "Core/Video/VideoUtils.h"
#include "Func/WinUtils.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/Utils/StringUtils.h"
#include "Gui/Dialogs/FloatingWindow.h"
#include "Core/i18n/Translator.h"
#include "Core/ServiceLocator.h"
#include "Core/SearchByImage.h"
#include "Func/Common.h"
#include "StringConvert.h"
#include "Gui/Win7JumpList.h"

#define SETTINGS_FILE_NAME _T("settings.xml")

namespace {
// Do not edit this array
std::string MouseClickCommandIndexToString[] = { "", "contextmenu", "addimages", "addimages", "addfolder",
        "importvideo", "screenshotdlg", "regionscreenshot", "fullscreenshot",
        "windowscreenshot", "windowhandlescreenshot", "freeformscreenshot", "showmainwindow",
        "open_screenshot_folder", "settings", "paste", "downloadimages", "mediainfo", "mediainfo",
        "shortenurl", "shortenurlclipboard", "reuploadimages", "uploadfromclipboard", "lastregionscreenshot" };

void RunIuElevated(const CString& params) {
    SHELLEXECUTEINFO TempInfo = { 0 };

    TCHAR buf[MAX_PATH];
    GetModuleFileName(0, buf, MAX_PATH - 1);
    CString s = WinUtils::GetAppFolder();

    CString Command = CString(buf);
    TempInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    TempInfo.fMask = 0;
    TempInfo.hwnd = NULL;
    TempInfo.lpVerb = _T("runas");
    TempInfo.lpFile = Command;
    TempInfo.lpParameters = params;
    TempInfo.lpDirectory = s;
    TempInfo.nShow = SW_NORMAL;

    ::ShellExecuteEx(&TempInfo);
}

/*
This function starts a new process of Uptooda with admin rights (Windows Vista and later)
The created process registers shell extensions and terminates
*/
void ApplyRegistrySettings() {
    SHELLEXECUTEINFO TempInfo = { 0 };

    TCHAR buf[MAX_PATH];
    GetModuleFileName(0, buf, MAX_PATH - 1);
    CString s = WinUtils::GetAppFolder();

    CString Command = CString(buf);
    TempInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    TempInfo.fMask = 0;
    TempInfo.hwnd = NULL;
    TempInfo.lpVerb = _T("runas");
    TempInfo.lpFile = Command;
    TempInfo.lpParameters = _T(" /integration");
    TempInfo.lpDirectory = s;
    TempInfo.nShow = SW_NORMAL;

    ::ShellExecuteEx(&TempInfo);
}

#define MY_CLSID _T("{535E39BD-5883-454C-AFFC-C54B66B18206}")

bool RegisterClsId()
{
    TCHAR Buffer[MAX_PATH + 1] = _T("CLSID\\");
    HKEY Key = 0;

    lstrcat(Buffer, MY_CLSID);
    RegCreateKeyEx(HKEY_CLASSES_ROOT, Buffer, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &Key, NULL);

    if (!Key)
        return false;

    HKEY TempKey = 0;
    RegCreateKeyEx(Key, _T("LocalServer32"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &TempKey, NULL);

    GetModuleFileName(0, Buffer, MAX_PATH);

    RegSetValueEx(TempKey, 0, 0, REG_SZ, (LPBYTE)Buffer, (lstrlen(Buffer) + 1) * sizeof(TCHAR));

    RegCloseKey(TempKey);

    RegCreateKeyEx(Key, _T("ProgID"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &TempKey, NULL);
    lstrcpy(Buffer, _T("ImageUploader.ContextMenuHandler.1"));
    RegSetValueEx(TempKey, 0, 0, REG_SZ, (LPBYTE)Buffer, (lstrlen(Buffer) + 1) * sizeof(TCHAR));
    RegCloseKey(TempKey);

    RegCloseKey(Key);

    Key = 0;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), 0, KEY_WRITE,
        &Key) == ERROR_SUCCESS) {
        lstrcpy(Buffer, _T("ImageUploader ContextMenuHandler"));
        RegSetValueEx(Key, MY_CLSID, 0, REG_SZ, (LPBYTE)Buffer, (lstrlen(Buffer) + 1) * sizeof(TCHAR));
        RegCloseKey(Key);
    }
    return true;
}

bool UnRegisterClsId() // Deleting CLSID record from registry
{
    TCHAR Buffer[MAX_PATH + 1] = _T("CLSID\\");
    lstrcat(Buffer, MY_CLSID);
    return SHDeleteKey(HKEY_CLASSES_ROOT, Buffer) == ERROR_SUCCESS;
}

/* Obsolete function; will be removed in future */
int AddToExplorerContextMenu(LPCTSTR Extension, LPCTSTR Title, LPCTSTR Command, bool DropTarget) {
    HKEY ExtKey = NULL;
    TCHAR Buffer[MAX_PATH];

    Buffer[0] = _T('.');
    lstrcpy(Buffer + 1, Extension); // Формируем строку вида ".ext"
    RegCreateKeyEx(HKEY_CLASSES_ROOT, Buffer, 0, 0, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, 0, &ExtKey, NULL);

    TCHAR ClassName[MAX_PATH] = _T("\0");
    DWORD BufSize = sizeof(ClassName) / sizeof(TCHAR);
    DWORD Type = REG_SZ;
    RegQueryValueEx(ExtKey, 0, 0, &Type, (LPBYTE)&ClassName, &BufSize); // Retrieving classname for extension
    RegCloseKey(ExtKey);

    if (Title == 0) {
        // Deleting
        wsprintf(Buffer, _T("%s\\shell\\iuploader"), (LPCTSTR)ClassName);
        SHDeleteKey(HKEY_CLASSES_ROOT, Buffer);
        return 0;
    }

    wsprintf(Buffer, _T("%s\\shell\\iuploader\\command"), (LPCTSTR)ClassName);

    if (!lstrlen(Buffer))
        return false;
    wsprintf(Buffer, _T("%s\\shell\\iuploader"), (LPCTSTR)ClassName);
    DWORD res = RegCreateKeyEx(HKEY_CLASSES_ROOT, Buffer, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &ExtKey,
        NULL);

    if (res != ERROR_SUCCESS) {
        ServiceLocator::instance()->logger()->write(ILogger::logWarning, TR("Settings"),
            CString(TR("Cannot create registry key for extension: ")) +
            Extension + _T("\r\n") + WinUtils::ErrorCodeToString(res));
        return 0;
    }

    RegSetValueEx(ExtKey, 0, 0, REG_SZ, (LPBYTE)Title, (lstrlen(Title) + 1) * sizeof(TCHAR));

    HKEY CommandKey;

    if (RegCreateKeyEx(ExtKey, _T("command"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &CommandKey,
        NULL) != ERROR_SUCCESS) {
    }
    HKEY DropTargetKey;
    if (DropTarget) {
        if (RegCreateKeyEx(ExtKey, _T("DropTarget"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &DropTargetKey,
            NULL) != ERROR_SUCCESS) {
            return 0;
        }
    }

    RegCloseKey(ExtKey);

    RegSetValueEx(CommandKey, 0, 0, REG_SZ, (LPBYTE)Command, (lstrlen(Command) + 1) * sizeof(TCHAR));
    RegCloseKey(CommandKey);

    if (DropTarget) {
        RegSetValueEx(DropTargetKey, _T("Clsid"), 0, REG_SZ, (LPBYTE)MY_CLSID, (lstrlen(MY_CLSID) + 1) * sizeof(TCHAR));
        RegCloseKey(DropTargetKey);
    }

    return 1; // That's means ALL OK! :)
}

}

WtlGuiSettings::~WtlGuiSettings() {
}

void WtlGuiSettings::setFloatWnd(CFloatingWindow* floatWnd) {
    floatWnd_ = floatWnd;
}


CString WtlGuiSettings::getShellExtensionFileName() {
    return WinUtils::GetAppFolder() + (WinUtils::IsWindows64Bit() ? _T("ExplorerIntegration64.dll") : _T("ExplorerIntegration.dll"));
}

void WtlGuiSettings::RegisterShellExtension(bool Register) {
    CString moduleName = getShellExtensionFileName();
    if (!WinUtils::FileExists(moduleName)) {
        return;
    }

    CRegistry Reg;
    Reg.SetRootKey(HKEY_LOCAL_MACHINE);

    bool canCreateRegistryKey = Register;

    if (Reg.SetKey("Software\\Uptooda", canCreateRegistryKey)) {
        Reg.WriteBool("ExplorerContextMenu", Register);
        CRegistry Reg2;
        Reg2.SetWOW64Flag(KEY_WOW64_64KEY); // Indicates that an application on 64-bit Windows should operate on the 64-bit registry view.
        Reg2.SetRootKey(HKEY_LOCAL_MACHINE);
        Reg2.SetKey("Software\\Uptooda\\Strings", true);
        Reg2.WriteString(_T("UploadImages"), TR("Upload images"));
        Reg2.WriteString(_T("UploadFiles"), TR("Upload files"));
        Reg2.WriteString(_T("UploadImagesOnly"), TR("Upload images only"));
        Reg2.WriteString(_T("ImportVideoFile"), TR("Import Video File"));
        Reg2.WriteString(_T("InformationAboutFile"), TR("Information about file"));
    }

    SHELLEXECUTEINFO TempInfo = { 0 };
    CString s = WinUtils::GetAppFolder();
    TempInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    TempInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    TempInfo.hwnd = NULL;
    BOOL b = FALSE;
    WinUtils::IsElevated(&b);

    if (IsWindowsVistaOrGreater() && !b) {
        TempInfo.lpVerb = _T("runas");
    } else {
        TempInfo.lpVerb = _T("open");
    }

    TempInfo.lpFile = _T("regsvr32");
    CString parameters = CString((Register ? _T("") : _T("/u "))) + _T("/s \"") + moduleName + _T("\"");
    TempInfo.lpParameters = parameters;
    TempInfo.lpDirectory = s;
    TempInfo.nShow = SW_NORMAL;

    ::ShellExecuteEx(&TempInfo);
    if (TempInfo.hProcess) {
        WaitForSingleObject(TempInfo.hProcess, INFINITE);
        CloseHandle(TempInfo.hProcess);
    }
}

/*
Determine where data folder is situated
and store it's path into DataFolder member
*/
void WtlGuiSettings::FindDataFolder()
{
    AppRuntimeInfo* params = AppRuntimeInfo::instance();
    if (WinUtils::IsDirectory(WinUtils::GetAppFolder() + _T("Data"))) {
        DataFolder = WinUtils::GetAppFolder() + _T("Data\\");
        SettingsFolder = W2U(DataFolder);

        /*params->setDataDirectory(IuStringUtils::Replace(W2U(DataFolder), "\\", "/"));
        params->setSettingsDirectory(IuStringUtils::Replace(SettingsFolder, "\\", "/"));*/
        params->setDataDirectory(W2U(DataFolder));
        params->setSettingsDirectory(SettingsFolder);
        IsPortable = true;
        return;
    }

    SettingsFolder = IuCoreUtils::WstringToUtf8(static_cast<LPCTSTR>(WinUtils::GetApplicationDataPath() + _T("Uptooda\\")));

    params->setSettingsDirectory(SettingsFolder);
    {
        CRegistry Reg;

        Reg.SetRootKey(HKEY_CURRENT_USER);
        if (Reg.SetKey("Software\\Uptooda", false)) {
            CString dir = Reg.ReadString("DataPath");

            if (!dir.IsEmpty() && WinUtils::IsDirectory(dir)) {
                DataFolder = dir;
                params->setDataDirectory(IuCoreUtils::WstringToUtf8((LPCTSTR)DataFolder));
                return;
            }
        }
    }
    {
        CRegistry Reg;
        Reg.SetRootKey(HKEY_LOCAL_MACHINE);
        if (Reg.SetKey("Software\\Uptooda", false)) {
            CString dir = Reg.ReadString("DataPath");

            if (!dir.IsEmpty() && WinUtils::IsDirectory(dir)) {
                DataFolder = dir;
                params->setDataDirectory(IuCoreUtils::WstringToUtf8((LPCTSTR)DataFolder));
                return;
            }
        }
    }

    if (WinUtils::FileExists(WinUtils::GetCommonApplicationDataPath() + SETTINGS_FILE_NAME)) {
        DataFolder = WinUtils::GetCommonApplicationDataPath() + _T("Uptooda\\");
        params->setDataDirectory(IuCoreUtils::WstringToUtf8((LPCTSTR)DataFolder));
    } else

    {
        DataFolder = WinUtils::GetApplicationDataPath() + _T("Uptooda\\");
        params->setDataDirectory(IuCoreUtils::WstringToUtf8((LPCTSTR)DataFolder));
    }
}

void WtlGuiSettings::fixInvalidServers() {
    std::string defaultImageServer = engineList_->getDefaultServerNameForType(CUploadEngineData::TypeImageServer);
    std::string defaultImageServerProfileName;

    CUploadEngineData * defaultImageUED = engineList_->byName(defaultImageServer);
    if (!defaultImageUED) {
        defaultImageUED = engineList_->firstEngineOfType(CUploadEngineData::TypeImageServer);
        if (!defaultImageUED) {
            LOG(ERROR) << "Unable to find any image servers in the servers list";
        } else {
            defaultImageServer = defaultImageUED->Name;
        }
    }
    CUploadEngineData* ue = {};

    auto& imageServerItem = imageServer.getByIndex(0);
    ue = imageServerItem.uploadEngineData();
    if (!ue) {
        imageServerItem.setServerName(defaultImageServer);
        imageServerItem.setProfileName(defaultImageServerProfileName);
    }

    auto& contextMenuServerItem = contextMenuServer.getByIndex(0);
    ue = contextMenuServerItem.uploadEngineData();
    if (!ue) {
        contextMenuServerItem.setServerName(defaultImageServer);
        contextMenuServerItem.setProfileName(defaultImageServerProfileName);
    }

    auto& quickScreenshotServerItem = quickScreenshotServer.getByIndex(0);
    ue = quickScreenshotServerItem.uploadEngineData();
    if (!ue) {
        quickScreenshotServerItem.setServerName(defaultImageServer);
        quickScreenshotServerItem.setProfileName(defaultImageServerProfileName);
    }

    ue = temporaryServer.uploadEngineData();
    if (!ue) {
        temporaryServer.setServerName(defaultImageServer);
        temporaryServer.setProfileName(defaultImageServerProfileName);
    }

    std::string defaultFileServerName = engineList_->getDefaultServerNameForType(CUploadEngineData::TypeFileServer);
    CUploadEngineData* uploadEngineData = engineList_->byName(defaultFileServerName);

    if (!uploadEngineData) {
        uploadEngineData = engineList_->firstEngineOfType(CUploadEngineData::TypeFileServer);
        if (uploadEngineData) {
            defaultFileServerName = uploadEngineData->Name;
        } else {
            LOG(WARNING) << "Unable to find any file servers in the server list";
        }
    }

    auto& fileServerItem = fileServer.getByIndex(0);
    ue = fileServerItem.uploadEngineData();
    if (!ue) {
        fileServerItem.setServerName(defaultFileServerName);
        fileServerItem.setProfileName("");
    }

    ue = urlShorteningServer.uploadEngineData();
    if (!ue) {
        std::string defaultServerName = engineList_->getDefaultServerNameForType(
            CUploadEngineData::TypeUrlShorteningServer);
        CUploadEngineData* uploadEngineData = engineList_->byName(defaultServerName);
        if (uploadEngineData) {
            urlShorteningServer.setServerName(defaultServerName);
            urlShorteningServer.setProfileName("");
        } else {
            uploadEngineData = engineList_->firstEngineOfType(CUploadEngineData::TypeUrlShorteningServer);
            if (uploadEngineData) {
                urlShorteningServer.setServerName(uploadEngineData->Name);
                urlShorteningServer.setProfileName("");
            } else {
                LOG(ERROR) << "Unable to find any URL shortening servers in the server list";
            }
        }
    }

    ue = imageSearchServer.uploadEngineData();
    if (!ue) {
        std::string defaultServerName = engineList_->getDefaultServerNameForType(CUploadEngineData::TypeSearchByImageServer);
        CUploadEngineData* uploadEngineData = engineList_->byName(defaultServerName);

        if (uploadEngineData) {
            imageSearchServer.setServerName(defaultServerName);
            imageSearchServer.setProfileName("");
        } else {
            uploadEngineData = engineList_->firstEngineOfType(CUploadEngineData::TypeSearchByImageServer);
            if (uploadEngineData) {
                imageSearchServer.setServerName(uploadEngineData->Name);
                imageSearchServer.setProfileName("");
            } else {
                LOG(ERROR) << "Unable to find any reverse image search servers in the server list";
            }
        }
    }
}

WtlGuiSettings::WtlGuiSettings() :
    CommonGuiSettings(),
    floatWnd_(nullptr)
{
    IsPortable = false;
    FindDataFolder();
    if (!WinUtils::IsDirectory(DataFolder)) {
        CreateDirectory(DataFolder, 0);
    }
    if (!WinUtils::IsDirectory(IuCoreUtils::Utf8ToWstring(SettingsFolder).c_str())) {
        CreateDirectory(IuCoreUtils::Utf8ToWstring(SettingsFolder).c_str(), 0);
    }
    BOOL isElevated = false;
    WinUtils::IsElevated(&isElevated);
    if (isElevated || CmdLine.IsOption(L"afterupdate")) {
        WinUtils::MakeDirectoryWritable(DataFolder);
    }
    CString copyFrom = WinUtils::GetAppFolder() + SETTINGS_FILE_NAME;
    CString copyTo = DataFolder + SETTINGS_FILE_NAME;
    if (WinUtils::FileExists(copyFrom) && !WinUtils::FileExists(copyTo)) {
        MoveFile(copyFrom, copyTo);
    }

    ExplorerCascadedMenu = true;

    if (!IsFFmpegAvailable()) {
        VideoSettings.Engine = VideoEngineDirectshow;
    }

    WatchClipboard = true;

    *m_Directory = 0;
    UseTxtTemplate = false;
    GroupByFilename = false;
    UseDirectLinks = true;
    TrayResult = trJustURL;
    DropVideoFilesToTheList = false;
    ConfirmOnExit = 1;
    EnableToastNotifications = true;

    ExplorerContextMenu = false;
    ExplorerVideoContextMenu = true;
    ExplorerContextMenu_changed = false;
    ThumbsPerLine = 4;
    SendToContextMenu_changed = false;
    SendToContextMenu = 0;
    QuickUpload = 1;
    ParseSubDirs = 1;
    RememberImageServer = true;
    RememberFileServer = true;

    AutomaticallyCheckUpdates = true;
    CheckFileTypesBeforeUpload = true;
    ShowPreviewForVideoFiles = true;

    ImageEditorPath = _T("mspaint.exe \"%1\"");
    AutoCopyToClipboard = false;
    AutoShowLog = true;

    //    StringToFont(_T("Tahoma,7,b,204"), &ThumbSettings.ThumbFont);
    VideoSettings.Font = _T("Tahoma,12,,204");

    VideoSettings.Columns = 3;
    VideoSettings.TileWidth = 200;
    VideoSettings.GapWidth = 5;
    VideoSettings.GapHeight = 7;
    VideoSettings.NumOfFrames = 8;
    VideoSettings.JPEGQuality = 100;
    VideoSettings.ShowMediaInfo = TRUE;
    VideoSettings.TextColor = RGB(0, 0, 0);
    VideoSettings.SnapshotsFolder.Empty();
    VideoSettings.SnapshotFileTemplate = _T("%f%_%cx%_%cy%_%uid%\\grab_%i%.png");

    VideoSettings.Engine = IsFFmpegAvailable() ? VideoEngineAuto : VideoEngineDirectshow;

    MediaInfoSettings.InfoType = 0; // generate short summary

    ScreenshotSettings.Format = 1;
    ScreenshotSettings.Quality = 85;
    ScreenshotSettings.WindowHidingDelay = 200;
    ScreenshotSettings.Delay = 1;
    ScreenshotSettings.BrushColor = RGB(255, 0, 0);
    ScreenshotSettings.ShowForeground = false;
    ScreenshotSettings.FilenameTemplate = _T("screenshot %y-%m-%d %h-%n-%s %i");
    ScreenshotSettings.CopyToClipboard = false;
    ScreenshotSettings.RemoveCorners = !IsWindows8OrGreater();
    ScreenshotSettings.AddShadow = false;
    ScreenshotSettings.RemoveBackground = false;
    ScreenshotSettings.OpenInEditor = true;
    ScreenshotSettings.UseOldRegionScreenshotMethod = false;
    ScreenshotSettings.CaptureCursor = false;
    ScreenshotSettings.MonitorMode = -1/*kAllMonitors*/;

    //ScreenRecordingSettings.FFmpegSettings.FFmpegCLIPath = W2U(WinUtils::GetAppFolder() + LR"(ffmpeg.exe)");

    CString dir = WinUtils::GetSystemSpecialPath(CSIDL_MYVIDEO);
    CString capturesDir = dir + _T("Captures\\");
    ScreenRecordingSettings.OutDirectory = W2U(WinUtils::IsDirectory(capturesDir) ? capturesDir : dir);
    TrayIconSettings.LeftClickCommandStr = _T(""); // without action
    TrayIconSettings.LeftDoubleClickCommandStr = _T("showmainwindow");

    TrayIconSettings.RightClickCommandStr = _T("contextmenu");
    TrayIconSettings.MiddleClickCommandStr = _T("regionscreenshot");

    TrayIconSettings.DontLaunchCopy = true;
    TrayIconSettings.TrayScreenshotAction = TRAY_SCREENSHOT_OPENINEDITOR;

    ImageEditorSettings.BackgroundColor = Gdiplus::Color(255, 255, 255);
    ImageEditorSettings.ForegroundColor = Gdiplus::Color(255, 0, 0);

    ImageEditorSettings.StepBackgroundColor = Gdiplus::Color(0, 255, 138);
    ImageEditorSettings.StepForegroundColor = Gdiplus::Color(255, 255, 255);
    ImageEditorSettings.PenSize = 12;
    ImageEditorSettings.RoundingRadius = ImageEditorSettings.PenSize;
    ImageEditorSettings.BlurRadius = 4.0f;
    ImageEditorSettings.AllowAltTab = true;
    ImageEditorSettings.AllowEditingInFullscreen = false;
    WinUtils::StringToFont(_T("Arial,12,b,204"), &ImageEditorSettings.Font);
    ImageEditorSettings.ArrowType = 0; // default arrow

    ImageReuploaderSettings.PasteHtmlOnCtrlV = true;
    Hotkeys_changed = false;

    testFileName = W2U(WinUtils::GetAppFolder()) + "testfile.jpg";
    testUrl = "https://github.com/zenden2k/image-uploader/issues";

    BindToManager();
}

bool WtlGuiSettings::PostLoadSettings(SimpleXml &xml) {
    CommonGuiSettings::PostLoadSettings(xml);
    SimpleXmlNode settingsNode = xml.getRoot(rootName_).GetChild("Settings");

    if (Language == L"T\u00FCrk\u00E7e") {  //fixes
        Language = _T("Turkish");
    } else if (Language == L"\u0423\u043A\u0440\u0430\u0457\u043D\u0441\u044C\u043A\u0430") {
        Language = _T("Ukrainian");
    } else if (Language == L"\u0420\u0443\u0441\u0441\u043a\u0438\u0439") {
        Language = _T("Russian");
    }

    if (!settingsNode["Image"]["Format"].IsNull()) {
        // for compatibility with old version configuration file
        LoadConvertProfile("Old profile", settingsNode);
    }

    /*if (CmdLine.IsOption(_T("afterinstall"))) {
        SaveSettings();
    }*/

    // Migrating from 1.3.0 to 1.3.1 (added ImageEditor has been addded)
    if (settingsNode["ImageEditor"].IsNull()) {
        if (TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_UPLOAD) {
            TrayIconSettings.TrayScreenshotAction = TRAY_SCREENSHOT_OPENINEDITOR;
        }

    }

    // Migrating from 1.3.2 to 1.3.3
    // Keep tray icon mouse commands as strings
    if (!settingsNode["TrayIcon"].IsNull()) {
        SimpleXmlNode trayIconNode = settingsNode["TrayIcon"];
        std::pair<std::string, CString*> mapToValues[] = {
            {"LeftClickCommand", &TrayIconSettings.LeftClickCommandStr},
            {"LeftDoubleClickCommand", &TrayIconSettings.LeftDoubleClickCommandStr},
            {"MiddleClickCommand", &TrayIconSettings.MiddleClickCommandStr},
            {"RightClickCommand", &TrayIconSettings.RightClickCommandStr},
        };

        for (const auto& pr : mapToValues) {
            SimpleXmlNode commandNode = trayIconNode.GetChild(pr.first, false);
            if (!commandNode.IsNull()) {
                std::string text = commandNode.Text();
                if (!text.empty()) {
                    int CommandIndex = atoi(text.c_str());
                    if (CommandIndex >= 0 && CommandIndex < ARRAY_SIZE(MouseClickCommandIndexToString)
                        ) {
                        *pr.second = MouseClickCommandIndexToString[CommandIndex].c_str();
                    }
                }
            }
        }
    }


    LoadConvertProfiles(settingsNode.GetChild("Image").GetChild("Profiles"));

    if (UploadBufferSize == 65536) {
        UploadBufferSize = 1024 * 1024;
    }

    // Loading some settings from registry
    if (loadFromRegistry_) {
        CRegistry Reg;
        Reg.SetRootKey(HKEY_LOCAL_MACHINE);
        if (Reg.SetKey("Software\\Uptooda", false)) {
            ExplorerContextMenu = Reg.ReadBool("ExplorerContextMenu", false);

        } else {
            ExplorerContextMenu = false;
        }
    }
    CRegistry Reg;
    Reg.SetRootKey(HKEY_CURRENT_USER);
    if (Reg.SetKey("Software\\Uptooda", false)) {
        ExplorerCascadedMenu = Reg.ReadBool("ExplorerCascadedMenu", true);
        ExplorerVideoContextMenu = Reg.ReadBool("ExplorerVideoContextMenu", true);
    }
    CRegistry Reg2;
    Reg2.SetRootKey(HKEY_CURRENT_USER);
    if (Reg2.SetKey("Software\\Uptooda", false)) {
        AutoStartup = Reg2.ReadBool("AutoStartup", false);
    }

    if (std::find(std::begin(VideoEngines), std::end(VideoEngines), VideoSettings.Engine) == std::end(VideoEngines)) {
        VideoSettings.Engine = VideoEngineAuto;
    }

    if (VideoSettings.Engine == VideoEngineFFmpeg && !IsFFmpegAvailable()) {
        VideoSettings.Engine = VideoEngineDirectshow;
    }

    notifyChange();
    return true;
}



bool WtlGuiSettings::PostSaveSettings(SimpleXml &xml)
{
    CommonGuiSettings::PostSaveSettings(xml);

    SaveConvertProfiles(xml.getRoot(rootName_).GetChild("Settings").GetChild("Image").GetChild("Profiles"));

    /*SaveServerProfileGroup(uploading.GetChild("UrlShorteningServerGroup"), urlShorteningServer);
    SaveServerProfileGroup(uploading.GetChild("TemporaryServerGroup"), temporaryServer);*/


    //std::cerr << "Saving setting to "<< IuCoreUtils::WstringToUtf8((LPCTSTR)fileName_);
    CRegistry Reg;
    Reg.SetRootKey(HKEY_CURRENT_USER);
    // if(ExplorerContextMenu)
    {
        bool canCreateRegistryKey = (ExplorerContextMenu);
        if (Reg.SetKey("Software\\Uptooda", canCreateRegistryKey)) {
            if (ExplorerContextMenu) {
                Reg.WriteBool("ExplorerCascadedMenu", ExplorerCascadedMenu);
                Reg.WriteBool("ExplorerContextMenu", ExplorerContextMenu);
                Reg.WriteBool("ExplorerVideoContextMenu", ExplorerVideoContextMenu);
                Reg.WriteString("Language", Language);
            } else {
                Reg.DeleteValue("ExplorerCascadedMenu");
                Reg.DeleteValue("ExplorerContextMenu");
                Reg.DeleteValue("ExplorerVideoContextMenu");
                Reg.DeleteValue("Language");
            }
        }
    }
    /*else
    {
    //Reg.DeleteKey("Software\\Uptooda");
    }*/
    EnableAutostartup(AutoStartup);

    if (SendToContextMenu_changed || ExplorerContextMenu_changed) {
        AutoStartup_changed = false;
        BOOL b;
        if (IsWindowsVistaOrGreater() && WinUtils::IsElevated(&b) != S_OK) {
            // Start new elevated process
            ApplyRegistrySettings();
        } else {
            // Process has already admin rights
            ApplyRegSettingsRightNow();
        }
    }

    ExplorerContextMenu_changed = false;
    SendToContextMenu_changed = false;

    if (ShowTrayIcon_changed) {
        ShowTrayIcon_changed = false;
        assert(floatWnd_);
        if (ShowTrayIcon) {
            if (!CFloatingWindow::IsRunningFloatingWnd()) {
                CmdLine.AddParam(_T("/tray"));
                floatWnd_->CreateTrayIcon();
            }
        } else {
            HWND TrayWnd = FindWindow(0, _T("ImageUploader_TrayWnd"));
            if (TrayWnd) {
                ::SendMessage(TrayWnd, WM_CLOSETRAYWND, 0, 0);
            }
        }
    } else if (ShowTrayIcon) {
        HWND TrayWnd = FindWindow(0, _T("ImageUploader_TrayWnd"));
        if (TrayWnd)
            SendMessage(TrayWnd, WM_RELOADSETTINGS, (floatWnd_->m_hWnd) ? 1 : 0, (Hotkeys_changed) ? 0 : 1);
    }

    Hotkeys_changed = false;
    return true;
}

void WtlGuiSettings::BindToManager() {
    CommonGuiSettings::BindToManager();
    /* binding settings */
    SettingsNode& general = mgr_["General"];
    general.n_bind(LastUpdateTime);
    general.n_bind(Language);
    general.n_bind(ExplorerContextMenu);
    /*general.n_bind(ExplorerVideoContextMenu);
    general.n_bind(ExplorerCascadedMenu);*/


    general.n_bind(ConfirmOnExit);
    general.n_bind(SendToContextMenu);
    general.n_bind(ParseSubDirs);
    general.n_bind(ImageEditorPath);
    //general.n_bind(AutoStartup);
    general.n_bind(ShowTrayIcon);
    general.n_bind(AutoCopyToClipboard);
    general.n_bind(EnableToastNotifications);
    general.n_bind(AutoShowLog);
    general.n_bind(ImagesFolder);
    general.n_bind(VideoFolder);
    general.n_bind(WatchClipboard);
    general.n_bind(RememberFileServer);
    general.n_bind(RememberImageServer);
    general.n_bind(Hotkeys);
    general.n_bind(DeviceId);

    SettingsNode& screenshot = mgr_["Screenshot"];
    screenshot.nm_bind(ScreenshotSettings, Delay);
    screenshot.nm_bind(ScreenshotSettings, Format);
    screenshot.nm_bind(ScreenshotSettings, Quality);
    screenshot.nm_bind(ScreenshotSettings, ShowForeground);
    screenshot.nm_bind(ScreenshotSettings, FilenameTemplate);
    screenshot.nm_bind(ScreenshotSettings, Folder);
    screenshot.nm_bind(ScreenshotSettings, AddShadow);
    screenshot.nm_bind(ScreenshotSettings, RemoveBackground);
    screenshot.nm_bind(ScreenshotSettings, RemoveCorners);
    screenshot.nm_bind(ScreenshotSettings, CopyToClipboard);
    screenshot.nm_bind(ScreenshotSettings, BrushColor);
    screenshot.nm_bind(ScreenshotSettings, WindowHidingDelay);
    screenshot.nm_bind(ScreenshotSettings, CaptureCursor);
    screenshot.nm_bind(ScreenshotSettings, OpenInEditor);
    screenshot.nm_bind(ScreenshotSettings, UseOldRegionScreenshotMethod);
    screenshot.nm_bind(ScreenshotSettings, MonitorMode);

    SettingsNode& imageEditor = mgr_["ImageEditor"];
    imageEditor.nm_bind(ImageEditorSettings, ForegroundColor);
    imageEditor.nm_bind(ImageEditorSettings, BackgroundColor);
    imageEditor.nm_bind(ImageEditorSettings, StepBackgroundColor);
    imageEditor.nm_bind(ImageEditorSettings, StepBackgroundColor);
    imageEditor.nm_bind(ImageEditorSettings, PenSize);
    imageEditor.nm_bind(ImageEditorSettings, RoundingRadius);
    imageEditor.nm_bind(ImageEditorSettings, BlurRadius);
    imageEditor.nm_bind(ImageEditorSettings, ArrowType);
    imageEditor.nm_bind(ImageEditorSettings, Font);
    imageEditor.nm_bind(ImageEditorSettings, FillTextBackground);
    imageEditor.nm_bind(ImageEditorSettings, InvertSelection);
    imageEditor.nm_bind(ImageEditorSettings, DrawBorder);

    imageEditor.nm_bind(ImageEditorSettings, AllowAltTab);
    imageEditor.nm_bind(ImageEditorSettings, CloseWindowAfterActionInFullScreen);
    screenshot.nm_bind(ImageEditorSettings, AllowEditingInFullscreen);
    //screenshot.nm_bind(ImageEditorSettings, SearchEngine);
    SettingsNode& image = mgr_["Image"];
    image["CurrentProfile"].bind(CurrentConvertProfileName);

    /*SettingsNode& thumbnails = mgr_["Thumbnails"];
    thumbnails.nm_bind(ThumbSettings, FileName);
    thumbnails.nm_bind(ThumbSettings, CreateThumbs);
    thumbnails.nm_bind(ThumbSettings, ThumbWidth);
    thumbnails.nm_bind(ThumbSettings, ThumbHeight);
    thumbnails.nm_bind(ThumbSettings, ScaleByHeight);
    thumbnails.nm_bind(ThumbSettings, FrameColor);
    thumbnails.nm_bind(ThumbSettings, ThumbColor1);
    thumbnails.nm_bind(ThumbSettings, ThumbColor2);
    thumbnails.nm_bind(ThumbSettings, UseServerThumbs);
    thumbnails.nm_bind(ThumbSettings, ThumbAddImageSize);
    thumbnails.nm_bind(ThumbSettings, DrawFrame);
    thumbnails.nm_bind(ThumbSettings, Quality);
    thumbnails.nm_bind(ThumbSettings, Format);
    thumbnails.nm_bind(ThumbSettings, Text);
    thumbnails["Text"]["@Color"].bind(ThumbSettings.ThumbTextColor);
    thumbnails["Text"]["@Font"].bind(ThumbSettings.ThumbFont);
    thumbnails["Text"]["@TextOverThumb"].bind(ThumbSettings.TextOverThumb);
    thumbnails["Text"]["@ThumbAlpha"].bind(ThumbSettings.ThumbAlpha);*/

    SettingsNode& video = mgr_["VideoGrabber"];
    video.nm_bind(VideoSettings, Columns);
    video.nm_bind(VideoSettings, TileWidth);
    video.nm_bind(VideoSettings, GapWidth);
    video.nm_bind(VideoSettings, GapHeight);
    video.nm_bind(VideoSettings, NumOfFrames);
    video.nm_bind(VideoSettings, JPEGQuality);
    video.nm_bind(VideoSettings, ShowMediaInfo);
    video.nm_bind(VideoSettings, TextColor);
    video.nm_bind(VideoSettings, Font);
    video.nm_bind(VideoSettings, Engine);
    video.nm_bind(VideoSettings, SnapshotsFolder);
    video.nm_bind(VideoSettings, SnapshotFileTemplate);

    SettingsNode& mediaInfo = mgr_["MediaInfo"];
    mediaInfo.nm_bind(MediaInfoSettings, InfoType);
    mediaInfo.nm_bind(MediaInfoSettings, EnableLocalization);

    SettingsNode& tray = mgr_["TrayIcon"];
    tray.nm_bind(TrayIconSettings, LeftDoubleClickCommandStr);
    tray.nm_bind(TrayIconSettings, LeftClickCommandStr);
    tray.nm_bind(TrayIconSettings, RightClickCommandStr);
    tray.nm_bind(TrayIconSettings, MiddleClickCommandStr);
    tray.nm_bind(TrayIconSettings, DontLaunchCopy);
    tray.nm_bind(TrayIconSettings, TrayScreenshotAction);

    SettingsNode& history = mgr_["History"];
    history.nm_bind(HistorySettings, EnableDownloading);
    history.nm_bind(HistorySettings, HistoryConverted);

    SettingsNode& imageReuploader = mgr_["ImageReuploader"];
    imageReuploader.nm_bind(ImageReuploaderSettings, PasteHtmlOnCtrlV);

    SettingsNode& upload = mgr_["Uploading"];
    //    upload.n_bind(UrlShorteningServer);
    upload.n_bind(QuickUpload);
    upload.n_bind(CodeLang);
    upload.n_bind(ThumbsPerLine);
    upload.n_bind(UseDirectLinks);
    upload.n_bind(UseTxtTemplate);
    upload.n_bind(GroupByFilename);
    upload.n_bind(DropVideoFilesToTheList);
    upload.n_bind(CodeType);
    upload.n_bind(MaxThreads);
    upload.n_bind(ScriptFileName);
    upload.n_bind(ExecuteScript);
    upload.n_bind(DeveloperMode);
    upload.n_bind(AutomaticallyCheckUpdates);
    upload.n_bind(CheckFileTypesBeforeUpload);
    upload.n_bind(ShowPreviewForVideoFiles);
    upload.n_bind(TrayResult);
    /*urlShorteningServer.bind(upload["UrlShorteningServer"]);
    temporaryServer.bind(upload["TemporaryServer"]);*/

    ConvertProfiles["Default"] = ImageConvertingParams();
    CurrentConvertProfileName = "Default";
    upload.n_bind(UploadBufferSize);
    upload.n_bind(FileRetryLimit);

    upload.n_bind(ActionRetryLimit);

    SettingsNode& proxy = upload["Proxy"];
    proxy["@UseProxy"].bind(ConnectionSettings.UseProxy);
    proxy["@NeedsAuth"].bind(ConnectionSettings.NeedsAuth);
    proxy.nm_bind(ConnectionSettings, ServerAddress);
    proxy.nm_bind(ConnectionSettings, ProxyPort);
    proxy.nm_bind(ConnectionSettings, ProxyType);
    proxy.nm_bind(ConnectionSettings, ProxyUser);
    proxy.nm_bind(ConnectionSettings, ProxyPassword);

    SettingsNode& serversChecker = mgr_["ServersChecker"];
    serversChecker.n_bind(testFileName);
    serversChecker.n_bind(testUrl);
}

// The following code should  be deleted in next releases
void WtlGuiSettings::ApplyRegSettingsRightNow()
{
    // Applying Startup settings
    // EnableAutostartup(AutoStartup);
    RegisterShellExtension(ExplorerContextMenu);

    // if(SendToContextMenu_changed)
    {
        CString ShortcutName = WinUtils::GetSendToPath() + _T("Uptooda.lnk");

        if (SendToContextMenu) {
            if (WinUtils::FileExists(ShortcutName))
                DeleteFile(ShortcutName);

            WinUtils::CreateShortCut(ShortcutName, CmdLine.ModuleName(), WinUtils::GetAppFolder(), _T(
                " /fromcontextmenu /upload"), 0, SW_SHOW, CmdLine.ModuleName(), 0);
        } else {
            DeleteFile(ShortcutName);
        }
    }

    // if(ExplorerImagesContextMenu_changed || ExplorerVideoContextMenu_changed)
        {
            TCHAR szFileName[MAX_PATH + 8] = _T("\"");

            GetModuleFileName(0, szFileName + 1, MAX_PATH);
            lstrcat(szFileName, _T("\" \"%1\""));

            LPTSTR szList = _T("jpg\0jpeg\0png\0bmp\0gif");
            int Res;

            UnRegisterClsId();

            // if(ExplorerContextMenu_changed)
            {
                while ((*szList) != 0) {
                    Res =
                        AddToExplorerContextMenu(szList, (/*ExplorerImagesContextMenu?TR("Upload images"):*/ 0),
                        szFileName,
                        true);
                    szList += lstrlen(szList) + 1;
                }
            }

            // if( ExplorerVideoContextMenu_changed)
            {
                szList = VIDEO_FORMATS;
                while ((*szList) != 0) {
                    Res =
                        AddToExplorerContextMenu(szList, (/*ExplorerVideoContextMenu?TR("Open by Image Uploader"):*/ 0),
                        szFileName,
                        false);
                    szList += lstrlen(szList) + 1;
                }
            }
        }
}



bool WtlGuiSettings::LoadConvertProfiles(SimpleXmlNode root)
{
    std::vector<SimpleXmlNode> profiles;
    root.GetChilds("Profile", profiles);

    for (size_t i = 0; i < profiles.size(); i++) {
        LoadConvertProfile("", profiles[i]);
    }
    return true;
}

bool WtlGuiSettings::LoadConvertProfile(const CString& name, SimpleXmlNode profileNode)
{
    SettingsManager mgr;
    ImageConvertingParams params;
    std::string saveTo = profileNode.Attribute("Name");
    if (!name.IsEmpty())
        saveTo = WCstringToUtf8(name);
    SettingsNode& image = mgr["Image"];
    BindConvertProfile(image, params);
    mgr.loadFromXmlNode(profileNode);
    ConvertProfiles[Utf8ToWCstring(saveTo)] = params;
    return true;
}

bool WtlGuiSettings::SaveConvertProfiles(SimpleXmlNode root)
{
    std::map<CString, ImageConvertingParams>::iterator it;
    for (it = ConvertProfiles.begin(); it != ConvertProfiles.end(); ++it) {
        SimpleXmlNode profile = root.CreateChild("Profile");
        ImageConvertingParams& params = it->second;
        profile.SetAttribute("Name", WCstringToUtf8(it->first));
        SettingsManager mgr;
        SettingsNode& image = mgr["Image"];
        BindConvertProfile(image, params);
        mgr.saveToXmlNode(profile);
    }
    return true;
}

void WtlGuiSettings::BindConvertProfile(SettingsNode& image, ImageConvertingParams& params)
{
    image.nm_bind(params, Quality);
    image.nm_bind(params, Format);
    image["NewWidth"].bind(params.strNewWidth);
    image["NewHeight"].bind(params.strNewHeight);
    image.nm_bind(params, AddLogo);
    image.nm_bind(params, AddText);
    image.nm_bind(params, ResizeMode);
    image.nm_bind(params, SmartConverting);
    image.nm_bind(params, PreserveExifInformation);
    image.nm_bind(params, SkipAnimated);
    image["Logo"].bind(params.LogoFileName);
    image["Logo"]["@LogoPosition"].bind(params.LogoPosition);
    image["Logo"]["@LogoBlend"].bind(params.LogoBlend);
    image["Text"].bind(params.Text);
    image["Text"]["@TextPosition"].bind(params.TextPosition);
    image["Text"]["@TextColor"].bind(params.TextColor);
    image["Text"]["@StrokeColor"].bind(params.StrokeColor);
    image["Text"]["@Font"].bind(params.Font);
}

void WtlGuiSettings::Uninstall() {
    BOOL b;
    bool beforeInstall = CmdLine.IsOption(_T("beforeinstall"));
    if (IsWindowsVistaOrGreater() && WinUtils::IsElevated(&b) != S_OK) {
        CString command = _T("/uninstall");
        if (beforeInstall) {
            command += _T(" /beforeinstall");
        }

        RunIuElevated(command);
        return;
    }

    if (!beforeInstall) {
        AutoStartup = false;
        SendToContextMenu = false;
        RegisterShellExtension(false);
        EnableAutostartup(false);
        CRegistry Reg;
        Reg.SetRootKey(HKEY_CURRENT_USER);
        Reg.DeleteWithSubkeys("Software\\Uptooda\\ContextMenuItems");
        Reg.DeleteKey("Software\\Uptooda");
        Reg.SetRootKey(HKEY_LOCAL_MACHINE);

        Reg.DeleteKey("Software\\Uptooda");
        WinUtils::RemoveBrowserKey();

        CRegistry reg3;
        reg3.SetRootKey(HKEY_CURRENT_USER);
        reg3.SetWOW64Flag(KEY_WOW64_64KEY);
        reg3.DeleteKey("Software\\Uptooda\\Strings");
        reg3.DeleteKey("Software\\Uptooda");
        CString ShortcutName = WinUtils::GetSendToPath() + _T("Uptooda.lnk");
        DeleteFile(ShortcutName);
    }

    Win7JumpList jumplist;
    jumplist.DeleteJumpList();
}

void WtlGuiSettings::EnableAutostartup(bool enable) {
    CRegistry Reg;
    Reg.SetRootKey(HKEY_CURRENT_USER);
    bool canCreateRegistryKey = enable;

    if (Reg.SetKey("Software\\Uptooda", canCreateRegistryKey)) {
        if (enable) {
            Reg.WriteBool("AutoStartup", enable);
        } else {
            Reg.DeleteValue("AutoStartup");
        }
    }

    if (enable) {
        HKEY hKey;
        CString StartupCommand = _T("\"") + CmdLine.ModuleName() + _T("\" /tray");
        LONG lRet, lRetOpen;
        lRet = RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"),
            0, KEY_WRITE, &hKey);
        if (!lRet) {
            lRetOpen = RegSetValueEx(hKey, _T("ImageUploader"), NULL, REG_SZ, (BYTE*)(LPCTSTR)StartupCommand,
                (StartupCommand.GetLength() + 1) * sizeof(TCHAR));
        }
        RegCloseKey(hKey);
    } else {
        // deleting from Startup (autorun)
        HKEY hKey;
        LONG lRet = RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_WRITE,
            &hKey);
        RegDeleteValue(hKey, _T("ImageUploader"));
        RegCloseKey(hKey);
    }
}

void WtlGuiSettings::PostLoadServerProfile(ServerProfile& profile) {
    CommonGuiSettings::PostLoadServerProfile(profile);

    ThumbCreatingParams& th = profile.getImageUploadParamsRef().getThumbRef();
    if (!th.Width && !th.Height && th.Size) {
        if (th.ResizeMode == ThumbCreatingParams::trByWidth) {
            th.Width = th.Size;
            th.Size = 0;
        }
        else if (th.ResizeMode == ThumbCreatingParams::trByHeight) {
            th.Height = th.Size;
            th.Size = 0;
        }
    }
}

CString WtlGuiSettings::getServerName()  {
    return imageServer.isEmpty()? _T("") : Utf8ToWCstring(imageServer.getByIndex(0).serverName());
}
CString WtlGuiSettings::getQuickServerName()  {
    return contextMenuServer.isEmpty() ? _T("") : Utf8ToWCstring(contextMenuServer.getByIndex(0).serverName());
}
CString WtlGuiSettings::getFileServerName()  {
    return fileServer.isEmpty() ? _T("") : Utf8ToWCstring(fileServer.getByIndex(0).serverName());
}

CString WtlGuiSettings::getSettingsFileName() const
{
    return IuCoreUtils::Utf8ToWstring(fileName_).c_str();
}

ServerSettingsStruct& WtlGuiSettings::ServerByName(CString name)
{
    return ServersSettings[IuCoreUtils::WstringToUtf8((LPCTSTR)name)].begin()->second;
}

ServerSettingsStruct& WtlGuiSettings::ServerByUtf8Name(const std::string& name)
{
    return ServersSettings[name].begin()->second;
}
