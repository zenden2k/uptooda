
#include <vector>
#include <boost/program_options.hpp>
#include <gtest/gtest.h>
#ifdef _MSC_VER
   // #include <vld.h> //Check for memory leaks
#endif
#include "versioninfo.h"
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <Core/Images/AbstractImage.h>

#include "sqtest.h"
#include "TestHelpers.h"
#include "Core/AppParams.h"
#include "Core/Scripting/Squirrelnc.h"
#include "Core/Scripting/API/ScriptAPI.h"
#include "Core/Logging.h"
#include "Core/Logging/ConsoleLogger.h"
#include "Core/Upload/ConsoleUploadErrorHandler.h"
#include "Core/Logging/MyLogSink.h"
#include "Core/ServiceLocator.h"
#include "Core/i18n/Translator.h"
#include "Core/3rdpart/xdgmime/xdgmime.h"

#ifdef _WIN32
#include "atlheaders.h"
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


#endif

void setAppVersion() {
    AppParams::AppVersionInfo appVersion;
    appVersion.FullVersion = IU_APP_VER;
    appVersion.FullVersionClean = IU_APP_VER_CLEAN;
    appVersion.Build = atoi(IU_BUILD_NUMBER);
    appVersion.BuildDate = IU_BUILD_DATE;
    appVersion.CommitHash = IU_COMMIT_HASH;
    appVersion.CommitHashShort = IU_COMMIT_HASH_SHORT;
    appVersion.BranchName = IU_BRANCH_NAME;
    AppParams::instance()->setVersionInfo(appVersion);
}

void printHandler(HSQUIRRELVM vm, const SQChar *s, ...)
{
    va_list vl;
    va_start(vl, s);
    vfprintf(stdout, s, vl);
    va_end(vl);

    (void)vm;
}

void errorHandler(HSQUIRRELVM vm, const SQChar *s, ...)
{
    va_list vl;
    va_start(vl, s);
    vfprintf(stderr, s, vl);
    va_end(vl);

    (void)vm;
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
    virtual std::wstring translateW(const char* str) override {
        return L"NOT_IMPLEMENTED";
    }
#endif
};

#ifdef _WIN32

CAppModule _Module;

// Convert UNICODE (UCS-2) command line arguments to utf-8
char ** convertArgv(int argc, _TCHAR* argvW[]) {
    char ** result = new char *[argc];
    for (int i = 0; i < argc; i++) {
        std::string unicodeString = IuCoreUtils::WstringToUtf8(argvW[i]);
        char *buffer = new char[unicodeString.length() + 1];
        strcpy(buffer, unicodeString.c_str());
        result[i] = buffer;
    }
    return result;
}

void freeArgv(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        delete[] argv[i];
    }
    delete[] argv;
}

class ArgDeleter {
public:
    ArgDeleter(int argc, char *argv[]) : argc_(argc), argv_(argv){
        
    }
    ~ArgDeleter() {
        freeArgv(argc_, argv_);
    }
    DISALLOW_COPY_AND_ASSIGN(ArgDeleter);
protected:
    int argc_;
    char **argv_;
};

int _tmain(int argc, _TCHAR* argvW[]) {
    char **argv = convertArgv(argc, argvW);
    //ArgDeleter deleter(argc, argv);
    //FLAGS_logtostderr = true;
#else
int main(int argc, char *argv[]){
#endif
    setAppVersion();
    ::testing::InitGoogleTest(&argc, argv);

    std::string testDataDir, testScriptsDir;
    try {
        namespace po = boost::program_options;
        po::options_description desc("General options");
        
        desc.add_options()
            ("help,h", "Show help")
            ("dir,d", po::value<std::string>(&testDataDir), "Test data directory")
            ("sqdir,s", po::value<std::string>(&testScriptsDir), "Squirrel tests directory (containing .nut files) ")
            ;

        po::variables_map vm;
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc)/*.allow_unregistered()*/.run();
        po::store(parsed, vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }
        po::notify(vm);


        boost::filesystem::path p(__FILE__);

        if (testDataDir.empty()) {
            testDataDir = (p.parent_path() / "TestData").string();
        }

        if (testScriptsDir.empty()) {
            testScriptsDir = (p.parent_path() / "../Core/Scripting/API/Tests/").string();
        }

        std::string dir = (p.parent_path() / "../../Data").string();
        char* cacheDir = strdup(dir.c_str());
        const char* dirs[2]
            = { cacheDir, nullptr };
        xdg_mime_set_dirs(dirs);
        free(cacheDir);

    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
        return 1;
    }


    if (!IuCoreUtils::DirectoryExists(testDataDir)) {
        std::cout << "Directory '" << testDataDir << "' does not exist." << std::endl;
        return 1;
    }
    if (!IuCoreUtils::DirectoryExists(testScriptsDir)) {
        std::cout << "Directory '" << testScriptsDir << "' does not exist." << std::endl;
        return 1;
    }
    TestHelpers::initDataPaths(testDataDir);
    

    //std::string rootDir = argv[1];
    //std::string testScriptsDir = rootDir + "/Source/Core/Scripting/API/Tests/";
    google::InitGoogleLogging("test");
    // Create and install global locale
    std::locale::global(boost::locale::generator().generate(""));
    // Make boost.filesystem use it
    boost::filesystem::path::imbue(std::locale());
    //BOOL res = SetProcessDefaultLayout(LAYOUT_RTL);

#if defined(_WIN32) && !defined(NDEBUG)
    // These global strings in GLOG are initially reserved with a small
    // amount of storage space (16 bytes). Resizing the string larger than its
    // initial size, after the _CrtMemCheckpoint call, can be reported as
    // a memory leak.
    // So for 'debug builds', where memory leak checking is performed,
    // reserve a large enough space so the string will not be resized later.
    // For these variables, _MAX_PATH should be fine.
    FLAGS_log_dir.reserve(_MAX_PATH); // comment out this line to trigger false memory leak
    FLAGS_log_link.reserve(_MAX_PATH);

    // Enable memory dump from within VS.
#endif
    FLAGS_logtostderr = false;
    FLAGS_alsologtostderr = true;
    auto defaultLogger = std::make_shared<ConsoleLogger>();
    auto uploadErrorHandler = std::make_shared<ConsoleUploadErrorHandler>();
    Translator translator;
    ServiceLocator* serviceLocator = ServiceLocator::instance();
    serviceLocator->setUploadErrorHandler(uploadErrorHandler);
    serviceLocator->setLogger(defaultLogger);
    serviceLocator->setTranslator(&translator);
    MyLogSink logSink(defaultLogger.get());
    google::AddLogSink(&logSink);

    AbstractImage::autoRegisterFactory<void>();

    Sqrat::SqratVM vm;
    vm.SetPrintFunc(&printHandler, &errorHandler);
    sqstd_seterrorhandlers(vm.GetVM());
    ScriptAPI::SetScriptName(vm, "test.nut");
    sq_pushroottable(vm.GetVM());
   
    SQRESULT result = sqtest_register_lib(vm.GetVM());
    if (SQ_FAILED(result)) {
        fprintf(stderr, "call to sqtest_register_lib failed.\n");
        return 1;
    }
    sq_pop(vm.GetVM(), 1);
    ScriptAPI::RegisterAPI(vm);
    sqtest_addtest(vm.GetVM(), (testScriptsDir + "/globals.nut").c_str());

    int res = RUN_ALL_TESTS();

    ScriptAPI::ClearVmData(vm);
    google::ShutdownGoogleLogging();
    return res;
}

