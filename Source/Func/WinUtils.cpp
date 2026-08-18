#include "WinUtils.h"

#include <sstream>

#include <Aclapi.h>
#include <ComDef.h>
#include <strsafe.h>
#include <TlHelp32.h>
#include <Winhttp.h>
#include <boost/format.hpp>
#include <wincrypt.h>

#include "3rdpart/GdiplusH.h"
#include "3rdpart/Registry.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Gui/Dialogs/HistoryWindow.h"
#include "Core/Utils/Win32Exception.h"

namespace WinUtils {

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

// Function that gets path to SendTo folder
CString GetSendToPath() {
    return GetSystemSpecialPath(CSIDL_SENDTO);
}

CString GetApplicationDataPath()
{
    return GetSystemSpecialPath(CSIDL_APPDATA);
}

CString GetCommonApplicationDataPath()
{
    return GetSystemSpecialPath(CSIDL_COMMON_APPDATA);
}

bool GetClipboardText(CString& text, HWND hwnd, bool raiseError)
{
    for (int i = 0; i < 4; i++) {
        if (OpenClipboard(hwnd)) {
            HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
            if (!hglb) {
                //if (OpenClipboard(hwnd)) {
                    hglb = GetClipboardData(CF_TEXT);
                //}
               
                if (!hglb) {
                    if (raiseError) {
                        LOG(ERROR) << "GetClipboardData call failed. ErrorCode=" << ::GetLastError();
                        //CloseClipboard();
                    }
                } else {
                    auto lpstr = static_cast<LPCSTR>(GlobalLock(hglb));
                    if (lpstr) {
                        text = lpstr;
                        GlobalUnlock(hglb);
                    }
                    CloseClipboard();
                    return true;
                }
                CloseClipboard();
                return false;
            }
            auto lpstr = static_cast<LPCWSTR>(GlobalLock(hglb));
            if (lpstr) {
                text = lpstr;
                GlobalUnlock(hglb);
            }
            CloseClipboard();
            return true;
        }
        Sleep(50); // Clipboard may be owned by another application, wait and try again
    }
   
    if ( raiseError ) {
        DWORD lastError = ::GetLastError();
        CString message;
        HWND clipboardOwner = GetClipboardOwner();
        if ( clipboardOwner ) {
            CString windowTitle = GuiTools::GetWindowText(clipboardOwner);
            TCHAR windowClassName[256] = _T("");
            GetClassName(clipboardOwner, windowClassName, 255);
            DWORD proccessId;
            GetWindowThreadProcessId(clipboardOwner, &proccessId);
            message += _T("\r\n");
            message += _T("Clipboard is owned by window:\r\n");
            message += _T("Title: '") + windowTitle + _T("'\r\n");
            message += _T("Class: '") + CString(windowClassName) + _T("'\r\n");
            message += _T("Process: '") + WinUtils::GetProcessName(proccessId) + _T("' (PID=") + IntToStr(proccessId) + _T(")\r\n");
        }
        LOG(ERROR) << "OpenClipboard call failed. ErrorCode=" << lastError << message;
    }
    return false;
}

bool CopyTextToClipboard(const CString& text) {
    const int cch = text.GetLength();
    if (!OpenClipboard(NULL)) {
        return false;
    }

    EmptyClipboard();
    HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (cch + 1) * sizeof(TCHAR));

    if (hglbCopy == NULL) {
        CloseClipboard();
        return false;
    }

    auto lptstrCopy = static_cast<LPTSTR>(GlobalLock(hglbCopy));
    memcpy(lptstrCopy, text.GetString(), text.GetLength() * sizeof(TCHAR));
    lptstrCopy[cch] = 0;
    GlobalUnlock(hglbCopy);
    SetClipboardData(CF_UNICODETEXT, hglbCopy);
    CloseClipboard();
    return true;
}

bool CopyHtmlToClipboard(const CString& text, bool emptyClipboard)
{
    if (!OpenClipboard(NULL)) {
        return false;
    }

    if (emptyClipboard) {
        EmptyClipboard();
    }

    const std::string textUtf8 = W2U(text);
    const std::string html = TextToClipboardHtmlFormat(textUtf8.c_str(), textUtf8.length());
    unsigned htmlClipboardFormatId = RegisterClipboardFormat(_T("HTML Format"));
    size_t cch = html.size();
    HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, cch + 1);

    if (hglbCopy == NULL) {
        CloseClipboard();
        return false;
    }

    auto lptstrCopy = static_cast<LPSTR>(GlobalLock(hglbCopy));

    if (lptstrCopy == NULL) {
        CloseClipboard();
        return false;
    }

    memcpy(lptstrCopy, html.c_str(), cch);
    lptstrCopy[cch] = 0;
    GlobalUnlock(hglbCopy);
    SetClipboardData(htmlClipboardFormatId, hglbCopy);
    CloseClipboard();
    return true;
}

std::string TextToClipboardHtmlFormat(const char* html, size_t length, const std::string& base_url) {
    // chromium//src/ui/base/clipboard/clipboard_util_win.cc
#define MAX_DIGITS 10
#define MAKE_NUMBER_FORMAT_1(digits) MAKE_NUMBER_FORMAT_2(digits)
#define MAKE_NUMBER_FORMAT_2(digits) "%0" #digits "u"
#define NUMBER_FORMAT MAKE_NUMBER_FORMAT_1(MAX_DIGITS)

    static const char* header = "Version:0.9\r\n"
        "StartHTML:" NUMBER_FORMAT "\r\n"
        "EndHTML:" NUMBER_FORMAT "\r\n"
        "StartFragment:" NUMBER_FORMAT "\r\n"
        "EndFragment:" NUMBER_FORMAT "\r\n";
    static const char* source_url_prefix = "SourceURL:";

    static const char* start_markup =
        "<html>\r\n<body>\r\n<!--StartFragment-->";
    static const char* end_markup =
        "<!--EndFragment-->\r\n</body>\r\n</html>";

    // Calculate offsets
    size_t start_html_offset = strlen(header) - strlen(NUMBER_FORMAT) * 4 +
        MAX_DIGITS * 4;
    if (!base_url.empty()) {
        start_html_offset += strlen(source_url_prefix) +
            base_url.length() + 2;  // Add 2 for \r\n.
    }
    size_t start_fragment_offset = start_html_offset + strlen(start_markup);
    size_t end_fragment_offset = start_fragment_offset + length;
    size_t end_html_offset = end_fragment_offset + strlen(end_markup);

    std::string result = str(boost::format(header) %
        start_html_offset %
        end_html_offset %
        start_fragment_offset %
        end_fragment_offset);
    if (!base_url.empty()) {
        result.append(source_url_prefix);
        result.append(base_url);
        result.append("\r\n");
    }
    result.append(start_markup);
    result.append(html, length);
    result.append(end_markup);

#undef MAX_DIGITS
#undef MAKE_NUMBER_FORMAT_1
#undef MAKE_NUMBER_FORMAT_2
#undef NUMBER_FORMAT
    return result;
}

