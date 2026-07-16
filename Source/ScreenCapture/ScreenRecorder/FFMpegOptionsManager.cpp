#include "FFMpegOptionsManager.h"

#include <algorithm>

#include <boost/process/v2/environment.hpp>

#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "VideoCodecs/X264VideoCodec.h"
#include "VideoCodecs/NvencVideoCodec.h"
#include "VideoCodecs/QsvVideoCodec.h"
#include "VideoCodecs/AmfVideoCodec.h"
#include "VideoCodecs/VP8VideoCodec.h"
#include "VideoCodecs/VP9VideoCodec.h"
#include "VideoCodecs/XvidVideoCodec.h"
#include "VideoCodecs/WebPVideoCodec.h"
#include "VideoCodecs/GifVideoCodec.h"
#include "AudioCodecs/AacAudioCodec.h"
#include "AudioCodecs/Mp3AudioCodec.h"
#include "AudioCodecs/OpusAudioCodec.h"
#include "Sources/DDAGrabSource.h"
#include "Sources/GDIGrabSource.h"

#ifdef _WIN32
#include "Sources/DirectShowSource.h"
#include "Core/CommonDefs.h"
#include "atlheaders.h"
#include <dshow.h>

IdNameArray GetDirectshowInputDevices(const IID& inputCategory) {
    IdNameArray result;
    CComPtr<ICreateDevEnum> pDevEnum;
    HRESULT hr = pDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum);
    if (FAILED(hr)) {
        LOG(ERROR) << "Failed to create device enumerator!" << std::endl;
        return result;
    }

    CComPtr<IEnumMoniker> pEnum;
    hr = pDevEnum->CreateClassEnumerator(inputCategory, &pEnum, 0);
    if (hr == S_FALSE) {
        return result;
    } else if (FAILED(hr)) {
        LOG(ERROR) << "Failed to enumerate video devices!" << std::endl;
        return result;
    }

    CComPtr<IMoniker> pMoniker;
    CComPtr<IMalloc> pMalloc;
    CoGetMalloc(1, &pMalloc);
    while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
        std::string id, name;
        CComPtr<IBindCtx> pBindCtx;

        CComPtr<IPropertyBag> pPropBag;
        LPOLESTR varDeviceDisplayName {};
        hr = CreateBindCtx(0, &pBindCtx);

        if (SUCCEEDED(hr)) {
            hr = pMoniker->GetDisplayName(pBindCtx, nullptr, &varDeviceDisplayName);
            if (SUCCEEDED(hr) && varDeviceDisplayName) {
                id = W2U(varDeviceDisplayName);
                pMalloc->Free(varDeviceDisplayName);
            }
        }

        hr = pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&pPropBag);
        if (SUCCEEDED(hr)) {
            CComVariant varName;
            
            hr = pPropBag->Read(L"FriendlyName", &varName, nullptr);
            if (SUCCEEDED(hr) && varName.vt == VT_BSTR) {
                name = W2U(varName.bstrVal);
            }
            /*if (id.empty()) {
                CComVariant varDevicePath;
                hr = pPropBag->Read(L"DevicePath", &varDevicePath, nullptr);
                if (SUCCEEDED(hr) && varDevicePath.vt == VT_BSTR) {
                    id = W2U(varDevicePath.bstrVal);
                }
            }*/
        }

        id = IuStringUtils::Replace(id, ":", "_");

        if (!id.empty()) {
            result.emplace_back( "[directshow]" + id, name.empty() ? id : name );
        }

        pMoniker.Release(); 
    }
    return result;
}

#endif

FFMpegOptionsManager::FFMpegOptionsManager() {
    videoCodecFactories_[X264VideoCodec::CODEC_ID] = [] { return std::make_unique<X264VideoCodec>(); };
    videoCodecFactories_[VP8VideoCodec::CODEC_ID] = [] { return std::make_unique<VP8VideoCodec>(); };
    videoCodecFactories_[VP9VideoCodec::CODEC_ID] = [] { return std::make_unique<VP9VideoCodec>(); };
    videoCodecFactories_[XvidVideoCodec::CODEC_ID] = [] { return std::make_unique<XvidVideoCodec>(); };
    videoCodecFactories_[NvencVideoCodec::H264_CODEC_ID] = [] { return NvencVideoCodec::createH264(); };
    videoCodecFactories_[NvencVideoCodec::HEVC_CODEC_ID] = [] { return NvencVideoCodec::createHevc(); };
    videoCodecFactories_[QsvVideoCodec::H264_CODEC_ID] = [] { return QsvVideoCodec::createH264(); };
    videoCodecFactories_[QsvVideoCodec::HEVC_CODEC_ID] = [] { return QsvVideoCodec::createHevc(); };
    videoCodecFactories_[AmfVideoCodec::H264_CODEC_ID] = [] { return AmfVideoCodec::createH264(); };
    videoCodecFactories_[AmfVideoCodec::HEVC_CODEC_ID] = [] { return AmfVideoCodec::createHevc(); };
    videoCodecFactories_[GifVideoCodec::CODEC_ID] = [] { return std::make_unique<GifVideoCodec>(); };
    videoCodecFactories_[WebPVideoCodec::CODEC_ID] = [] { return std::make_unique<WebPVideoCodec>(); };

#ifdef _WIN32
    videoSourceFactories_[GDIGrabSource::SOURCE_ID] = [](const std::string& param) { return std::make_unique<GDIGrabSource>(); };
    if (IsWindows8OrGreater()) {
        videoSourceFactories_[DDAGrabSource::SOURCE_ID] = [](const std::string& param) { return std::make_unique<DDAGrabSource>(); };
    }

    videoSourceFactories_[DirectShowSource::SOURCE_ID] = [](const std::string& deviceId) { return std::make_unique<DirectShowSource>(deviceId); };
    audioSourceFactories_[DirectShowSource::SOURCE_ID] = [](const std::string& deviceId) { return std::make_unique<FFmpegSource>("DirectShow (fake)", true); };
#endif

    audioCodecFactories_[AacAudioCodec::CODEC_ID] = [] { return std::make_unique<AacAudioCodec>(); };
    audioCodecFactories_[Mp3AudioCodec::CODEC_ID] = [] { return std::make_unique<Mp3AudioCodec>(); };
    audioCodecFactories_[OpusAudioCodec::CODEC_ID] = [] { return std::make_unique<OpusAudioCodec>(); };
}

