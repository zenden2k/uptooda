#include <string_view>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QStyleFactory>
#include <QTemporaryDir>

#include <boost/filesystem/path.hpp>
#include <boost/locale.hpp>

// #include <3rdparty/qtdotnetstyle.h>
#include "Core/3rdpart/dotenv.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/CommonDefs.h"
#include "Core/Logging.h"
#include "Core/Logging/MyLogSink.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/QtGuiSettings.h"
#include "Core/i18n/Translator.h"
#include "Gui/AppStyle.h"
#include "Gui/LogWindow.h"
#include "Gui/MainWindow.h"
#include "Gui/VirtualFileDrop.h"
#include "QtDefaultLogger.h"
#include "QtScriptDialogProvider.h"
#include "QtUploadErrorHandler.h"
#include "Video/QtImage.h"

#ifdef _WIN32
#include "Func/GdiPlusInitializer.h"
#include "Video/MediaFoundationFrameGrabber.h"
#endif
#include "BoostTranslator.h"
#include "versioninfo.h"
#include "Func/LangClass.h"

#ifdef _WIN32
CAppModule _Module;
QString dataFolder = "Data/";
#else
QString dataFolder = "/usr/share/uptooda/";
#endif
QtGuiSettings Settings;
std::unique_ptr<LogWindow> logWindow;

class MyApplication : public QApplication {
public:
    MyApplication(int& argc, char** argv, int flags = ApplicationFlags) : QApplication(argc, argv, flags) {
        AbstractImage::autoRegisterFactory<void>();
    }
#ifdef _WIN32
    MediaFoundationInitializer mediaFoundationInitializer_;
#endif
protected:
    bool notify(QObject* receiver, QEvent* event) override {
        if (event->type() == QEvent::KeyPress) {
            auto keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_L && keyEvent->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier)) {
                logWindow->show();
                logWindow->raise();
                logWindow->activateWindow();
                // TODO: do what you need to do
                return true;
            }
        }
        return QApplication::notify(receiver, event);
    }
};

#if defined(_MSC_VER) && !defined(NDEBUG)
extern "C" const char* __asan_default_options() {
    return "continue_on_error=1";
}
#endif

std::string GetLogDirectory(int argc, char* argv[]) {
    constexpr std::string_view LOG_DIR_OPTION = "--log_dir";

    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == LOG_DIR_OPTION && i + 1 < argc) {
            return argv[i + 1];
        }

        if (argument.compare(0, LOG_DIR_OPTION.size(), LOG_DIR_OPTION) == 0 && argument.size() > LOG_DIR_OPTION.size()
            && argument[LOG_DIR_OPTION.size()] == '=') {
            return std::string(argument.substr(LOG_DIR_OPTION.size() + 1));
            }
    }

    return {};
}

int main(int argc, char* argv[]) {
    ServiceLocator::instance()->setSettings(&Settings);
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
    FLAGS_logtostderr = false;
    FLAGS_alsologtostderr = true;
    // Enable memory dump from within VS.
#else
    FLAGS_logtostderr = true;
#endif
    std::string logDirectory = GetLogDirectory(argc, argv);
    if (!logDirectory.empty()) {
        FLAGS_log_dir = logDirectory;
        FLAGS_logtostderr = false;
    }

    google::InitGoogleLogging(argv[0]);

    AppRuntimeInfo::AppVersionInfo appVersion;
    appVersion.FullVersion = IU_APP_VER;
    appVersion.FullVersionClean = IU_APP_VER_CLEAN;
    appVersion.Build = std::stoi(IU_BUILD_NUMBER);
    appVersion.BuildDate = IU_BUILD_DATE;
    appVersion.CommitHash = IU_COMMIT_HASH;
    appVersion.CommitHashShort = IU_COMMIT_HASH_SHORT;
    appVersion.BranchName = IU_BRANCH_NAME;
    AppRuntimeInfo::instance()->setVersionInfo(appVersion);

    MyApplication a(argc, argv);
    auto *trans = new BoostTranslator(&a);


    VirtualFileDrop::installConverter();
    if (QStyle* fusionStyle = QStyleFactory::create(QStringLiteral("Fusion"))) {
        MyApplication::setStyle(fusionStyle);
    }
    ApplyApplicationStyle(a);
    logWindow = std::make_unique<LogWindow>();
    // logWindow->show();
    auto logger = std::make_shared<QtDefaultLogger>(logWindow.get());
    auto myLogSink_ = std::make_unique<MyLogSink>(logger.get());
    google::AddLogSink(myLogSink_.get());

#ifdef _WIN32
    GdiPlusInitializer gdiPlusInitializer;
#endif
    auto engineList = std::make_unique<CUploadEngineList>();
    auto errorHandler = std::make_shared<QtUploadErrorHandler>(logger.get(), engineList.get());
    QtScriptDialogProvider dlgProvider;
    auto serviceLocator = ServiceLocator::instance();

    serviceLocator->setUploadErrorHandler(errorHandler);
    serviceLocator->setLogger(logger);
    serviceLocator->setDialogProvider(&dlgProvider);
    serviceLocator->setSettings(&Settings);

    QString appDirectory = QCoreApplication::applicationDirPath();
    QString settingsFolder;
    setlocale(LC_ALL, "");

    if (QFileInfo::exists(appDirectory + "/Data/servers.xml")) {
        dataFolder = appDirectory + "/Data/";
        settingsFolder = dataFolder;
    }
#ifndef _WIN32
    else {
        dataFolder = "/usr/share/uptooda/";
    }

#ifndef __APPLE__
    settingsFolder = getenv("HOME") + QString("/.config/uptooda/");
    QDir settingsDir = QDir::root();
    settingsDir.mkpath(settingsFolder);
#endif

#endif
    qDebug() << "Data directory:" << dataFolder;
    qDebug() << "Settings directory:" << settingsFolder;
    AppRuntimeInfo* params = AppRuntimeInfo::instance();
    std::string dataFolderU8 = Q2U(dataFolder);
    params->setDataDirectory(dataFolderU8);
    params->setSettingsDirectory(Q2U(settingsFolder));
    dotenv::init(dotenv::Preserve, (dataFolderU8 + ".env").c_str());

    QTemporaryDir dir;
    if (dir.isValid()) {
        params->setTempDirectory(Q2U(dir.path()));
    } else {
        LOG(ERROR) << "Unable to create temp directory!";
    }


    Settings.LoadSettings(AppRuntimeInfo::instance()->settingsDirectory(), "uptooda.xml");

    if (!engineList->loadFromFile(AppRuntimeInfo::instance()->dataDirectory() + "servers.xml",
                                  Settings.ServersSettings)) {
        QMessageBox::warning(nullptr, "Failure", "Unable to load servers.xml");
    }

    std::filesystem::path messagesPath = std::filesystem::path(AppRuntimeInfo::instance()->dataDirectory()) / "Lang/locale/";
    CLang lang;
    lang.LoadLanguage(Settings.Language, messagesPath);
    MyApplication::installTranslator(trans);

    serviceLocator->setTranslator(&lang);
    ServiceLocator::instance()->setEngineList(engineList.get());
    // google::AddLogSink(&logSink);
    // serviceLocator->setUploadErrorHandler(&uploadErrorHandler);
    // serviceLocator->setLogger(&defaultLogger);

    Settings.setEngineList(engineList.get());

    MainWindow w(engineList.get(), logWindow.get());
    w.show();

    int res = a.exec();

    // google::RemoveLogSink(&logSink);
    Settings.SaveSettings();
    google::ShutdownGoogleLogging();
    logWindow.reset();
    return res;
}
