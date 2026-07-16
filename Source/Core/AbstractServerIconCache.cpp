#include "AbstractServerIconCache.h"

#include "Core/Upload/UploadEngine.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"

namespace {
std::string StrToLower(const std::string& str) {
#ifdef IU_QT
    return QString::fromStdString(str).toLower().toStdString();
#elif defined(_WIN32)
    CString s = IuCoreUtils::Utf8ToWstring(str).c_str();
    s.MakeLower();
    return IuCoreUtils::WstringToUtf8(s.GetString());
#else
    return IuStringUtils::ToLower(str);
#endif
}
}

std::string AbstractServerIconCache::getIconNameForServer(const std::string& name, bool returnFullPath /*= false*/) {
    
    CUploadEngineData* ued = engineList_->byName(name);

    std::string serverName = IuStringUtils::Replace(name, "\\", "_");
    serverName = IuStringUtils::Replace(serverName, "/", "_");

    std::string iconFileName = iconsDir_ + StrToLower(serverName) + ".ico";

    if (!IuCoreUtils::FileExists(iconFileName)) {
        if (ued && !ued->PluginName.empty()) {
            iconFileName = iconsDir_ + StrToLower(ued->PluginName) + ".ico";
            if (!IuCoreUtils::FileExists(iconFileName)) {
                iconFileName.clear();
            }
        } else {
            iconFileName.clear();
        }
    }

    if (iconFileName.empty()) {
        iconFileName = iconsDir_ + "default.ico";
    }

    if (returnFullPath) {
        return iconFileName;
    } else {
        return IuCoreUtils::ExtractFileName(iconFileName);
    }
}
