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

#include "CommonGuiSettings.h"

#include "Core/ServiceLocator.h"

#ifndef IU_QT
#include "Func/WinUtils.h"
#endif

CommonGuiSettings::CommonGuiSettings()
    : DefaultImageUploadParams(false) {
    // Default values of settings
    MaxThreads = 3;
    DeveloperMode = false;

#ifndef IU_QT
    HistorySettings.EnableDownloading = true;
    HistorySettings.HistoryConverted = false;
#endif
#ifdef _WIN32
    ScreenRecordingSettings.Backend = IsWindows8OrGreater() ? ScreenRecordingStruct::ScreenRecordingBackendDirectX: ScreenRecordingStruct::ScreenRecordingBackendFFmpeg;
#endif
}

CommonGuiSettings::~CommonGuiSettings() {
    if (!IsFFmpegAvailable()) {
        auto it = std::remove(std::begin(VideoEngines), std::end(VideoEngines), VideoEngineFFmpeg);
        VideoEngines.erase(it, VideoEngines.end());
    }
}

bool CommonGuiSettings::IsFFmpegAvailable() {
#ifndef IU_ENABLE_FFMPEG
    return false;
#else
    #if IU_FFMPEG_STANDALONE
        CString appFolder = WinUtils::GetAppFolder();
        return WinUtils::FileExists(appFolder + "avcodec-59.dll");
    #else
        return true;
    #endif
#endif
}

bool CommonGuiSettings::LoadServerProfiles(SimpleXmlNode root)
{
    std::vector<SimpleXmlNode> servers;
    root.GetChilds("ServerProfile", servers);

    for (size_t i = 0; i < servers.size(); i++) {
        SimpleXmlNode serverProfileNode = servers[i];
        std::string profileName = serverProfileNode.Attribute("ServerProfileId");
        ServerProfile sp;
        SettingsManager mgr;
        sp.bind(mgr.root());

        mgr.loadFromXmlNode(serverProfileNode);
        ServerProfiles[Utf8ToSettingsString(profileName)] = sp;
    }
    return true;
}

bool CommonGuiSettings::SaveServerProfiles(SimpleXmlNode root)
{
    for (ServerProfilesMap::iterator it = ServerProfiles.begin(); it != ServerProfiles.end(); ++it) {
        SimpleXmlNode serverProfileNode = root.CreateChild("ServerProfile");

        std::string profileName = SettingsStringToUtf8(it->first);
        SettingsManager mgr;
        it->second.bind(mgr.root());
        mgr["@ServerProfileId"].bind(profileName);

        mgr.saveToXmlNode(serverProfileNode);
    }
    return true;
}

void CommonGuiSettings::LoadServerProfile(SimpleXmlNode root, ServerProfile& profile) {
    SettingsManager mgr;
    profile.bind(mgr.root());
    mgr.loadFromXmlNode(root);
}

bool CommonGuiSettings::LoadServerProfileGroup(SimpleXmlNode root, ServerProfileGroup& group) {
    std::vector<SimpleXmlNode> servers;
    root.GetChilds("ServerProfileItem", servers);
    if (!servers.empty()) {
        group.getItems().clear();
        for (size_t i = 0; i < servers.size(); i++) {
            SimpleXmlNode serverProfileNode = servers[i];
            ServerProfile sp;
            SettingsManager mgr;
            sp.bind(mgr.root());

            mgr.loadFromXmlNode(serverProfileNode);
            group.addItem(sp);
        }
    }
    return true;
}
bool CommonGuiSettings::SaveServerProfileGroup(SimpleXmlNode root, ServerProfileGroup& group) {
    for (auto& item: group.getItems()) {
        SimpleXmlNode serverProfileNode = root.CreateChild("ServerProfileItem");
        SettingsManager mgr;
        SettingsNode& image = mgr.root();
        item.bind(image);

        mgr.saveToXmlNode(serverProfileNode);
    }
    return true;
}

