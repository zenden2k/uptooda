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

#include "StringUtils.h"

#include <cctype>
#include <cstdio>
#include <algorithm>
#include <random>

#ifdef _WIN32
#include <windows.h>
#else
#include <strings.h>
#endif
#include <utf8/unchecked.h>

namespace IuStringUtils {

std::string Trim(const std::string& str)
{
    std::string res;
    // Trim Both leading and trailing spaces
    size_t startpos = str.find_first_not_of(" \t\r\n"); // Find the first character position after excluding leading blank spaces
    size_t endpos = str.find_last_not_of(" \t\r\n"); // Find the first character position from reverse af

    // if all spaces or empty return an empty string
    if ((std::string::npos == startpos) || (std::string::npos == endpos))
    {
        res.clear();
    }
    else
        res = str.substr( startpos, endpos - startpos + 1 );
    return res;
}

std::string_view TrimSV(std::string_view str)
{
    std::string_view res;
    // Trim Both leading and trailing spaces
    size_t startpos = str.find_first_not_of(" \t\r\n"); // Find the first character position after excluding leading blank spaces
    size_t endpos = str.find_last_not_of(" \t\r\n"); // Find the first character position from reverse af

    // if all spaces or empty return an empty string
    if ((std::string::npos == startpos) || (std::string::npos == endpos))
    {
        return res;
    }
    return str.substr( startpos, endpos - startpos + 1 );
}

void Split(const std::string& str, const std::string& delimiters, std::vector<std::string>& tokens, int maxCount)
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);
    int counter = 0;
    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        counter++;
        if (counter == maxCount)
        {
            tokens.emplace_back(str.substr(lastPos, str.length()));
            break;
        }
        else
            // Found a token, add it to the vector.
            tokens.emplace_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

std::vector<std::string_view> SplitSV(std::string_view strv, std::string_view delims, int maxCount) {
    std::vector<std::string_view> output;
    size_t first = 0;
    int counter = 0;
    while (first < strv.size())
    {
        counter++;
        if (counter == maxCount)
        {
            output.emplace_back(strv.substr(first));
            break;
        } else {
            const auto second = strv.find_first_of(delims, first);

            if (first != second)
                output.emplace_back(strv.substr(first, second - first));

            if (second == std::string_view::npos)
                break;

            first = second + 1;
        }
    }

    return output;
}

std::string Replace(const std::string& text, const std::string& s, const std::string& d)
{
    std::string result = text;
    for (size_t index = 0; index = result.find(s, index), index != std::string::npos; )
    {
        result.replace(index, s.length(), d);
        index += d.length();
    }
    return result;
}

// As far as C++ standart doesn't have a case insensitive string comparison function, I wrote my own
int stricmp(const char* s1, const char* s2)
{
#ifdef _WIN32
    return lstrcmpiA(s1, s2);
#else
    return strcasecmp(s1, s2);
    /*for (; *s1 && *s2 && (toupper((unsigned char)*s1) == toupper((unsigned char)*s2)); ++s1, ++s2)
        ;
    return *s1 - *s2;*/
#endif
}

std::string ToLower(const std::string& str) {
#ifdef _WIN32
    if (str.empty())
        return str;

    // Determine the required buffer size for wide characters
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (wideSize == 0)
        return str; // Conversion error

    // Convert UTF-8 to UTF-16
    std::wstring wideStr(wideSize - 1, 0); // -1 to remove null-terminator
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wideStr[0], wideSize);

    // Convert to lower case
    CharLowerBuffW(&wideStr[0], static_cast<DWORD>(wideStr.length()));

    // Determine the size for UTF-8
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size == 0)
        return str; // Conversion error

    // Convert back to UTF-8
    std::string result(utf8Size - 1, 0); // -1 to remove null-terminator
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &result[0], utf8Size, nullptr, nullptr);

    return result;
#else
    // Linux fallback - ASCII only
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
#endif
}

std::string ToUpper(const std::string& str) {
#ifdef _WIN32
    if (str.empty())
        return str;

    // Determine the required buffer size for wide characters
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (wideSize == 0)
        return str; // Conversion error

    // Convert UTF-8 to UTF-16
    std::wstring wideStr(wideSize - 1, 0); // -1 to remove null-terminator
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wideStr[0], wideSize);

    // Convert to upper case
    CharUpperBuffW(&wideStr[0], static_cast<DWORD>(wideStr.length()));

    // Determine the size for UTF-8
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size == 0)
        return str; // Conversion error

    // Convert back to UTF-8
    std::string result(utf8Size - 1, 0); // -1 to remove null-terminator
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &result[0], utf8Size, nullptr, nullptr);

    return result;
#else
    // Linux fallback - ASCII only
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return result;
#endif
}

std::string Tail(std::string const& source, size_t length) {
    if (length >= source.size()) {
        return source;
    }
    return source.substr(source.size() - length);
}

std::string ConvertUnixLineEndingsToWindows(const std::string& text) {
    if ( text.empty() ) {
        return text;
    }
    const char* srcString = text.c_str();
    size_t srcLen = text.length();
    char * res = new char[(srcLen*2)+1];
    char * cur = res;
    const char * srcEnd = srcString + srcLen;
    const char* src = srcString;
    while ( src < srcEnd ) {
        if ( *src == '\r' ) {
            *cur = *src;
            src++;
            cur++;
            *cur = *src;
        } else if ( *src == '\n' ) {
            *cur = '\r';
            cur++;
            *cur = '\n';

        } else {
            *cur = *src;
        }
        src++;
        cur++;
    }
    *cur = 0;
    std::string result(res, cur-res-1);
    delete[] res;
    return result;
}

size_t LengthOfUtf8String(const std::string &utf8_string) {
    return utf8::unchecked::distance(utf8_string.begin(), utf8_string.end());
}

