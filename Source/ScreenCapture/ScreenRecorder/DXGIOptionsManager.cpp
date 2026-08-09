#include "DXGIOptionsManager.h"

#include <algorithm>

#include <mfapi.h>

#include "Func/WinUtils.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Core/CommonDefs.h"
#include "../3rdpart/capture.hpp"

namespace {

std::string GuidToStr(const GUID& guid){
    return W2U(WinUtils::GUIDToString(guid));
}

}

DXGIOptionsManager::DXGIOptionsManager() {
    videoCodecs_ = {
        { GuidToStr(MFVideoFormat_H264), "H.264" },
        //{ GuidToStr(MFVideoFormat_H265), "H.265" },
        { GuidToStr(MFVideoFormat_HEVC), "HEVC" },
        { GuidToStr(MFVideoFormat_VP80), "VP8" },
        { GuidToStr(MFVideoFormat_VP90), "VP9" },
        { GuidToStr(MFVideoFormat_WMV3), "WMV" },
    };

    audioCodecs_ = {
        { GuidToStr(MFAudioFormat_AAC), "AAC" },
        { GuidToStr(MFAudioFormat_MP3), "MP3" },
        { GuidToStr(MFAudioFormat_FLAC), "FLAC" },
        //{ GuidToStr(MFAudioFormat_WMAudioV9), "WMA" },
        //{ GuidToStr(MFAudioFormat_Vorbis), "Vorbis" },
    };

    std::vector<VISTAMIXER> mixers;
    EnumVistaMixers(mixers);
    for (const auto& mixer : mixers) {
        AUDCLNT_SHAREMODE shareMode = AUDCLNT_SHAREMODE_SHARED;
        if (mixer.Mode == 2) {
            shareMode = AUDCLNT_SHAREMODE_EXCLUSIVE;
        } 
        AudioSource source;
        source.Name = IuCoreUtils::WstringToUtf8(mixer.name);
        source.NumChannels = mixer.NumChannels(shareMode);
        audioSources_[IuCoreUtils::WstringToUtf8(mixer.id)] = std::move(source);
    }
}

std::optional<DXGIOptionsManager::VideoCodecInfo> DXGIOptionsManager::getVideoCodecInfo(const std::string& codecId) {
    VideoCodecInfo info;
    info.CodecId = codecId;
    return info;
}

std::optional<DXGIOptionsManager::AudioCodecInfo> DXGIOptionsManager::getAudioCodecInfo(const std::string& codecId) {
    AudioCodecInfo info;
    info.CodecId = codecId;
    return info;
}

std::optional<DXGIOptionsManager::AudioSource> DXGIOptionsManager::getAudioSourceInfo(const std::string& sourceId) {
    const auto it = audioSources_.find(sourceId);
    if (it != audioSources_.end()) {
        return it->second;
    }
    return std::nullopt;
}

IdNameArray DXGIOptionsManager::getVideoCodecs() {
    return videoCodecs_;
}

IdNameArray DXGIOptionsManager::getAudioCodecs() {
    return audioCodecs_;
}

IdNameArray DXGIOptionsManager::getVideoSources() {
    IdNameArray result;

    /* auto compareFunc = [](const IdNamePair& a, const IdNamePair& b) {
        return IuStringUtils::stricmp(a.second.c_str(), b.second.c_str()) < 0;
    };

    for (const auto& [sourceId, v] : videoSourceFactories_) {
        auto source = createSource(sourceId);
        if (!source || source->hidden()) {
            continue;
        }
        result.push_back(std::make_pair(sourceId, source->name()));
    }
    std::sort(result.begin(), result.end(), compareFunc);

#ifdef _WIN32
    IdNameArray inputDevices = GetDirectshowInputDevices(CLSID_VideoInputDeviceCategory);
    std::sort(inputDevices.begin(), inputDevices.end(), compareFunc);
    result.insert(result.end(), inputDevices.begin(), inputDevices.end());
#endif
*/
    return result;
}

IdNameArray DXGIOptionsManager::getAudioSources() {
    IdNameArray result;
    
    for (const auto& [id, source]: audioSources_) {
        result.emplace_back( id, source.Name );
    }

    auto compareFunc = [](const IdNamePair& a, const IdNamePair& b) {
        return IuStringUtils::stricmp(a.second.c_str(), b.second.c_str()) < 0;
    };

    std::sort(result.begin(), result.end(), compareFunc);

    return result;
}
