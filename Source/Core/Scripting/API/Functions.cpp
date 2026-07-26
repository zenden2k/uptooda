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

#include "Functions.h"

#include <iomanip>
#include <optional>
#include <sstream>
#include <random>
#include <json/json.h>

#include "Core/BasicConstants.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Scripting/Squirrelnc.h"
#include "Core/Logging.h"

#include "Core/Network/NetworkClient.h"
#ifdef _WIN32
    #include <windows.h>
    #ifndef IU_CLI
        #include "Func/IuCommonFunctions.h"
        #include "Core/i18n/Translator.h"
        #include "Func/WinUtils.h"
    #endif
#include "Core/Images/Utils.h"
#endif

#include "Core/Scripting/DialogProvider.h"

#include "Core/Utils/StringUtils.h"
#include "Core/Utils/CryptoUtils.h"
#include "Core/3rdpart/codepages.h"
#include "Core/Utils/TextUtils.h"
#include "ScriptAPI.h"
#include "Core/ServiceLocator.h"
#include "Core/Upload/UploadErrorHandler.h"
#include "Core/Utils/DesktopUtils.h"
#include "Core/i18n/Translator.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Utils/SystemUtils.h"
#include "ScriptFunctionsImpl.h"
#include "Core/3rdpart/dotenv.h"

using namespace Sqrat;

namespace ScriptAPI {

std::string GetScriptsDirectory()
{
    return AppRuntimeInfo::instance()->dataDirectory() + "/Scripts/";
}

std::string GetAppLanguageFile()
{
    std::string languageFile = AppRuntimeInfo::instance()->languageFile();
    if ( languageFile.empty() ) {
        return "English";
    }
    return IuCoreUtils::ExtractFileNameNoExt(languageFile);
}

SQInteger GetAppVersion(HSQUIRRELVM vm) {
    Sqrat::Table res(vm);
    auto version = AppRuntimeInfo::instance()->GetAppVersion();
    if (version) {
        res.SetValue("Major", static_cast<SQInteger>(version->Major));
        res.SetValue("Minor", static_cast<SQInteger>(version->Minor));
        res.SetValue("Release", static_cast<SQInteger>(version->Release));
        res.SetValue("Build", static_cast<SQInteger>(version->Build));
    }
    res.SetValue("Gui", AppRuntimeInfo::instance()->isGui());
    Sqrat::PushVar(vm, res);
    return 1;
}

SQInteger GetCurrentScriptFileName(HSQUIRRELVM vm) {
    std::string res = GetScriptName(vm);
    Sqrat::PushVar(vm, res);
    return 1;
}

SQInteger IncludeScript(HSQUIRRELVM vm) {
    try {
        Sqrat::Var<std::string> filenameVar(vm, 2);
        if (filenameVar.value.empty()) {
            throw Sqrat::Exception("include() requires non-empty filename string");
        }

        std::string absolutePath;
        const auto& filename = filenameVar.value;

        if (filename[0] == '/' || (filename.length() > 1 && filename[1] == ':')) {
            absolutePath = filename;
        } else {
            std::string dir = IuCoreUtils::ExtractFilePath(GetScriptName(vm));
            if (dir.empty()) {
                dir = GetScriptsDirectory();
            }
            absolutePath = dir + filename;
        }

        if (!IuCoreUtils::FileExists(absolutePath)) {
            throw Sqrat::Exception("include() failed: file \"" + absolutePath + "\" not found");
        }

        std::string scriptText;
        if (!IuCoreUtils::ReadUtf8TextFile(absolutePath, scriptText)) {
            throw Sqrat::Exception("include() failed: could not read file \"" + absolutePath + "\"");
        }

        Sqrat::Script script(vm);
        script.CompileString(scriptText, IuCoreUtils::ExtractFileName(absolutePath));
        script.Run();

        Sqrat::PushVar(vm, script);
        return 1;
    } catch (const Sqrat::Exception& e) {
        return sq_throwerror(vm, e.what());
    } catch (const std::exception& e) {
        return sq_throwerror(vm, ("include() error: " + std::string(e.what())).c_str());
    }
}

void CleanUpFunctions() {
}

class ScriptTranslator {
public:
    explicit ScriptTranslator(const std::string& locale) {
        std::string absolutePath = GetScriptsDirectory() + "Lang/" + locale + ".json";
        std::string jsonText;
        if (!IuCoreUtils::ReadUtf8TextFile(absolutePath, jsonText)) {
            return;
        }

        Json::Reader reader;
        reader.parse(jsonText, translationRoot_, false);
    }