bool CommonGuiSettings::PostLoadSettings(SimpleXml &xml)
{
    BasicSettings::PostLoadSettings(xml);
    SimpleXmlNode settingsNode = xml.getRoot(rootName_).GetChild("Settings");
    LoadServerProfiles(settingsNode.GetChild("Uploading").GetChild("ServerProfiles"));

    auto uploading = settingsNode.GetChild("Uploading");

    ServerProfile oldImageServer, oldFileServer, oldQuickScreenshotServer, oldContextMenuServer;
    // Load the old format of chosen servers (just reading)
    LoadServerProfile(uploading.GetChild("Server"), oldImageServer);
    LoadServerProfile(uploading.GetChild("FileServer"), oldFileServer);
    LoadServerProfile(uploading.GetChild("QuickScreenshotServer"), oldQuickScreenshotServer);
    LoadServerProfile(uploading.GetChild("ContextMenuServer"), oldContextMenuServer);
    /*LoadServerProfile(uploading.GetChild("UrlShorteningServer"), oldUrlShorteningServer);
    LoadServerProfile(uploading.GetChild("TemporaryServer"), oldTemporaryServer);*/

    imageServer = oldImageServer;
    fileServer = oldFileServer;
    quickScreenshotServer = oldQuickScreenshotServer;
    /*urlShorteningServer = oldUrlShorteningServer;
    temporaryServer = oldTemporaryServer;*/

    // Load the new format of chosen servers
    LoadServerProfileGroup(uploading.GetChild("ServerGroup"), imageServer);
    LoadServerProfileGroup(uploading.GetChild("FileServerGroup"), fileServer);
    LoadServerProfileGroup(uploading.GetChild("QuickScreenshotServerGroup"), quickScreenshotServer);
    LoadServerProfileGroup(uploading.GetChild("ContextMenuServerGroup"), contextMenuServer);

    PostLoadServerProfileGroup(imageServer);
    PostLoadServerProfileGroup(fileServer);
    PostLoadServerProfileGroup(contextMenuServer);
    PostLoadServerProfileGroup(quickScreenshotServer);
    PostLoadServerProfile(urlShorteningServer);
    PostLoadServerProfile(temporaryServer);
    PostLoadServerProfile(imageSearchServer);

    return true;
}

void CommonGuiSettings::PostLoadServerProfileGroup(ServerProfileGroup& profileGroup) {
    for(auto& item: profileGroup.getItems()) {
        PostLoadServerProfile(item);
    }
}

void CommonGuiSettings::PostLoadServerProfile(ServerProfile& profile) {
    if (!profile.profileName().empty() && ServersSettings[profile.serverName()].find(profile.profileName()) == ServersSettings[profile.serverName()].end()) {
        profile.setProfileName("");
    }
}

void CommonGuiSettings::BindToManager() {
    BasicSettings::BindToManager();
    SettingsNode& upload = mgr_["Uploading"];
    urlShorteningServer.bind(upload["UrlShorteningServer"]);
    temporaryServer.bind(upload["TemporaryServer"]);
    imageSearchServer.bind(upload["ImageSearchServer"]);
    DefaultImageUploadParams.bind(upload["DefaultImageUploadParams"]);
    ServerListSettings.bind(upload["ServerList"]);

    SettingsNode& video = mgr_["VideoGrabber"];
    video.nm_bind(VideoSettings, NumOfFrames);
    video.nm_bind(VideoSettings, JPEGQuality);
    video.nm_bind(VideoSettings, Engine);

    SettingsNode& screenRecording = mgr_["ScreenRecording"];
    screenRecording.nm_bind(ScreenRecordingSettings, Backend);
    screenRecording.nm_bind(ScreenRecordingSettings, OutDirectory);
    screenRecording.nm_bind(ScreenRecordingSettings, FileNameTemplate);
    screenRecording.nm_bind(ScreenRecordingSettings, FrameRate);
    screenRecording.nm_bind(ScreenRecordingSettings, CaptureCursor);
    screenRecording.nm_bind(ScreenRecordingSettings, Delay);
    screenRecording.nm_bind(ScreenRecordingSettings, MonitorMode);

    ScreenRecordingSettings.FFmpegSettings.bind(screenRecording["FFmpeg"]);
    ScreenRecordingSettings.DXGISettings.bind(screenRecording["DirectX"]);
}