DWORD MsgWaitForSingleObject(HANDLE pHandle, DWORD dwMilliseconds)
{
    while ((MsgWaitForMultipleObjects(1, &pHandle, FALSE, dwMilliseconds,
        /*QS_ALLEVENTS*/ QS_SENDMESSAGE)) != WAIT_OBJECT_0)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 1;
}


// Создание ярлыка
// Входные параметры:
//  pwzShortCutFileName - путь и имя ярлыка, например, "C:\\Блокнот.lnk"
//  Если не указан путь, ярлык будет создан в папке, указанной в следующем параметре.
//  Прим.: Windows сама НЕ добавляет к имени расширение .lnk
//  pszPathAndFileName  - путь и имя exe-файла, например, "C:\\Windows\\NotePad.Exe"
//  pszWorkingDirectory - рабочий каталог, например, "C:\\Windows"
//  pszArguments        - аргументы командной строки, например, "C:\\Doc\\Text.Txt"
//  wHotKey             - горячая клавиша, например, для Ctrl+Alt+A     HOTKEY(HOTKEYF_ALT|HOTKEYF_CONTROL,'A')
//  iCmdShow            - начальный вид, например, SW_SHOWNORMAL
//  pszIconFileName     - путь и имя файла, содержащего иконку, например, "C:\\Windows\\NotePad.Exe"
//  int iIconIndex      - индекс иконки в файле, нумеруется с 0
bool CreateShortCut(
                                         LPCWSTR pwzShortCutFileName,
                                         LPCTSTR pszPathAndFileName,
                                         LPCTSTR pszWorkingDirectory,
                                         LPCTSTR pszArguments,
                                         WORD wHotKey,
                                         int iCmdShow,
                                         LPCTSTR pszIconFileName,
                                         int iIconIndex)
{
    IShellLink* pSL;
    IPersistFile* pPF;
    HRESULT hRes;
    CoInitialize(NULL);

    // Получение экземпляра компонента "Ярлык"
    hRes = CoCreateInstance(CLSID_ShellLink, 0,  CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID*>(&pSL));

    if ( SUCCEEDED(hRes) )
    {
        hRes = pSL->SetPath(pszPathAndFileName);
        if ( SUCCEEDED(hRes) )
        {
            hRes = pSL->SetArguments(pszArguments);
            // if( SUCCEEDED(hRes) )
            {
                hRes = pSL->SetWorkingDirectory(pszWorkingDirectory);
                if ( SUCCEEDED(hRes) )
                {
                    hRes = pSL->SetIconLocation(pszIconFileName, iIconIndex);
                    if ( SUCCEEDED(hRes) )
                    {
                        //    hRes = pSL->SetHotkey(wHotKey);
                        //    if( SUCCEEDED(hRes) )
                        {
                            hRes = pSL->SetShowCmd(iCmdShow);
                            if ( SUCCEEDED(hRes) )
                            {
                                // Получение компонента хранилища параметров
                                hRes = pSL->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&pPF));
                                if ( SUCCEEDED(hRes) )
                                {
                                    // Сохранение созданного ярлыка
                                    hRes = pPF->Save(pwzShortCutFileName, TRUE);
                                    pPF->Release();
                                }
                            }
                        }
                    }
                }
            }
        }
        pSL->Release();
    }
    return SUCCEEDED(hRes);
}

bool CreateFolder(LPCTSTR szFolder)
{
    if (!szFolder || !lstrlen(szFolder))
        return FALSE;

    DWORD dwAttrib = GetFileAttributes(szFolder);

    // already exists ?
    if (dwAttrib != 0xffffffff)
        return ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

    // recursively create from the top down
    TCHAR* szPath = _tcsdup(szFolder);
    TCHAR* p = _tcsrchr(szPath, '\\');

    if (p)
    {
        // The parent is a dir, not a drive
        *p = '\0';

        // if can't create parent
        if (!CreateFolder(szPath))
        {
            free(szPath);
            return FALSE;
        }
        free(szPath);

        if (!::CreateDirectory(szFolder, NULL))
            return FALSE;
    }

    return TRUE;
}

bool CreateFilePath(LPCTSTR szFilePath)
{
    TCHAR* szPath = _tcsdup(szFilePath);
    TCHAR* p = _tcsrchr(szPath, '\\');

    BOOL bRes = FALSE;

    if (p)
    {
        *p = '\0';

        bRes = CreateFolder(szPath);
    }

    free(szPath);

    return bRes!=FALSE;
}

HICON GetAssociatedIcon (LPCTSTR filename, bool Small)
{
    SHFILEINFO Info;
    DWORD Flags;

    if (Small)
        Flags = SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES;
    else
        Flags = SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES /* | SHGFI_ADDOVERLAYS*/;
    SHGetFileInfo (filename, FILE_ATTRIBUTE_NORMAL, &Info, sizeof(Info), Flags);
    return Info.hIcon;
}

bool IsDirectory(LPCTSTR szFileName)
{
    DWORD res = GetFileAttributes(szFileName);
    return (res&FILE_ATTRIBUTE_DIRECTORY) && (res != INVALID_FILE_ATTRIBUTES);
}

bool IsWine()
{
    HMODULE hDll = LoadLibrary(_T("ntdll.dll"));

    if (hDll)
    {
        bool res =  GetProcAddress(hDll, "wine_get_version") != 0;
        FreeLibrary(hDll);
        return res;
    }
    return false;
}

bool IsWindows64Bit()
{
    SYSTEM_INFO si;
    PGNSI pGNSI;
    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
    pGNSI = reinterpret_cast<PGNSI>(GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GetNativeSystemInfo"));
    if (NULL != pGNSI) {
        pGNSI(&si);
    } else {
        GetSystemInfo(&si);
    }
    return si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64
        || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64
           || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64
    ;
}

CString GetSystemSpecialPath(int csidl)
{
    CString result;
    LPITEMIDLIST pidl;
    TCHAR szSendtoPath [MAX_PATH];
    LPMALLOC pMalloc;    

    if (SUCCEEDED( SHGetSpecialFolderLocation ( NULL, csidl, &pidl )))
    {
        if (SHGetPathFromIDList(pidl, szSendtoPath))
        {
            result = szSendtoPath;
        }

        if (SUCCEEDED(SHGetMalloc(&pMalloc)))
        {
            pMalloc->Free ( pidl );
            pMalloc->Release();
        }
    }
    if (result.Right(1) != _T("\\"))
        result += _T("\\");
    return result;
}

CString FormatWindowsErrorMessage(DWORD idCode) {
    LPVOID lpMsgBuf = nullptr;
    DWORD len = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        idCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPTSTR>(&lpMsgBuf),
        0,
        nullptr);

    if (len == 0 || lpMsgBuf == nullptr) {
        DWORD formatErr = GetLastError();
        CString res;
        res.Format(_T("Unknown error (code %lu, FormatMessage failed with %lu)"), idCode, formatErr);
        if (lpMsgBuf != nullptr)
            LocalFree(lpMsgBuf);
        return res;
    }

    CString res = static_cast<LPCTSTR>(lpMsgBuf);
    LocalFree(lpMsgBuf);

    res.TrimRight(_T("\r\n"));

    return res;
}

