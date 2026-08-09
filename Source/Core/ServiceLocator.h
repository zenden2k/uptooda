#ifndef IU_CORE_SERVICELOCATOR_H
#define IU_CORE_SERVICELOCATOR_H

#include <memory>

#include "ProgramWindow.h"
#include "Core/Logging/Logger.h"
#include "Network/INetworkClient.h"

class BasicSettings;
class UploadManager;
class IDialogProvider;
class IUploadErrorHandler;
class CUploadEngineListBase;
class LocalFileCache;
class IHistoryManager;
class ServiceLocatorPrivate;
class ITranslator;
class ITaskRunner;
class CMyEngineList;
class CLogWindow;
class UrlShorteningFilter;
class TaskDispatcher;
class AbstractServerIconCache;
class NetworkDebugger;

class ServiceLocator {
    ServiceLocator();
public:
    ServiceLocator(ServiceLocator const&) = delete;
    void operator=(ServiceLocator const&) = delete;
    ~ServiceLocator() = default;

    std::shared_ptr<ILogger> logger();
    void setLogger(std::shared_ptr<ILogger> logger);
    ITranslator* translator();
    void setTranslator(ITranslator* transl);
    IDialogProvider* dialogProvider();
    void setDialogProvider(IDialogProvider* dialogProvider);
    std::shared_ptr<IUploadErrorHandler> uploadErrorHandler();
    void setUploadErrorHandler(std::shared_ptr<IUploadErrorHandler> errorHandler);
    ITaskRunner* taskRunner();
    void setTaskRunner(ITaskRunner* dispatcher);
    UploadManager* uploadManager() const;
    void setUploadManager(UploadManager* manager);
    void setMyEngineList(CMyEngineList* list);
    CMyEngineList* myEngineList() const;
    void setEngineList(CUploadEngineListBase* engineList);
    CUploadEngineListBase* engineList();
    LocalFileCache* localFileCache();
    IHistoryManager* historyManager();
    void setHistoryManager(IHistoryManager* historyManager);
    IProgramWindow* programWindow();
    void setProgramWindow(IProgramWindow* window);
    CLogWindow* logWindow() const;
    void setLogWindow(CLogWindow* logWindow);
    BasicSettings* basicSettings() const;
    void setSettings(BasicSettings* settingsInstance);
    std::shared_ptr<UrlShorteningFilter> urlShorteningFilter() const;
    void setUrlShorteningFilter(std::shared_ptr<UrlShorteningFilter> filter);
    void setNetworkDebugger(std::shared_ptr<NetworkDebugger> debugger);
    std::shared_ptr<NetworkDebugger> networkDebugger() const;

    template<class T> T* settings() const {
        T* res = dynamic_cast<T*>(basicSettings());
        //assert(res);
        return res;
    }

    void setNetworkClientFactory(std::shared_ptr<INetworkClientFactory> factory);
    std::shared_ptr<INetworkClientFactory> networkClientFactory() const;

    void setTaskDispatcher(TaskDispatcher* dispatcher);
    TaskDispatcher* taskDispatcher() const;

    void setServerIconCache(AbstractServerIconCache* cache);
    AbstractServerIconCache* serverIconCache() const;

    static ServiceLocator* instance();

protected:
    std::unique_ptr<ServiceLocatorPrivate> d_ptr;
};
#endif
