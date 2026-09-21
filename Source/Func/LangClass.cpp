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

#include "LangClass.h"

#include <filesystem>
#include <boost/locale.hpp>

#include "Core/Utils/CoreUtils.h"
#include "Func/WinUtils.h"
#include "Gui/Helpers/LangHelper.h"

CLang::CLang()
{
    localeName_ = "en_US";
    language_ = "en";
    isRTL_ = false;
}
void CLang::SetDirectory(LPCTSTR Directory)
{
    m_Directory = Directory;
}

bool CLang::LoadLanguage(LPCTSTR Lang)
{
    boost::locale::generator gen;
    std::string path = W2U(WinUtils::GetAppFolder() + _T("Lang\\locale\\"));
    gen.add_messages_path(path);
    gen.add_messages_domain("imageuploader");

    std::string langA = W2U(Lang);
    std::locale locale;

    try {
        locale = gen(langA + ".UTF-8");
        std::locale::global(locale);
    } catch (const std::exception& ex) {
        LOG(ERROR) << ex.what();
    }

    localeName_ = Lang;
    if (std::has_facet<boost::locale::info>(locale)) {
        language_ = U2WC(std::use_facet<boost::locale::info>(locale).language());
    } else {
        language_ = Lang;
    }

    const auto& locales = LangHelper::instance()->getLocaleList();
    auto it = locales.find(langA);

    m_sLang = it == locales.end() ? localeName_ : U2W(it->second);

    return true;
}

CString CLang::GetLanguageName() const
{
    return m_sLang;
}

CString CLang::getLanguage() const
{
    return language_;
}

CString CLang::getLocale() const
{
    return localeName_;
}

#ifndef IU_SHELLEXT
std::string CLang::getCurrentLanguage() {
    return W2U(m_sLang);
}

std::string CLang::getCurrentLocale() {
    return W2U(localeName_);
}

std::string CLang::translate(const char* str) {
    return boost::locale::translate(str);
}

std::wstring CLang::translateW(const char* str)  {
    return IuCoreUtils::Utf8ToWstring(boost::locale::translate(str));
}
#endif

bool CLang::isRTL() const {
    return std::string(_("LAYOUT_DIRECTION")) == "RTL";
}

CString CLang::getCurrentLanguageFile() const {
    return currentLanguageFile_;
}

std::string CLang::getLanguageDisplayName() const {
    return W2U(m_sLang);
}
