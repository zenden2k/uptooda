#include "IuCommonFunctions.h"

#include <random>

#include "WinUtils.h"
#include "Core/Utils/CoreUtils.h"
#include "3rdpart/Registry.h"
#include "Core/Utils/CryptoUtils.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/Utils/StringUtils.h"

namespace IuCommonFunctions {
int screenshotIndex = 1;

CString GetDataFolder() {
    CString result = U2W(AppRuntimeInfo::instance()->dataDirectory());

    if (result.Right(1) != "\\" && result.Right(1) != "/") {
        result += "\\";
    }
    return result;
}

BOOL CreateTempFolder(CString& IUCommonTempFolder, CString& IUTempFolder) {
    TCHAR ShortPath[1024];
    GetTempPath(ARRAY_SIZE(ShortPath), ShortPath);
    TCHAR TempPath[1024];
    if (!GetLongPathName(ShortPath, TempPath, ARRAY_SIZE(TempPath))) {
        lstrcpy(TempPath, ShortPath);
    }
    DWORD pid = GetCurrentProcessId() ^ 0xa1234568;
    IUCommonTempFolder.Format(_T("%stmd_iu_temp"), static_cast<LPCTSTR>(TempPath));

    if (!CreateDirectory(IUCommonTempFolder, 0)) {
        DWORD errorCode = GetLastError();
        if (errorCode != ERROR_ALREADY_EXISTS) {
            LOG(ERROR) << "Unable to create temp folder: " << std::endl
                << IUCommonTempFolder << std::endl << WinUtils::ErrorCodeToString(errorCode);
            return false;
        }
    }
    IUTempFolder.Format(_T("%s\\iu_temp_%x"), static_cast<LPCTSTR>(IUCommonTempFolder), pid);

    if (!CreateDirectory(IUTempFolder, 0)) {
        DWORD errorCode = GetLastError();
        if (errorCode != ERROR_ALREADY_EXISTS) {
            LOG(ERROR) << "Unable to create temp folder: " << std::endl
                << IUTempFolder << std::endl << WinUtils::ErrorCodeToString(errorCode);
            return false;
        }
    }

    IUTempFolder += _T("\\");
    return TRUE;
}

WIN32_FIND_DATA wfd;
HANDLE findfile = 0;

int GetNextImgFile(LPCTSTR folder, CString& szBuffer) {
    CString buffer2 = folder + CString(_T("*.*"));

    if (!findfile) {
        findfile = FindFirstFile(buffer2, &wfd);
        if (!findfile)
            goto error;
    } else {
        if (!FindNextFile(findfile, &wfd))
            goto error;
    }
    if (lstrlen(wfd.cFileName) < 1)
        goto error;

    szBuffer = wfd.cFileName;

    return TRUE;

error:
    if (findfile)
        FindClose(findfile);
    return FALSE;
}

void ClearTempFolder(CString folder) {
    if (folder.IsEmpty()) {
        return;
    }
    int lastCharPos = folder.GetLength() - 1;
    if (folder[lastCharPos] != _T('\\') && folder[lastCharPos] != _T('/')) {
        folder += _T("\\");
    }
    CString buffer;
    CString buffer2;

    findfile = 0;
    while (GetNextImgFile(folder, buffer)) {
#ifdef DEBUG
        if (buffer == _T("log.txt")) {
            continue;
        }
#endif
        if (buffer == _T(".") || buffer == _T("..")) {
            continue;
        }
        buffer2 = CString(folder) + buffer;
        DeleteFile(buffer2);
    }
    if (!RemoveDirectory(folder)) {
        WinUtils::DeleteDir2(folder);
    }
}

CString FindDataFolder() {
    CString DataFolder;
    if (WinUtils::IsDirectory(WinUtils::GetAppFolder() + _T("Data"))) {
        DataFolder = WinUtils::GetAppFolder() + _T("Data\\");
        return DataFolder;
    }

#if !defined(IU_SERVERLISTTOOL) && !defined  (IU_CLI)
    {
        CRegistry Reg;

        Reg.SetRootKey(HKEY_CURRENT_USER);
        if (Reg.SetKey(_T("Software\\Uptooda"), false)) {
            CString dir = Reg.ReadString(_T("DataPath"));

            if (!dir.IsEmpty() && WinUtils::IsDirectory(dir)) {
                DataFolder = dir;
                return DataFolder;
            }
        }
    }
    {
        CRegistry Reg;
        Reg.SetRootKey(HKEY_LOCAL_MACHINE);
        // Unable to use wow64 flag because of Registry Virtualization enabled in the explorer.exe process
        CString keyStr =
#ifdef _WIN64
            _T("Software\\Wow6432Node\\Uptooda");
#else
        _T("Software\\Uptooda");
#endif

        if (Reg.SetKey(keyStr, false)) {
            CString dir = Reg.ReadString(_T("DataPath"));
            if (!dir.IsEmpty() && WinUtils::IsDirectory(dir)) {
                DataFolder = dir;
                return DataFolder;
            }
        }
    }

    if (WinUtils::FileExists(WinUtils::GetCommonApplicationDataPath() + L"Settings.xml")) {
        DataFolder = WinUtils::GetCommonApplicationDataPath() + _T("Uptooda\\");
    } else
#endif

    {
        DataFolder = WinUtils::GetApplicationDataPath() + _T("Uptooda\\");
    }
    return DataFolder;
}

CString GenerateFileName(const CString& templateStr, int index, const CPoint& size, time_t t, const CString& originalName) {
    static std::mt19937 mt{std::random_device{}()};
    CString result = templateStr;
    tm* timeinfo = localtime(&t);
    CString indexStr;
    CString day, month, year;
    CString hours, seconds, minutes;
    indexStr.Format(_T("%03d"), index);
    const std::thread::id threadId = std::this_thread::get_id();
    std::uniform_int_distribution<int> dist(0, 100);
    CString md5 = Utf8ToWstring(IuCoreUtils::CryptoUtils::CalcMD5HashFromString(
        IuCoreUtils::ThreadIdToString(threadId) + std::to_string(GetTickCount() + dist(mt)))).c_str();
    result.Replace(_T("%md5"), md5);
    result.Replace(_T("%width%"), WinUtils::IntToStr(size.x));
    result.Replace(_T("%height%"), WinUtils::IntToStr(size.y));
    year.Format(_T("%04d"), 1900 + timeinfo->tm_year);
    month.Format(_T("%02d"), timeinfo->tm_mon + 1);
    day.Format(_T("%02d"), timeinfo->tm_mday);
    hours.Format(_T("%02d"), timeinfo->tm_hour);
    seconds.Format(_T("%02d"), timeinfo->tm_sec);
    minutes.Format(_T("%02d"), timeinfo->tm_min);
    result.Replace(_T("%y"), year);
    result.Replace(_T("%m"), month);
    result.Replace(_T("%d"), day);
    result.Replace(_T("%h"), hours);
    result.Replace(_T("%n"), minutes);
    result.Replace(_T("%s"), seconds);
    result.Replace(_T("%i"), indexStr);
    return result;
}

CString MakeScreenshotFileName(const ScreenshotData& screenshotData, SIZE size) {
    const auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    const ScreenshotSettingsStruct& screenshotSettings = settings->ScreenshotSettings;
    CString suggestingFileName;
    if (!screenshotSettings.Folder.IsEmpty()) {
        suggestingFileName = screenshotSettings.Folder;
        TCHAR lastChar = suggestingFileName[suggestingFileName.GetLength() - 1];
        if (lastChar != _T('\\') && lastChar != _T('/')) {
            suggestingFileName += _T("\\");
        }
    }

    suggestingFileName += GenerateFileName(screenshotSettings.FilenameTemplate, screenshotData.index, size, screenshotData.time);

    if (screenshotSettings.Folder.IsEmpty()) {
        suggestingFileName = WinUtils::DoExtractFileName(suggestingFileName);
    }

    return suggestingFileName;
}

const std::unordered_set<std::string> supportedImageExtensions = {
    "jpg", "jpeg", "jpe", "jif", "jfif", "png", "bmp", "gif", "tif", "tiff", "webp", "heic", "heif", "avif", "emf",
    "wmf"
};

bool IsImage(LPCTSTR szFileName) {
    if (!szFileName) {
        return false;
    }
    LPCTSTR szExt = WinUtils::GetFileExt(szFileName);
    if (!lstrlen(szExt)) {
        return false;
    }
    CString find = szExt;
    find.MakeLower();
    return supportedImageExtensions.find(W2U(find)) != supportedImageExtensions.end();
}

bool IsImage(const std::string& fileName) {
    if (fileName.empty()) {
        return false;
    }
    std::string ext = IuCoreUtils::ExtractFileExt(fileName);
    if (ext.empty()) {
        return false;
    }
    ext = IuStringUtils::ToLower(ext);
    return supportedImageExtensions.find(ext) != supportedImageExtensions.end();
}

const std::unordered_set<std::string>& GetSupportedImageExtensions() {
    return supportedImageExtensions;
}

CString PrepareFileDialogImageFilter() {
    CString result;
    for (const auto& item : GetSupportedImageExtensions()) {
        result += _T("*.");
        result += U2W(item);
        result += _T(";");
    }
    return result;
}
}