std::optional<FFMpegOptionsManager::VideoCodecInfo> FFMpegOptionsManager::getVideoCodecInfo(const std::string& codecId) {
    auto codec = createVideoCodec(codecId);
    if (!codec) {
        return std::nullopt;
    }
    VideoCodecInfo info;
    info.CodecId = codecId;
    info.CodecName = codec->name();
    info.Presets = codec->presets();
    info.DefaultPresetId = codec->defaultPreset();
    info.CanUseBitrate = codec->canUseBitrate();
    info.CanUseQuality = codec->canUseQuality();
    return info;
}

std::optional<FFMpegOptionsManager::AudioCodecInfo> FFMpegOptionsManager::getAudioCodecInfo(const std::string& codecId) {
    auto codec = createAudioCodec(codecId);
    if (!codec) {
        return std::nullopt;
    }
    AudioCodecInfo info;
    info.CodecId = codecId;
    info.CodecName = codec->name();
    info.Qualities = codec->qualities();
    info.DefaultQuality = codec->defaultQuality();
    info.CanUseBitrate = codec->canUseBitrate();
    info.CanUseQuality = codec->canUseQuality();
    return info;
}

std::unique_ptr<FFmpegVideoCodec> FFMpegOptionsManager::createVideoCodec(const std::string& codecId) {
    auto it = videoCodecFactories_.find(codecId);
    if (it != videoCodecFactories_.end()) {
        return it->second();
    }
    return {};
}

std::unique_ptr<FFmpegSource> FFMpegOptionsManager::createSource(const std::string& id) {
    const auto [sourceId, param] = FFmpegSource::parseSourceId(id);
   
    auto it = videoSourceFactories_.find(sourceId);
    if (it != videoSourceFactories_.end()) {
        return it->second(param);
    }
    return {};
}

std::unique_ptr<AudioCodec> FFMpegOptionsManager::createAudioCodec(const std::string& codecId) {
    auto it = audioCodecFactories_.find(codecId);
    if (it != audioCodecFactories_.end()) {
        return it->second();
    }
    return {};
}

IdNameArray FFMpegOptionsManager::getVideoCodecs() {
    IdNameArray result;

    for (const auto& [codecId, v] : videoCodecFactories_) {
        auto codec = createVideoCodec(codecId);
        if (!codec) {
            continue;
        }
        result.emplace_back(codecId, codec->name());
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return IuStringUtils::stricmp(a.second.c_str(), b.second.c_str()) < 0;
    });

    return result;
}

IdNameArray FFMpegOptionsManager::getAudioCodecs() {
    IdNameArray result;

    for (const auto& [codecId, v] : audioCodecFactories_) {
        auto codec = createAudioCodec(codecId);
        if (!codec) {
            continue;
        }
        result.emplace_back(codecId, codec->name());
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return IuStringUtils::stricmp(a.second.c_str(), b.second.c_str()) < 0;
    });

    return result;
}

IdNameArray FFMpegOptionsManager::getVideoSources() {
    IdNameArray result;

    auto compareFunc = [](const IdNamePair& a, const IdNamePair& b) {
        return IuStringUtils::stricmp(a.second.c_str(), b.second.c_str()) < 0;
    };

    for (const auto& [sourceId, v] : videoSourceFactories_) {
        auto source = createSource(sourceId);
        if (!source || source->hidden()) {
            continue;
        }
        result.emplace_back(sourceId, source->name());
    }
    std::sort(result.begin(), result.end(), compareFunc);

#ifdef _WIN32
    IdNameArray inputDevices = GetDirectshowInputDevices(CLSID_VideoInputDeviceCategory);
    std::sort(inputDevices.begin(), inputDevices.end(), compareFunc);
    result.insert(result.end(), inputDevices.begin(), inputDevices.end());
#endif

    return result;
}

IdNameArray FFMpegOptionsManager::getAudioSources() {
    IdNameArray result;

    auto compareFunc = [](const IdNamePair& a, const IdNamePair& b) {
        return IuStringUtils::stricmp(a.second.c_str(), b.second.c_str()) < 0;
    };

    for (const auto& [sourceId, v] : audioSourceFactories_) {
        auto source = createSource(sourceId);
        if (!source || source->hidden()) {
            continue;
        }
        result.emplace_back(sourceId, source->name());
    }
    std::sort(result.begin(), result.end(), compareFunc);

#ifdef _WIN32
    IdNameArray inputDevices = GetDirectshowInputDevices(CLSID_AudioInputDeviceCategory);
    std::sort(inputDevices.begin(), inputDevices.end(), compareFunc);
    result.insert(result.end(), inputDevices.begin(), inputDevices.end());
#endif

    return result;
}

std::string FFMpegOptionsManager::findFFmpegExecutable() {
    return boost::process::v2::environment::find_executable("ffmpeg").string();
}