bool FileExists(LPCTSTR FileName)
{
    return FileName && GetFileAttributes(FileName) != INVALID_FILE_ATTRIBUTES;
}

CString TrimString(const CString& source, int nMaxLen)
{
    int nLen = source.GetLength();
    if(nLen <= nMaxLen) return source;

    int PartSize = (nMaxLen-3) / 2;
    return source.Left(PartSize)+_T("...")+source.Right(PartSize);
}

CString TrimStringEnd(const CString& source, int nMaxLen) {
    int nLen = source.GetLength();
    if (nLen <= nMaxLen) return source;

    int PartSize = nMaxLen - 3;
    return source.Left(PartSize) + _T("...");
}

CString GetAppFolder()
{
    CString szFileName;
    CString szPath;
    HINSTANCE inst;
#if defined(IU_WTL)
    inst =  _Module.GetModuleInstance();
#else
    inst = GetModuleHandle(0);
#endif
    szFileName = GetModuleFullName(inst);
    ExtractFilePath(szFileName, szPath.GetBuffer(szFileName.GetLength() + 1), szFileName.GetLength() + 1);
    szPath.ReleaseBuffer();
    return szPath;
}

CString GetModuleFullName(HMODULE module) {
    std::unique_ptr<TCHAR[]> buf;
    DWORD  bufLen = MAX_PATH;
    DWORD  retLen; 
    CString res;

    while (32768 >= bufLen) {
        buf.reset(new TCHAR[bufLen]);

        if ((retLen = GetModuleFileName(module, buf.get(), bufLen)) == 0) {
            /* GetModuleFileName failed */
            break;
        } else if (bufLen > retLen) {
            /* Success */
            res = buf.get();
            break;
        }

        bufLen *= 2;
    }

    return res;
}

CString GetAppFileName() {
    return GetModuleFullName(nullptr);
}

LPTSTR ExtractFilePath(LPCTSTR FileName, LPTSTR buf, size_t bufferSize)
{  
    int i, len = lstrlen(FileName);
    for(i=len; i>=0; i--)
    {
        if (FileName[i] == _T('\\') || FileName[i] == _T('/'))
            break;
    }
    lstrcpyn(buf, FileName, (std::min)(i + 2, static_cast<int>(bufferSize)-1));
    return buf;
}

CString DoExtractFileName(const CString& fileName) {
    int i;
    const int len = fileName.GetLength();

    for (i = len - 1; i >= 0; i--) {
        if (fileName[i] == _T('\\') || fileName[i] == _T('/'))
            break;
    }

    return fileName.Right(len - i - 1);
}

//  Function doesn't allocate new string, it returns  pointer
//        to a part of source string
LPCTSTR GetFileExt(LPCTSTR szFileName)
{
    if(!szFileName) return 0;
    int nLen = lstrlen(szFileName);

    LPCTSTR szReturn = szFileName+nLen;
    for( int i=nLen-1; i>=0; i-- )
    {
        if(szFileName[i] == '.')
        {
            szReturn = szFileName + i + 1;
            break;
        }
        else if(szFileName[i] == '\\' || szFileName[i] == '/') break;
    }
    return szReturn;
}

CString GetFilePath(const CString& strFullPath)
{
    int nPos = strFullPath.ReverseFind(_T('\\'));
    if (nPos == -1)
    {
        nPos = strFullPath.ReverseFind(_T('/'));
    }

    if (nPos != -1)
    {
        return strFullPath.Left(nPos);
    }

    return {};
}

bool IsStrInList(LPCTSTR szExt,LPCTSTR szList)
{
    if(!szList || !szExt) return false;

    while ((*szList)!=0)
    {
        if(lstrcmpi(szExt,szList)==0) return true;
        szList += lstrlen(szList)+1;
    }
    return false;
}

CString GetOnlyFileName(const CString& szFilename) {
    CString tempName = DoExtractFileName(szFilename);
    int dotPos = tempName.ReverseFind(_T('.'));
    if(dotPos != -1) {
        tempName=tempName.Left(dotPos);
    }

    return tempName;
}

CString IntToStr(int n)
{
    CString Result;
    Result.Format(_T("%d"), n);
    return Result;
}

bool NewBytesToString(int64_t nBytes, LPTSTR szBuffer, int nBufSize)
{
    std::string res = IuCoreUtils::FileSizeToString(nBytes);
    StringCchCopy(szBuffer, nBufSize, IuCoreUtils::Utf8ToWstring(res).c_str());
    return true;
}

bool IsElevated() 
{
    BOOL pbElevated = false;
    ATLASSERT( IsWindowsVistaOrGreater() );

    HRESULT hResult = E_FAIL; // assume an error occured
    HANDLE hToken  = NULL;

    if ( !::OpenProcessToken(
        ::GetCurrentProcess(),
        TOKEN_QUERY,
        &hToken ) )
    {
        ATLASSERT( FALSE );
        return true;
    }

    TOKEN_ELEVATION te = { 0 };
    DWORD dwReturnLength = 0;

    if ( !::GetTokenInformation(
        hToken,
        static_cast<TOKEN_INFORMATION_CLASS>(TokenElevation),
        &te,
        sizeof(te),
        &dwReturnLength ) )
    {
        ATLASSERT( FALSE );
    }
    else
    {
        ATLASSERT( dwReturnLength == sizeof(te) );

        hResult = te.TokenIsElevated ? S_OK : S_FALSE;

        //if ( pbElevated)
            pbElevated = (te.TokenIsElevated != 0);
    }

    ::CloseHandle( hToken );

    return pbElevated!=FALSE;
}


void DeleteDir2(LPCTSTR Dir)
{
    if (!Dir)
        return;
    TCHAR szBuffer[MAX_PATH];
    StringCchCopy(szBuffer, MAX_PATH, Dir);

    int nLen = lstrlen(szBuffer) - 1;
    if (nLen >= 0 && szBuffer[nLen] == _T('\\'))
        szBuffer[nLen] = 0;

    SHFILEOPSTRUCT FileOp;
    ZeroMemory(&FileOp, sizeof(FileOp));
    FileOp.hwnd = NULL;
    FileOp.wFunc = FO_DELETE;
    FileOp.pFrom = szBuffer;
    FileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    SHFileOperation(&FileOp);
}

