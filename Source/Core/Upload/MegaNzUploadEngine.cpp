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

#include "MegaNzUploadEngine.h"

#include <chrono>
#include <cassert>
#include <memory>
#include <thread>

#include <megaapi.h>
#include "Core/Upload/ServerSync.h"
#include "Uploader.h"
#include "Core/Settings/BasicSettings.h"
#include "FileUploadTask.h"
#include "Core/CommonDefs.h"
#include "Core/i18n/Translator.h"
#ifdef _WIN32
#include "Func/IuCommonFunctions.h"
#include "3rdpart/GdiplusH.h"
#include "Core/Images/Utils.h"
#endif
#include "Core/AppRuntimeInfo.h"

#define APP_KEY "0dxDFKqD"
#define USER_AGENT APP_NAME_A

using namespace mega;

#ifdef _WIN32

class MyGfxProcessor : public mega::MegaGfxProcessor {
public:
    MyGfxProcessor();
    bool readBitmap(const std::string& path) override;
    int getWidth() override;
    int getHeight() override;
    int getBitmapDataSize(int width, int height, int px, int py, int rw, int rh, int hint) override;
    bool getBitmapData(char *bitmapData, size_t size) override;
    void freeBitmap() override;

    ~MyGfxProcessor() override;
protected:
    std::unique_ptr<Gdiplus::Bitmap> bitmap_;
    HGLOBAL hGlobal_;
    CComPtr<IStream> istream_;
};

MyGfxProcessor::MyGfxProcessor() {
    hGlobal_ = nullptr;
}
bool MyGfxProcessor::readBitmap(const std::string& path) {
    // path parameter contains utf-16 string
    CString widePath(reinterpret_cast<const wchar_t*>(path.data()), path.size() / 2);

    if (!IuCommonFunctions::IsImage(widePath)) {
        return false;
    }
    std::unique_ptr<GdiPlusImage> img = ImageUtils::LoadImageFromFileExtended(widePath);
    if (!img) {
        return false;
    }
    bitmap_ = std::unique_ptr<Gdiplus::Bitmap>(img->releaseBitmap());
    if (!bitmap_) {
        return false;
    }
    return true;
}

int MyGfxProcessor::getWidth() {
    if (bitmap_) {
        return bitmap_->GetWidth();
    }
    return 0;
}

int MyGfxProcessor::getHeight() {
    if (bitmap_) {
        return bitmap_->GetHeight();
    }
    return 0;
}

int MyGfxProcessor::getBitmapDataSize(int width, int height, int px, int py, int rw, int rh, int hint) {
    using namespace Gdiplus;
    if (!bitmap_) {
        return 0;
    }
    std::unique_ptr<Bitmap> res = std::make_unique<Bitmap>(width, height);
    Graphics gr(res.get());
    gr.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    gr.DrawImage(bitmap_.get(), 0, 0, width, height);

    std::unique_ptr<Bitmap> resultBmp;
    if (px == 0 && py == 0 && rw == width && rh == height) {
        resultBmp = std::move(res);
    } else {
        std::unique_ptr<Bitmap> buffer = std::make_unique<Bitmap>(rw, rh);
        Graphics gr2(buffer.get());
        gr2.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        gr2.DrawImage(res.get(), 0, 0, px, py, rw, rh, UnitPixel);
        resultBmp = std::move(buffer);
    }

    if (!resultBmp) {
        return 0;
    }

    HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &istream_);
    if (FAILED(hr)) {
        return 0;
    }

    // Saving image to memory stream
    try {
        if (ImageUtils::SaveImageToFile(resultBmp.get(), CString(), istream_, hint == GFX_HINT_FORMAT_PNG ? ImageUtils::sifPNG : ImageUtils::sifJPEG, 85)) {
            hr = GetHGlobalFromStream(istream_, &hGlobal_);
            if (FAILED(hr)) {
                return 0;
            }
            return GlobalSize(hGlobal_);
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Failed to save image: " << ex.what();
    }
    return 0;
}

bool MyGfxProcessor::getBitmapData(char* bitmapData, size_t size) {
    if (!hGlobal_) {
        return false;
    }
    LPVOID pimage = GlobalLock(hGlobal_);
    if (pimage == nullptr) {
        return false;
    }
    memcpy(bitmapData, pimage, size);
    GlobalUnlock(hGlobal_);
    istream_.Release();
    return true;
}

