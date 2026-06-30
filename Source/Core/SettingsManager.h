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

#ifndef IU_CORE_SETTINGSMANAGER_H
#define IU_CORE_SETTINGSMANAGER_H

#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <utility>
#include "Core/Utils/CoreTypes.h"
#include "Core/Utils/SimpleXml.h"
#include "Core/Utils/StringUtils.h"
#include "Core/Settings/StringConvert.h"

#define n_bind(a) operator[]( #a ).bind(a)
#define nm_bind(b,a) operator[]( #a ).bind(b.a)

class SettingsNodeBase
{
    public:
        virtual std::string getValue()=0;
        virtual void setValue(const std::string&)=0;
        virtual ~SettingsNodeBase() = default;
};

// Проверка: есть ли у типа begin()/end()
template <typename T, typename = void>
struct has_begin_end : std::false_type {};

template <typename T>
struct has_begin_end<T, std::void_t<decltype(std::begin(std::declval<T>()),
                                              std::end(std::declval<T>()))>>
    : std::true_type {};

// "Настоящий контейнер" = есть begin/end И это не std::string
template <typename T>
struct is_container
    : std::integral_constant<bool,
          has_begin_end<T>::value && !std::is_same<std::decay_t<T>, std::string>::value>
{};

// Перегрузка для контейнеров
template <typename Container,
          typename std::enable_if<is_container<Container>::value, int>::type = 0>
std::string myToString(const Container& value)
{
    return IuStringUtils::Join(value, ";");
}

// Перегрузка для всего остального (включая std::string, int, double и т.п.)
template <typename T,
          typename std::enable_if<!is_container<T>::value, int>::type = 0>
std::string myToString(const T& value)
{
    std::stringstream str;
    str << value;
    return str.str();
}

// Перегрузка для контейнеров
template <typename Container,
          typename std::enable_if<is_container<Container>::value, int>::type = 0>
void myFromString(const std::string& text, Container& value)
{
    value.clear();
    IuStringUtils::Split(text, ";", value);
}

// Перегрузка для всего остального (включая std::string, int, double и т.п.)
template <typename T,
          typename std::enable_if<!is_container<T>::value, int>::type = 0>
void myFromString(const std::string& text, T& value)
{
    std::stringstream str(text);
    str >> value;
}

inline void myFromString(const std::string& text, std::string & value)
{
    value = text;
}

template<class T> class SettingsNodeVariant: public SettingsNodeBase
{
    private:
        T* value_;
    public:
        explicit SettingsNodeVariant(T* value)
        {
            value_ = value;
        }

        std::string getValue() override {
            return myToString(*value_);
        }

        void setValue(const std::string& text) override {
            myFromString(text, *value_ );
        }

        virtual ~SettingsNodeVariant() = default;
};

class SettingsNode
{
    public:
        SettingsNode();
        virtual ~SettingsNode();
        template<class T> void bind(T& var)
        {
            delete binded_value_;
            binded_value_ = new SettingsNodeVariant<T>(&var);
        }
        SettingsNode& operator[](const std::string&);
        void saveToXmlNode(SimpleXmlNode parentNode, const std::string& name, bool isRoot = false) const;
        void loadFromXmlNode(SimpleXmlNode parentNode, const std::string& name, bool isRoot = false);
    protected:
        SettingsNodeBase * binded_value_;
        std::map<std::string, SettingsNode*> childs_;
        DISALLOW_COPY_AND_ASSIGN(SettingsNode);
};

class SettingsManager
{
    public:
        SettingsManager();
        SettingsNode& operator[](const std::string&);
        SettingsNode& root();
        void saveToXmlNode(SimpleXmlNode parentNode) const;
        void loadFromXmlNode(SimpleXmlNode parentNode);
    protected:
        SettingsNode root_;
        DISALLOW_COPY_AND_ASSIGN(SettingsManager);
};
#endif
