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

#include "MediaInfoHelper.h"

#include <strsafe.h>
#include <MediaInfo/MediaInfo.h>

#include "Core/CommonDefs.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/i18n/Translator.h"
#include "WinUtils.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/ServiceLocator.h"

#define TR_COND(s) (enableLocalization ? TR(s): _T(s))

namespace MediaInfoHelper {

// Заменяет все вхождения from на to в строке str
void StrReplaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::wstring::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

// Добавляет str2 в поток, если он не пуст
inline void AddStr(std::wostringstream& out, const CString& str2)
{
    if (str2.GetLength() > 0) out << str2.GetString();
}

// Добавляет prefix + str2 + postfix в поток, если str2 не пуст
inline void AddStr(std::wostringstream& out, const CString& str2, const CString& postfix,
                    const CString& prefix = CString(_T("")))
{
    if (str2.GetLength() > 0) out << prefix.GetString() << str2.GetString() << postfix.GetString();
}


#define VIDEO(a) mi.Get(Stream_Video, 0, _T(a), Info_Text, Info_Name).c_str()
#define AUDIO(n, a) mi.Get(Stream_Audio, n, _T(a), Info_Text, Info_Name).c_str()

std::string TimestampToStr(int64_t duration, int64_t units) {
    int hours, mins, secs, us;
    secs = static_cast<int>(duration / units);
    us = static_cast<int>(duration % units);
    mins = secs / 60;
    secs %= 60;
    hours = mins / 60;
    mins %= 60;
    char buffer[100];
    sprintf(buffer, "%02d:%02d:%02d", hours, mins, secs/*, (int)((100 * us) / units)*/);
    return buffer;
}

bool FindMediaInfoDllPath() {
    /*MediaInfoDllPath = 0;

    CString MediaDll = WinUtils::GetAppFolder() + _T("\\Modules\\MediaInfo.dll");
    if (WinUtils::FileExists(MediaDll)) {
        StringCchCopy(MediaInfoDllPath, ARRAY_SIZE(MediaInfoDllPath), MediaDll);
        return true;
    } else {
        HKEY ExtKey;
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\KLCodecPack"), 0,KEY_QUERY_VALUE, &ExtKey/* NULL);
        TCHAR ClassName[MAX_PATH] = _T("\0");
        DWORD BufSize = sizeof(ClassName) / sizeof(TCHAR);
        DWORD Type = REG_SZ;
        RegQueryValueEx(ExtKey, _T("installdir"), 0, &Type, reinterpret_cast<LPBYTE>(&ClassName), &BufSize);
        RegCloseKey(ExtKey);
        CString MediaDll2 = CString(ClassName) + _T("\\Tools\\MediaInfo.dll");
        if (WinUtils::FileExists(MediaDll2)) {
            StringCchCopy(MediaInfoDllPath, ARRAY_SIZE(MediaInfoDllPath), MediaDll2);
            return true;
        }
    }*/
    return false;
}

bool IsMediaInfoAvailable() {
    return true;
}

bool GetMediaFileInfo(LPCWSTR fileName, CString& buffer, CString& fullInfo, bool enableLocalization) {
    using namespace MediaInfoLib;
    MediaInfo mi;

    mi.Option(__T("Internet"), __T(""));
    if (enableLocalization) {
        CString path = WinUtils::GetAppFolder();
        CString langDir = path + CString(_T("Modules\\MediaInfoLang\\"));
        auto* translator = ServiceLocator::instance()->translator();
        std::string locale = translator->getCurrentLocale();
        CString lang = U2W(locale).Left(2);
        if (!lang.IsEmpty()) {
            CString langFilePath = langDir + lang + _T(".csv");
            std::string langFileContents;
            if (IuCoreUtils::ReadUtf8TextFile(W2U(langFilePath), langFileContents)) {
                mi.Option(__T("Language"), IuCoreUtils::Utf8ToWstring(langFileContents));
            }
        }
    } else {
        mi.Option(__T("Language"), _T(""));
    }

    mi.Open(fileName);

    std::wostringstream result;
    CString videoFormat, videoVersion, videoCodec, videoFrameRate, videoBitrate, videoNominalBitrate, videoBitsperPixel;
    CString audioFormat, audioFormatProfile, audioSampleRate, audioChannels, audioBitrate, audioBitrateMode,
            audioLanguage;

    int count = mi.Count_Get(Stream_Audio); //Count of audio streams in file
    int VideoCount = mi.Count_Get(Stream_Video); //Count of video streams in file
    int SubsCount = mi.Count_Get(Stream_Text);
    fullInfo = mi.Inform().c_str();

    result << TR_COND("Filename: ") << WinUtils::DoExtractFileName(fileName).GetString() << L"\r\n";

    CString fileSize = mi.Get(Stream_General, 0, _T("FileSize/String"), Info_Text, Info_Name).c_str();
    fileSize.Replace(_T("iB"), _T("B")); // MiB --> MB
    result << TR_COND("Filesize: ") << fileSize.GetString() << L"\r\n";

    CString Duration;
    CString DurationStr = mi.Get(Stream_General, 0, _T("Duration"), Info_Text, Info_Name).c_str();
    if (!DurationStr.IsEmpty()) {
        uint64_t duration = IuCoreUtils::StringToInt64(W2U(DurationStr));
        Duration = U2W(TimestampToStr(duration, 1000));
    } else {
        Duration = mi.Get(Stream_General, 0, _T("Duration/String"), Info_Text, Info_Name).c_str();
    }

    AddStr(result, Duration, _T("\r\n"), TR_COND("Duration: "));

    if (count + VideoCount > 1) {// if file contains более одного аудио/видео потока
        AddStr(result, mi.Get(Stream_General, 0, _T("OverallBitRate/String"), Info_Text, Info_Name).c_str(),
               _T("\r\n"), TR_COND("Overall bitrate: "));
    }

    if (VideoCount) { // Если файл содержит один или несколько видеопотоков
        // Информация берётся только из первого видеопотока
        // (поддержки нескольких видеопотоков нет)
        std::wostringstream videoTotal;
        videoTotal << TR_COND("Video: ");

        videoFormat = VIDEO("Format");
        videoVersion = VIDEO("Format_Version");

        videoTotal << videoFormat.GetString();

        videoVersion.Replace(_T("Version "), _T(""));
        if (videoVersion.GetLength()) {
            videoTotal << L" " << videoVersion.GetString();
        }

        videoCodec = VIDEO("CodecID/Hint");
        if (videoCodec.GetLength())
            videoTotal << L", " << videoCodec.GetString();

        videoTotal << L", " << VIDEO("Width") << L"x" << VIDEO("Height");

        std::wstring videoTotalStr = videoTotal.str();
        StrReplaceAll(videoTotalStr, L"MPEG Video", L"MPEG");
        StrReplaceAll(videoTotalStr, L"MPEG-4 Visual", L"MPEG4");
        videoTotal.str(videoTotalStr);
        videoTotal.seekp(0, std::ios_base::end);

        CString displayRatio = VIDEO("DisplayAspectRatio/String");
        if (!displayRatio.IsEmpty()) {
            videoTotal << L" (" << displayRatio.GetString() << L")";
        }

        videoFrameRate = VIDEO("FrameRate");
        if (!videoFrameRate.IsEmpty()) {
            videoTotal << L", " << videoFrameRate.GetString() << L" fps";
        }

        videoNominalBitrate = VIDEO("BitRate_Nominal/String");
        videoBitrate = VIDEO("BitRate/String");

        if (!videoNominalBitrate.IsEmpty()) // Номинальный битрейт приоритетнее фактического
        {
            videoTotal << L", " << videoNominalBitrate.GetString();
        } else if (videoBitrate.GetLength()) {
            videoTotal << L", " << videoBitrate.GetString();
        }

        videoBitsperPixel = VIDEO("Bits-(Pixel*Frame)");
        if (!videoBitsperPixel.IsEmpty()) {
            videoTotal << L" (" << videoBitsperPixel.GetString() << L" bit/pixel)";
        }

        result << videoTotal.str() << L"\r\n";
    }  // Конец получения информации о видеопотоке

    for (int i = 0; i < count; i++) {
        std::wostringstream AudioTotal;
        CString buf;
        buf.Format(CString(TR_COND("Audio")) + _T(" #%d: "), i + 1);

        if (count > 1)
            AudioTotal << buf.GetString();
        else
            AudioTotal << TR_COND("Audio") << L": ";

        audioFormat = AUDIO(i, "Format");
        audioFormatProfile = AUDIO(i, "Format_Profile");
        audioSampleRate = AUDIO(i, "SamplingRate/String");
        audioChannels = AUDIO(i, "Channel(s)");
        audioBitrate = AUDIO(i, "BitRate/String");
        audioBitrateMode = AUDIO(i, "BitRate_Mode");
        audioLanguage = AUDIO(i, "Language/String");

        AddStr(AudioTotal, audioFormat);
        AddStr(AudioTotal, audioFormatProfile, _T(""), _T(" "));
        AddStr(AudioTotal, audioSampleRate, _T(""), _T(", "));
        AddStr(AudioTotal, audioChannels, _T(" ch"), _T(", "));
        AddStr(AudioTotal, audioBitrate, _T(""), _T(", "));
        AddStr(AudioTotal, audioBitrateMode, _T(""), _T(", "));
        AddStr(AudioTotal, audioLanguage, _T(")"), _T(" ("));
        AudioTotal << L"\r\n";

        std::wstring audioTotalStr = AudioTotal.str();
        StrReplaceAll(audioTotalStr, L"MPEG Audio Layer ", L"MP");
        StrReplaceAll(audioTotalStr, L" ,", L"");
        StrReplaceAll(audioTotalStr, L",,", L",");

        result << audioTotalStr;
    } // Конец получения информации об аудиопотоках

    if (SubsCount > 0) {
        std::wostringstream SubsTotal;

        for (int i = 0; i < SubsCount; i++) {
            CString mode = mi.Get(Stream_Text, i, _T("Language_More"), Info_Text, Info_Name).c_str();
            AddStr(SubsTotal, mi.Get(Stream_Text, i, _T("Language/String"), Info_Text, Info_Name).c_str(),
                   _T(""), i ? _T(", ") : _T(""));

            if (mode == _T("Forced")) AddStr(SubsTotal, _T(" (forced)"));
        }

        std::wstring subsTotalStr = SubsTotal.str();
        if (!subsTotalStr.empty()) {
            result << TR_COND("Subtitles: ") << subsTotalStr << L"\r\n";
        }
    }

    mi.Close();
    buffer = result.str().c_str();
    return true;
}

CString GetLibraryVersion() {
    MediaInfoLib::MediaInfo MI;
    return MI.Option(__T("Info_Version")).c_str();
}

CString GetLibraryPath() {
    return L"";
}

}
