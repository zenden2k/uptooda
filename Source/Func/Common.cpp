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

#include "Common.h"

#include "atlheaders.h"
#include "Func/CmdLine.h"
#include "Func/WinUtils.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/WtlGuiSettings.h"

bool IULaunchCopy(CString additionalParams)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    TCHAR Buffer[MAX_PATH * 40];
    GetModuleFileName(0, Buffer, sizeof(Buffer) / sizeof(TCHAR));

    CString TempCmdLine = CString(_T("\"")) + CmdLine[0] + CString(_T("\""));
    for (size_t i = 1; i < CmdLine.GetCount(); i++)
    {
        if (!lstrcmpi(CmdLine[i], _T("-Embedding")))
            continue;
        TempCmdLine = TempCmdLine + " \"" + CmdLine[i] + "\"";
    }

    TempCmdLine += _T(" ") + additionalParams;
    // Start the child process.
    if ( !CreateProcess(
            NULL,              // No module name (use command line).
            const_cast<LPWSTR>(TempCmdLine.GetString()), // Command line.
            NULL,                // Process handle not inheritable.
            NULL,                // Thread handle not inheritable.
            FALSE,               // Set handle inheritance to FALSE.
            0,                   // No creation flags.
            NULL,                // Use parent's environment block.
            NULL,                // Use parent's starting directory.
            &si,                 // Pointer to STARTUPINFO structure.
            &pi )                // Pointer to PROCESS_INFORMATION structure.
         )

        return false;
    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    return true;
}

bool IULaunchCopy(CString params, const CAtlArray<CString>& files)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    TCHAR Buffer[MAX_PATH * 40];
    GetModuleFileName(0, Buffer, sizeof(Buffer) / sizeof(TCHAR));

    CString TempCmdLine = CString(_T("\"")) + Buffer + CString(_T("\""));
    if (!params.IsEmpty())
        TempCmdLine += _T(" ") + params + _T(" ");

    for (size_t i = 0; i < files.GetCount(); i++)
    {
        TempCmdLine = TempCmdLine + " \"" + files[i] + "\"";
    }

    // Start the child process.
    if ( !CreateProcess(
            NULL,                        // No module name (use command line).
            (LPWSTR)(LPCTSTR)TempCmdLine, // Command line.
            NULL,                // Process handle not inheritable.
            NULL,                // Thread handle not inheritable.
            FALSE,               // Set handle inheritance to FALSE.
            0,                   // No creation flags.
            NULL,                // Use parent's environment block.
            NULL,                // Use parent's starting directory.
            &si,                 // Pointer to STARTUPINFO structure.
            &pi )                // Pointer to PROCESS_INFORMATION structure.
         )

        return false;

    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    return true;
}

void IU_RunElevated(CString params)
{
    SHELLEXECUTEINFO TempInfo = {};
    CString appDir = WinUtils::GetAppFolder();
    CString Command = CmdLine[0];
    CString parameters = _T(" ") + params;
    TempInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    TempInfo.fMask = 0;
    TempInfo.hwnd = NULL;
    if (IsWindowsVistaOrGreater())
        TempInfo.lpVerb = _T("runas");
    else
        TempInfo.lpVerb = _T("open");
    TempInfo.lpFile = Command;
    TempInfo.lpParameters = parameters;
    TempInfo.lpDirectory = appDir;
    TempInfo.nShow = SW_NORMAL;

    ::ShellExecuteEx(&TempInfo);
}

/*
CString GetUniqFileName(const CString& filePath)
{
    if (!WinUtils::FileExists(filePath))
        return filePath;
    CString path = U2W(IuCoreUtils::ExtractFilePath(W2U(filePath)));

    CString name;
    name = WinUtils::GetOnlyFileName(filePath);

    CString extension = GetFileExt(filePath);
    CString result;
    for (int i = 2;; i++)
    {
        result = path + name + WinUtils::IntToStr(i) + (extension.IsEmpty() ? _T("") : _T(".") + extension);
        if (!WinUtils::FileExists(result))
            break;
    }
    return result;
}*/

