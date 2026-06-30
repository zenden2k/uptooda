#include "AppParams.h"

#include "Core/Utils/StringUtils.h"
#include "Core/Utils/CoreUtils.h"

const char kPathSeparator =
#ifdef _WIN32
    '\\';
#else
    '/';
#endif

AppParams::AppParams() {
    isGui_ = true;
}

void AppParams::setVersionInfo(const AppVersionInfo& info) {
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


std::string AppParams::dataDirectory() const
{
    return dataDirectory_;
}

void AppParams::setDataDirectory(const std::string& directory)
{
    dataDirectory_ = directory;
}

std::string AppParams::settingsDirectory() const
{    
    return settingsDirectory_;
}

void AppParams::setSettingsDirectory(const std::string& directory)
{
    settingsDirectory_ = directory;
}

std::string AppParams::languageFile() const
{
    return languageFile_;
}

void AppParams::setLanguageFile(const std::string& languageFile)
{
    languageFile_ = languageFile;
}

void AppParams::setTempDirectory(const std::string& directory) {
    tempDirectory_ = directory;
    if (!tempDirectory_.empty()) {
        size_t pos = tempDirectory_.length() - 1;
        if (tempDirectory_[pos] != '\\' && tempDirectory_[pos] != '/') {
            tempDirectory_ += kPathSeparator;
        }
    }
}

std::string AppParams::tempDirectory() const {
    return tempDirectory_;
}

AppParams::AppVersionInfo const * AppParams::GetAppVersion() const {
    return &versionInfo_;
}

void AppParams::setIsGui(bool isGui) {
    isGui_ = isGui;
}

bool AppParams::isGui() const {
    return isGui_;
}

#ifdef _WIN32
CString AppParams::tempDirectoryW() const {
    return IuCoreUtils::Utf8ToWstring(tempDirectory_).c_str();
}
#endif

