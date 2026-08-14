#include "ConsoleScriptDialogProvider.h"

#include <iostream>
#include <map>

#include "Core/Network/NetworkClient.h"
#include "Core/Utils/DesktopUtils.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/ConsoleUtils.h"
#include "Core/Utils/StringUtils.h"

std::string ConsoleScriptDialogProvider::askUserCaptcha(INetworkClient* nm, const std::string& url) {
    std::lock_guard<std::mutex> guard(ConsoleUtils::instance()->getOutputMutex());
    DesktopUtils::ShellOpenUrl(url);
    std::cerr << "Enter text from the image:" << std::endl;
//#ifdef _WIN32
    std::wstring result;
    std::wcin >> result;
    return IuCoreUtils::WstringToUtf8(result);
//#else
   /* std::string result;
    std::cin >> result;
    return result;*/
}

std::string ConsoleScriptDialogProvider::inputDialog(const std::string& text, const std::string& defaultValue) {
    std::lock_guard<std::mutex> guard(ConsoleUtils::instance()->getOutputMutex());
    std::string result;
    std::cerr << std::endl << text << std::endl;
    std::cin >> result;
    return result;
}

std::string ConsoleScriptDialogProvider::messageBox(const std::string& message, const std::string& title, const std::string& buttons, const std::string& type) {
    std::cerr << "----------";
#ifdef _WIN32
    std::wcerr << IuCoreUtils::Utf8ToWstring(title);
#else
    std::cerr << IuCoreUtils::Utf8ToSystemLocale(title);
#endif
    std::cerr << "----------" << std::endl;
#ifdef _WIN32
    std::wcerr << IuCoreUtils::Utf8ToWstring(message) << std::endl;;
#else
    std::cerr << IuCoreUtils::Utf8ToSystemLocale(message) << std::endl;;
#endif
    if (buttons.empty() || buttons == "OK") {
        getc(stdin);
        return "OK";
    }
    else {
        std::vector<std::string> tokens;
        std::map<char, std::string> buttonsMap;
        IuStringUtils::Split(buttons, "_", tokens, 10);
        for (int i = 0; i < tokens.size(); i++) {
            if (i != 0) {
                std::cerr << "/";
            }
            buttonsMap[tokens[i][0]] = tokens[i];
            std::cerr << "(" << tokens[i][0] << ")" << IuStringUtils::ToLower(tokens[i]).c_str() + 1;
        }
        std::cerr << ": ";
        char res;
        std::cin >> res;
        res = static_cast<char>(toupper(res));
        return buttonsMap[res];
    }
}