BOOL IU_CreateFolder(LPCTSTR szFolder)
{
    if (!szFolder || !lstrlen(szFolder))
        return FALSE;

    DWORD dwAttrib = GetFileAttributes(szFolder);

    // already exists ?
    if (dwAttrib != 0xffffffff)
        return ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

    // recursively create from the top down
    TCHAR* szPath = _tcsdup(szFolder);
    if (!szPath) {
        return FALSE;
    }
    TCHAR* p = _tcsrchr(szPath, '\\');

    if (p)
    {
        // The parent is a dir, not a drive
        *p = '\0';

        // if can't create parent
        if (!IU_CreateFolder(szPath))
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

//CMyEngineList* _EngineList;

void DecodeString(LPCTSTR szSource, CString& Result, LPSTR code)
{
    TCHAR szDestination[1024];
    int br = strlen(code);
    int n = lstrlen(szSource) / 2;
    int j = 0;
    ZeroMemory(szDestination, n * 2);

    int i;
    PBYTE data = (PBYTE)szDestination;
    *szDestination = 0;

    for (i = 0; i < n; i++)
    {
        if (j >= br)
            j = 0;

        BYTE b;
        b = static_cast<BYTE>((szSource[i * 2] - _T('A')) * 16 + (szSource[i * 2 + 1] - _T('A')));
        b = b ^ code[j];
        data[i] = b;
        j++;
    }
    data[i] = 0;
    Result = szDestination;
}

void EncodeString(LPCTSTR szSource, CString& Result, LPSTR code)
{
    TCHAR szDestination[1024];
    int br = strlen(code);
    int n = lstrlen(szSource) * 2;
    int j = 0;

    PBYTE data = (PBYTE)szSource;
    *szDestination = 0;
    for (int i = 0; i < n; i++)
    {
        if (j >= br)
            j = 0;

        BYTE b;
        b = data[i] ^ code[j];
        TCHAR bb[2] = {0, 0};
        bb[0] = _T('A') + b / 16;
        lstrcat(szDestination, bb);
        bb[0] = _T('A') + b % 16;
        lstrcat(szDestination, bb);
        j++;
    }
    Result = szDestination;
}

BOOL IU_CreateFilePath(LPCTSTR szFilePath)
{
    TCHAR* szPath = _tcsdup(szFilePath);
    if (!szPath) {
        return FALSE;
    }
    TCHAR* p = _tcsrchr(szPath, '\\');

    BOOL bRes = FALSE;

    if (p)
    {
        *p = '\0';

        bRes = IU_CreateFolder(szPath);
    }

    free(szPath);

    return bRes;
}

CString HotkeyToString(CString funcName, CString menuItemText) {
    auto *settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    int cur = settings->Hotkeys.getFuncIndex(funcName);
    if (cur < 0) {
        return menuItemText;
    }

    CHotkeyItem item = settings->Hotkeys[cur];
    CString hotkeyStr = item.globalKey.toString();
    if (hotkeyStr.IsEmpty()) {
        return menuItemText;
    }
    return menuItemText + _T("\t") + hotkeyStr;
}

void OpenDocumentation(HWND parent, const CString& file, const CString& id /*= {}*/) {
    CString docsFolder = WinUtils::GetAppFolder() + "Docs\\";
    CString actualFile;

    if (!file.IsEmpty()) {
        CString locale = U2W(ServiceLocator::instance()->translator()->getCurrentLocale());
        CString tryFile = docsFolder + locale + _T("\\") + file + _T(".html");
        CString tryFile2 = docsFolder + _T("en_US\\") + file + _T(".html");

        if (WinUtils::FileExists(tryFile)) {
            actualFile = tryFile;
        } else if (WinUtils::FileExists(tryFile2)) {
            actualFile = tryFile2;
        } 

        /*if (!actualFile.IsEmpty()) {
            actualFile += _T("#");
            actualFile += id;
        }*/
   }

    if (actualFile.IsEmpty()) {
        actualFile = docsFolder + _T("index.html");
    }

    try {
        WinUtils::ShellOpenFileOrUrl(actualFile, parent, {}, true);
    } catch (const Win32Exception& ex) {
        GuiTools::LocalizedMessageBox(parent, TR("Cannot open documentation: ") + ex.getMessage(), TR("Error"), MB_ICONERROR); 
    }
}

bool DisableWindowsPrintScreenKeyInterception() {
    HKEY hKey;
    DWORD value = 1, size = sizeof(value);
    LSTATUS res = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Keyboard", 0, KEY_ALL_ACCESS, &hKey);
    if (res == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"PrintScreenKeyForSnippingEnabled", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size);

        if (value != 0) {
            value = 0;
            RegSetValueExW(
                hKey, L"PrintScreenKeyForSnippingEnabled", 0,
                REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value)
            );
            SendMessageTimeoutW(
                HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"Control Panel\\Keyboard"),
                SMTO_ABORTIFHUNG, 5000, nullptr
            );
        }

        RegCloseKey(hKey);
        return true;
    }


    LOG(ERROR) << L"Cannot open registry key for disabling default PrintScreen key interception."
        << std::endl << WinUtils::FormatWindowsErrorMessage(res) << std::endl;
    return false;
}