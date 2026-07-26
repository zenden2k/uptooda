#include "AppRuntimeInfo.h"

#include "Core/Utils/StringUtils.h"
#include "Core/Utils/CoreUtils.h"

constexpr char kPathSeparator =
#ifdef _WIN32
    '\\';
#else
    '/';
#endif

AppRuntimeInfo::AppRuntimeInfo() {
    isGui_ = true;
}

void AppRuntimeInfo::setVersionInfo(const AppVersionInfo& info) {
    versionInfo_ = info;
    std::vector<std::string> tokens;
    IuStringUtils::Split(versionInfo_.FullVersionClean, ".", tokens, 3);
    if (tokens.size() >= 3) {
        versionInfo_.Major = std::stoi(tokens[0]);
        versionInfo_.Minor = std::stoi(tokens[1]);
        versionInfo_.Release = std::stoi(tokens[2]);
    }
#ifdef USE_OPENSSL
    versionInfo_.CurlWithOpenSSL = true;
#endif
}


std::string AppRuntimeInfo::dataDirectory() const
{
    return dataDirectory_;
}

void AppRuntimeInfo::setDataDirectory(const std::string& directory)
{
    dataDirectory_ = directory;
}

std::string AppRuntimeInfo::settingsDirectory() const
{    
    return settingsDirectory_;
}

void AppRuntimeInfo::setSettingsDirectory(const std::string& directory)
{
    settingsDirectory_ = directory;
}

std::string AppRuntimeInfo::languageFile() const
{
    return languageFile_;
}

void AppRuntimeInfo::setLanguageFile(const std::string& languageFile)
{
    languageFile_ = languageFile;
}

void AppRuntimeInfo::setTempDirectory(const std::string& directory) {
    tempDirectory_ = directory;
    if (!tempDirectory_.empty()) {
        size_t pos = tempDirectory_.length() - 1;
        if (tempDirectory_[pos] != '\\' && tempDirectory_[pos] != '/') {
            tempDirectory_ += kPathSeparator;
        }
    }
}

std::string AppRuntimeInfo::tempDirectory() const {
    return tempDirectory_;
}

AppRuntimeInfo::AppVersionInfo const * AppRuntimeInfo::GetAppVersion() const {
    return &versionInfo_;
}

void AppRuntimeInfo::setIsGui(bool isGui) {
    isGui_ = isGui;
}

bool AppRuntimeInfo::isGui() const {
    return isGui_;
}

#ifdef _WIN32
CString AppRuntimeInfo::tempDirectoryW() const {
    return IuCoreUtils::Utf8ToWstring(tempDirectory_).c_str();
}
#endif

