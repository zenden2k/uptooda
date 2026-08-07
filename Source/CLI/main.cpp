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

#include <chrono>
#include <cmath>
#include <iostream>
#include <condition_variable>
#include <csignal>
#include <gflags/gflags.h>

#include <boost/format.hpp>
#include <curl/curl.h>
#include <argparse/argparse.hpp>

#include "Core/Upload/Uploader.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Network/NetworkClientFactory.h"
#include "Core/UploadEngineList.h"
#include "Core/Upload/UploadManager.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UploadSession.h"
#include "Core/Upload/ConsoleUploadErrorHandler.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Core/OutputGenerator/OutputGeneratorFactory.h"
#include "Core/Upload/ScriptUploadEngine.h"
#include "Core/ServiceLocator.h"
#include "Core/Utils/StringUtils.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/Settings/CliSettings.h"
#include "Core/Logging.h"
#include "Core/Logging/MyLogSink.h"
#include "Core/Logging/ConsoleLogger.h"
#include "Core/i18n/Translator.h"
#include "ConsoleScriptDialogProvider.h"
#include "Core/Utils/ConsoleUtils.h"
#include "Core/Scripting/ScriptsManager.h"
#include "Core/Images/AbstractImage.h"
#include "Core/3rdpart/xdgmime/xdgmime.h"
#include "Core/3rdpart/termcolor.hpp"
#include "Core/BasicConstants.h"
#include "Core/3rdpart/dotenv.h"

#ifdef _WIN32
    #include <cstdio>
    #include <windows.h>
    #include <fcntl.h>
    #include <io.h>
    #include "Func/IuCommonFunctions.h"
    #include "Func/GdiPlusInitializer.h"
    #include "Func/UpdatePackage.h"
    #include "Core/COMInitializer.h"
#else
    #include <sys/stat.h>
    #include <sys/time.h>
#endif

#include "versioninfo.h"

#ifdef _WIN32

CAppModule _Module;
std::string dataFolder = "Data/";
#else
std::string dataFolder = "/usr/share/uptooda-cli/";
#endif
CliSettings Settings;
std::vector<std::string> filesToUpload;
std::string serverName;
std::string login;
std::string password;
std::string folderId;
std::map<std::string,std::string> serverParameters;
bool printServerParameters;
std::string proxy;
int proxyPort;
int proxyType; /* CURLPROXY_HTTP, CURLPROXY_HTTPS, CURLPROXY_SOCKS4, CURLPROXY_SOCKS4A,
			   CURLPROXY_SOCKS5, CURLPROXY_SOCKS5_HOSTNAME */
std::string proxyUser;
std::string proxyPassword;
int maxRetries = MAX_RETRIES_PER_FILE;
int maxRetriesPerAction = MAX_RETRIES_PER_ACTION;
int thumbWidth = 0;
int thumbHeight = 0;
bool useSystemProxy = false;

std::unique_ptr<CUploadEngineList> list;

namespace OutputGenerator = Uptooda::Core::OutputGenerator;
OutputGenerator::CodeType codeType = OutputGenerator::ctClickableThumbnails;
OutputGenerator::CodeLang codeLang = OutputGenerator::clPlain;
bool autoUpdate = false;
std::shared_ptr<UploadSession> session;

std::mutex finishSignalMutex;
std::condition_variable finishSignal;
volatile bool finished = false;
int funcResult = 0;
struct TaskUserData {
    int index;
};

std::vector<std::unique_ptr<TaskUserData>> userDataArray;

void SignalHandler (int param) {
    if ( session ) {
        session->stop();
    }
}

class Translator : public ITranslator {
public:
    std::string getCurrentLanguage() override {
        return "English";
    }
    std::string getCurrentLocale() override {
        return "en_US";
    }
    std::string translate(const char* str) override{
        return str;
    }
#ifdef _WIN32
    std::wstring translateW(const char* str) override {
        return L"NOT_IMPLEMENTED";
    }
#endif
};
Translator translator; // dummy translator