CString GetUniqFileName(const CString& filePath)
{
    TCHAR path[MAX_PATH];
    if (!WinUtils::FileExists(filePath))
        return filePath;
    WinUtils::ExtractFilePath(filePath, path, MAX_PATH);
    CString name;
    name = WinUtils::GetOnlyFileName(filePath);
    CString extension = WinUtils::GetFileExt(filePath);
    CString result;
    for (int i = 2;; i++)
    {
        result = path + name + WinUtils::IntToStr(i) + (extension.IsEmpty() ? _T("") : _T(".") + extension);
        if (!WinUtils::FileExists(result))
            break;
    }
    return result;
}

size_t GetFolderFileList(std::vector<CString>& list, CString folder, CString mask) {
    WIN32_FIND_DATA wfd;
    ZeroMemory(&wfd, sizeof(wfd));
    HANDLE findfile = 0;

    for (;;) {
        if (!findfile) {
            findfile = FindFirstFile(folder + _T("\\") + mask, &wfd);
            if (!findfile)
                break;
        }
        else {
            if (!FindNextFile(findfile, &wfd))
                break;
        }
        if (lstrlen(wfd.cFileName) < 1)
            break;

        list.emplace_back(wfd.cFileName);
    }

    if (findfile)
        FindClose(findfile);

    return list.size();
}

bool FontToString(const LOGFONT * lFont, CString &Result)
{
    CString buffer;
    if( !lFont  ) return false;

    int nPixelsPerInch;
    int nFontSize;
    //HDC hScreenDC;
    bool bBold = false;
    bool bItalic = false;
    bool bUnderline = false;
    bool bStrikeOut = false;
    TCHAR szBold[2][2]={_T("\0"),_T("b")};
    TCHAR szItalic[2][2]={_T("\0"),_T("i")};
    TCHAR szUnderline[2][2]={_T("\0"),_T("u")};
    TCHAR szStrikeOut[2][2]={_T("\0"),_T("s")};

    HDC dc = ::GetDC(0);
    nPixelsPerInch = GetDeviceCaps(dc , LOGPIXELSY );
    ReleaseDC(0, dc);

    if( nPixelsPerInch <= 0 ) return false;

    nFontSize = - MulDiv( lFont->lfHeight, 72, nPixelsPerInch );

    if(lFont->lfWeight >= FW_BOLD) bBold = true;

    if(lFont->lfUnderline == (char)true) bUnderline = true;
    if(lFont->lfItalic) bItalic = true;
    if(lFont->lfStrikeOut == (char)true) bStrikeOut = true;

    buffer.Format(_T("%s, %d, %s%s%s%s, %d"),lFont->lfFaceName,nFontSize,
        szBold[bBold],szItalic[bItalic],szUnderline[bUnderline],szStrikeOut[bStrikeOut],(int)lFont->lfCharSet);
    Result = buffer;
    return true;
}

bool StringToFont(LPCTSTR szBuffer, LPLOGFONT lFont, HDC targetDc)
{
    TCHAR szFontName[LF_FACESIZE] = _T("Ms Sans Serif");

    TCHAR szFontSize[MAX_PATH]=_T("");
    TCHAR szFormat[MAX_PATH] = _T("");
    TCHAR szCharset[MAX_PATH] = _T("");
    bool bBold=false;
    bool bItalic=false;
    bool bUnderline=false;
    bool bStrikeOut=false;
    int nFontSize=10;
    int nCharSet=ANSI_CHARSET;

    ExtractStrFromList(szBuffer,0,szFontName,sizeof(szFontName)/sizeof(TCHAR));
    if(ExtractStrFromList(szBuffer,1,szFontSize,sizeof(szFontSize)/sizeof(TCHAR)))
    {
        _stscanf(szFontSize,_T("%d"),&nFontSize);
    }

    ExtractStrFromList(szBuffer,2,szFormat,sizeof(szFontSize)/sizeof(TCHAR));

    if(_tcschr(szFormat, 'b')) bBold=true;
    if(_tcschr(szFormat, 'u')) bUnderline=true;
    if(_tcschr(szFormat, 'i')) bItalic=true;
    if(_tcschr(szFormat, 's')) bStrikeOut=true;

    if( ExtractStrFromList(szBuffer,3,szCharset,sizeof(szCharset)/sizeof(TCHAR)))
    {
        _stscanf(szCharset,_T("%d"),&nCharSet);
    }

    ZeroMemory(lFont,sizeof(LOGFONT));
    lstrcpy(lFont->lfFaceName,szFontName);
    HDC dc = targetDc ? targetDc : ::GetDC(nullptr);

    lFont->lfHeight = -MulDiv(nFontSize, GetDeviceCaps(dc , LOGPIXELSY), 72);

    if (!targetDc) {
        ReleaseDC(nullptr, dc);
    }

    lFont->lfItalic=bItalic;
    lFont->lfStrikeOut=bStrikeOut;
    lFont->lfWeight=bBold?FW_BOLD:FW_NORMAL;
    lFont->lfUnderline=bUnderline;
    lFont->lfCharSet=static_cast<BYTE>(nCharSet);

    return true;
}

bool ExtractStrFromList(
                                LPCTSTR szString /* Source string */,
                                int nIndex, /* Zero based item index */
                                LPTSTR szBuffer /* Destination buffer */,
                                LONG nSize, /* Length in characters of destionation buffer */
                                LPCTSTR szDefString,
                                TCHAR cSeparator /* Character to be separator in list */)
{
    int nStringLen = 0;
    LPCTSTR szSeparator,szPrevSep;

    szSeparator=szPrevSep=0;

    int i;
    int nStart,nLen,nNumOfItems;
    nStart=nLen=-1;
    nNumOfItems=1;

    if(!szString || !szBuffer) return false;

    nStringLen = lstrlen(szString);

    if(nStringLen<=0) goto lbl_copydef;

    while (*szString == _T(' ')) szString++;

    nStringLen = lstrlen(szString);

    if(nStringLen<=0) goto lbl_copydef;

    szPrevSep=szString;

    nStart=0;
    nLen=0;

    for(i=0;i<nStringLen+1;i++)
    {
        if(szString[i]==0)
        {
            if(nIndex<nNumOfItems)
                nLen=i-nStart;
            break;
        }

        else if(szString[i]==cSeparator)
        {
            nNumOfItems++;
            if( nNumOfItems-1 == nIndex)
            {
                nStart=i+1;

            }
            else if( nNumOfItems-2 == nIndex)
            {
                nLen=i-nStart;
                break;

            }
        }
    }

    if(nLen>nSize-1) nLen = nSize-1;

    if(nLen<=0) goto lbl_copydef;

    lstrcpyn(szBuffer, szString + nStart, nLen+1);

    goto lbl_allok;

lbl_copydef:
    if(szDefString)
        StringCchCopy(szBuffer, nSize, szDefString);
    return false;

lbl_allok:
    return true;
}