void MyGfxProcessor::freeBitmap() {
    hGlobal_ = nullptr;
    bitmap_.reset();
}

MyGfxProcessor::~MyGfxProcessor() {
}

#endif // End of #ifdef _WIN32

class MyListener : public mega::MegaListener {
public:
    explicit MyListener(CMegaNzUploadEngine* engine) {
        engine_ = engine;
    }
    ~MyListener() {

    }
    void onRequestFinish(mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError* e) override;
    //Currently, this callback is only valid for the request fetchNodes()
    void onRequestUpdate(mega::MegaApi*api, mega::MegaRequest *request) override;
    void onRequestTemporaryError(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError* error) override;
    void onTransferFinish(mega::MegaApi* api, mega::MegaTransfer *transfer, mega::MegaError* error) override;
    void onTransferUpdate(mega::MegaApi *api, mega::MegaTransfer *transfer) override;
    void onTransferTemporaryError(mega::MegaApi *api, mega::MegaTransfer *transfer, mega::MegaError* error) override;
    void onUsersUpdate(mega::MegaApi* api, mega::MegaUserList *users) override;
    void onNodesUpdate(mega::MegaApi* api, mega::MegaNodeList *nodes) override;
private:
    CMegaNzUploadEngine* engine_;
};
CMegaNzUploadEngine::CMegaNzUploadEngine(std::shared_ptr<ServerSync> serverSync, ServerSettingsStruct* settings, ErrorMessageCallback errorCallback) :
    CAdvancedUploadEngine(std::move(serverSync), settings, std::move(errorCallback))
{
    setServerSettings(settings);
    loginFinished_ = false;
    loginSuccess_ = false;
    fetchNodesFinished_ = false;
    fetchNodesSuccess_ = false;
    uploadFinished_ = false;
    uploadSuccess_ = false;
    exportFinished_ = false;
    exportSuccess_ = false;
    createFolderFinished_ = false;
    createFolderSuccess_ = false;
    renameFolderFinished_ = false;
    renameFolderSuccess_ = false;
    folderList_ = nullptr;
#ifdef _WIN32
    proc_ = std::make_unique<MyGfxProcessor>();
    gfxProvider_.reset(MegaGfxProvider::createExternalInstance(proc_.get()));
    megaApi_ = std::make_unique<MegaApi>(APP_KEY, gfxProvider_.get(), /* static_cast<const char*>(nullptr)*/ AppRuntimeInfo::instance()->tempDirectory().c_str(), USER_AGENT);
#else
    megaApi_.reset(new MegaApi(APP_KEY, (const char *)NULL, USER_AGENT));
#endif
    megaApi_->setLogLevel(MegaApi::LOG_LEVEL_INFO);
    listener_ = std::make_unique<MyListener>(this);
    megaApi_->addListener(listener_.get());
    proxy_ = std::make_unique<MegaProxy>();
    BasicSettings& Settings = *ServiceLocator::instance()->basicSettings();

    if (Settings.ConnectionSettings.UseProxy == ConnectionSettingsStruct::kUserProxy) {
        if (Settings.ConnectionSettings.ProxyType != 0) {
            log(ErrorInfo::mtError, "This proxy type is not supported by Mega.Nz engine.");
        }
        else {
            proxy_->setProxyType(MegaProxy::PROXY_CUSTOM);
            proxy_->setProxyURL((std::string("http://") + Settings.ConnectionSettings.ServerAddress + ":" + std::to_string(Settings.ConnectionSettings.ProxyPort) + "/").c_str());
            if (Settings.ConnectionSettings.NeedsAuth) {
                proxy_->setCredentials(Settings.ConnectionSettings.ProxyUser.c_str(), std::string(Settings.ConnectionSettings.ProxyPassword).c_str());
            }
        }
    }
    else if (Settings.ConnectionSettings.UseProxy == ConnectionSettingsStruct::kSystemProxy) {
        proxy_.reset(megaApi_->getAutoProxySettings());
    }
    else {
        proxy_->setProxyType(MegaProxy::PROXY_NONE);
    }
    if (proxy_) {
        megaApi_->setProxySettings(proxy_.get());
    }
}

CMegaNzUploadEngine::~CMegaNzUploadEngine()
{
    megaApi_->removeListener(listener_.get());
}