#ifdef _WIN32
void DoUpdates(bool force = false);
#endif

std::chrono::steady_clock::time_point lastProgressTime;

void PrintServerList() {
    const std::pair<CUploadEngineData::ServerType, std::string> serverTypes[] = {
        {CUploadEngineData::TypeImageServer, "Image"},
        {CUploadEngineData::TypeFileServer, "File"},
        {CUploadEngineData::TypeVideoServer, "Video"},
    };

    for (const auto& [serverType, typeString] : serverTypes) {
        std::cout << termcolor::green << typeString << " servers:" << termcolor::reset << std::endl;
        for (const auto& ued : *list) {
            if (!ued->hasType(serverType)) {
                continue;
            }
            std::cout << ued->Name;
            if (!ued->DisplayName.empty() && ued->DisplayName != ued->Name) {
                std::cout << " [" << ued->DisplayName << "]";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

const CUploadEngineData* getServerByName(const std::string& name)
{
    const CUploadEngineData* uploadEngineData = list->byName(serverName);
    if (!uploadEngineData) {
        for (int i = 0; i < list->count(); i++) {
            if ((IuStringUtils::ToLower(list->byIndex(i)->Name).find(IuStringUtils::ToLower((name)))) != std::string::npos)
                return list->byIndex(i);
        }
    }
    return uploadEngineData;
}

void OnUploadSessionFinished(UploadSession* session) {
    using namespace OutputGenerator;
    int taskCount = session->taskCount();
    std::vector<OutputGenerator::UploadObject> uploadedList;
    for (int i = 0; i < taskCount; i++) {
        auto task = session->getTask(i);
        UploadResult* res = task->uploadResult();
        //auto* fileTask = dynamic_cast<FileUploadTask*>(task.get());

        OutputGenerator::UploadObject uo;
        uo.fillFromUploadResult(res, task.get());
        if (!task->uploadSuccess()) {
            funcResult++;
        }
        uploadedList.push_back(uo);
    }
    OutputGenerator::OutputGeneratorFactory factory;
    OutputGenerator::GeneratorID gid = static_cast<OutputGenerator::GeneratorID>(codeLang);
    auto generator = factory.createOutputGenerator(gid, codeType);
    generator->setPreferDirectLinks(true);
    //ConsoleUtils::instance()->SetCursorPos(0, taskCount + 2);

    std::cerr<<std::endl<<"Result:"<<std::endl;

    ConsoleUtils::instance()->printUnicode(stdout, generator->generate(uploadedList));

    std::cerr<<std::endl;

    {
        // LOG(ERROR) << "Sending finish signal" << std::endl;
        std::lock_guard<std::mutex> lk(finishSignalMutex);
        finished = true;
    }

    finishSignal.notify_one();
}

constexpr auto TOTAL_DOTS = 30;

void PrintProgress(UploadTask* task) {
    UploadProgress* progress = task->progress();
    double fractiondownloaded = static_cast<double>(progress->uploaded) / progress->totalUpload;

    if (fractiondownloaded > 100)
        fractiondownloaded = 0;
    // part of the progressmeter that's already "full"
    int dotz = static_cast<int>(floor(fractiondownloaded * TOTAL_DOTS));

    // create the "meter"
    int ii = 0;
    fprintf(stderr, "%3.0f%% [", fractiondownloaded * 100);
    // part  that's full already
    for (; ii < dotz; ii++) {
        fprintf(stderr, "#");
    }
    // remaining part (spaces)
    for (; ii < TOTAL_DOTS; ii++) {
        fprintf(stderr, " ");
    }
    // and back to line begin - do not forget the fflush to avoid output buffering problems!
    fprintf(stderr, "]");
    fprintf(stderr, " %s/%s", IuCoreUtils::FileSizeToString(progress->uploaded).c_str(),
        IuCoreUtils::FileSizeToString(progress->totalUpload).c_str());
    // remaining part (spaces)
    /*for (int i = 0; i < 30; i++) {
            fprintf(stderr, " ");
        }*/

    fflush(stderr);
}

void UploadTaskProgress(UploadTask* task) {

    UploadProgress* progress = task->progress();

    using namespace std::chrono_literals;
    std::chrono::steady_clock::time_point cur = std::chrono::steady_clock::now();

    if (progress->totalUpload != progress->uploaded && cur - lastProgressTime < 200ms) { // Windows console output is too slow
        return;
    }
    std::lock_guard<std::mutex> guard(ConsoleUtils::instance()->getOutputMutex());
    auto* userData = static_cast<TaskUserData*>(task->userData());
    lastProgressTime = cur;

    if (progress->totalUpload == 0) {
        return;
    }

    if (termcolor::_internal::is_atty(std::cerr) /* || (std::abs(fractiondownloaded - 1) <= 0.001) */ || task->isFinished()) {
        fprintf(stderr, "\r#%d ", userData->index);
        PrintProgress(task);
    }
}

void OnUploadTaskStatusChanged(UploadTask* task) {
    std::lock_guard<std::mutex> guard(ConsoleUtils::instance()->getOutputMutex());
    UploadProgress* progress = task->progress();
    auto* userData = static_cast<TaskUserData*>(task->userData());
    bool taskFinished = task->status() == UploadTask::StatusFinished || task->status() == UploadTask::StatusFailure;

    if (termcolor::_internal::is_atty(std::cerr)) {
        ConsoleUtils::instance()->clearLine(stderr);
        fprintf(stderr, "\r#%d ", userData->index);
    } else {
        fprintf(stderr, "\n#%d ", userData->index);
        if (taskFinished) {
            PrintProgress(task);
            fprintf(stderr, "\n#%d ", userData->index);
        }
    }

    std::string statusText = progress->statusText;
    if (taskFinished) {
        if (task->status() == UploadTask::StatusFinished) {
            std::cerr << termcolor::green;
        } else {
            std::cerr << termcolor::red;
        }
        std::cerr << statusText << termcolor::reset << " ";
    } else {
        ConsoleUtils::instance()->printUnicode(stderr, statusText);
    }
}

void OnTaskFinished(UploadTask*, bool) {
    std::lock_guard<std::mutex> guard(ConsoleUtils::instance()->getOutputMutex());
    fprintf(stderr, "\r\n");
}

void OnQueueFinished(CFileQueueUploader*) {
    {
        // LOG(ERROR) << "Sending finish signal" << std::endl;
        std::lock_guard<std::mutex> lk(finishSignalMutex);
        finished = true;
    }

    finishSignal.notify_one();
}
int func() {
#ifdef _WIN32
    GdiPlusInitializer gdiPlusInitializer;
    std::string dFolder = dataFolder;
    if (!dFolder.empty() && dFolder.back() == '\\') {
        dFolder.pop_back();
    }
    char* cacheDir = _strdup(dFolder.c_str());
    if (cacheDir) {
        const char* dirs[2]
            = { cacheDir, nullptr };
        xdg_mime_set_dirs(dirs);
        free(cacheDir);
    }
#endif
    defer d([] {
        xdg_mime_set_dirs(nullptr);
        xdg_mime_shutdown();
    });
    auto uploadErrorHandler = std::make_shared<ConsoleUploadErrorHandler>(list.get());
    ServiceLocator* serviceLocator = ServiceLocator::instance();
    serviceLocator->setUploadErrorHandler(uploadErrorHandler);
    serviceLocator->setNetworkClientFactory(std::make_shared<NetworkClientFactory>());
    ConsoleScriptDialogProvider dialogProvider;

    serviceLocator->setDialogProvider(&dialogProvider);
    serviceLocator->setTranslator(&translator);

    Settings.setEngineList(list.get());
    ServiceLocator::instance()->setEngineList(list.get());
    auto networkClientFactory = std::make_shared<NetworkClientFactory>();
    auto scriptsManager = std::make_unique<ScriptsManager>(networkClientFactory);
    std::unique_ptr<UploadEngineManager> uploadEngineManager;
    uploadEngineManager = std::make_unique<UploadEngineManager>(list.get(), uploadErrorHandler, networkClientFactory);
    std::string scriptsDirectory = AppRuntimeInfo::instance()->dataDirectory() + "/Scripts/";
    uploadEngineManager->setScriptsDirectory(scriptsDirectory);
    auto uploadManager = std::make_shared<UploadManager>(uploadEngineManager.get(), list.get(), scriptsManager.get(), uploadErrorHandler, networkClientFactory, &Settings, 1);

    if (useSystemProxy) {
        Settings.ConnectionSettings.UseProxy = ConnectionSettingsStruct::kSystemProxy;
    } else if (!proxy.empty()) {
        Settings.ConnectionSettings.UseProxy = ConnectionSettingsStruct::kUserProxy;
        Settings.ConnectionSettings.ServerAddress= proxy;
        Settings.ConnectionSettings.ProxyPort = proxyPort;

        if(!proxyUser.empty()) {
            Settings.ConnectionSettings.NeedsAuth = true;
            Settings.ConnectionSettings.ProxyUser = proxyUser;
            Settings.ConnectionSettings.ProxyPassword.fromPlainText(proxyPassword);
        }
    }

    const CUploadEngineData* uploadEngineData = nullptr;
    if (!serverName.empty()) {
        uploadEngineData = getServerByName(serverName);
        if (!uploadEngineData) {
            std::cerr << "No such server '" << serverName << "'!" << std::endl;
            return 0;
        }
    } else {
        std::cerr << "Server not set " << std::endl;
        return -1;
    }

    if (uploadEngineData->NeedAuthorization == CUploadEngineData::naObligatory && login.empty())
	{
		std::cerr<<"Server '"<<uploadEngineData->Name<<"' requires authentication! Use -u and -p options."<<std::endl;
		return -1;
	}

    ServerProfile serverProfile(uploadEngineData->Name);
    serverProfile.setProfileName(login);
    serverProfile.setShortenLinks(false);

    ImageUploadParams iup;
    auto& thumb = iup.getThumbRef();
    if (thumbWidth) {
        thumb.Width = thumbWidth;
        thumb.ResizeMode = ThumbCreatingParams::trByWidth;
    }
    if (thumbHeight) {
        thumb.Height = thumbHeight;
        thumb.ResizeMode = ThumbCreatingParams::trByHeight;
    }
    if (thumbWidth && thumbHeight) {
        thumb.ResizeMode = ThumbCreatingParams::trByBoth;
    }
    serverProfile.setImageUploadParams(iup);

    ServerSettingsStruct& s = Settings.ServersSettings[uploadEngineData->Name][login];

    s.authData.Password = password;
    s.authData.Login = login;
    if(!login.empty()) {
        s.authData.DoAuth = true;
    }

    s.setParam("FolderID", folderId);
    serverProfile.setFolderId(folderId);

    for (const auto& [k,v]: serverParameters) {
        s.setParam(k, v);
    }
    session = std::make_shared<UploadSession>(false);
    for(size_t i=0; i<filesToUpload.size(); i++) {
        if(!IuCoreUtils::FileExists(filesToUpload[i]))
        {
            std::string errorMessage = str(boost::format("File '%s' doesn't exist!\n")%filesToUpload[i]);
            ConsoleUtils::instance()->printUnicode(stderr, errorMessage);
            funcResult++;
            continue;
        }

        std::shared_ptr<FileUploadTask> task = std::make_shared<FileUploadTask>(filesToUpload[i], IuCoreUtils::ExtractFileName(filesToUpload[i]));
        task->setServerProfile(serverProfile);
        task->addTaskFinishedCallback(OnTaskFinished);
        task->setOnUploadProgressCallback(UploadTaskProgress);
        task->setOnStatusChangedCallback(OnUploadTaskStatusChanged);
        auto userData = std::make_unique<TaskUserData>();
        userData->index = i;
        task->setUserData(userData.get());
        userDataArray.push_back(std::move(userData));
        session->addTask(task);
    }
    if (session->taskCount() == 0) {
        return funcResult;
    }
    session->addSessionFinishedCallback(UploadSession::SessionFinishedCallback(OnUploadSessionFinished));

    uploadManager->setOnQueueFinishedCallback(OnQueueFinished);
    uploadManager->addSession(session);

    // Wait until upload session is finished
    std::unique_lock<std::mutex> lk(finishSignalMutex);
    while (!finished) {
        finishSignal.wait(lk/*, [] {return finished;}*/);
    }
	return funcResult;
}

void PrintServerParamList()
{
    if (serverName.empty()) {
        throw std::invalid_argument("Server name is empty");
    }
    const CUploadEngineData* ued = getServerByName(serverName);
    if (!ued) {
        throw std::invalid_argument("No such server");
    }

    ServerProfile profile(ued->Name);
    auto uploadErrorHandler = std::make_shared<ConsoleUploadErrorHandler>(list.get());
    ServiceLocator* serviceLocator = ServiceLocator::instance();
    serviceLocator->setUploadErrorHandler(uploadErrorHandler);
    auto networkClientFactory = std::make_shared<NetworkClientFactory>();
    auto scriptsManager = std::make_unique<ScriptsManager>(networkClientFactory);
    auto uploadEngineManager = std::make_unique<UploadEngineManager>(
        list.get(), uploadErrorHandler, networkClientFactory);
    std::string scriptsDirectory = AppRuntimeInfo::instance()->dataDirectory() + "/Scripts/";
    uploadEngineManager->setScriptsDirectory(scriptsDirectory);
    ParameterList parameterList;
    auto pluginLoader = std::dynamic_pointer_cast<CAdvancedUploadEngine>(uploadEngineManager->getUploadEngine(profile));
    if (pluginLoader) {
        std::cout << "Parameters of server '" << ued->Name << "':" << std::endl;
        pluginLoader->getServerParamList(parameterList);
        int i = 0;
        for (auto& parameter : parameterList) {
            std::cout << ++i << ") " << parameter->getTitle() << std::endl
                      << "Name: " << parameter->getName() << " Type: " << parameter->getType() << std::endl;

            std::string description = parameter->getDescription();

            if (!description.empty()) {
                std::cout << description << std::endl;
            }
        }
    } else {
        throw std::invalid_argument("This server cannot have parameters");
    }
}

#ifdef _WIN32
class Updater: public CUpdateStatusCallback {
public:
    explicit Updater(const CString& tempDirectory): m_UpdateManager(std::make_shared<NetworkClientFactory>(), tempDirectory){
        m_UpdateManager.setUpdateStatusCallback(this);
    }

    void updateServers() {
        std::cout<<"Checking for updates..."<<std::endl;
        if (!m_UpdateManager.CheckUpdates()) {
            std::cout<<"Error while updating"<<std::endl;
            return;
        }

        Settings.LastUpdateTime = static_cast<int>(time(0));
        if (m_UpdateManager.AreUpdatesAvailable())
        {
            for (const auto & i : m_UpdateManager.m_updateList)
            {
                std::wcerr << "Beginning to update: "<< i.displayName() << std::endl;
            }

            m_UpdateManager.DoUpdates();
            if (m_UpdateManager.successPackageUpdatesCount())
            {
                std::cerr << "Successfully updated!";
            }
        }
        else
        {
            std::cerr << "All is up-to-date" << std::endl;
        }
    }

    void updateStatus(int packageIndex, const CString& status) override {
        if (m_UpdateManager.m_updateList.size() > packageIndex+1) {
            fprintf(stderr, "%s : %s", IuCoreUtils::WstringToUtf8(m_UpdateManager.m_updateList[packageIndex].displayName().GetString()).c_str(), IuCoreUtils::WstringToUtf8(status.GetString()).c_str());
            fprintf(stderr, "\r");
            fflush(stderr);
        }
    }
protected:
    CUpdateManager m_UpdateManager;
};

void DoUpdates(bool force) {
	if(force || time(0) - Settings.LastUpdateTime > 3600*24*7 /* 7 days */) {
        CString tempFolder, commonTempFolder;
        IuCommonFunctions::CreateTempFolder(tempFolder, commonTempFolder);
        Updater upd(tempFolder);
		upd.updateServers();
        IuCommonFunctions::ClearTempFolder(tempFolder);
		Settings.LastUpdateTime = time(0);
	}
}

// Convert UNICODE (UCS-2) command line arguments to utf-8
char ** convertArgv(int argc, _TCHAR* argvW[]) {
	char ** result = new char *[argc];
	for ( int i = 0; i < argc; i++) {
		std::string unicodeString = IuCoreUtils::WstringToUtf8(argvW[i]);
		char *buffer = new char[unicodeString.length()+1];
		strcpy(buffer, unicodeString.c_str());
		result[i] = buffer;
	}
	return result;
}

int _tmain(int argc, _TCHAR* argvW[]) {
	char **argv = convertArgv(argc, argvW);
	FLAGS_logtostderr = true;
#else
int main(int argc, char *argv[]){
#endif
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    ConsoleUtils::instance();
    AppRuntimeInfo::AppVersionInfo appVersion;
    appVersion.FullVersion = IU_APP_VER;
    appVersion.FullVersionClean = IU_APP_VER_CLEAN;
    appVersion.Build = std::stoi(IU_BUILD_NUMBER);
    appVersion.BuildDate = IU_BUILD_DATE;
    appVersion.CommitHash = IU_COMMIT_HASH;
    appVersion.CommitHashShort = IU_COMMIT_HASH_SHORT;
    appVersion.BranchName = IU_BRANCH_NAME;
    AppRuntimeInfo::instance()->setVersionInfo(appVersion);

    argparse::ArgumentParser program("uptooda-cli", appVersion.FullVersion);

    program.add_argument("-l", "--list")
        .help("print server list (hosting services) and exit")
        .action([=](const auto& s) {
            PrintServerList();
            std::exit(0);
        })
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    program.add_argument("-s", "--server")
        .help("choose server by name")
        .required()
        .metavar("NAME")
        .store_into(serverName);

    program.add_argument("-cl", "--code_lang")
        .help("code language (bbcode|html|json|plain)")
        .action([&](const std::string& cl) {
            const std::unordered_map<std::string, OutputGenerator::CodeLang> types = {
                { "plain", OutputGenerator::clPlain },
                { "html", OutputGenerator::clHTML },
                { "bbcode", OutputGenerator::clBBCode },
                { "json", OutputGenerator::clJSON }
            };
            auto it = types.find(cl);
            if (it != types.end()) {
                codeLang = it->second;
            } else {
                throw std::invalid_argument("Invalid code language");
            }
        })
        .default_value("plain")
        .nargs(1);

    program.add_argument("-ct", "--code_type")
        .help("code type (TableOfThumbnails|ClickableThumbnails|Images|Links)")
        .action([&](const std::string& ct) {
            const std::unordered_map<std::string, OutputGenerator::CodeType> types = {
                { "TableOfThumbnails", OutputGenerator::ctTableOfThumbnails },
                { "ClickableThumbnails", OutputGenerator::ctClickableThumbnails },
                { "Images", OutputGenerator::ctImages },
                { "Links", OutputGenerator::ctLinks }
            };
            auto it = types.find(ct);
            if (it != types.end()) {
                codeType = it->second;
            } else {
                throw std::invalid_argument("Invalid code type");
            }
     })
     .default_value("Links")
     .nargs(1);

    program.add_argument("-u", "--user")
        .help("user name (login)")
        .metavar("USERNAME")
        .store_into(login);

    program.add_argument("-p", "--password")
        .help("password")
        .metavar("PASSWORD")
        .store_into(password);

    program.add_argument("-fl", "--folder_id")
        .help("the ID of remote folder/album (supported by some servers)")
        .metavar("ID")
        .store_into(folderId);

    program.add_argument("-r", "--retries")
        .help("maximum number of attempts (per file)")
        .store_into(maxRetries);

    program.add_argument("-a", "--retries_per_action")
        .help("maximum number of attempts (per action)")
        .store_into(maxRetriesPerAction);

    program.add_argument("-tw", "--thumb_width")
        .help("thumbnail width (supported by some servers)")
        .store_into(thumbWidth);

    program.add_argument("-th", "--thumb_height")
        .help("thumbnail height (supported by some servers)")
        .store_into(thumbHeight);

    program.add_argument("-sp", "--server_param")
        .help("set parameter of remote server (NAME:VALUE)")
        .metavar("NAME:VALUE")
        .append();
       // .nargs(argparse::nargs_pattern::at_least_one);

    program.add_argument("-pl", "--param_list")
        .help("print server parameter list and exit")
        .action([=](const auto& s) {
            PrintServerParamList();
            std::exit(0);
        })
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    program.add_argument("-pr", "--proxy")
        .help("proxy address:port")
        .action([&](const std::string& pr) {
            const std::unordered_map<std::string, OutputGenerator::CodeType> types = {
                { "TableOfThumbnails", OutputGenerator::ctTableOfThumbnails },
                { "ClickableThumbnails", OutputGenerator::ctClickableThumbnails },
                { "Images", OutputGenerator::ctImages },
                { "Links", OutputGenerator::ctLinks }
            };
            std::vector<std::string> tokens;
            IuStringUtils::Split(pr, ":", tokens, 2);
            if (tokens.size() > 1) {
                proxy = tokens[0];
                proxyPort = atoi(tokens[1].c_str());
            } else {
                throw std::invalid_argument("Invalid proxy");
            }
        });

    program.add_argument("-pu", "--proxy_user")
         .help("proxy username (login)")
         .metavar("USERNAME")
         .store_into(login);

    program.add_argument("-pp", "--proxy_password")
         .help("proxy password")
         .metavar("PASSWORD")
         .store_into(password);


    program.add_argument("-pt", "--proxy_type")
         .help("proxy type (http|https|socks4|socks4a|socks5|socks5dns)")
         .choices("http", "socks4", "socks4a", "socks5", "socks5dns", "https")
         .action([&](const std::string& type) {
            std::map<std::string, int> types;
            types["http"] = CURLPROXY_HTTP;
            types["socks4"] = CURLPROXY_SOCKS4;
            types["socks4a"] = CURLPROXY_SOCKS4A;
            types["socks5"] = CURLPROXY_SOCKS5;
            types["socks5dns"] = CURLPROXY_SOCKS5_HOSTNAME;
            types["https"] = CURLPROXY_HTTPS;

            auto it = types.find(type);
            if (it != types.end()) {
                proxyType = it->second;
            } else {
                 throw std::invalid_argument("Invalid proxy type");
            }
         })
         .default_value("http")
         .nargs(1);

#ifdef _WIN32
    program.add_argument("-ps", "--proxy_system")
         .help("use system proxy settings (this option is supported only on Windows)")
         .flag()
         .store_into(useSystemProxy);

    program.add_argument("-up", "--update")
        .help("update servers.xml (the 'Data' directory must be writable, otherwise update will fail)")
        .action([=](const auto& s) {
            DoUpdates(true);
            std::exit(0);
        })
        .default_value(false)
        .implicit_value(true)
        .nargs(0);
#endif
    /*program.add_argument("-d", "--disable_update")
       .help(Disable auto-updating servers.xml")
       .action([=](const auto& s) {
           autoUpdate = false;
       })
       .default_value(false)
       .implicit_value(true)
       .nargs(0);*/

    program.add_argument("files")
         .help("files to upload on remote server")
         .remaining();

    list = std::make_unique<CUploadEngineList>();
    auto defaultLogger = std::make_shared<ConsoleLogger>();

    ServiceLocator* serviceLocator = ServiceLocator::instance();
    serviceLocator->setSettings(&Settings);
    serviceLocator->setLogger(defaultLogger);
    MyLogSink logSink(defaultLogger.get());
    google::AddLogSink(&logSink);

    int res  = 0;
    std::string appDirectory = IuCoreUtils::ExtractFilePath(argv[0]);
    std::string settingsFolder;
    setlocale(LC_ALL, "");
    //signal(SIGINT, SignalHandler);
#ifdef _WIN32
    //SetConsoleCtrlHandler ();
    //SetConsoleTitle(_T("imgupload");
#endif

    if(IuCoreUtils::FileExists(appDirectory + "/Data/servers.xml")) {
        dataFolder = appDirectory+"/Data/";
        settingsFolder = dataFolder;
    }
#ifndef _WIN32
    else {
        dataFolder = "/usr/share/uptooda-cli/";
    }
#ifndef __APPLE__
    settingsFolder = getenv("HOME")+std::string("/.config/uptooda-cli/");
    IuCoreUtils::CreateDir(settingsFolder, 0700);
#endif

#endif

#ifdef _WIN32
    COMInitializer comInitializer(COM_MULTI_THREADED);
#endif
    AppRuntimeInfo* appParams = AppRuntimeInfo::instance();
    appParams->setDataDirectory(dataFolder);
    appParams->setSettingsDirectory(settingsFolder);
    appParams->setIsGui(false);
    dotenv::init(dotenv::Preserve, (dataFolder + ".env").c_str());
#ifdef _WIN32
    TCHAR ShortPath[1024];
    GetTempPath(ARRAY_SIZE(ShortPath), ShortPath);
    TCHAR TempPath[1024];
    if (!GetLongPathName(ShortPath,TempPath, ARRAY_SIZE(TempPath)) ) {
        lstrcpy(TempPath, ShortPath);
    }
    appParams->setTempDirectory(IuCoreUtils::WstringToUtf8(TempPath));
#else
    appParams->setTempDirectory("/var/tmp/");
#endif

    if (!list->loadFromFile(dataFolder + "servers.xml", Settings.ServersSettings)) {
        std::cerr<<"Cannot load server list!"<<std::endl;
    }

    if (IuCoreUtils::FileExists(dataFolder + "userservers.xml") && !list->loadFromFile(dataFolder + "userservers.xml", Settings.ServersSettings)) {
        std::cerr<<"Cannot load server list userservers.xml!"<<std::endl;
    }
    Settings.LoadSettings(settingsFolder,"settings_cli.xml");

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << termcolor::red << err.what() << termcolor::reset << std::endl;
        std::cerr << program;
        return 1;
    }

    list->setNumOfRetries(maxRetries, maxRetriesPerAction);

    try {
        filesToUpload = program.get<std::vector<std::string>>("files");
        /*for (auto& file : filesToUpload) {
            if (!IuCoreUtils::FileExists(file)) {
                std::string errorMessage = str(boost::format("File '%s' doesn't exist!\n") % file);
                ConsoleUtils::instance()->printUnicode(stderr, errorMessage);
            }
        }*/
    } catch (std::logic_error& e) {
        std::cerr << termcolor::red << "No files provided" << termcolor::reset << std::endl;
        return 0;
    }

    try {
        auto params = program.get<std::vector<std::string>>("--server_param");
        for (const auto& param : params) {
            auto it = param.find(':');
            if (it != std::string::npos) {
                serverParameters[param.substr(0, it)] = param.substr(it + 1);
            } else {
                throw std::invalid_argument(str(boost::format("Invalid server parameter '%s'") % param));
            }
        }
    } catch (std::logic_error& e) {

    } catch (const std::exception& e) {
        std::cerr << termcolor::red << e.what() << termcolor::reset << std::endl;
        return 0;
    }

    if (filesToUpload.empty()) {
        return 0;
    }

#ifdef _WIN32
    if (autoUpdate) {
        DoUpdates();
    }
#endif

    res = func();

    if (!Settings.SaveSettings()) {
        std::cerr<<"Cannot save settings!"<<std::endl;
    }

    CScriptUploadEngine::DestroyScriptEngine();
#ifdef _WIN32
    //SetConsoleOutputCP(oldcp);
#endif

    return res;
}