CString StringSection(const CString& str,TCHAR sep, int index) {
    CString result;
    ExtractStrFromList(str, index, result.GetBuffer(256),256,_T(""),sep);
    result.ReleaseBuffer();
    return result;
}

// Show file properties dialog for single file
// For multiple files, see SHMultiFileProperties function
bool ShowFilePropertiesDialog(HWND hWnd, const CString& fileName) {

    SHELLEXECUTEINFO ShInfo;
    ZeroMemory(&ShInfo, sizeof(SHELLEXECUTEINFO));
    ShInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShInfo.nShow  = SW_SHOW;
    ShInfo.fMask  = SEE_MASK_INVOKEIDLIST | SEE_MASK_IDLIST;
    ShInfo.hwnd   = hWnd;
    ShInfo.lpVerb = TEXT("properties");
    ShInfo.lpFile = fileName;
    return ShellExecuteEx(&ShInfo) != FALSE;
}

bool GetClipboardHtml(CString& text, CString& outSourceUrl) {
    UINT clipboardFormat = RegisterClipboardFormat(_T("HTML Format"));
    if ( OpenClipboard(NULL) ) {
        HGLOBAL hglb = GetClipboardData(clipboardFormat);
        auto lpstr = static_cast<LPCSTR>(GlobalLock(hglb));
        std::string ansiString = lpstr;

        std::istringstream f(ansiString);
        std::string line;    
        int startFragment = -1;
        int endFragment = -1;
        std::string sourceUrl;
        bool result = false;

        while (std::getline(f, line)) {
            std::vector<std::string> tokens;
            IuStringUtils::Split(line, ":", tokens, 2);
            if ( tokens.size() == 2) {
                if ( tokens[0] == "StartFragment") {
                    startFragment = atoi(tokens[1].c_str());
                } else if ( tokens[0] == "EndFragment" ) {
                    endFragment = atoi(tokens[1].c_str());
                } else if ( tokens[0] == "SourceURL" ) {
                    sourceUrl = tokens[1];
                }
            } else {
                break;
            }
        }
        if ( startFragment != -1 && endFragment != -1 ) {
            text = IuCoreUtils::Utf8ToWstring( ansiString.substr(startFragment, endFragment - startFragment) ).c_str();
            outSourceUrl = IuCoreUtils::Utf8ToWstring(sourceUrl).c_str();
            result = true;
        }
        
        GlobalUnlock(hglb);
        CloseClipboard();

        return result;
    }

    return false;
}

float GetMonitorScaleFactor()
{
    float res = 1.0;
    /*MONITORINFOEX LogicalMonitorInfo;
    LogicalMonitorInfo.cbSize = sizeof(MONITORINFOEX);       
    GetMonitorInfo(hMonitor, &LogicalMonitorInfo);
    int LogicalMonitorWidth = LogicalMonitorInfo.rcMonitor.right – LogicalMonitorInfo.rcMonitor.left;
    int LogicalDesktopWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    typedef LONG (WINAPI * QueryDisplayConfig_FuncType)(UINT32,UINT32 *,DISPLAYCONFIG_PATH_INFO *,UINT32 *);
    typedef LONG (WINAPI * GetDisplayConfigBufferSizes_FuncType) (UINT32, UINT32*,UINT32 *);

    HMODULE lib = LoadLibrary(_T("user32.dll"));
    QueryDisplayConfig_FuncType QueryDisplayConfigFunc = GetProcAddress(lib, _T("QueryDisplayConfig"));
    GetDisplayConfigBufferSizes_FuncType GetDisplayConfigBufferSizeFunc =  GetProcAddress(lib, _T("GetDisplayConfigBufferSize"));
    if ( QueryDisplayConfigFunc && GetDisplayConfigBufferSizeFunc ) {
        UINT32 NumPathArrayElements = 0;
        UINT32 NumModeInfoArrayElements = 0;
        if ( GetDisplayConfigBufferSizeFunc(QDC_DATABASE_CURRENT, &NumPathArrayElements, &NumModeInfoArrayElements) != ERROR_SUCCESS ) {
            return res;
        }
        if ( !NumModeInfoArrayElements ) {
            return res;
        }
        DISPLAYCONFIG_TOPOLOGY_ID CurrentTopologyId = DISPLAYCONFIG_TOPOLOGY_INTERNAL;
        DISPLAYCONFIG_PATH_INFO* PathInfoArray = new DISPLAYCONFIG_PATH_INFO[NumPathArrayElements];
        memset(PathInfoArray, 0, sizeof(DISPLAYCONFIG_PATH_INFO) * NumPathArrayElements);
        DISPLAYCONFIG_MODE_INFO* ModeInfoArray = new DISPLAYCONFIG_PATH_INFO[NumModeInfoArrayElements];
        memset(ModeInfoArray, 0, sizeof(DISPLAYCONFIG_PATH_INFO) * NumModeInfoArrayElements);

        if ( QueryDisplayConfigFunc(QDC_DATABASE_CURRENT, &NumPathArrayElements, PathInfoArray, NumModeInfoArrayElements, ModeInfoArray, &CurrentTopologyId) != ERROR_SUCCESS ) {
            return res;
        }

        res = (LogicalMonitorWidth / LogicalDesktopWidth) / ModeInfoArray[0].sourceMode.width

    }*/
    return res;
}

CString GetLastErrorAsString()
{
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0)
        return _T("No error message has been recorded");

    LPTSTR messageBuffer = nullptr;

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&messageBuffer), 0, nullptr) != 0) {

        CString res = messageBuffer;

        //Free the buffer.
        LocalFree(messageBuffer);

        return res;
    }
    return {};
}

