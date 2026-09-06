#ifndef IU_CORE_SETTINGS_COMMONGUISETTINGS_H
#define IU_CORE_SETTINGS_COMMONGUISETTINGS_H

#pragma once


#include <cstdint>
#include <map>
#include <string>
#include <unordered_set>
#include "Core/SettingsManager.h"
#include "Core/Upload/UploadEngine.h"
#include "Core/Upload/ServerProfile.h"
#include "BasicSettings.h"
#include "Core/Upload/ServerProfileGroup.h"
#include "Gui/Interfaces/IFavoriteServers.h"

#ifdef IU_QT
    #include <QString>
    typedef QString SettingsString;
    #define Utf8ToSettingsString(s) QString::fromStdString(s)
    #define SettingsStringToUtf8(s) s.toStdString()

#else
    //#include "atlheaders.h"
    typedef CString SettingsString;
    #define Utf8ToSettingsString(s) Utf8ToWCstring(s)
    #define SettingsStringToUtf8(s) WCstringToUtf8(s)
#endif

#ifndef _WIN32
    #define TCHAR char
    #define _T(a) a
#endif

#include "EncodedPassword.h"

#include "Core/Images/ImageParams.h"

typedef std::map<SettingsString, ServerProfile> ServerProfilesMap;


struct VideoSettingsStruct {
    int Columns = 3;
    int TileWidth = 200;
    int GapWidth = 5;
    int GapHeight = 7;
    int NumOfFrames = 8;
    int JPEGQuality = 100;
    std::string Engine = "Auto";
    bool ShowMediaInfo = true;
    std::string Font = "Tahoma,12,,204";
    uint32_t TextColor = 0;
    std::string SnapshotsFolder;
    std::string SnapshotFileTemplate = "%type%_%i%_%random(6)%.png";
};

struct FFMpegSettingsStruct {
    std::string FFmpegCLIPath;

    int VideoQuality = 70;
    int VideoBitrate = 3000; // kbps
    bool UseQuality = true;
    std::string VideoCodecId = "x264", VideoPresetId;
    std::string VideoSourceId;
    std::string AudioSourceId;
    std::string AudioCodecId;
    std::string AudioQuality;

    FFMpegSettingsStruct();
    void bind(SettingsNode& n);
};

struct DXGISettingsStruct {
    int VideoQuality = 70;
    int VideoBitrate = 3000; // kbps
    bool UseQuality = true;
    std::string VideoCodecId = "{34363248-0000-0010-8000-00AA00389B71}"; //h.264
    std::string VideoPresetId;
    std::string VideoSourceId;
    std::vector<std::string> AudioSources;
    std::string AudioCodecId = "{00001610-0000-0010-8000-00AA00389B71}"; //aac
    int AudioBitrate = 192;

    void bind(SettingsNode& n);
};

struct ScreenRecordingStruct {
    inline static const std::string ScreenRecordingBackendFFmpeg = "FFmpeg";
    inline static const std::string ScreenRecordingBackendDirectX = "DirectX";

    std::string Backend;

    int FrameRate = 30;
    int Delay = 0;
    bool CaptureCursor = true;
    int MonitorMode = -1; // kAllMonitors
    //std::string Preset;
    std::string OutDirectory;
    std::string FileNameTemplate = "%type% %y-%m-%d %h-%n-%s";
    FFMpegSettingsStruct FFmpegSettings;
    DXGISettingsStruct DXGISettings;
};

struct ScreenshotSettingsStruct {
    int Format = 1;
    int Quality = 85;
    int Delay = 1;
    int WindowHidingDelay = 200;
    bool ShowForeground = false;
    bool CopyToClipboard = false;
    uint32_t BrushColor = 0x000000ff;
    std::string FilenameTemplate = "%type% %y-%m-%d %h-%n-%s %i";
    std::string Folder;
    bool RemoveCorners = false;
    bool AddShadow = false;
    bool RemoveBackground = false;
    bool OpenInEditor = true;
    bool UseOldRegionScreenshotMethod = false;
    bool CaptureCursor = false;
    int MonitorMode = -1; // kAllMonitors
};

