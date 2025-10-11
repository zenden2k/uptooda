/*

    Image Uploader -  free application for uploading images/files to the Internet

    Copyright 2007-2018 Sergey Svistunov (zenden2k@yandex.ru)

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

#include <cmath>
#include <iostream>
#include <condition_variable>
#include <boost/format.hpp>

#include <curl/curl.h>
#include <signal.h>

#include "Core/Upload/Uploader.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Network/NetworkClientFactory.h"
#include "Core/UploadEngineList.h"
#include "Core/Upload/UploadManager.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UploadSession.h"
#include "Core/Upload/ConsoleUploadErrorHandler.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Core/OutputCodeGenerator.h"
#include "Core/Upload/ScriptUploadEngine.h"
#include "Core/ServiceLocator.h"
#include "Core/Utils/StringUtils.h"
#include "Core/AppParams.h"
#include "Core/Settings/CliSettings.h"
#include "Core/Logging.h"
#include "Core/Logging/MyLogSink.h"
#include "Core/Logging/ConsoleLogger.h"
#include "Core/i18n/Translator.h"
#include "ConsoleScriptDialogProvider.h"
#include "Core/Utils/ConsoleUtils.h"
#include "Core/Scripting/ScriptsManager.h"

#ifdef _WIN32
    #include <windows.h>
    #include "Func/UpdatePackage.h"
    #include <fcntl.h>
    #include <io.h>
    #include <stdio.h>
    #include "Func/IuCommonFunctions.h"
    //#ifndef NDEBUG
        //#include <vld.h>
    //#endif
#else
    #include <sys/stat.h>
    #include <sys/time.h>
#endif
#include "versioninfo.h"


#define IU_CLI_VER "0.2.9"

#ifdef _WIN32

CAppModule _Module;
std::string dataFolder = "Data/";
#else
std::string dataFolder = "/usr/share/imageuploader/";
#endif
CliSettings Settings;
std::vector<std::string> filesToUpload;
std::string serverName;
std::string login;
std::string password;
std::string folderId;
std::string proxy;
int proxyPort;
int proxyType; /* CURLPROXY_HTTP, CURLPROXY_SOCKS4, CURLPROXY_SOCKS4A, 
			   CURLPROXY_SOCKS5, CURLPROXY_SOCKS5_HOSTNAME */
std::string proxyUser;
std::string proxyPassword;

bool useSystemProxy = false;

std::unique_ptr<CUploadEngineList> list;

ZOutputCodeGenerator::CodeType codeType = ZOutputCodeGenerator::ctClickableThumbnails;
ZOutputCodeGenerator::CodeLang codeLang = ZOutputCodeGenerator::clPlain;
bool autoUpdate = false;
std::shared_ptr<UploadSession> session;

std::mutex finishSignalMutex;
std::condition_variable finishSignal;
bool finished = false;

struct TaskUserData {
    int index;
};

std::vector<std::unique_ptr<TaskUserData>> userDataArray;

void SignalHandler (int param) {
    if ( session ) {
        session->stop();
    }
}

void PrintWelcomeMessage() {
    std::cerr << "imgupload v" << IU_CLI_VER << " (based on core of IU v" << IU_APP_VER << " build " << IU_BUILD_NUMBER << ")" << std::endl;
}

class Translator : public ITranslator {
public:
    virtual std::string getCurrentLanguage() override {
        return "English";
    }
    virtual std::string getCurrentLocale() override {
        return "en_US";
    }
    virtual std::string translate(const char* str) override{
        return str;
    }
#ifdef _WIN32
    virtual const wchar_t* translateW(const wchar_t* str) override {
        return str;
    }
#endif
};
Translator translator; // dummy translator

#ifdef _WIN32
void DoUpdates(bool force = false);
#endif

int64_t lastProgressTime = 0;

void PrintUsage(bool help = false) {
   std::cerr<<"USAGE:  "<<"imgupload [OPTIONS] filename1 filename2 ..."<<std::endl;
   if ( !help ) {
	std::cerr<<"Use '--help' option for more detailed information."<<std::endl;
   }
}