bool MakeDirectoryWritable(LPCTSTR lpPath) {
    HANDLE hDir = CreateFile(lpPath,READ_CONTROL|WRITE_DAC,0,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if (hDir == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    ACL* pOldDACL = nullptr;
    SECURITY_DESCRIPTOR* pSD = NULL;
    if (GetSecurityInfo(hDir, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, (void**)&pSD) == ERROR_SUCCESS) {
        PSID pSid = NULL;
        SID_IDENTIFIER_AUTHORITY authNt = SECURITY_NT_AUTHORITY;

        if (AllocateAndInitializeSid(&authNt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &pSid)) {
            EXPLICIT_ACCESS ea = { 0 };
            ea.grfAccessMode = GRANT_ACCESS;
            ea.grfAccessPermissions = GENERIC_ALL;
            ea.grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
            ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
            ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea.Trustee.ptstrName = (LPTSTR)pSid;

            ACL* pNewDACL = 0;
            /*DWORD err = */SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);

            if (pNewDACL) {
                SetSecurityInfo(hDir, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDACL, NULL);
            }

            FreeSid(pSid);
            if (pNewDACL) {
                LocalFree(pNewDACL);
            }
        }
        // We do not need to free pOldDACL because this memory block is freed together with SECURITY_DESCRIPTOR 
        // Otherwise invalid deallocation will occur.
        /*if (pOldDACL) { 
            LocalFree(pOldDACL);
        }*/
        if (pSD) {
            LocalFree(pSD);
        }
    }

    CloseHandle(hDir);
    return true;
}


bool IsProcessRunning(DWORD pid) {
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!process) {
        return false;
    }

    CloseHandle(process);
    return true;
}

int GetInternetExplorerMajorVersion()
{
    CRegistry Reg;
    Reg.SetRootKey( HKEY_LOCAL_MACHINE );
    if ( Reg.SetKey( _T("Software\\Microsoft\\Internet Explorer") , FALSE ) ) {
        CString version = Reg.ReadString(_T("svcVersion"));
        if ( version.IsEmpty() ) {
            version = Reg.ReadString(_T("Version"));
        }
        int dotPos = version.Find(L'.');
        if ( dotPos != -1 ) {
            return StrToInt(version.Left(dotPos));
        }
    }
    return 7;
}

const TCHAR* GetBrowserKey() {
    //32bit OS
    //\Software\Microsoft\Internet Explorer\Main\FeatureControl\FEATURE_BROWSER_EMULATION
    //32bit app on 64bit OS
    //\SOFTWARE\Wow6432Node\Microsoft\Internet Explorer\MAIN\FeatureControl\FEATURE_BROWSER_EMULATION

    /*if ( sizeof(void*) == 4 && IsWindows64Bit() ) {
        return _T("SOFTWARE\\Wow6432Node\\Microsoft\\Internet Explorer\\MAIN\\FeatureControl\\FEATURE_BROWSER_EMULATION");
    }*/
    return _T("SOFTWARE\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION");
}

void RemoveBrowserKey(){
    CRegistry Reg;
    Reg.SetRootKey( HKEY_CURRENT_USER );
    if ( Reg.SetKey( GetBrowserKey(), false ) ) {
        Reg.DeleteKey(DoExtractFileName(GetAppFileName()));
    }
}

void UseLatestInternetExplorerVersion(bool IgnoreIDocDirective) {
    // Thank to https://www.daniweb.com/software-development/vbnet/code/442963/make-the-webbrowser-control-give-you-the-installed-ie-version-rendering

    int value = 0;
    int majorVersion = GetInternetExplorerMajorVersion();
    //Value reference: http://msdn.microsoft.com/en-us/library/ee330730%28v=VS.85%29.aspx
    //IDOC Reference:  http://msdn.microsoft.com/en-us/library/ms535242%28v=vs.85%29.aspx
    //majorVersion = 7;
    if (majorVersion <= 11) {
        switch (majorVersion) {
        case 8:
            value = IgnoreIDocDirective ? 8888 : 8000;
            break;
        case 9:
            value = IgnoreIDocDirective ? 9999 : 9000;
            break;
        case 10:
            value = IgnoreIDocDirective ? 10001 : 10000;
            break;
        case 11:
            value = IgnoreIDocDirective ? 11001 : 11000;
            break;
        default:
            return;

        }
    } else {
        value = IgnoreIDocDirective ? 11001 : 11000;
    }

    CRegistry Reg;
    Reg.SetRootKey( HKEY_CURRENT_USER );
    if ( Reg.SetKey( GetBrowserKey(), true ) ) {
        Reg.WriteDword(DoExtractFileName(GetAppFileName()), value);
    }
}

bool DisplaySystemPrintDialogForImage(const std::vector<CString>& files, HWND hwnd) {
    static const CLSID CLSID_PrintPhotosDropTarget ={ 0x60fd46de, 0xf830, 0x4894, { 0xa6, 0x28, 0x6f, 0xa8, 0x1b, 0xc0, 0x19, 0x0d } };

    CComPtr<IShellFolder> desktop; // namespace root for parsing the path
    HRESULT hr = SHGetDesktopFolder(&desktop);
    if (!SUCCEEDED(hr)) {
        return false;
    }

    CComPtr<IShellItem> psi;
    CComPtr<IDataObject> pDataObject;

    std::vector<LPITEMIDLIST> list;

    for (const auto& fileName : files) {
        PIDLIST_RELATIVE newPIdL;
        hr = desktop->ParseDisplayName(hwnd, nullptr, const_cast<LPWSTR>(static_cast<LPCTSTR>(fileName)), nullptr, &newPIdL, nullptr);
        if (SUCCEEDED(hr)) {
            list.push_back(newPIdL);
        }
    }
   
    if (!list.empty()) {
        hr = desktop->GetUIObjectOf(hwnd, list.size(), const_cast<LPCITEMIDLIST*>(&list[0]), IID_IDataObject, 0, reinterpret_cast<void**>(&pDataObject));
        if (SUCCEEDED(hr)) {
            // Create the Photo Printing Wizard drop target.
            CComPtr<IDropTarget> spDropTarget;

            hr = CoCreateInstance(CLSID_PrintPhotosDropTarget, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spDropTarget));
            if (SUCCEEDED(hr)) {
                // Drop the data object onto the drop target.
                POINTL pt = { 0 };
                DWORD dwEffect = DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY;

                spDropTarget->DragEnter(pDataObject, MK_LBUTTON, pt, &dwEffect);

                spDropTarget->Drop(pDataObject, MK_LBUTTON, pt, &dwEffect);
                return true;
            }
        }
    }

    return false;
}

void TimerWait(int Delay)
{
    HANDLE hTimer = CreateWaitableTimer(0, TRUE, 0);
    LARGE_INTEGER interval;
    interval.QuadPart = -Delay * 10000;
    SetWaitableTimer(hTimer, &interval, 0, 0, 0, 0);
    MsgWaitForSingleObject(hTimer, INFINITE);
    CloseHandle(hTimer);
}

CString ConvertRelativePathToAbsolute(const CString& fileName)
{
    TCHAR result[MAX_PATH+1];
    if (!GetFullPathName(fileName, MAX_PATH, result, 0))
    {
        return fileName;
    }
    return result;
}

