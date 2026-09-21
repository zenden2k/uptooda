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

#include "CoreUtils.h"

#include <ctime>
#include <cmath>
#include <locale>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <sstream>

#ifdef _WIN32
    #include <WinSock.h>
    #include "Func/WinUtils.h"
#else
    #ifdef __APPLE__
        #include <sys/uio.h>
    #else
        //#include <sys/io.h>
    #endif
    #include <sys/stat.h>
#endif

#include "Core/3rdpart/UriParser.h"
#include "Core/3rdpart/xdgmime/xdgmime.h"
#include "MimeTypeHelper.h"
#include "StringUtils.h"
#include "IOException.h"

#ifdef _WIN32

/* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = ((unsigned __int64)116444736000000000ULL);

/*
* PostgreSQL's implementation of gettimeofday for windows:
*
* timezone information is stored outside the kernel so tzp isn't used anymore.
*
* Note: this function is not for Win32 high precision timing purpose. See
* elapsed_time().
*/
int
gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    FILETIME    file_time;
    SYSTEMTIME  system_time;
    ULARGE_INTEGER ularge;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;

    tp->tv_sec = static_cast<long>((ularge.QuadPart - epoch) / 10000000L);
    tp->tv_usec = static_cast<long>(system_time.wMilliseconds * 1000);

    return 0;
}
#endif

namespace IuCoreUtils {