bool CommonGuiSettings::PostSaveSettings(SimpleXml &xml) {
    BasicSettings::PostSaveSettings(xml);
    SaveServerProfiles(xml.getRoot(rootName_).GetChild("Settings").GetChild("Uploading").GetChild("ServerProfiles"));

    SimpleXmlNode uploading = xml.getRoot(rootName_).GetChild("Settings").GetChild("Uploading");
    SaveServerProfileGroup(uploading.GetChild("ServerGroup"), imageServer);
    SaveServerProfileGroup(uploading.GetChild("FileServerGroup"), fileServer);
    SaveServerProfileGroup(uploading.GetChild("QuickScreenshotServerGroup"), quickScreenshotServer);
    SaveServerProfileGroup(uploading.GetChild("ContextMenuServerGroup"), contextMenuServer);
    return true;
}


FFMpegSettingsStruct::FFMpegSettingsStruct() {
#ifdef _WIN32
    VideoSourceId = /*IsWindows8OrGreater() ? "ddagrab" : */"gdigrab";
#endif
}

void FFMpegSettingsStruct::bind(SettingsNode& node) {
    node.n_bind(FFmpegCLIPath);
    node.n_bind(VideoQuality);
    node.n_bind(VideoBitrate);
    node.n_bind(UseQuality);
    node.n_bind(VideoSourceId);
    node.n_bind(VideoCodecId);
    node.n_bind(VideoPresetId);
    node.n_bind(AudioSourceId);
    node.n_bind(AudioCodecId);
    node.n_bind(AudioQuality);
}

void DXGISettingsStruct::bind(SettingsNode& node) {
    node.n_bind(VideoQuality);
    node.n_bind(VideoBitrate);
    node.n_bind(UseQuality);
    node.n_bind(VideoSourceId);
    node.n_bind(VideoCodecId);
    node.n_bind(VideoPresetId);
    node.n_bind(AudioSources);
    node.n_bind(AudioCodecId);
    node.n_bind(AudioBitrate);
}

ThumbCreatingParams ImageUploadParams::getThumb() const {
    auto settings = ServiceLocator::instance()->settings<CommonGuiSettings>();
    if (UseDefaultThumbSettings) {
        return settings->DefaultImageUploadParams.Thumb;
    }
    return Thumb;
}

void ImageUploadParams::setThumb(const ThumbCreatingParams& tcp) {
    Thumb = tcp;
}

bool ServerListSettingsStruct::isServerFavorite(const std::string& server) {
    return FavoriteServers.find(server) != FavoriteServers.end();
}

bool ServerListSettingsStruct::isServerBlacklisted(const std::string& server) {
    return BlacklistedServers.find(server) != BlacklistedServers.end();
}

void ServerListSettingsStruct::addServerToFavorites(const std::string& serverId) {
    removeServerFromBlacklist(serverId);
    FavoriteServers.insert(serverId);
}

void ServerListSettingsStruct::removeServerFromFavorites(const std::string& serverId) {
    auto it = FavoriteServers.find(serverId);
    if (it != FavoriteServers.end()) {
        FavoriteServers.erase(it);
    }
}

void ServerListSettingsStruct::addServerToBlacklist(const std::string& serverId) {
    removeServerFromFavorites(serverId);
    BlacklistedServers.insert(serverId);
}

void ServerListSettingsStruct::removeServerFromBlacklist(const std::string& serverId) {
    auto it = BlacklistedServers.find(serverId);
    if (it != BlacklistedServers.end()) {
        BlacklistedServers.erase(it);
    }
}

void ServerListSettingsStruct::bind(SettingsNode& node) {
    node.n_bind(FavoriteServers);
    node.n_bind(BlacklistedServers);
    node.n_bind(ViewMode);
    node.n_bind(ShowFavoritesOnly);
    node.n_bind(HideBlackListed);
}