std::wstring strtows(const std::string &str, UINT codePage)
{
    std::wstring ws;
    int n = MultiByteToWideChar(codePage, 0, str.c_str(), str.size() + 1, /*dst*/NULL, 0);
    if (n)
    {
        ws.resize(n - 1);
        if (MultiByteToWideChar(codePage, 0, str.c_str(), str.size() + 1, /*dst*/&ws[0], n) == 0)
            ws.clear();
    }
    return ws;
}

std::string wstostr(const std::wstring &ws, UINT codePage)
{
    std::string str;
    int n = WideCharToMultiByte(codePage, 0, ws.c_str(), ws.size() + 1, /*dst*/NULL, 0, /*defchr*/0, NULL);
    if (n)
    {
        str.resize(n - 1);
        if (WideCharToMultiByte(codePage, 0, ws.c_str(), ws.size() + 1, /*dst*/&str[0], n, /*defchr*/0, NULL) == 0)
            str.clear();
    }
    return str;
}

std::string AnsiToUtf8(const std::string &str, int codepage)
{

    return wstostr(strtows(str, codepage), CP_UTF8);
}

std::string Utf8ToAnsi(const std::string &str, int codepage)
{

    return wstostr(strtows(str, CP_UTF8), codepage);
}

CString GetProcessName(DWORD pid)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (pe32.th32ProcessID == pid) {
                    CloseHandle(hSnapshot);
                    return pe32.szExeFile;
                }
                
            }
            while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return {};
}

std::string chcp(const std::string &str, UINT codePageSrc, UINT codePageDst)
{
    return wstostr(strtows(str, codePageSrc), codePageDst);
}

CString ErrorCodeToString(DWORD idCode, HMODULE mod)
{
    LPVOID lpMsgBuf;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
    if (mod != NULL) {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    } else {
        flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }

    if (!FormatMessage(flags, mod, idCode, 0, reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, NULL)) {
        LOG(WARNING) << _T("Failed to format message! Error code %d\n") << GetLastError();
    }
    
    CString res = static_cast<LPCTSTR>(lpMsgBuf);
    LocalFree(lpMsgBuf);
    return res;
}

CString ExpandEnvironmentStrings(const CString& s)
{
    DWORD len = ::ExpandEnvironmentStrings(s, NULL, 0);
    if (len == 0)
        return s;

    std::unique_ptr<TCHAR[]> buf = std::make_unique<TCHAR[]>(len + 1);
    if (::ExpandEnvironmentStrings(s, buf.get(), len) == 0)
        return s;

    return buf.get();
}

void ArgvQuote(const std::wstring& argument, std::wstring& commandLine, bool force){
    if (force == false &&
        argument.empty() == false &&
        argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        commandLine.append(argument);
    } else {
        commandLine.push_back(L'"');

        for (auto It = argument.begin();; ++It) {
            unsigned NumberBackslashes = 0;

            while (It != argument.end() && *It == L'\\') {
                ++It;
                ++NumberBackslashes;
            }

            if (It == argument.end()) {
                // Escape all backslashes, but let the terminating
                // double quotation mark we add below be interpreted
                // as a metacharacter.
                commandLine.append(NumberBackslashes * 2, L'\\');
                break;
            } else if (*It == L'"') {
                //
                // Escape all backslashes and the following
                // double quotation mark.
                //
                commandLine.append(NumberBackslashes * 2 + 1, L'\\');
                commandLine.push_back(*It);
            } else {
                //
                // Backslashes aren't special here.
                //
                commandLine.append(NumberBackslashes, L'\\');
                commandLine.push_back(*It);
            }
        }

        commandLine.push_back(L'"');
    }
}

bool GetProxyInfo(CString& proxyAddress, CString& proxyBypass)
{
    proxyAddress.Empty();
    proxyBypass.Empty();

    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxy_info;
    if (FALSE == WinHttpGetIEProxyConfigForCurrentUser(&proxy_info))
        return false;

    if (proxy_info.lpszProxy)
        proxyAddress = proxy_info.lpszProxy;

    if (proxy_info.lpszProxyBypass)
        proxyBypass = proxy_info.lpszProxyBypass;

    if (proxy_info.lpszProxy) {
        GlobalFree(proxy_info.lpszProxy);
    }
    if (proxy_info.lpszProxyBypass) {
        GlobalFree(proxy_info.lpszProxyBypass);
    }
    if (proxy_info.lpszAutoConfigUrl) {
        GlobalFree(proxy_info.lpszAutoConfigUrl);
    }
    return !proxyAddress.IsEmpty();
}

bool ShellOpenFileOrUrl(CString path, HWND wnd, CString directory, bool throwOnError) {
    SHELLEXECUTEINFO ShInfo;
    ZeroMemory(&ShInfo, sizeof(SHELLEXECUTEINFO));
    ShInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShInfo.nShow = SW_SHOWNORMAL;
    ShInfo.fMask = SEE_MASK_DEFAULT;
    ShInfo.hwnd = wnd;
    ShInfo.lpVerb = TEXT("open");
    ShInfo.lpFile = path;
    ShInfo.lpDirectory = directory;

    if (ShellExecuteEx(&ShInfo) == FALSE) {
        DWORD error = GetLastError();
        if (error != ERROR_CANCELLED) {
            CString errorMessage = WinUtils::FormatWindowsErrorMessage(error);
            if (throwOnError) {
                throw Win32Exception(errorMessage);
            }
            LOG(ERROR) << "ShellExecuteEx failed. " << std::endl << errorMessage;
        }
        return false;
    }
    return true;
}

bool ShowFilesInFolder(const std::vector<CString>& files, HWND wnd) {
    PIDLIST_ABSOLUTE folderPidl = nullptr;

    CComPtr<IShellFolder> desktop; // namespace root for parsing the path
    HRESULT hr = SHGetDesktopFolder(&desktop);

    if (!SUCCEEDED(hr)) {
        return false;
    }

    std::vector<LPCITEMIDLIST> list;
    for (const auto& file : files) {
        PIDLIST_RELATIVE newPIdL;
        hr = desktop->ParseDisplayName(wnd, nullptr, const_cast<LPWSTR>(file.GetString()), nullptr, &newPIdL, nullptr);
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
        } else {
            return true;
        }

        // SHBindToParent does not allocate a new PIDL; 
        // it simply receives a pointer through this parameter.
        // Therefore, we are not responsible for freeing this resource.
        /*for (auto& pidl : list) {
            SHFree(const_cast<LPITEMIDLIST>(pidl));
        }*/
    }
    return false;
}

