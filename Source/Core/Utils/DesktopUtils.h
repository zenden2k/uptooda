#ifndef IU_CORE_UTILS_DESKTOPUTILS_H
#define IU_CORE_UTILS_DESKTOPUTILS_H

#pragma once
#include <string>

namespace DesktopUtils {

/**
Opens a file or a URL in associated application
**/
bool ShellOpenUrl(const std::string& url);

}
#endif