void PrintHelp() {
   std::cerr<<"\r\nAvailable options:"<<std::endl;
   std::cerr<<" -l Prints server list"<<std::endl;
   std::cerr<<" -s <server_name>"<<std::endl;
   std::cerr<<" -u <username>"<<std::endl;
   std::cerr<<" -p <password>"<<std::endl;
   std::cerr<<" -cl <bbcode|html|plain> (Default: plain)"<<std::endl;
   std::cerr<<" -ct <TableOfThumbnails|ClickableThumbnails|Images|Links>"<<std::endl;
   std::cerr<<" -fl <folder_id> ID of remote folder (supported by some servers)\r\n"
	   << "     Note that this is not the folder\'s name! \r\n"
	   << "     How to obtain it: open Image Uploader GUI version's\r\n"
	   << "     configuration file 'settings.xml' in text editor, \r\n"
	   << "     find your server under 'ServersParams' node,\r\n"
	   << "     and copy value of the '_FolderID' attribute"<<
	   std::endl;
   std::cerr<<" -pr <x.x.x.x:xxxx> Proxy address "<<std::endl;
   std::cerr<<" -pt <http|socks4|socks4a|socks5|socks5dns> Proxy type  (default http)"<<std::endl;
   std::cerr<<" -pu <username> Proxy username"<<std::endl;
   std::cerr<<" -pp <password> Proxy password"<<std::endl;
#ifdef _WIN32
    std::cerr << " -ps Use system proxy settings (this option supported only on Windows)" << std::endl;
    //std::cerr<<" --disable-update Disable auto-updating servers.xml"<<std::endl;
	std::cerr<<std::endl<<" -up   Update servers.xml\r\n"
		<<"     the 'Data' directory must be writable, otherwise update will fail"
		<<std::endl;
#endif
}

void PrintServerList()
{
    for (const auto& ued : *list) {
        if (!ued.hasType(CUploadEngineData::TypeImageServer) && !ued.hasType(CUploadEngineData::TypeFileServer)) {
		   continue;
        }
        std::cout << ued.Name << std::endl;
   }
}

