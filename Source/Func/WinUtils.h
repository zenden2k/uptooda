#ifndef FUNC_WINUTILS_H
#define FUNC_WINUTILS_H

#include <optional>
#include <vector>
#include <string>

#include "atlheaders.h"
#include "Core/Utils/Win32Exception.h"

namespace Gdiplus {
    class Bitmap;
}

std::wstring Utf8ToWstring(const std::string &str);
#define MYRGB(a,color) Color(a,GetRValue(color),GetGValue(color),GetBValue(color))

#define WCstringToUtf8(str) WinUtils::wstostr((static_cast<LPCTSTR>(str)), CP_UTF8)
#define Utf8ToWCstring(str) CString(Utf8ToWstring(str).c_str())

namespace WinUtils {
    bool IsDirectory(LPCTSTR szFileName);
    bool IsWindows64Bit();
    bool IsWine();
    // Function that gets path to SendTo folder
    CString GetSendToPath();
    CString GetSystemSpecialPath(int csidl);
    CString GetApplicationDataPath();
    CString GetCommonApplicationDataPath();
    bool    CopyTextToClipboard(const CString& text);
    bool    GetClipboardText(CString &text, HWND hwnd = NULL, bool raiseError = false);
    bool CopyHtmlToClipboard(const CString& text, bool emptyClipboard = false);
    bool GetClipboardHtml(CString& text, CString& outSourceUrl);
    DWORD MsgWaitForSingleObject(HANDLE pHandle, DWORD dwMilliseconds);

    bool CreateShortCut(LPCWSTR pwzShortCutFileName, LPCTSTR pszPathAndFileName, LPCTSTR pszWorkingDirectory, LPCTSTR pszArguments, 
                        WORD wHotKey, int iCmdShow, LPCTSTR pszIconFileName, int iIconIndex);
    bool CreateFolder(LPCTSTR szFolder);
    bool CreateFilePath(LPCTSTR szFilePath);
    HICON GetAssociatedIcon (LPCTSTR filename, bool Small);

    // File path functions
    LPTSTR ExtractFilePath(LPCTSTR FileName,LPTSTR buf, size_t bufferSize);
    CString DoExtractFileName(const CString & fileName);
    LPCTSTR GetFileExt(LPCTSTR szFileName);
    CString GetOnlyFileName(const CString& szFilename);
    CString GetFilePath(const CString& strFullPath);
    bool FileExists(LPCTSTR FileName);
    CString TrimString(const CString& source, int nMaxLen);
    CString TrimStringEnd(const CString& source, int nMaxLen);
    bool IsStrInList(LPCTSTR szExt,LPCTSTR szList);
    bool NewBytesToString(int64_t nBytes, LPTSTR szBuffer, int nBufSize);
    bool ShowFilePropertiesDialog(HWND hWnd, const CString& fileName);
    CString IntToStr(int n);
    CString GetModuleFullName(HMODULE module = NULL);
    CString GetAppFolder();
    CString GetAppFileName();
    CString FormatWindowsErrorMessage(DWORD idCode);
    bool IsElevated();
    void DeleteDir2(LPCTSTR Dir);
    CString GetUniqFileName(const CString& filePath);
    size_t GetFolderFileList(std::vector<CString>& list, CString folder, CString mask);

    inline COLORREF RGB2COLORREF(unsigned int color) {
        return RGB(GetBValue(color), GetGValue(color), GetRValue(color));
    }

    inline unsigned int COLORREF2RGB( COLORREF color){
        return RGB(GetBValue(color), GetGValue(color), GetRValue(color));
    }

    bool FontToString(LOGFONT const * lFont, CString &Result);
    bool StringToFont(LPCTSTR szBuffer,LPLOGFONT lFont, HDC targetDc = nullptr);

    bool ExtractStrFromList(
        LPCTSTR szString /* Source string */,
        int nIndex, /* Zero based item index */
        LPTSTR szBuffer /* Destination buffer */,
        LONG nSize ,/* Length in characters of destination buffer */
        LPCTSTR szDefString = _T(""),
        TCHAR cSeparator = _T(',') /* Character to be separator in list */);
    CString StringSection(const CString& str,TCHAR sep, int index);

    float GetMonitorScaleFactor();
    CString GetLastErrorAsString();
    bool MakeDirectoryWritable(LPCTSTR lpPath);
    int GetInternetExplorerMajorVersion();
    void RemoveBrowserKey();
    void UseLatestInternetExplorerVersion(bool IgnoreIDocDirective = false);
    void TimerWait(int Delay);
    CString ConvertRelativePathToAbsolute(const CString& fileName);
    bool IsProcessRunning(DWORD pid);
    std::wstring strtows(const std::string &str, UINT codePage);
    std::string wstostr(const std::wstring &ws, UINT codePage);
    std::string AnsiToUtf8(const std::string &str, int codepage);
    std::string Utf8ToAnsi(const std::string &str, int codepage);
    CString GetProcessName(DWORD pid);
    CString ErrorCodeToString(DWORD idCode, HMODULE mod = NULL);
    CString ExpandEnvironmentStrings(const CString& s);
    void ArgvQuote(const std::wstring& argument, std::wstring& commandLine, bool force);
    bool GetProxyInfo(CString& proxyAddress, CString& proxyBypass);
    std::string TextToClipboardHtmlFormat(const char* html, size_t length, const std::string& base_url = std::string());
    bool DisplaySystemPrintDialogForImage(const std::vector<CString>& files, HWND hwnd = NULL);

    /**
     * @throws Win32Exception
     */
    bool ShellOpenFileOrUrl(CString path, HWND wnd = nullptr, CString directory = {}, bool throwOnError = false);
    bool ShowFileInFolder(CString fileName, HWND wnd = nullptr);
    bool ShowFilesInFolder(const std::vector<CString>& files, HWND wnd);
    //SYSTEMTIME SystemTimeAdd(const SYSTEMTIME& s, double seconds);
    time_t SystemTimeToTime(const SYSTEMTIME &s);
    bool CheckFileName(const CString& fileName);
    HRESULT IsElevated(/*__out_opt */ BOOL * pbElevated);
    CString TimestampToString(time_t t);
    BOOL TranslateAcceleratorForWindow(HWND hwnd, HACCEL hacc, LPMSG pmsg);
    std::optional<GUID> GenerateFakeUUIDv4(const GUID& baseGUID);
    CString GUIDToString(const GUID& guid);
    CString NormalizeLineEndings(const CString& text);
}

#endif
