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

#include "MediaInfo/MediaInfoHelper.h"

#include <cstdint>
#include <iomanip>
#include <sstream>

#include <MediaInfo/MediaInfo.h>

#include "../Func/WinUtils.h"
#include "Core/CommonDefs.h"
#include "Core/ServiceLocator.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/i18n/Translator.h"

namespace MediaInfoHelper {
namespace {

void ReplaceAll(std::string& value, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return;
    }

    size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos) {
        value.replace(pos, from.length(), to);
        pos += to.length();
    }
}

void AddString(std::stringstream& output, const std::string& value) {
    if (!value.empty()) {
        output << value;
    }
}

void AddString(std::stringstream& output, const std::string& value, const std::string& postfix,
               const std::string& prefix = { }) {
    if (!value.empty()) {
        output << prefix << value << postfix;
    }
}

MediaInfoLib::String U8TOMI(const std::string& value) {
#if defined(UNICODE) || defined(_UNICODE)
    return IuCoreUtils::Utf8ToWstring(value);
#else
    return value;
#endif
}

std::string MITOU8(const MediaInfoLib::String& value) {
#if defined(UNICODE) || defined(_UNICODE)
    return IuCoreUtils::WstringToUtf8(value);
#else
    return value;
#endif
}

std::string TimestampToString(int64_t duration, int64_t units) {
    int64_t seconds = duration / units;
    const int64_t minutes = seconds / 60;
    seconds %= 60;
    const int64_t hours = minutes / 60;

    std::stringstream output;
    output << std::setfill('0') << std::setw(2) << hours << ':' << std::setw(2) << minutes % 60 << ':' << std::setw(2)
           << seconds;
    return output.str();
}

std::string GetValue(MediaInfoLib::MediaInfo& mediaInfo, MediaInfoLib::stream_t stream, size_t streamNumber,
                     const char* field) {
    return MITOU8(mediaInfo.Get(stream, streamNumber, U8TOMI(field),
                                               MediaInfoLib::Info_Text, MediaInfoLib::Info_Name));
}

std::string Translate(const char* text, bool enableLocalization) { return enableLocalization ? _(text).str() : text; }

} // namespace

