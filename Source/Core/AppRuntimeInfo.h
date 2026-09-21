#ifndef IU_CORE_APPPARAMS_H
#define IU_CORE_APPPARAMS_H

#include <string>

#include "Core/Utils/Singleton.h"

#ifdef _WIN32
#include "atlheaders.h"
#endif

class AppRuntimeInfo: public Singleton<AppRuntimeInfo>
{
    public:
        struct AppVersionInfo {
            // Just first 5 fields must be filled
            std::string FullVersion;
            std::string FullVersionClean;
            std::string BuildDate;
            std::string BranchName;
            std::string CommitHash;
            std::string CommitHashShort;

            unsigned int Major;
            unsigned int Minor;
            unsigned int Release;
            unsigned int Build;
            bool CurlWithOpenSSL;

            AppVersionInfo() {
                Major = 0;
                Minor = 0;
                Release = 0;
                Build = 0;
                CurlWithOpenSSL = false;
            }
        };
		AppRuntimeInfo();
        std::string dataDirectory() const;
        void setDataDirectory(const std::string& directory);
        std::string settingsDirectory() const;
        void setSettingsDirectory(const std::string& directory);
        std::string languageFile() const;
        void setLanguageFile(const std::string& languageFile);
        void setTempDirectory(const std::string& directory);
        std::string tempDirectory() const;
#ifdef _WIN32
        CString tempDirectoryW() const;
#endif
        AppVersionInfo const* GetAppVersion() const;
        void setVersionInfo(const AppVersionInfo& info);
        void setIsGui(bool isGui);
        bool isGui() const;
    protected:
        std::string dataDirectory_;
        std::string settingsDirectory_;
        std::string languageFile_;
        std::string tempDirectory_;
        AppVersionInfo versionInfo_;
        bool isGui_;
};

#endif