    FILE * FopenUtf8(const char * filename, const char * mode)
{
#ifdef _MSC_VER
    return _wfopen(Utf8ToWstring(filename).c_str(), Utf8ToWstring(mode).c_str());
#else
    return fopen(Utf8ToSystemLocale(filename).c_str(), mode);
#endif
}

int Fseek64(FILE* stream, int64_t offset, int origin)
{
#ifdef _MSC_VER
    return _fseeki64(stream, offset, origin);
#else
    return fseeko64(stream, offset, origin);
#endif
}

int64_t Ftell64(FILE *a)
{
#ifdef __CYGWIN__
    return ftell(a);
#elif defined (_WIN32)
    return _ftelli64(a);
#else
    return ftello(a);
#endif
}

bool FileExists(const std::string& fileName) {
    std::error_code ec;
    bool exists = std::filesystem::exists(std::filesystem::u8path(fileName), ec);
    return !ec && exists;
}

std::codecvt_base::result fromWstring(const std::wstring& str, const std::locale& loc, std::string& out) {
    typedef std::codecvt<wchar_t, char, std::mbstate_t> codecvt_type;

    const codecvt_type& cdcvt = std::use_facet<codecvt_type>(loc);
    std::mbstate_t state {};

    std::size_t max_len = str.size() * cdcvt.max_length();
    std::vector<char> chars(max_len + 1);

    const wchar_t* in_next = nullptr;
    char* out_next = nullptr;

    std::codecvt_base::result r = cdcvt.out(
        state,
        str.c_str(), str.c_str() + str.size(), in_next,
        chars.data(), chars.data() + max_len, out_next);

    if (r == std::codecvt_base::ok || r == std::codecvt_base::noconv) {
        out.assign(chars.data(), out_next - chars.data());
    } else {
        out.clear();
    }

    return r;
}

std::codecvt_base::result toWstring(const std::string& str, const std::locale& loc, std::wstring& out) {
    using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;

    const codecvt_type& cdcvt = std::use_facet<codecvt_type>(loc);
    std::mbstate_t state {};

    std::vector<wchar_t> wchars(str.size() + 1);

    const char* in_next = nullptr;
    wchar_t* out_next = nullptr;

    std::codecvt_base::result r = cdcvt.in(
        state,
        str.c_str(),
        str.c_str() + str.size(),
        in_next,
        wchars.data(),
        wchars.data() + wchars.size(),
        out_next);

    if (r == std::codecvt_base::ok || r == std::codecvt_base::noconv) {
        out.assign(wchars.data(), out_next);
    }

    return r;
}

std::string SystemLocaleToUtf8(const std::string& str) {
    try {
        std::locale const oloc = std::locale("");
        std::wstring wideStr;

        std::codecvt_base::result result = toWstring(str, oloc, wideStr);

        if (result != std::codecvt_base::ok && result != std::codecvt_base::noconv) {
            return str;
        }

        return WstringToUtf8(wideStr);

    } catch (const std::exception& e) {
        return str;
    }
}

std::string Utf8ToSystemLocale(const std::string& str)
{
   std::wstring wideStr = Utf8ToWstring(str);
   std::locale const oloc = std::locale ("");
   std::string out;
   /*std::codecvt_base::result r = */fromWstring (wideStr, oloc, out);
   return out;
}

std::string ExtractFileName(const std::string& fileName) {
    /*std::string temp = fileName;
    /*int Qpos = temp.find_last_of('?');
    if(Qpos>=0) temp = temp.substr(0, Qpos-1);*/
    int i, len = fileName.length();
    for (i = len - 1; i >= 0; i--) {
        if (fileName[i] == '\\' || fileName[i] == '/') {
            break;
        }
    }
    return fileName.substr(i + 1);
}

std::string ExtractFileExt(const std::string& fileName) {
    if (fileName.empty()) {
        return {};
    }

    size_t len = fileName.length();

    for (size_t i = len; i > 0; i--) {
        char ch = fileName[i - 1];
        if (ch == '.') {
            return fileName.substr(i);
        }

        if (ch == '\\' || ch == '/') {
            break;
        }
    }
    return {};
}

std::string ExtractFilePath(const std::string& fileName) {
    if (fileName.empty()) {
        return {};
    }

    size_t pos = fileName.find_last_of("/\\");

    if (pos != std::string::npos) {
        return fileName.substr(0, pos + 1);
    }

    return {};
}

std::string ExtractFileNameNoExt(const std::string& fileName)
{
    std::string result = ExtractFileName(fileName);
    size_t pos = result.find_last_of('.');
    if (pos != std::string::npos) {
        result = result.substr(0, pos);
    }
    return result;
}

std::string ExtractFileNameFromUrl(const std::string& url)
{
    const uriparser::Uri uri(url);
    return ExtractFileName(uri.path());
}

std::string IncrementFileName(const std::string& originalFileName, int counter) {
    const std::string ext = ExtractFileExt(originalFileName);
    std::string name = ExtractFileNameNoExt(originalFileName);
    name += "(" + std::to_string(counter) + ")";
    if (!ext.empty()) {
        name += "." + ext;
    }
    return name;
}

bool ReadUtf8TextFile(const std::string& utf8Filename, std::string& data)
{
    std::ifstream file(std::filesystem::u8path(utf8Filename), std::ios::binary);
    if (!file) {
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    if (!file) {
        return false;
    }

    auto fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    if (!file) {
        return false;
    }

    if (fileSize == 0) {
        data.clear();
        return true;
    }

    // Read first bytes to determine encoding
    std::vector<unsigned char> header(std::min(fileSize, size_t { 4 }));
    file.read(reinterpret_cast<char*>(header.data()), header.size());
    if (!file && !file.eof()) {
        return false;
    }

    size_t headerBytesRead = static_cast<size_t>(file.gcount());
    if (headerBytesRead == 0) {
        return false;
    }

    size_t contentOffset = 0;
    bool isUtf16Le = false;
    bool isUtf16Be = false;

    // Check BOM
    if (headerBytesRead >= 3 && header[0] == 0xEF && header[1] == 0xBB && header[2] == 0xBF) {
        // UTF-8 BOM
        contentOffset = 3;
    } else if (headerBytesRead >= 2) {
        if (header[0] == 0xFF && header[1] == 0xFE) {
            // UTF-16 LE BOM
            isUtf16Le = true;
            contentOffset = 2;
        } else if (header[0] == 0xFE && header[1] == 0xFF) {
            // UTF-16 BE BOM
            isUtf16Be = true;
            contentOffset = 2;
        }
    }

    // Handle UTF-16
    if (isUtf16Le || isUtf16Be) {
        if (fileSize < contentOffset) {
            data.clear();
            return true;
        }

        size_t utf16Size = fileSize - contentOffset;
        // For odd size, read only even number of bytes
        size_t utf16BytesToRead = utf16Size - (utf16Size % 2);

        if (utf16BytesToRead == 0) {
            data.clear();
            return true;
        }

        file.seekg(contentOffset, std::ios::beg);
        if (!file) {
            return false;
        }

        std::vector<char16_t> utf16Data(utf16BytesToRead / 2);
        file.read(reinterpret_cast<char*>(utf16Data.data()), utf16BytesToRead);

        size_t charsRead = static_cast<size_t>(file.gcount()) / 2;
        // Resize to actual read size
        utf16Data.resize(charsRead);

        // Convert byte order for UTF-16 BE
        if (isUtf16Be) {
            for (auto& ch : utf16Data) {
                ch = (ch << 8) | (ch >> 8); // swap bytes
            }
        }

        std::u16string utf16Str(utf16Data.begin(), utf16Data.end());

        try {
            data = Utf16ToUtf8(utf16Str);
            return true;
        } catch (const std::exception& ex) {
            LOG(ERROR) << "UTF-16 to UTF-8 conversion failed: " << ex.what();
            return false;
        }
    }

    // Handle UTF-8 (or file without BOM)
    file.seekg(contentOffset, std::ios::beg);
    if (!file) {
        return false;
    }

    size_t contentSize = fileSize - contentOffset;

    try {
        data.resize(contentSize);
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Failed to allocate memory: " << ex.what();
        return false;
    }

    if (contentSize > 0) {
        file.read(data.data(), contentSize);

        // Use actual bytes read, not expected size
        size_t bytesRead = static_cast<size_t>(file.gcount());
        data.resize(bytesRead);
    }

    return true;
}

void PutFileContents(const std::string& utf8Filename, const std::string& content) {
    try {
        std::filesystem::path filepath(std::filesystem::u8path(utf8Filename));

        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            throw IOException("Cannot open file for writing: " + utf8Filename);
        }

        file.write(content.data(), static_cast<std::streamsize>(content.size()));

        if (file.bad()) {
            throw IOException("Error writing to file: " + utf8Filename);
        }

        file.flush();
    } catch (const std::filesystem::filesystem_error& e) {
        throw IOException("Filesystem error for '" + utf8Filename + "': " + e.what());
    }
}

std::string GetFileContents(const std::string& filename) {
    std::ifstream file(std::filesystem::u8path(filename), std::ios::binary);
    if (!file) {
        throw IOException("Cannot open file: " + filename);
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size == -1) {
        throw IOException("Cannot determine file size for: " + filename);
    }

    std::string data;
    data.resize(static_cast<size_t>(size));

    file.read(data.data(), size);

    if (file.bad()) {
        throw IOException("Error reading file: " + filename);
    }

    auto bytes_read = file.gcount();
    if (bytes_read != size) {
        data.resize(static_cast<size_t>(bytes_read));
    }

    return data;
}

std::string GetFileContentsEx(const std::string& filename, int64_t offset, size_t size, bool allowPartialRead) {
    namespace fs = std::filesystem;

    fs::path filepath = fs::u8path(filename);

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory), "Failed to open file: " + filename);
    }

    file.seekg(0, std::ios::end);
    const int64_t fileSize = file.tellg();
    if (offset < 0 || offset > fileSize) {
        throw std::out_of_range("Invalid file offset");
    }

    if (size == 0) {
        return {};
    }

    const uint64_t bytesAvailable = static_cast<uint64_t>(fileSize - offset);
    if (!allowPartialRead && size > bytesAvailable) {
        throw std::out_of_range("Requested size exceeds available data");
    }
    const uint64_t readSize = allowPartialRead ? std::min(static_cast<uint64_t>(size), bytesAvailable) : static_cast<uint64_t>(size);

    file.seekg(offset, std::ios::beg);
    std::string buffer(readSize, '\0');
    file.read(buffer.data(), readSize);

    if (!file) {
        throw std::runtime_error("Failed to read the requested amount of data");
    }

    return buffer;
}