    bool loaded() const {
        return !translationRoot_.isNull();
    }

    std::optional<std::string> translate(const std::string& key) {
        if (!loaded()) {
            return std::nullopt;
        }
        std::vector<std::string> tokens;
        IuStringUtils::Split(key, ".", tokens, -1);
        const Json::Value* root = &translationRoot_;
        int count = tokens.size();
        for (int i = 0; i < count; i++) {
            std::string token = tokens[i];
            if (!root->isMember(token)) {
                break;
            }
            root = &(*root)[token];
            if (root->type() != Json::objectValue && i + 1 != count) {
                break;
            }
            if (i + 1 == count && root->type() == Json::stringValue) {
                return root->asString();
            }
        }
        return std::nullopt;
    }

private:
    Json::Value translationRoot_;
};

std::string Translate(const std::string& key, const std::string& originalText) {
    static ScriptTranslator scriptTranslator(GetAppLocale());
    static ScriptTranslator englishScriptTranslator("en");

    auto result = scriptTranslator.translate(key);
    if (result) {
        return *result;
    }

    result = englishScriptTranslator.translate(key);
    if (result) {
        return *result;
    }

    auto translator = ServiceLocator::instance()->translator();
    if (translator) {
        return translator->translate(originalText.c_str());
    }
    return originalText;
}

SQInteger ScriptTranslate(HSQUIRRELVM vm) {
    try {
        Sqrat::Var<std::string> key(vm, 2);
        Sqrat::Var<std::string> originalText(vm, 3);
        if (key.value.empty()) {
            throw Sqrat::Exception("ScriptTranslate() requires non-empty first argument");
        }

        std::string result = Translate(key.value, originalText.value);

        Sqrat::PushVar(vm, result);
        return 1;
    } catch (const Sqrat::Exception& e) {
        return sq_throwerror(vm, e.what());
    } catch (const std::exception& e) {
        return sq_throwerror(vm, ("ScriptTranslate() error: " + std::string(e.what())).c_str());
    }
}

std::string AskUserCaptcha(INetworkClient* nm, const std::string& url)
{
    return ServiceLocator::instance()->dialogProvider()->askUserCaptcha(nm, url);
}

std::string InputDialog(const std::string& text, const std::string& defaultValue) {
    return ServiceLocator::instance()->dialogProvider()->inputDialog(text, defaultValue);
}

std::string GetFileMimeType(const std::string& filename)
{
    return IuCoreUtils::GetFileMimeType(filename);
}

std::string AnsiToUtf8(const std::string& str, int codepage)
{
#ifdef _WIN32
    return IuCoreUtils::ConvertToUtf8(str, NameByCodepage(codepage));
#else
    LOG(WARNING) << "AnsiToUtf8 not implemented";
    return str; // FIXME
#endif
}

std::string scriptUtf8ToAnsi(const std::string& str, int codepage )
{
#ifdef _WIN32
    return IuCoreUtils::Utf8ToAnsi(str, codepage);
#else
    LOG(WARNING) << "Utf8ToAnsi not implemented";
    return str; // FIXME
#endif
}

SQInteger WriteLog(HSQUIRRELVM vm) {
    Sqrat::Var<std::string> typeStr(vm, 2);
    Sqrat::Var<std::string> message(vm, 3);

    ILogger::LogMsgType msgType = ILogger::logWarning;
    if ( typeStr.value == "error" ) {
        msgType = ILogger::logError;
    } else if (typeStr.value == "info") {
        msgType = ILogger::logInformation;
    }
    ServiceLocator::instance()->logger()->write(msgType, "Script Engine", message.value, "Script: " + IuCoreUtils::ExtractFileName(GetScriptName(vm)), GetCurrentTopLevelFileName());
    return 0;
}

std::string md5(const std::string& data)
{
    return IuCoreUtils::CryptoUtils::CalcMD5HashFromString(data);
}

void sleep(int msec) {
#ifdef _WIN32
    ::Sleep(msec);
#else
    ::sleep(ceil(msec/1000.0));
#endif
}

std::string JsonEscapeString( const std::string& src) {
    return Json::valueToQuotedString(src.data());
}

std::string GetTempDirectory() {
    return AppRuntimeInfo::instance()->tempDirectory();
}

std::string url_encode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << '%' << std::setw(2) << int((unsigned char) c);
    }

    return escaped.str();
}

