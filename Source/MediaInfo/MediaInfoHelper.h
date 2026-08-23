#ifndef IU_FUNC_MEDIAINFOHELPER_H
#define IU_FUNC_MEDIAINFOHELPER_H

#pragma once

#include <string>

namespace MediaInfoHelper {

bool GetMediaFileInfo(const std::string& fileName, std::string& summary, std::string& fullInfo,
                      bool enableLocalization = true);
std::string GetLibraryVersion();

}

#endif // IU_FUNC_MEDIAINFOHELPER_H