int CMegaNzUploadEngine::getFolderList(CFolderList& FolderList) {
    if (!loginFinished_) {
        if (!doLogin()) {
            return 0;
        }
    }
    folderList_ = &FolderList;
    if (ensureNodesFetched()) {
        std::string parentId = FolderList.parentFolder().getId();

        if (parentId.empty()) {
            FolderList.AddFolder("/", "", std::string("/"), "", 0);
        } else {
            MegaNode* parent = megaApi_->getNodeByPath(parentId.c_str());
            if (parent) {
                cancelToken_.reset(mega::MegaCancelToken::createInstance());
                MegaNodeList* list = megaApi_->getChildren(parent, 1, cancelToken_.get());

                if (list) {
                    for (int i = 0; i < list->size(); i++) {
                        MegaNode* node = list->get(i);
                        if (node->isFolder()) {
                            std::string nodeId = parentId;

                            // nodeId cannot be empty at this line, so we can avoid checking for emptiness here.
                            if (nodeId.back() != '/') {
                                nodeId.append("/");
                            }
                            nodeId += node->getName();
                            FolderList.AddFolder(node->getName(), "", nodeId, parentId, 0);
                        }
                    }
                }
                delete list;
                delete parent;
            }
        }
    }

    return fetchNodesSuccess_ ? 1 : 0;
}