void DebugMessage(const std::string& msg, bool isResponseBody) {
    ServiceLocator::instance()->uploadErrorHandler()->DebugMessage(msg,isResponseBody);
}

std::string MessageBox(const std::string& message, const std::string& title, const std::string& buttons, const std::string& type) {
    return ServiceLocator::instance()->dialogProvider()->messageBox(message, title, buttons, type);
}

void parseJSONObj(const Json::Value& root, Sqrat::Array& obj);
void parseJSONObj(const Json::Value& root, Sqrat::Table& obj);

template<class T,class V> void setObjValues(T key, Json::Value::const_iterator it, V &obj) {
    using namespace Json;

    try {
        switch (it->type()) {
        case nullValue:
            obj.SetValue(key, Sqrat::Object());
            break;
        case intValue:      // signed integer value
        case uintValue:     // unsigned integer value
            obj.SetValue(key, static_cast<SQInteger>(it->asInt64()));
            break;
        case realValue:  // double value
            obj.SetValue(key, it->asFloat());
            break;
        case stringValue:   // UTF-8 string value
            obj.SetValue(key, it->asString().data());
            break;
        case booleanValue:  // bool value
            obj.SetValue(key, it->asBool());
            break;
        case arrayValue: { // array value (ordered list)
            Sqrat::Array newArr(obj.GetVM());
            parseJSONObj(*it, newArr);
            obj.SetValue(key, newArr);
        }
            break;
        case objectValue: {
            Sqrat::Table newObj(obj.GetVM());
            parseJSONObj(*it, newObj);
            obj.SetValue(key, newObj);
            break;
        }

        }
    } catch (const std::logic_error & ex) {
        LOG(WARNING) << "setObjValue()" << std::endl << ex.what();
    }
}

void parseJSONObj(const Json::Value& root, Sqrat::Array& obj) {
    obj = Sqrat::Array(obj.GetVM(), root.size());
    for(auto it = root.begin(); it != root.end(); ++it) {
        int key = it.key().asInt();
        setObjValues(key, it, obj);
    }
}

void parseJSONObj(const Json::Value& root, Sqrat::Table& obj) {
    //obj = Sqrat::Table (o);
    for(auto it = root.begin(); it != root.end(); ++it) {
        std::string key = it.key().asString();
        setObjValues(key.c_str(), it, obj);
    }
}

