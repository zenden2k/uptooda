#include "DesktopUtils.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include "CoreUtils.h"
#include "StringUtils.h"

namespace DesktopUtils {

bool ShellOpenUrl(const std::string& url) {
#ifdef _WIN32
    std::wstring wideUrl = IuCoreUtils::Utf8ToWstring(url);
    SHELLEXECUTEINFO ShInfo;
    ZeroMemory(&ShInfo, sizeof(SHELLEXECUTEINFO));
    ShInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShInfo.nShow = SW_SHOWNORMAL;
    ShInfo.fMask = SEE_MASK_DEFAULT;
    ShInfo.lpVerb = TEXT("open");
    ShInfo.lpFile = wideUrl.c_str();
    ShInfo.lpDirectory = TEXT("");

    return ShellExecuteEx(&ShInfo) == TRUE;
#else
#ifdef __APPLE__
    return system(("open \"" + url + "\"").c_str());
#else
    return system(("xdg-open \"" + url + "\" >/dev/null 2>&1 & ").c_str());
#endif
#endif
}

}
