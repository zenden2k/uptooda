#ifndef IU_CORE_UTILS_SYSTEMUTILS_WIN_H
#define IU_CORE_UTILS_SYSTEMUTILS_WIN_H

#include "SystemUtils.h"

#include <string>
#include <windows.h>

namespace IuCoreUtils
{
typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

bool IsOs64Bit() {
    SYSTEM_INFO si;
    PGNSI pGNSI;
    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
    pGNSI = reinterpret_cast<PGNSI>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetNativeSystemInfo"));
    if (nullptr != pGNSI) {
        pGNSI(&si);
    } else {
        GetSystemInfo(&si);
    }
    return si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64
        || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64
        || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64;
}

std::string GetOsName() {
    return "Windows";
}

std::string GetOsVersion() {
    OSVERSIONINFOEX osvi;

    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&osvi));

    char res[40];
    sprintf(res, "%u.%u.%u SP %u.%u", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, 
        osvi.wServicePackMajor, osvi.wServicePackMinor);
    
    return res;
}

}

#endif