void parseJSONObj(const Json::Value& root, HSQUIRRELVM vm) {
    using namespace Json;
    try {
        switch (root.type()) {
        case intValue: // signed integer value
        case uintValue: // unsigned integer value
            Sqrat::PushVar(vm, static_cast<SQInteger>(root.asInt64()));
            break;
        case realValue: // double value
            Sqrat::PushVar(vm, root.asFloat());
            break;
        case stringValue: // UTF-8 string value
            Sqrat::PushVar(vm, root.asString().data());
            break;
        case booleanValue: // bool value
            Sqrat::PushVar(vm, root.asBool());
            break;
        case arrayValue: {// array value (ordered list)
                Sqrat::Array obj(vm, root.size());
                for (auto it = root.begin(); it != root.end(); ++it) {
                    int key = it.key().asInt();
                    setObjValues(key, it, obj);
                }
                Sqrat::PushVar(vm, obj);
            }
        break;
        case objectValue: {
            Sqrat::Table obj(vm);
            for (auto it = root.begin(); it != root.end(); ++it) {
                std::string key = it.key().asString();
                setObjValues(key.data(), it, obj);
            }
            Sqrat::PushVar(vm, obj);
        }
        break;
        default:
            sq_pushnull(vm);
        }
    } catch (const std::logic_error& ex) {
        LOG(WARNING) << "parseJSONObj()" << std::endl << ex.what();
    }
}

SQInteger ParseJSON(HSQUIRRELVM vm) {
    Sqrat::Var<std::string> jsonStr(vm, 2); // 2 = first argument (JSON string)

    Json::Value root;
    Json::Reader reader;
    if (reader.parse(jsonStr.value, root, false)) {
        parseJSONObj(root, vm);
        return 1;
    }
    sq_pushnull(vm);
    return 1;
}

Json::Value sqValueToJson(const Sqrat::Object& obj ) {
    switch ( obj.GetType() ) {
        case OT_NULL:
            return Json::Value(Json::nullValue);

        case OT_INTEGER:
            return SQINT_TO_JSON_VALUE(obj.Cast<int>());

        case OT_FLOAT:
            return obj.Cast<float>();

        case OT_BOOL:
            return obj.Cast<bool>();

        case OT_STRING:
            return obj.Cast<std::string>();
    }
    return Json::Value(Json::nullValue);
}
Json::Value sqObjToJson(const Sqrat::Object& obj ) {
    HSQUIRRELVM vm = obj.GetVM();
    Json::Value res;
    Sqrat::Object::iterator it;

    switch ( obj.GetType() ) {
            case OT_NULL:
            case OT_INTEGER:
            case OT_FLOAT:
            case OT_BOOL:
            case OT_STRING:
                return sqValueToJson(obj);
            case OT_TABLE:
                res = Json::Value(Json::objectValue);
                while(obj.Next(it) ) {
                    res[it.getName()] = sqObjToJson(Sqrat::Object(it.getValue(), vm));
                }
                return res;
            case OT_ARRAY:
                res = Json::Value(Json::arrayValue);
                while(obj.Next(it) ) {
                    res[Sqrat::Object(it.getKey(), vm).Cast<int>()] = sqObjToJson(Sqrat::Object(it.getValue(), vm));
                }
                return res;
    }
    return Json::Value(Json::nullValue);
}

std::string ToJSON(const Sqrat::Object&  obj) {
    Json::Value root = sqObjToJson(obj);
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";
    return Json::writeString(builder, root);
}

int64_t ScriptGetFileSize(const std::string& filename) {
    return IuCoreUtils::GetFileSize(filename);
}

std::string GetFileContents(const std::string& filename) {
    try {
        return IuCoreUtils::GetFileContents(filename);
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception in GetFileContents:" << std::endl << e.what();
    }
    return {};
}

bool PutFileContents(const std::string& filename, const std::string& content) {
    try {
        IuCoreUtils::PutFileContents(filename, content);
        return true;
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception in PutFileContents:" << std::endl << e.what();
    }
    return false;
}

std::string GetFileContentsEx(const std::string& filename, SQInteger offset, SQInteger size, bool allowPartialRead) {
    try {
        return IuCoreUtils::GetFileContentsEx(filename, offset, static_cast<size_t>(size), allowPartialRead);
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception in GetFileContentsEx:" << std::endl << e.what();
    }
    return {};
}

std::string Md5Crypt(const std::string& password, const std::string& salt) {
    try {
        return IuCoreUtils::CryptoUtils::Md5Crypt(password.c_str(), salt.c_str());
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception in Md5Crypt:" << std::endl
                   << e.what();
    }
    return {};
}

