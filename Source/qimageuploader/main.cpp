#include <QApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QDebug>
#include <QKeyEvent>
#include <QMessageBox>

#include <boost/filesystem/path.hpp>
#include <boost/locale.hpp>

//#include <3rdparty/qtdotnetstyle.h>
#include "Core/Logging.h"
#include "Core/Logging/MyLogSink.h"
#include "Gui/MainWindow.h"
#include "Core/CommonDefs.h"
#include "Core/ServiceLocator.h"
#include "Core/AppRuntimeInfo.h"
#include "QtUploadErrorHandler.h"
#include "QtDefaultLogger.h"
#include "QtScriptDialogProvider.h"
#include "Gui/LogWindow.h"
#include "Core/Settings/QtGuiSettings.h"
#include "Video/QtImage.h"
#include "Core/i18n/Translator.h"
#include "Core/3rdpart/dotenv.h"

#ifdef _WIN32
    #include "Func/GdiPlusInitializer.h"
    #include "Video/MediaFoundationFrameGrabber.h"
#endif
#include "versioninfo.h"

#ifdef _WIN32
CAppModule _Module;
QString dataFolder = "Data/";
#else
QString dataFolder = "/usr/share/uptooda/";
#endif
QtGuiSettings Settings;
std::unique_ptr<LogWindow> logWindow;
class Translator : public ITranslator {
public:
	std::string getCurrentLanguage() override {
		return "English";
	}
	std::string getCurrentLocale() override {
		return "en_US";
	}
	std::string translate(const char* str) override {
		return str;
	}
#ifdef _WIN32
    std::wstring translateW(const char* str) override {
		return IuCoreUtils::Utf8ToWstring(str);
	}
#endif
};
Translator translator; // dummy translator

class MyApplication : public QApplication
{
public:
    MyApplication(int &argc, char **argv, int flags = ApplicationFlags): QApplication(argc, argv, flags)
    {
        AbstractImage::autoRegisterFactory<void>();
    }
#ifdef _WIN32
    MediaFoundationInitializer mediaFoundationInitializer_;
#endif
protected:

    bool notify(QObject *receiver, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_L && keyEvent->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier) )
            {
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

int main(int argc, char *argv[])
{
    ServiceLocator::instance()->setSettings(&Settings);
#if defined(_WIN32) && !defined(NDEBUG)
    // These global strings in GLOG are initially reserved with a small
    // amount of storage space (16 bytes). Resizing the string larger than its
    // initial size, after the _CrtMemCheckpoint call, can be reported as
    // a memory leak.
    // So for 'debug builds', where memory leak checking is performed,
    // reserve a large enough space so the string will not be resized later.
    // For these variables, _MAX_PATH should be fine.
    FLAGS_log_dir.reserve(_MAX_PATH);  // comment out this line to trigger false memory leak
    FLAGS_log_link.reserve(_MAX_PATH);
    FLAGS_logtostderr = false;
    FLAGS_alsologtostderr = true;
    // Enable memory dump from within VS.
#else
        FLAGS_logtostderr = true;
#endif

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
    logWindow = std::make_unique<LogWindow>();
    //logWindow->show();
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
    serviceLocator->setTranslator(&translator);
    serviceLocator->setUploadErrorHandler(errorHandler);
    serviceLocator->setLogger(logger);
    serviceLocator->setDialogProvider(&dlgProvider);
    serviceLocator->setSettings(&Settings);

    QString appDirectory = QCoreApplication::applicationDirPath();
    QString settingsFolder;
    setlocale(LC_ALL, "");

    if(QFileInfo::exists(appDirectory + "/Data/servers.xml")){
        dataFolder = appDirectory+"/Data/";
        settingsFolder = dataFolder;
    }
#ifndef _WIN32
   else {
dataFolder = "/usr/share/uptooda/";
   }

#ifndef __APPLE__
settingsFolder = getenv("HOME")+QString("/.config/uptooda/");
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

	if (!engineList->loadFromFile(AppRuntimeInfo::instance()->dataDirectory() + "servers.xml", Settings.ServersSettings)) {
		QMessageBox::warning(nullptr, "Failure", "Unable to load servers.xml");
	}
    ServiceLocator::instance()->setEngineList(engineList.get());
    //google::AddLogSink(&logSink);
    //serviceLocator->setUploadErrorHandler(&uploadErrorHandler);
    //serviceLocator->setLogger(&defaultLogger);

	Settings.setEngineList(engineList.get());
	
    //QApplication::setStyle("Fusion");
    MainWindow w(engineList.get(), logWindow.get());
    w.show();
    
    int res =  a.exec();

    //google::RemoveLogSink(&logSink);
	Settings.SaveSettings();
    google::ShutdownGoogleLogging();
    logWindow.reset();
    return res;
}