std::string TimeStampToString(time_t t)
{
    char buf[256]{0};
    // TODO: add system locale support
    tm * timeinfo = localtime ( &t );
    if (timeinfo) {
        sprintf(buf, "%02d.%02d.%04d %02d:%02d:%02d", (int)timeinfo->tm_mday, (int)timeinfo->tm_mon + 1, (int)1900 + timeinfo->tm_year, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    }
    return buf;
}

int64_t StringToInt64(const std::string& str)
{
    return strtoll(str.c_str(), nullptr, 10);
}

int64_t GetFileSize(const std::string& utf8Filename) {
    if (utf8Filename.empty()) {
        return -1;
    }

    std::error_code ec;
    auto path = std::filesystem::u8path(utf8Filename);

    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        return -1;
    }

    auto size = std::filesystem::file_size(path, ec);
    return ec ? -1 : static_cast<int64_t>(size);
}

std::string FileSizeToString(int64_t sizeBytes) {
    if (sizeBytes < 0) {
        return {};
    }
    const char* units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
    const int numUnits = sizeof(units) / sizeof(units[0]);
    double size = static_cast<double>(sizeBytes);
    int unitIndex = 0;

    while (size >= 1024 && unitIndex < numUnits - 1) {
        size /= 1024;
        ++unitIndex;
    }

    std::ostringstream out;
    if (std::floor(size) == size) {
        out << static_cast<int64_t>(size);
    } else {
        out << std::fixed << std::setprecision(1) << size;
    }

    out << ' ' << units[unitIndex];
    return out.str();
}