boost::format FormatNoExcept(const char* str) {
    using namespace boost::io;
    try {
        boost::format fmter(str);
        fmter.exceptions(no_error_bits);
        //fmter.exceptions(all_error_bits ^ (too_many_args_bit | too_few_args_bit | bad_format_string_bit));
        return fmter;
    } catch (const std::exception& ex) {
        LOG(ERROR) << "boost::format exception" << std::endl << ex.what();
    }

    boost::format fmter("");
    fmter.exceptions(no_error_bits);
    //fmter.exceptions(all_error_bits ^ (too_many_args_bit | too_few_args_bit | bad_format_string_bit));
    return fmter;

}

boost::format FormatNoExcept(const std::string& str) {
    return FormatNoExcept(str.c_str());
}

boost::wformat FormatWideNoExcept(const wchar_t* str) {
    using namespace boost::io;
    try {
        boost::wformat fmter(str);
        fmter.exceptions(no_error_bits);
        return fmter;
    } catch (const std::exception& ex) {
        LOG(ERROR) << "boost::wformat exception" << std::endl << ex.what();
    }

    boost::wformat fmter(L"");
    fmter.exceptions(no_error_bits);
    return fmter;
}

boost::wformat FormatWideNoExcept(const std::wstring& str) {
    return FormatWideNoExcept(str.c_str());
}

inline auto ToFolded(char ch, int opts) {
    return (opts & FoldCase) ? std::tolower(static_cast<unsigned char>(ch)) : ch;
}

bool MatchBracket(std::string_view& pattern, char ch, int opts)
{
    bool inverted = false;
    if (pattern.empty())
        return false;

    if (pattern[0] == '!' || pattern[0] == '^') {
        inverted = true;
        pattern.remove_prefix(1);
    }

    bool matched = false;
    while (!pattern.empty() && pattern[0] != ']') {
        char start = pattern[0];
        pattern.remove_prefix(1);

        if (!pattern.empty() && start == '\\' && !(opts & NoEscape)) {
            start = pattern[0];
            pattern.remove_prefix(1);
        }

        char end = start;
        if (!pattern.empty() && pattern[0] == '-') {
            pattern.remove_prefix(1);
            if (pattern.empty())
                return false;

            end = pattern[0];
            pattern.remove_prefix(1);
            if (end == '\\' && !(opts & NoEscape)) {
                if (pattern.empty())
                    return false;
                end = pattern[0];
                pattern.remove_prefix(1);
            }
        }

        start = ToFolded(start, opts);
        end = ToFolded(end, opts);
        char folded = ToFolded(ch, opts);

        if (folded >= start && folded <= end) {
            matched = true;
        }
    }

    if (pattern.empty())
        return false;
    pattern.remove_prefix(1);
    return matched != inverted;
}

int PatternMatch(std::string_view pat, std::string_view str, int opts) {
    while (!pat.empty()) {
        char pc = pat[0];
        pat.remove_prefix(1);
        pc = ToFolded(pc, opts);

        switch (pc) {
        case '?':
            if (str.empty())
                return NoMatch;
            if ((opts & FileName) && str[0] == '/')
                return NoMatch;
            if ((opts & Period) && str[0] == '.' && (str.length() == 1 || ((opts & FileName) && str[1] == '/')))
                return NoMatch;
            str.remove_prefix(1);
            break;

        case '\\':
            if (!(opts & NoEscape) && !pat.empty()) {
                pc = pat[0];
                pat.remove_prefix(1);
                pc = ToFolded(pc, opts);
            }
            if (str.empty() || ToFolded(str[0], opts) != pc)
                return NoMatch;
            str.remove_prefix(1);
            break;

        case '*':
            if ((opts & Period) && !str.empty() && str[0] == '.' && (str.length() == 1 || ((opts & FileName) && str[1] == '/')))
                return NoMatch;

            while (!pat.empty() && (pat[0] == '?' || pat[0] == '*')) {
                if (((opts & FileName) && !str.empty() && str[0] == '/') || (pat[0] == '?' && str.empty()))
                    return NoMatch;
                if (pat[0] == '?')
                    str.remove_prefix(1);
                pat.remove_prefix(1);
            }

            if (pat.empty())
                return 0;

            if (!str.empty()) {
                auto next_pat = pat;
                char next_pc = next_pat[0];
                if (!(opts & NoEscape) && next_pc == '\\') {
                    next_pat.remove_prefix(1);
                    if (next_pat.empty())
                        return NoMatch;
                    next_pc = next_pat[0];
                }
                next_pc = ToFolded(next_pc, opts);

                for (size_t i = 0; i < str.length(); ++i) {
                    if ((pat[0] == '[' || ToFolded(str[i], opts) == next_pc) && PatternMatch(pat, str.substr(i), opts & ~Period) == 0)
                        return 0;
                    if ((opts & FileName) && str[i] == '/')
                        break;
                }
            }
            return NoMatch;

        case '[':
            if (str.empty())
                return NoMatch;
            if ((opts & Period) && str[0] == '.' && (str.length() == 1 || ((opts & FileName) && str[1] == '/')))
                return NoMatch;

            if (!MatchBracket(pat, str[0], opts))
                return NoMatch;
            str.remove_prefix(1);
            break;

        default:
            if (str.empty() || pc != ToFolded(str[0], opts))
                return NoMatch;
            str.remove_prefix(1);
        }
    }

    return str.empty() || ((opts & LeadingDir) && str == "/") ? 0 : NoMatch;
}

std::string RandomString(std::size_t length)
{
    const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

    std::string random_string;
    random_string.reserve(length);

    for (std::size_t i = 0; i < length; ++i) {
        random_string += CHARACTERS[distribution(generator)];
    }

    return random_string;
}

}