struct ServerListSettingsStruct: IFavoriteServers {
public:
#ifndef IU_QT
    int ViewMode = LV_VIEW_DETAILS;
#endif
    bool ShowFavoritesOnly = false;
    bool HideBlackListed = false;
    std::unordered_set<std::string> FavoriteServers;
    std::unordered_set<std::string> BlacklistedServers;

    bool isServerFavorite(const std::string& serverId) override;
    bool isServerBlacklisted(const std::string& serverId) override;
    void addServerToFavorites(const std::string& serverId);
    void removeServerFromFavorites(const std::string& serverId);
    void addServerToBlacklist(const std::string& serverId);
    void removeServerFromBlacklist(const std::string& serverId);

    void bind(SettingsNode& node);
};

#ifndef IU_QT
struct MediaInfoSettingsStruct {
    int InfoType = 0; // 0 - short summary, 1 - full info
    bool EnableLocalization = true;
};

struct HistorySettingsStruct {
    bool EnableDownloading;
    bool HistoryConverted;
};
#endif

class CommonGuiSettings : public BasicSettings {
    public:   
        CommonGuiSettings();
        ~CommonGuiSettings() override;

        std::string Language;
        std::string ImageEditorPath;
        bool ConfirmOnExit = true;
        bool DropVideoFilesToTheList = false;
        bool AutomaticallyCheckUpdates = true;
        bool EnableToastNotifications = true;
        bool ShowPreviewForVideoFiles = true;

        bool UseDirectLinks = true;

        ServerProfile urlShorteningServer, temporaryServer, imageSearchServer;
        ServerProfileGroup imageServer, fileServer, quickScreenshotServer, contextMenuServer;
        ServerProfilesMap ServerProfiles;
        VideoSettingsStruct VideoSettings;
        ScreenRecordingStruct ScreenRecordingSettings;
        ScreenshotSettingsStruct ScreenshotSettings;
        ImageUploadParams DefaultImageUploadParams;

        ServerListSettingsStruct ServerListSettings;

#ifndef IU_QT
        CString DataFolder;
        CString m_SettingsDir;
        bool ShowTrayIcon = false;
        bool ShowTrayIcon_changed = false;
        bool AutoStartup = false;
        bool AutoStartup_changed = false;
        int ThumbsPerLine = 4;

        MediaInfoSettingsStruct MediaInfoSettings;
        HistorySettingsStruct HistorySettings;

        int CodeLang = 0;
        int CodeType = 0;

        bool IsPortable = true;
        CString VideoFolder, ImagesFolder;

        std::map<CString, ImageConvertingParams> ConvertProfiles;

    protected:
        CString CurrentConvertProfileName;
#endif
    protected:
        void BindToManager();
        bool PostSaveSettings(SimpleXml &xml) override;
        bool PostLoadSettings(SimpleXml &xml) override;
        bool LoadServerProfiles(const SimpleXmlNode& root);
        bool SaveServerProfiles(SimpleXmlNode root);
        static void LoadServerProfile(const SimpleXmlNode& root, ServerProfile& profile);
        static bool LoadServerProfileGroup(const SimpleXmlNode& root, ServerProfileGroup& group);
        static bool SaveServerProfileGroup(SimpleXmlNode root, ServerProfileGroup& group);
        void PostLoadServerProfileGroup(ServerProfileGroup& profile);
        virtual void PostLoadServerProfile(ServerProfile& profile);

    public:
        inline static const std::string VideoEngineDirectshow = "DirectShow";
        inline static const std::string VideoEngineDirectshow2 = "DirectShow v2";
        inline static const std::string VideoEngineMediaFoundation = "Media Foundation"; 
        inline static const std::string VideoEngineFFmpeg = "FFmpeg";
        inline static const std::string VideoEngineAuto = "Auto";
        
        inline static std::vector<std::string> VideoEngines = {
#ifdef _WIN32
            VideoEngineDirectshow,
            VideoEngineDirectshow2,
            VideoEngineMediaFoundation,
#endif
            VideoEngineFFmpeg,
            VideoEngineAuto
        };

        inline static std::vector<std::string> ScreenRecordingBackends = {
#ifdef _WIN32
            ScreenRecordingStruct::ScreenRecordingBackendDirectX,
#endif
            ScreenRecordingStruct::ScreenRecordingBackendFFmpeg
        };

        static bool IsFFmpegAvailable();
    };
#endif