bool parseCommandLine(int argc, char *argv[])
{
    if(argc == 1)
    {
        PrintUsage();
        return false;
    }
    int i = 1;
    while(i < argc)
    {
        char *opt = argv[i];
        if(!IuStringUtils::stricmp(opt, "--help"))
        {
            PrintUsage(true);
            PrintHelp();
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-s"))
        {
            if(i+1 == argc)
                return false;
            serverName = argv[++i];
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-l"))
        {
            PrintServerList();
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-cl"))
        {
            if(i+1 == argc)
                return false;
            char * codelang = argv[++i];
            if(!IuStringUtils::stricmp(codelang, "plain"))
                codeLang =  ZOutputCodeGenerator::clPlain;
            else if(!IuStringUtils::stricmp(codelang, "html"))
                codeLang =  ZOutputCodeGenerator::clHTML;
            else if(!IuStringUtils::stricmp(codelang, "bbcode"))
                codeLang =  ZOutputCodeGenerator::clBBCode;
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-ct"))
        {
            if(i+1 == argc)
                return false;
            char * codetype = argv[++i];
            if(!IuStringUtils::stricmp(codetype, "TableOfThumbnails"))
                codeType =  ZOutputCodeGenerator::ctTableOfThumbnails;
            else if(!IuStringUtils::stricmp(codetype, "ClickableThumbnails"))
                codeType =  ZOutputCodeGenerator::ctClickableThumbnails;
            else if(!IuStringUtils::stricmp(codetype, "Images"))
                codeType =  ZOutputCodeGenerator::ctImages;
            else if(!IuStringUtils::stricmp(codetype, "Links"))
                codeType =  ZOutputCodeGenerator::ctLinks;
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-u"))
        {
            if(i+1 == argc)
                return false;
            login = argv[++i];
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-p"))
        {
            if(i+1 == argc)
                return false;
            password = argv[++i];
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-fl"))
        {
            if(i+1 == argc)
                return false;
            folderId = argv[++i];
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-pr"))
        {
            if(i+1 == argc)
                return false;
            proxy = argv[++i];
            std::vector<std::string> tokens;
            IuStringUtils::Split(proxy,":",tokens,2);
            if ( tokens.size() > 1) {
                proxy = tokens[0];
                proxyPort = atoi(tokens[1].c_str());
            }
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-pu"))
        {
            if(i+1 == argc)
                return false;
            proxyUser = argv[++i];

            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-pp"))
        {
            if(i+1 == argc)
                return false;
            proxyPassword = argv[++i];

            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-pt"))
        {
            if(i+1 == argc)
                return false;
            std::map<std::string, int> types;
            std::string type = argv[++i];
            types["http"] = CURLPROXY_HTTP;
            types["socks4"] = CURLPROXY_SOCKS4;
            types["socks4a"] = CURLPROXY_SOCKS4A;
            types["socks5"] = CURLPROXY_SOCKS5;
            types["socks5dns"] = CURLPROXY_SOCKS5_HOSTNAME;
            auto it = types.find(type);
            if ( it != types.end() ) {
                proxyType = it->second;
            } else {
                std::cerr<<"Invalid proxy type"<<std::endl;
            }

            i++;
            continue;
        }
#ifdef _WIN32
        else if (!IuStringUtils::stricmp(opt, "-ps")) {
            useSystemProxy = true;
            i++;
            continue;
        }
        else if(!IuStringUtils::stricmp(opt, "-up"))
        {
            DoUpdates(true);
            i++;
            return 0;
        }
        else if(!IuStringUtils::stricmp(opt, "--disable-update"))
        {
            autoUpdate = false;
            i++;
            continue;
        }

#endif

        std::string fileName =
#ifndef _WIN32
                IuCoreUtils::SystemLocaleToUtf8(argv[i]);
#else
                argv[i];
#endif
        if ( !IuCoreUtils::FileExists(fileName) ) {
            std::string errorMessage = str(boost::format("File '%s' doesn't exist!\n") % fileName);
            ConsoleUtils::instance()->PrintUnicode(stderr, errorMessage);
            return false;
        }
        filesToUpload.push_back(fileName);
        i++;
    }

    return true;
}

CUploadEngineData* getServerByName(const std::string& name) {
    CUploadEngineData*   uploadEngineData = list->byName(serverName);
    if(!uploadEngineData) {
        for(int i=0; i<list->count(); i++)
        {
            if((IuStringUtils::toLower(list->byIndex(i)->Name).find(IuStringUtils::toLower((name)))) != std::string::npos)
                return list->byIndex(i);
        }
    }
    return uploadEngineData;
}

void OnUploadSessionFinished(UploadSession* session) {
    int taskCount = session->taskCount();
    std::vector<ZUploadObject> uploadedList;
    for ( int i = 0; i < taskCount; i++ ) {
        auto task = session->getTask(i);
        UploadResult* res = task->uploadResult();
        FileUploadTask* fileTask = dynamic_cast<FileUploadTask*>(task.get());
        if ( task->uploadSuccess() ) {
            ZUploadObject uo;
            uo.directUrl = res->directUrl;
            uo.thumbUrl = res->thumbUrl;
            uo.viewUrl = res->downloadUrl;
            uo.serverName = task->serverName();
            if ( fileTask ) {
                uo.localFilePath = fileTask->getFileName();
                uo.displayFileName = fileTask->getDisplayName();
            }
            uo.uploadFileSize = task->getDataLength();
            uploadedList.push_back(uo);
        }
    }
    ZOutputCodeGenerator generator;
    generator.setLang(codeLang);
    generator.setType(codeType);
    //ConsoleUtils::instance()->SetCursorPos(0, taskCount + 2);
    if ( !uploadedList.empty() ) {
        std::cerr<<std::endl<<"Result:"<<std::endl;
        ConsoleUtils::instance()->printUnicode(stdout, generator.generate(uploadedList));
        std::cerr<<std::endl;
    }
    {
        // LOG(ERROR) << "Sending finish signal" << std::endl;
        std::lock_guard<std::mutex> lk(finishSignalMutex);
        finished = true;
    }

    finishSignal.notify_one();
}

void UploadTaskProgress(UploadTask* task) {
    std::lock_guard<std::mutex> guard(ConsoleUtils::instance()->getOutputMutex());
    UploadProgress* progress = task->progress();
    TaskUserData *userData = reinterpret_cast<TaskUserData*>(task->userData());

    struct timeval tp;
    gettimeofday(&tp, NULL);
    int64_t ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    if ( progress->totalUpload != progress->uploaded && ms - lastProgressTime < 500 ) { // Windows console output is too slow
        return;
    }
    lastProgressTime = ms;
    int totaldotz=30;
    if(progress->totalUpload == 0)
        return;

    //ConsoleUtils::instance()->SetCursorPos(0, 2 + userData->index);
    double fractiondownloaded = static_cast<double>(progress->uploaded) / progress->totalUpload;
    if(fractiondownloaded > 100)
        fractiondownloaded = 0;
    // part of the progressmeter that's already "full"
    int dotz = static_cast<int>(floor(fractiondownloaded * totaldotz));

    // create the "meter"
    int ii=0;
    fprintf(stderr, "%3.0f%% [",fractiondownloaded*100);
    // part  that's full already
    for ( ; ii < dotz;ii++) {
        fprintf(stderr,"=");
    }
    // remaining part (spaces)
    for ( ; ii < totaldotz;ii++) {
        fprintf(stderr," ");
    }
    // and back to line begin - do not forget the fflush to avoid output buffering problems!
    fprintf(stderr,"]");
    fprintf(stderr," %s/%s", IuCoreUtils::fileSizeToString(progress->uploaded).c_str(),
            IuCoreUtils::fileSizeToString(progress->totalUpload).c_str());
    fprintf(stderr,"\r");
    fflush(stderr);
}

void OnUploadTaskStatusChanged(UploadTask* task) {
    std::lock_guard<std::mutex> guard(ConsoleUtils::instance()->getOutputMutex());
    UploadProgress* progress = task->progress();
    TaskUserData *userData = reinterpret_cast<TaskUserData*>(task->userData());
    //ConsoleUtils::instance()->SetCursorPos(55, 2 + userData->index);
    fputs(progress->statusText.c_str(), stderr);
    fprintf(stderr, "\r");
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
	int res = 0;
    auto uploadErrorHandler = std::make_shared<ConsoleUploadErrorHandler>();
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
    std::string scriptsDirectory = AppParams::instance()->dataDirectory() + "/Scripts/";
    uploadEngineManager->setScriptsDirectory(scriptsDirectory);
    std::shared_ptr<UploadManager> uploadManager = std::make_shared<UploadManager>(uploadEngineManager.get(), list.get(), scriptsManager.get(), uploadErrorHandler, networkClientFactory, 1);


    if (useSystemProxy) {
        Settings.ConnectionSettings.UseProxy = ConnectionSettingsStruct::kSystemProxy;
    } else if ( !proxy.empty()) {
        Settings.ConnectionSettings.UseProxy = ConnectionSettingsStruct::kUserProxy;
        Settings.ConnectionSettings.ServerAddress= proxy;
        Settings.ConnectionSettings.ProxyPort = proxyPort;

        if( !proxyUser.empty()) {
            Settings.ConnectionSettings.NeedsAuth = true;
            Settings.ConnectionSettings.ProxyUser = proxyUser;
            Settings.ConnectionSettings.ProxyPassword.fromPlainText(proxyPassword);
        }
    }

    CUploadEngineData* uploadEngineData = 0;
    if(!serverName.empty()) {
        uploadEngineData = getServerByName(serverName);
        if(!uploadEngineData) {
            std::cerr<<"No such server '"<<serverName<<"'!"<<std::endl;
            return 0;
        }
    } else {
        std::cerr << "Server not set " << std::endl;
        return -1;
        //int index = list.getRandomImageServer();
        //uploadEngineData = list.byIndex(index);
    }

    if (uploadEngineData->NeedAuthorization == CUploadEngineData::naObligatory && login.empty())
	{
		std::cerr<<"Server '"<<uploadEngineData->Name<<"' requires authentication! Use -u and -p options."<<std::endl;
		return -1;
	}

    ServerProfile serverProfile(uploadEngineData->Name);
    serverProfile.setProfileName(login);
    serverProfile.setShortenLinks(false);

    ServerSettingsStruct& s = Settings.ServersSettings[uploadEngineData->Name][login];

    s.authData.Password = password;
    s.authData.Login = login;
    if(!login.empty()) {
        s.authData.DoAuth = true;
    }

    s.setParam("FolderID", folderId);
    serverProfile.setFolderId(folderId);

    session.reset(new UploadSession(false));
    for(size_t i=0; i<filesToUpload.size(); i++) {
        if(!IuCoreUtils::FileExists(filesToUpload[i]))
        {
            std::string errorMessage = str(boost::format("File '%s' doesn't exist!")%filesToUpload[i]);
            ConsoleUtils::instance()->PrintUnicode(stderr, errorMessage);
            res++;
            continue;
        }

        std::shared_ptr<FileUploadTask> task(new FileUploadTask(filesToUpload[i], IuCoreUtils::ExtractFileName(filesToUpload[i])));
        task->setServerProfile(serverProfile);
        task->setOnUploadProgressCallback(UploadTaskProgress);
        task->setOnStatusChangedCallback(OnUploadTaskStatusChanged);
        TaskUserData *userData = new TaskUserData;
        userData->index = i;
        task->setUserData(userData);
        userDataArray.push_back(std::unique_ptr<TaskUserData>(userData));
        session->addTask(task);
    }
    session->addSessionFinishedCallback(UploadSession::SessionFinishedCallback(OnUploadSessionFinished));
    //ConsoleUtils::instance()->InitScreen();
    //ConsoleUtils::instance()->Clear();
    //PrintWelcomeMessage();
    uploadManager->setOnQueueFinishedCallback(OnQueueFinished);
    uploadManager->addSession(session);

    // Wait until upload session is finished
    std::unique_lock<std::mutex> lk(finishSignalMutex);
    while (!finished) {
        finishSignal.wait(lk/*, [] {return finished;}*/);
    }
	return res;	
}

#ifdef _WIN32
class Updater: public CUpdateStatusCallback {
public:
    Updater(const CString& tempDirectory) :m_UpdateManager(std::make_shared<NetworkClientFactory>(), tempDirectory){
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
            for (size_t i = 0; i < m_UpdateManager.m_updateList.size(); i++)
            {
                std::cerr<<"Beginning to update: "<< IuCoreUtils::WstringToUtf8((LPCTSTR)m_UpdateManager.m_updateList[i].displayName())<<std::endl;
            }

            m_UpdateManager.DoUpdates();
            if (m_UpdateManager.successPackageUpdatesCount())
            {
                std::cerr<<"Succesfully updated!";
            }
        }
        else
        {

            std::cerr<<"All is up-to-date"<<std::endl;
        }

    }
    virtual void updateStatus(int packageIndex, const CString& status) override {
        //std::wcout << (LPCTSTR)m_UpdateManager.m_updateList[packageIndex].displayName() <<" : "<< (LPCTSTR)status<<std::endl;
        if ( m_UpdateManager.m_updateList.size() > packageIndex+1) {
            fprintf(stderr, "%s : %s", IuCoreUtils::WstringToUtf8((LPCTSTR)m_UpdateManager.m_updateList[packageIndex].displayName()).c_str(), IuCoreUtils::WstringToUtf8((LPCTSTR)status).c_str());
            fprintf(stderr, "\r");
            fflush(stderr);
        }
        //std::cout<< "\r"<<IuCoreUtils::WstringToUtf8((LPCTSTR)m_UpdateManager.m_updateList[packageIndex].displayName()) <<" : "<< IuCoreUtils::WstringToUtf8((LPCTSTR)status);
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
    google::InitGoogleLogging(argv[0]);

    AppParams::AppVersionInfo appVersion;
    appVersion.FullVersion = IU_APP_VER;
    appVersion.FullVersionClean = IU_APP_VER_CLEAN;
    appVersion.Build = std::stoi(IU_BUILD_NUMBER);
    appVersion.BuildDate = IU_BUILD_DATE;
    appVersion.CommitHash = IU_COMMIT_HASH;
    appVersion.CommitHashShort = IU_COMMIT_HASH_SHORT;
    appVersion.BranchName = IU_BRANCH_NAME;
    AppParams::instance()->setVersionInfo(appVersion);
    list = std::make_unique<CUploadEngineList>();
    std::shared_ptr<ConsoleLogger> defaultLogger = std::make_shared<ConsoleLogger>();
    
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
        dataFolder = "/usr/share/imgupload/";
    }
#ifndef __APPLE__
    settingsFolder = getenv("HOME")+std::string("/.config/imgupload/");
    mkdir(settingsFolder.c_str(), 0700);
#endif

#endif
    AppParams* params = AppParams::instance();
    params->setDataDirectory(dataFolder);
    params->setSettingsDirectory(settingsFolder);
    params->setIsGui(false);
#ifdef _WIN32
    TCHAR ShortPath[1024];
    GetTempPath(ARRAY_SIZE(ShortPath), ShortPath);
    TCHAR TempPath[1024];
    if (!GetLongPathName(ShortPath,TempPath, ARRAY_SIZE(TempPath)) ) {
        lstrcpy(TempPath, ShortPath);
    }
    params->setTempDirectory(IuCoreUtils::WstringToUtf8(TempPath));
#else
    params->setTempDirectory("/var/tmp/");
#endif
    PrintWelcomeMessage();
    if(! list->loadFromFile(dataFolder + "servers.xml", Settings.ServersSettings)) {
        std::cerr<<"Cannot load server list!"<<std::endl;
    }

    if( IuCoreUtils::FileExists(dataFolder + "userservers.xml") && !list->loadFromFile(dataFolder + "userservers.xml", Settings.ServersSettings)) {
        std::cerr<<"Cannot load server list userservers.xml!"<<std::endl;
    }
    Settings.LoadSettings(settingsFolder,"settings_cli.xml");


    if(!parseCommandLine(argc, argv))
    {
        return 0;
    }
    if(!filesToUpload.size())
        return 0;

#ifdef _WIN32
    if (autoUpdate) {
        DoUpdates();
    }
#endif

    res = func();

    if ( !Settings.SaveSettings() ) {
        std::cerr<<"Cannot save settings!"<<std::endl;
    }

    CScriptUploadEngine::DestroyScriptEngine();
#ifdef _WIN32
    //SetConsoleOutputCP(oldcp);
#endif
    return res;
}