int CMegaNzUploadEngine::createFolder(const CFolderItem& parent, CFolderItem& folder) {
    if (!loginFinished_) {
        if (!doLogin()) {
            return 0;
        }
    }

    if (needStop()) {
        return 0;
    }

    createFolderSuccess_ = false;
    createFolderFinished_ = false;
    if (ensureNodesFetched()) {
        std::unique_ptr<MegaNode> parentNode;
        if (parent.getId().empty()) {
            parentNode.reset(megaApi_->getRootNode());
        } else {
            parentNode.reset(megaApi_->getNodeByPath(parent.getId().c_str()));
        }

        if (!parentNode || !parentNode->isFolder()) {
            log(ErrorInfo::mtError, "Unable to find parent folder");
            return 0;
        }
        newFolder_ = &folder;
        parentFolder_ = &parent;
        megaApi_->createFolder(folder.getTitle().c_str(), parentNode.get());
        while (!createFolderFinished_ && !needStop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        newFolder_ = nullptr;
        parentFolder_ = nullptr;
        return createFolderSuccess_ ? 1 : 0;
    }
    return 0;
}

int CMegaNzUploadEngine::modifyFolder(CFolderItem& folder) {
    if (!loginFinished_) {
        if (!doLogin()) {
            return 0;
        }
    }

    if (needStop()) {
        return 0;
    }

    renameFolderFinished_ = false;
    renameFolderSuccess_ = false;
    if (ensureNodesFetched()) {
        std::unique_ptr<MegaNode> node(megaApi_->getNodeByPath(folder.getId().c_str()));

        if (!node || !node->isFolder()) {
            log(ErrorInfo::mtError, "Unable to find folder '"+folder.getId()+"'");
            return 0;
        }
        megaApi_->renameNode(node.get(), folder.getTitle().c_str());
        while (!renameFolderFinished_ && !needStop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return renameFolderSuccess_ ? 1 : 0;
    }

    return 0;
}

int CMegaNzUploadEngine::getAccessTypeList(std::vector<std::string>& list) {
    list.emplace_back("Default");
    return 1;
}

int CMegaNzUploadEngine::getServerParamList(ParameterList& list)
{
    return 0;
}

int CMegaNzUploadEngine::doLogin() {
    loginFinished_ = false;
    loginSuccess_ = false;
    if (currentTask_) {
        currentTask_->setStatusText(_("Logging in..."));
    }

    megaApi_->login(m_ServersSettings->authData.Login.c_str(), m_ServersSettings->authData.Password.c_str());
    while (!loginFinished_ && !needStop()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return loginSuccess_ ? 1 : 0;
}

int CMegaNzUploadEngine::doLogout() {
    return 0;
}
bool CMegaNzUploadEngine::isAuthenticated() {
    return loginSuccess_;
}
bool CMegaNzUploadEngine::supportsLogout() {
    return false;
};

int CMegaNzUploadEngine::doProcessTask(std::shared_ptr<UploadTask> task, UploadParams& params) {
    if (task->type() == UploadTask::TypeAuth) {
        return doLogin();
    } if (task->type() == UploadTask::TypeFolder) {
        return processFolderTask(task);
    } else {
        return doUpload(task, params);
    }
}

int CMegaNzUploadEngine::doUpload(std::shared_ptr<UploadTask> task, UploadParams& params)
{
    if (task->type() != UploadTask::TypeFile) {
        return 0;
    }
    currentTask_ = task;

    if (!loginFinished_) {
        if (!doLogin()) {
            return 0;
        }
    }

    if (ensureNodesFetched()) {
        createNewFolderIfNeeded(task);
        publicLink_.clear();
        fileTask_ = std::dynamic_pointer_cast<FileUploadTask>(task);
        if (!fileTask_) {
            return 0;
        }
        std::unique_ptr<MegaNode> root;
        std::string folderId = fileTask_->serverProfile().folderId();
        if (folderId.empty()) {
            root.reset(megaApi_->getRootNode());
        } else {
            root.reset(megaApi_->getNodeByPath(folderId.c_str()));
            if (root == nullptr) {
                log(ErrorInfo::mtError, "Folder '" + folderId + "' not found");
                return 0;
            }
        }

        std::string origFileName = IuCoreUtils::ExtractFileName(fileTask_->getFileName());
        std::string newFileName = origFileName;
        std::unique_ptr<MegaNode> node(megaApi_->getChildNode(root.get(), origFileName.c_str()));
        int i = 2;
        while (node) {
            newFileName = IuCoreUtils::IncrementFileName(origFileName, i++);
            node.reset(megaApi_->getChildNode(root.get(), newFileName.c_str()));
        }

        uploadFinished_ = false;
        uploadSuccess_ = false;
        cancelToken_.reset(mega::MegaCancelToken::createInstance());
        megaApi_->startUpload(fileTask_->getFileName().c_str(), root.get(), newFileName.c_str(), MegaApi::INVALID_CUSTOM_MOD_TIME, nullptr, false, false, cancelToken_.get());
        while (!uploadFinished_ && !needStop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!uploadSuccess_ || needStop()) {
            return 0;
        }
        while (!exportFinished_ && !needStop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (exportSuccess_) {
            params.setViewUrl(publicLink_);
            return 1;
        }

    }
    return 0;
}

bool CMegaNzUploadEngine::supportsSettings()
{
    return false;
}

bool CMegaNzUploadEngine::supportsBeforehandAuthorization()
{
    return false;
}


void CMegaNzUploadEngine::stop() {
    if (cancelToken_) {
        cancelToken_->cancel();
    }
}

bool CMegaNzUploadEngine::ensureNodesFetched() {
    std::unique_ptr<MegaNode> root(megaApi_->getRootNode());
    if (!root) {
        fetchNodesFinished_ = false;
        fetchNodesSuccess_ = false;
        if (currentTask_) {
            currentTask_->setStatusText(_("Fetching filesystem..."));
        }
        megaApi_->fetchNodes();
        while (!fetchNodesFinished_ && !needStop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    root.reset(megaApi_->getRootNode());
    return root != nullptr;
}

void MyListener::onRequestFinish(MegaApi* api, MegaRequest *request, MegaError* e)
{
    switch (request->getType()) {
    case MegaRequest::TYPE_LOGIN:
    {
        engine_->loginSuccess_ = e->getErrorCode() == MegaError::API_OK;
        if (!engine_->loginSuccess_) {
            engine_->log(ErrorInfo::mtError, "Login failed: " + std::string(e->getErrorString()));
        }
        engine_->loginFinished_ = true;
        break;
    }
    case MegaRequest::TYPE_FETCH_NODES:
    {
        if (e->getErrorCode() == MegaError::API_OK) {
            engine_->fetchNodesSuccess_ = true;
        } else {
            engine_->log(ErrorInfo::mtError, "Unable to fetch remote filesystem: " + std::string(e->getErrorString()));
        }
        engine_->fetchNodesFinished_ = true;

        break;
    }
    case MegaRequest::TYPE_EXPORT:
    {
        if (e->getErrorCode() == MegaError::API_OK) {
            MegaHandle handle = request->getNodeHandle();
            std::unique_ptr<MegaNode> node(api->getNodeByHandle(handle));
            char* link = node->getPublicLink();
            if (link) {
                engine_->publicLink_ = link;
                //std::cout << "***** Public link " << link;
                engine_->exportSuccess_ = true;
                delete[] link;
            }
        }
        engine_->exportFinished_ = true;
        break;
    }
    case MegaRequest::TYPE_CREATE_FOLDER:
    {
        if (e->getErrorCode() == MegaError::API_OK) {
            MegaHandle handle = request->getNodeHandle();
            std::unique_ptr<MegaNode> node(api->getNodeByHandle(handle));
            if (node) {
                const char* nodeName = node->getName();
                if (nodeName) {
                    assert(engine_->parentFolder_ && engine_->newFolder_);
                    std::string id = engine_->parentFolder_->getId();
                    if (id.empty() || id.back() != '/') {
                        id += "/";
                    }
                    id += nodeName;
                    engine_->newFolder_->setId(id);
                    engine_->createFolderSuccess_ = true;
                }
            }
        } else {
            engine_->log(ErrorInfo::mtError, "Unable to create folder: " + std::string(e->getErrorString()));
        }
        engine_->newFolder_ = nullptr;
        engine_->parentFolder_ = nullptr;
        engine_->createFolderFinished_ = true;
    }
    break;
    case MegaRequest::TYPE_RENAME:
    {
        if (e->getErrorCode() == MegaError::API_OK) {
            engine_->renameFolderSuccess_ = true;
        } else {
            engine_->log(ErrorInfo::mtError, "Unable to rename folder: " + std::string(e->getErrorString()));
        }
        engine_->renameFolderFinished_ = true;
    }
    break;
    default:
        break;
    }
}

//Currently, this callback is only valid for the request fetchNodes()
void MyListener::onRequestUpdate(MegaApi*api, MegaRequest *request)
{
    //std::cout << "***** Loading filesystem " << request->getTransferredBytes() << " / " << request->getTotalBytes() << std::endl;
}

void MyListener::onRequestTemporaryError(MegaApi *api, MegaRequest *request, MegaError* error)
{
    engine_->log(ErrorInfo::mtWarning, "Mega.nz request error: " + std::string(error->getErrorString()));
}

void MyListener::onTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError* error)
{
    if (error->getErrorCode()) {
        engine_->log(ErrorInfo::mtError, "Transfer finished with error: " + std::string(error->getErrorString()));
    } else {
        MegaHandle handle = transfer->getNodeHandle();
        std::unique_ptr<MegaNode> node(api->getNodeByHandle(handle));
        engine_->exportFinished_ = false;
        engine_->exportSuccess_ = false;
        engine_->uploadSuccess_ = true;
        if (!engine_->needStop()) {
            api->exportNode(node.get(), 0, false, false);
        }
    }

    engine_->uploadFinished_ = true;
}

void MyListener::onTransferUpdate(MegaApi *api, MegaTransfer *transfer)
{
    InfoProgress prInfo;
    prInfo.IsUploading = true;
    prInfo.Total = transfer->getTotalBytes();
    prInfo.Uploaded = transfer->getTransferredBytes();

    if (engine_->fileTask_) {
        CUploader* uploader = engine_->currentUploader();
        if (uploader) {
            uploader->SetStatus(stUploading);
            engine_->fileTask_->uploadProgress(prInfo);
            /*if (uploader->onProgress)
                uploader->onProgress(uploader, prInfo);*/
        }
    }

    if (engine_->needStop()) {
        api->cancelTransferByTag(transfer->getTag());
    }
}

void MyListener::onTransferTemporaryError(MegaApi *api, MegaTransfer *transfer, MegaError* error)
{
    //std::cout << "***** Temporary error in transfer: " << error->getErrorString() << std::endl;
}

void MyListener::onUsersUpdate(MegaApi* api, MegaUserList *users)
{
    if (users == NULL) {
        //Full account reload
        return;
    }
    //std::cout << "***** There are " << users->size() << " new or updated users in your account" << std::endl;
}

void MyListener::onNodesUpdate(MegaApi* api, MegaNodeList *nodes)
{
    if (nodes == NULL) {
        //Full account reload
        return;
    }

    //std::cout << "***** There are " << nodes->size() << " new or updated node/s in your account" << std::endl;
}