std::string GetAppLanguage() {
    return Impl::GetAppLanguageImpl();
}

std::string GetAppLocale() {
    return Impl::GetAppLocaleImpl();
}

std::string GetCurrentThreadId()
{
    return IuCoreUtils::ThreadIdToString(std::this_thread::get_id());
}
#undef random
// older versions of Squirrel Standard Library have broken srand() function
int random()
{
    static thread_local std::mt19937 generator{ std::random_device{}() };
    std::uniform_int_distribution<int> dist;
    return dist(generator);
}

std::string ExtractFileName(const std::string& path) {
    return IuCoreUtils::ExtractFileName(path);
}

std::string GetFileExtension(const std::string& path) {
    return IuCoreUtils::ExtractFileExt(path);
}

SQInteger GetImageInfo(HSQUIRRELVM vm) {
    Sqrat::Table tbl(vm);
    Sqrat::Var<const SQChar*> strVar(vm, 2); // 2 = first argument (not 'this')
    if (!strVar.value) {
        return sq_throwerror(vm, "Expected a string argument!");
    }

    int width = 0, height = 0;
#ifdef _WIN32
    ImageUtils::ImageInfo ii = ImageUtils::GetImageInfo(U2W(strVar.value));
    width = ii.width;
    height = ii.height;
#endif
    tbl.SetValue("Width", width);
    tbl.SetValue("Height", height);
    Sqrat::PushVar(vm, tbl);
    return 1;
}

std::string GetDeviceId() {
    auto settings = ServiceLocator::instance()->basicSettings();
    if (!settings) {
        return {};
    }
    return settings->DeviceId;
}

std::string GetDeviceName() {
    std::string res = IuCoreUtils::GetOsName() + "_PC";
#ifdef _WIN32
    TCHAR  computerName[MAX_PATH];
    DWORD  bufCharCount = MAX_PATH;
    if (GetComputerName(computerName, &bufCharCount)) {
        res += "_" + IuCoreUtils::WstringToUtf8(computerName);
    }
#endif
    return res;
}