std::string ToString(double value, int precision)
{
    std::ostringstream stream;
    stream << std::fixed;
    stream << std::setprecision(precision);
    stream << value;
    return stream.str();
}

std::string GetDefaultExtensionForMimeType(const std::string& mimeType) {
    return MimeTypeHelper::getDefaultExtensionForMimeType(mimeType);
}

std::string ThreadIdToString(const std::thread::id& id)
{
    std::stringstream threadIdSS;
    threadIdSS << id;
    return threadIdSS.str();
}

void DatePlusDays(struct tm* date, int days){
    const time_t ONE_DAY = 24 * 60 * 60;

    // Seconds since start of epoch
    time_t date_seconds = mktime(date) + (days * ONE_DAY);

    // Update caller's date
    // Use localtime because mktime converts to UTC so may change date
    *date = *localtime(&date_seconds);
}

bool CopyFileToDest(const std::string& src, const std::string& dest, bool overwrite)
{
    std::error_code ec;
    std::filesystem::copy(std::filesystem::u8path(src), std::filesystem::u8path(dest),
            overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none,
            ec
    );
    return !ec;
}

bool CreateDir(const std::string& path, unsigned int mode) {
    std::error_code ec;

    std::filesystem::create_directories(std::filesystem::u8path(path), ec);
    if (ec) {
        return false;
    }
#ifndef _WIN32
    chmod(path.c_str(), mode);
#endif
    return true;
}

bool RemoveFile(const std::string& utf8Filename) {
    std::error_code ec;
    std::filesystem::remove(std::filesystem::u8path(utf8Filename), ec);
    return !ec;
}

bool DirectoryExists(const std::string& path) {
    std::error_code ec;
    bool isDir = std::filesystem::is_directory(path, ec);
    return !ec && isDir;
}

void OnThreadExit(void (*func)()) {
    class ThreadExiter
    {
        std::set<ThreadExitFunctionPointer> exitFuncs_;
    public:
        ThreadExiter() = default;
        ThreadExiter(ThreadExiter const&) = delete;
        void operator=(ThreadExiter const&) = delete;
        ~ThreadExiter()
        {
            for(auto func: exitFuncs_) {
                func();
            }
            exitFuncs_.clear();
        }
        void add(ThreadExitFunctionPointer func)
        {
            exitFuncs_.emplace(func);
        }
    };

    thread_local ThreadExiter exiter;
    exiter.add(func);
}

std::mutex xdgMimeMutex;
constexpr auto DefaultMimeType = "application/octet-stream";

std::string GetFileMimeType(const std::string& fileName) {
    std::lock_guard<std::mutex> lk(xdgMimeMutex);
    struct stat st;

    auto* mime = xdg_mime_get_mime_type_for_file(fileName.c_str(), &st);
    if (!mime) {
        return DefaultMimeType;
    }
    std::string result = mime;
    return result;
}

std::string GetFileMimeTypeByName(const std::string& fileName) {
    std::lock_guard<std::mutex> lk(xdgMimeMutex);
    struct stat st;
    auto* mime = xdg_mime_get_mime_type_for_file(fileName.c_str(), &st);
    if (!mime) {
        return DefaultMimeType;
    }
    return mime;
}

std::string GetFileMimeTypeByContents(const std::string& fileName) {
    FILE* f = FopenUtf8(fileName.c_str(), "rb");
    if (!f) {
        return DefaultMimeType;
    }
    char buffer[256] {};
    size_t readBytes = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);
    int resultPrio = 0;
    std::lock_guard<std::mutex> lk(xdgMimeMutex);
    auto* mime = xdg_mime_get_mime_type_for_data(buffer, readBytes, &resultPrio);

    if (!mime) {
        return DefaultMimeType;
    }
    return mime;
}

std::string GenerateRandomFilename(const std::string& path, int suffixLen)
{
    if (path.empty()) {
        return IuStringUtils::RandomString(suffixLen);
    }
    if (!suffixLen) {
        suffixLen = 8;
    }
    size_t i, len = path.length();

    for (i = len; i > 0; i--) {
        char c = path[i - 1];
        if (c == '\\' || c == '/') {
            break;
        }
    }

    std::string filename = path.substr(i);
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        return path.substr(0, i) + filename.substr(0, dotPos) + "_" + IuStringUtils::RandomString(suffixLen) + filename.substr(dotPos);
    }
    return path.substr(0, i) + filename + "_" + IuStringUtils::RandomString(suffixLen);
}

} // end of namespace IuCoreUtils