bool GetMediaFileInfo(const std::string& fileName, std::string& summary, std::string& fullInfo,
                      bool enableLocalization) {
    using namespace MediaInfoLib;

    MediaInfo mediaInfo;
    mediaInfo.Option(U8TOMI("Internet"), U8TOMI(""));

    if (enableLocalization) {
        const std::string languageDirectory = W2U(WinUtils::GetAppFolder()) + "Modules\\MediaInfoLang\\";
        const std::string locale = ServiceLocator::instance()->translator()->getCurrentLocale();
        const std::string language = locale.substr(0, 2);
        if (!language.empty()) {
            std::string languageFileContents;
            if (IuCoreUtils::ReadUtf8TextFile(languageDirectory + language + ".csv", languageFileContents)) {
                mediaInfo.Option(U8TOMI("Language"), U8TOMI(languageFileContents));
            }
        }
    } else {
        mediaInfo.Option(U8TOMI("Language"), U8TOMI(""));
    }

    mediaInfo.Open(U8TOMI(fileName));

    const size_t audioCount = mediaInfo.Count_Get(Stream_Audio);
    const size_t videoCount = mediaInfo.Count_Get(Stream_Video);
    const size_t subtitleCount = mediaInfo.Count_Get(Stream_Text);
    fullInfo = MITOU8(mediaInfo.Inform());

    std::stringstream result;
    result << Translate("Filename: ", enableLocalization) << IuCoreUtils::ExtractFileName(fileName) << "\r\n";

    std::string fileSize = GetValue(mediaInfo, Stream_General, 0, "FileSize/String");
    ReplaceAll(fileSize, "iB", "B");
    result << Translate("Filesize: ", enableLocalization) << fileSize << "\r\n";

    std::string duration;
    const std::string durationValue = GetValue(mediaInfo, Stream_General, 0, "Duration");
    if (!durationValue.empty()) {
        duration = TimestampToString(IuCoreUtils::StringToInt64(durationValue), 1000);
    } else {
        duration = GetValue(mediaInfo, Stream_General, 0, "Duration/String");
    }
    AddString(result, duration, "\r\n", Translate("Duration: ", enableLocalization));

    if (audioCount + videoCount > 1) {
        AddString(result, GetValue(mediaInfo, Stream_General, 0, "OverallBitRate/String"), "\r\n",
                  Translate("Overall bitrate: ", enableLocalization));
    }

    if (videoCount != 0) {
        std::stringstream video;
        video << Translate("Video: ", enableLocalization);
        video << GetValue(mediaInfo, Stream_Video, 0, "Format");

        std::string version = GetValue(mediaInfo, Stream_Video, 0, "Format_Version");
        ReplaceAll(version, "Version ", "");
        if (!version.empty()) {
            video << ' ' << version;
        }

        const std::string codec = GetValue(mediaInfo, Stream_Video, 0, "CodecID/Hint");
        if (!codec.empty()) {
            video << ", " << codec;
        }

        video << ", " << GetValue(mediaInfo, Stream_Video, 0, "Width") << 'x'
              << GetValue(mediaInfo, Stream_Video, 0, "Height");

        std::string videoText = video.str();
        ReplaceAll(videoText, "MPEG Video", "MPEG");
        ReplaceAll(videoText, "MPEG-4 Visual", "MPEG4");
        video.str({ });
        video.clear();
        video << videoText;

        const std::string displayRatio = GetValue(mediaInfo, Stream_Video, 0, "DisplayAspectRatio/String");
        if (!displayRatio.empty()) {
            video << " (" << displayRatio << ')';
        }

        const std::string frameRate = GetValue(mediaInfo, Stream_Video, 0, "FrameRate");
        if (!frameRate.empty()) {
            video << ", " << frameRate << " fps";
        }

        const std::string nominalBitrate = GetValue(mediaInfo, Stream_Video, 0, "BitRate_Nominal/String");
        const std::string bitrate = GetValue(mediaInfo, Stream_Video, 0, "BitRate/String");
        if (!nominalBitrate.empty()) {
            video << ", " << nominalBitrate;
        } else if (!bitrate.empty()) {
            video << ", " << bitrate;
        }

        const std::string bitsPerPixel = GetValue(mediaInfo, Stream_Video, 0, "Bits-(Pixel*Frame)");
        if (!bitsPerPixel.empty()) {
            video << " (" << bitsPerPixel << " bit/pixel)";
        }

        result << video.str() << "\r\n";
    }

    for (size_t i = 0; i < audioCount; ++i) {
        std::stringstream audio;
        if (audioCount > 1) {
            audio << Translate("Audio", enableLocalization) << " #" << i + 1 << ": ";
        } else {
            audio << Translate("Audio", enableLocalization) << ": ";
        }

        AddString(audio, GetValue(mediaInfo, Stream_Audio, i, "Format"));
        AddString(audio, GetValue(mediaInfo, Stream_Audio, i, "Format_Profile"), "", " ");
        AddString(audio, GetValue(mediaInfo, Stream_Audio, i, "SamplingRate/String"), "", ", ");
        AddString(audio, GetValue(mediaInfo, Stream_Audio, i, "Channel(s)"), " ch", ", ");
        AddString(audio, GetValue(mediaInfo, Stream_Audio, i, "BitRate/String"), "", ", ");
        AddString(audio, GetValue(mediaInfo, Stream_Audio, i, "BitRate_Mode"), "", ", ");
        AddString(audio, GetValue(mediaInfo, Stream_Audio, i, "Language/String"), ")", " (");
        audio << "\r\n";

        std::string audioText = audio.str();
        ReplaceAll(audioText, "MPEG Audio Layer ", "MP");
        ReplaceAll(audioText, " ,", "");
        ReplaceAll(audioText, ",,", ",");
        result << audioText;
    }

    if (subtitleCount != 0) {
        std::stringstream subtitles;
        for (size_t i = 0; i < subtitleCount; ++i) {
            AddString(subtitles, GetValue(mediaInfo, Stream_Text, i, "Language/String"), "", i != 0 ? ", " : "");
            if (GetValue(mediaInfo, Stream_Text, i, "Language_More") == "Forced") {
                AddString(subtitles, " (forced)");
            }
        }

        if (!subtitles.str().empty()) {
            result << Translate("Subtitles: ", enableLocalization) << subtitles.str() << "\r\n";
        }
    }

    mediaInfo.Close();
    summary = result.str();
    return true;
}

std::string GetLibraryVersion() {
    MediaInfoLib::MediaInfo mediaInfo;
    return MITOU8(mediaInfo.Option(U8TOMI("Info_Version")));
}

} // namespace MediaInfoHelper