std::string GetEnvDecode(const std::string& name) {
    std::string res = dotenv::getenv(name.c_str());

    if (res.rfind("ENC:", 0) == 0) {
        static const std::string keyStr = IuCoreUtils::CryptoUtils::Base64Decode(ENV_A_E_K);
        try {
            res = IuCoreUtils::CryptoUtils::DecryptAES(res.substr(4), keyStr);
        } catch (const std::exception& e) {
            LOG(ERROR) << "Error while decoding variable " << name << ": " << e.what() << std::endl;
            return {};
        }
    }

    return res;
}
void RegisterFunctions(Sqrat::SqratVM& vm)
{
    Sqrat::RootTable& root = vm.GetRootTable();

    root
        .Func("GetScriptsDirectory", GetScriptsDirectory)
        .Func("GetAppLanguageFile", GetAppLanguageFile)
        .SquirrelFunc("include", IncludeScript)
        .SquirrelFunc("Translate", ScriptTranslate)
        .SquirrelFunc("GetAppVersion", GetAppVersion)
        .Func("random", random)
        .Func("Random", random)
        .Func("sleep", sleep)
        .Func("Sleep", sleep)
        .Func("AnsiToUtf8", AnsiToUtf8)
        .Func("Utf8ToAnsi", scriptUtf8ToAnsi)
        .Func("ExtractFileName", ExtractFileName)
        .Func("GetFileExtension", GetFileExtension)
        .Func("AskUserCaptcha", AskUserCaptcha)
        .Func("InputDialog", InputDialog)
        .Func("GetFileMimeType", GetFileMimeType)
        .Func("JsonEscapeString", JsonEscapeString)
        .Func("ShellOpenUrl", DesktopUtils::ShellOpenUrl)
        .SquirrelFunc("ParseJSON", ParseJSON)
        .Func("ToJSON", ToJSON)
        .Func("GetFileContents", GetFileContents)
        .Func("GetFileContentsEx", GetFileContentsEx)
        .Func("GetTempDirectory", GetTempDirectory)
        .Func("ExtractFileNameNoExt", IuCoreUtils::ExtractFileNameNoExt)
        .Func("ExtractFilePath", IuCoreUtils::ExtractFilePath)
        .Func("GenerateRandomFilename", IuCoreUtils::GenerateRandomFilename)
        .Func("RandomString", IuStringUtils::RandomString)
        .Func("StrReplace", IuStringUtils::Replace)
        .Func("CopyFile", IuCoreUtils::CopyFileToDest)
        .Func("CreateDirectory", IuCoreUtils::CreateDir)
        .Func("FileExists", IuCoreUtils::FileExists)
        .Func("MoveFileOrFolder", IuCoreUtils::MoveFileOrFolder)
        .Func("PutFileContents", PutFileContents)
        .Func("DeleteFile", IuCoreUtils::RemoveFile)
        .Func("GetAppLanguage", GetAppLanguage)
        .Func("GetAppLocale", GetAppLocale)
        .Func("HtmlEntitiesDecode", IuTextUtils::DecodeHtmlEntities)
        .Func("GetFileSize", ScriptGetFileSize)
        .Func("GetFileSizeDouble", ScriptGetFileSize)
        .SquirrelFunc("GetImageInfo", GetImageInfo)
        .SquirrelFunc("WriteLog", WriteLog)
        .SquirrelFunc("GetCurrentScriptFileName", GetCurrentScriptFileName)
        .Func("GetCurrentThreadId", GetCurrentThreadId)
        .Func("MessageBox", MessageBox)
        .Func("GetDeviceId", GetDeviceId)
        .Func("GetDeviceName", GetDeviceName)
        .Func("GetEnvDecode", GetEnvDecode);

    using namespace IuCoreUtils;
    root
        .Func("md5", md5)
        .Func("Md5", md5)
        .Func("Md5Crypt", Md5Crypt)
        .Func("md5_file", &CryptoUtils::CalcMD5HashFromFile)
        .Func("Md5FromFile", &CryptoUtils::CalcMD5HashFromFile)
        .Func("sha1", &CryptoUtils::CalcSHA1HashFromString)
        .Func("Sha1", &CryptoUtils::CalcSHA1HashFromString)
        .Func("sha1_file", &CryptoUtils::CalcSHA1HashFromFile)
        .Func("Sha1FromFile", &CryptoUtils::CalcSHA1HashFromFile)
        .Func("sha1_file_prefix", &CryptoUtils::CalcSHA1HashFromFileWithPrefix)
        .Func("Sha1FromFileWithPrefix", &CryptoUtils::CalcSHA1HashFromFileWithPrefix)
        .Func("hmac_sha1", &CryptoUtils::CalcHMACSHA1HashFromString)
        .Func("HmacSha1", &CryptoUtils::CalcHMACSHA1HashFromString)
        .Func("Sha256", &CryptoUtils::CalcSHA256HashFromString)
        .Func("Sha256FromFile", &CryptoUtils::CalcSHA256HashFromFile)
        .Func("Sha512", &CryptoUtils::CalcSHA512HashFromString)
        .Func("Sha512FromFile", &CryptoUtils::CalcSHA512HashFromFile)
        .Func("Base64Decode", &CryptoUtils::Base64Decode)
        .Func("Base64Encode", &CryptoUtils::Base64Encode)
        .Func("url_encode", url_encode)
        .Func("UrlEncode", url_encode)
        .Func("DebugMessage", DebugMessage);
}
//    atexit(&CleanUpFunctions);


void RegisterShortTranslateFunctions(Sqrat::SqratVM& vm) {
    Sqrat::RootTable& root = vm.GetRootTable();

    Function func(root, "tr");
    if (func.IsNull()) {
        root.SquirrelFunc("tr", ScriptTranslate);
    }
}

}
