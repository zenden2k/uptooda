#ifndef IU_CORE_SETTINGS_STRINGCONVERT_H
#define IU_CORE_SETTINGS_STRINGCONVERT_H

#include "EncodedPassword.h"

#ifdef _WIN32

#include "atlheaders.h"
inline std::string myToString(const CString& value)
{
    return IuCoreUtils::WstringToUtf8(static_cast<LPCTSTR>(value));
}

inline void myFromString(const std::string& text, CString& value)
{
    value = IuCoreUtils::Utf8ToWstring(text).c_str();
}
#endif

template<class T> std::string myToString(const EnumWrapper<T>& value)
{
    return std::to_string(value.value_);
}

template<class T> void myFromString(const std::string& text, EnumWrapper<T>& value)
{
    value = static_cast<T>(atoi(text.c_str()));
}

#if !defined(IU_SHELLEXT)


inline std::string myToString(const CEncodedPassword& value) {
    return (value.toEncodedData());
}

inline void myFromString(const std::string& text, CEncodedPassword& value) {
    value.fromEncodedData((text));
}

#endif

#endif