bool ShowFileInFolder(CString fileName, HWND wnd) {
    /*SHELLEXECUTEINFO ShInfo;

    ZeroMemory(&ShInfo, sizeof(SHELLEXECUTEINFO));
    ShInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShInfo.nShow = SW_SHOWNORMAL;
    ShInfo.fMask = SEE_MASK_DEFAULT;
    ShInfo.hwnd = wnd;
    ShInfo.lpFile = _T("explorer.exe");
    CString parameters = _T("/select, ") + fileName;
    ShInfo.lpParameters = parameters;

    if (ShellExecuteEx(&ShInfo) == FALSE) {
        DWORD error = GetLastError();
        if (error != ERROR_CANCELLED) {
            LOG(ERROR) << "ShellExecuteEx failed. " << std::endl << WinUtils::FormatWindowsErrorMessage(error);
        }
        return false;
    }
    return true;*/
    return ShowFilesInFolder({ fileName }, wnd);
}

/*SYSTEMTIME SystemTimeAdd(const SYSTEMTIME& s, double seconds) {

    FILETIME f;
    SystemTimeToFileTime(&s, &f);

    ULARGE_INTEGER u;
    memcpy(&u, &f, sizeof(u));

    const double c_dSecondsPer100nsInterval = 100. * 1.E-9;
    u.QuadPart += static_cast<ULONGLONG>(seconds / c_dSecondsPer100nsInterval);

    memcpy(&f, &u, sizeof(f));
    SYSTEMTIME res;
    FileTimeToSystemTime(&f, &res);
    return res;
}*/

time_t SystemTimeToTime(const SYSTEMTIME &st) {
    std::tm tm;

    tm.tm_sec = st.wSecond;
    tm.tm_min = st.wMinute;
    tm.tm_hour = st.wHour;
    tm.tm_mday = st.wDay;
    tm.tm_mon = st.wMonth - 1;
    tm.tm_year = st.wYear - 1900;
    tm.tm_isdst = -1;

    return std::mktime(&tm);
}

bool CheckFileName(const CString& fileName)
{
    return fileName.FindOneOf(_T("\\/:*?\"<>|")) < 0;
}

HRESULT IsElevated(/*__out_opt*/ BOOL* pbElevated) {
    HRESULT hResult = E_FAIL; // assume an error occured
    HANDLE hToken = NULL;

    if (!::OpenProcessToken(
        ::GetCurrentProcess(),
        TOKEN_QUERY,
        &hToken))
    {
        ATLASSERT(FALSE);
        return hResult;
    }

    TOKEN_ELEVATION te = { 0 };
    DWORD dwReturnLength = 0;

    if (!::GetTokenInformation(
        hToken,
        (TOKEN_INFORMATION_CLASS)TokenElevation,
        &te,
        sizeof(te),
        &dwReturnLength))
    {
        //ATLASSERT( FALSE );
    }
    else
    {
        ATLASSERT(dwReturnLength == sizeof(te));

        hResult = te.TokenIsElevated ? S_OK : S_FALSE;

        if (pbElevated)
            *pbElevated = (te.TokenIsElevated != 0);
    }

    ::CloseHandle(hToken);

    return hResult;
}

SYSTEMTIME TimeToSystemTime(std::time_t timestamp)
{
    ULARGE_INTEGER time_value;
    time_value.QuadPart = (timestamp * 10000000LL) + 116444736000000000LL;

    FILETIME ft;
    ft.dwLowDateTime = time_value.LowPart;
    ft.dwHighDateTime = time_value.HighPart;

    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    return st;
}

CString TimestampToString(time_t t)
{
    SYSTEMTIME systime = TimeToSystemTime(t);
    WCHAR pFmt[MAX_PATH] = { 0 },
    pFmtTime[MAX_PATH] = { 0 };

    if (!SystemTimeToTzSpecificLocalTime(nullptr, &systime, &systime)) {
        return {};
    }
    GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &systime, nullptr, pFmt, MAX_PATH, nullptr);

    GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &systime, nullptr, pFmtTime, MAX_PATH);

    CString result;
    result.Format(_T("%s %s"), pFmt, pFmtTime);
    return result;
}

BOOL TranslateAcceleratorForWindow(HWND hwnd, HACCEL hacc, LPMSG pmsg) {
    if (hwnd == pmsg->hwnd || IsChild(hwnd, pmsg->hwnd)) {
        return TranslateAccelerator(hwnd, hacc, pmsg);
    }
    else {
        return FALSE;
    }
}

std::optional<GUID> GenerateFakeUUIDv4(const GUID& baseGUID) {
    std::string utf8Path = W2U(WinUtils::GetAppFileName());

    // Concatenate the base GUID and the UTF-8 application path
    std::string input(reinterpret_cast<const char*>(&baseGUID), sizeof(GUID));
    input += utf8Path;

    // Initialize CryptoAPI
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hash[20]; // SHA-1 produces a 20-byte digest
    DWORD hashLen = sizeof(hash);

    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        // Failed to acquire crypto context
        return std::nullopt;
    }

    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return std::nullopt;
    }

    if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.data()), static_cast<DWORD>(input.size()), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return std::nullopt;
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0) || hashLen < sizeof(GUID)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return std::nullopt;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    // Use the first 16 bytes of the SHA-1 hash as the GUID
    GUID guid;
    memcpy(&guid, hash, sizeof(GUID));

    // Set version to 4 (UUIDv4 = 0100xxxx in Data3)
    guid.Data3 = (guid.Data3 & 0x0FFF) | (4 << 12);

    // Set variant to RFC 4122 (10xxxxxx in Data4[0])
    guid.Data4[0] = (guid.Data4[0] & 0x3F) | 0x80;

    return guid;
}

CString GUIDToString(const GUID& guid) {
    TCHAR guidStr[39];
    StringFromGUID2(guid, guidStr, std::size(guidStr));
    return guidStr;
}

CString NormalizeLineEndings(const CString& text) {
    CString result;
    result.Preallocate(text.GetLength() + text.GetLength() / 10); // Reserve memory

    for (int i = 0; i < text.GetLength(); i++) {
        TCHAR ch = text[i];

        if (ch == _T('\n')) {
            // Check if there's a \r character before \n
            if (i == 0 || text[i - 1] != _T('\r')) {
                result += _T('\r'); // Add \r before \n
            }
            result += ch;
        } else if (ch == _T('\r')) {
            result += ch;
            // Check if \n follows after \r
            if (i + 1 < text.GetLength() && text[i + 1] == _T('\n')) {
                // Leave as is, \n will be processed on next iteration
            } else {
                // Standalone \r, add \n
                result += _T('\n');
            }
        } else {
            result += ch;
        }
    }

    return result;
}

bool NoModifiersPressed() {
    return !(GetKeyState(VK_CONTROL) & 0x8000) &&
           !(GetKeyState(VK_SHIFT)   & 0x8000) &&
           !(GetKeyState(VK_MENU)    & 0x8000); // Alt
}

}

std::wstring Utf8ToWstring(const std::string &str)
{
    return WinUtils::strtows(str, CP_UTF8);
}


