#include "WinServerIconCache.h"

#include <ComDef.h>

#include <utility>

#include "Core/UploadEngineList.h"
#include "Gui/IconBitmapUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Func/WinUtils.h"
#include "Gui/Helpers/DPIHelper.h"

WinServerIconCache::WinServerIconCache(CUploadEngineListBase* engineList, std::string iconsDir)
    : AbstractServerIconCache(engineList, std::move(iconsDir))
{
    iconBitmapUtils_ = std::make_unique<IconBitmapUtils>();
    engineList->onServerAdded.connect([this](CUploadEngineListBase*, const std::string& name) {
        onServerAdded(name);
    });
}

WinServerIconCache::~WinServerIconCache(){
    if (future_.valid()) {
        future_.wait();
    }
    for (const auto& it : serverIcons_) {
        DestroyIcon(it.second.icon);
        DeleteObject(it.second.bm);
    }
}

WinServerIconCache::WinIcon WinServerIconCache::tryIconLoad(const std::string& name, int dpi, bool smallIcon) {
    std::lock_guard lk(cacheMutex_);
    const int w = DPIHelper::GetSystemMetricsForDpi(smallIcon ? SM_CXSMICON : SM_CXICON, dpi);
    const int h = DPIHelper::GetSystemMetricsForDpi(smallIcon ? SM_CYSMICON : SM_CYICON, dpi);

    auto key = std::make_pair(w, name);
    const auto iconIt = serverIcons_.find(key);
    if (iconIt != serverIcons_.end()) {
        return iconIt->second;
    }

    HICON icon = nullptr;
    CString iconFileName = IuCoreUtils::Utf8ToWstring(getIconNameForServer(name, true)).c_str();

    /*if (!WinUtils::FileExists(iconFileName)) {
        serverIcons_[name] = {};
        return {};
    }*/

    HRESULT hr = LoadIconWithScaleDown(nullptr, iconFileName, w, h, &icon);

    if (FAILED(hr)) {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            serverIcons_[key] = {};
            return {}; 
        } /*else {
            _com_error err(hr);
            LOG(WARNING) << "LoadIconWithScaleDown" << std::endl << err.ErrorMessage();
        }*/
    }

    if (!icon) {
        icon = static_cast<HICON>(LoadImage(nullptr, iconFileName, IMAGE_ICON, w, h, LR_LOADFROMFILE));
    }

    if (!icon) {
        serverIcons_[key] = {};
        return {};
    }
    WinIcon item(icon, iconBitmapUtils_->HIconToBitmapPARGB32(icon, dpi));
    serverIcons_[key] = item;
    return item;
}

NativeBitmap WinServerIconCache::getIconBitmapForServer(const std::string& name, int dpi, bool smallIcon) {
    return tryIconLoad(name, dpi, smallIcon).bm;
}

NativeIcon WinServerIconCache::getIconForServer(const std::string& name, int dpi, bool smallIcon) {
    return tryIconLoad(name, dpi, smallIcon).icon;
}

NativeIcon WinServerIconCache::getBigIconForServer(const std::string& name, int dpi) {
    CString iconFileName = IuCoreUtils::Utf8ToWstring(getIconNameForServer(name, true)).c_str();

    if (iconFileName.IsEmpty()) {
        return {};
    }
    const int w = DPIHelper::GetSystemMetricsForDpi(SM_CXICON, dpi);
    const int h = DPIHelper::GetSystemMetricsForDpi(SM_CYICON, dpi);
    HICON icon {};
    HRESULT hr = LoadIconWithScaleDown(nullptr, iconFileName, w, h, &icon);

    if (FAILED(hr)) {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            return {};
        } /*else {
            _com_error err(hr);
            LOG(WARNING) << "getBigIconForServer() LoadIconWithScaleDown" << std::endl
                         << err.ErrorMessage();
        }*/
    }

    if (!icon) {
        icon = static_cast<HICON>(LoadImage(nullptr, iconFileName, IMAGE_ICON, w, h, LR_LOADFROMFILE));
    }

    return icon;
}

void WinServerIconCache::loadIcons(int dpi, bool smallIcons) {
    std::unique_ptr<CImageList, ImageListDeleter> imageList(new CImageList, ImageListDeleter {});
    const int iconWidth = DPIHelper::GetSystemMetricsForDpi(smallIcons ? SM_CXSMICON: SM_CXICON, dpi);
    const int iconHeight = DPIHelper::GetSystemMetricsForDpi(smallIcons ? SM_CYSMICON : SM_CYICON, dpi);
    imageList->Create(iconWidth, iconHeight, ILC_COLOR32, 3, 3);
    std::unordered_map<std::string, int> indexes(engineList_->count());
    for (int i = 0; i < engineList_->count(); i++) {
        const CUploadEngineData* ued = engineList_->byIndex(i);
        [[maybe_unused]] auto icon = getIconForServer(ued->Name, dpi, smallIcons);
        [[maybe_unused]] int iconIndex = imageList->AddIcon(icon);
        indexes[ued->Name] = i;
    }

    std::lock_guard lk(imageListsMutex_);
    imageLists_[std::make_pair(dpi, smallIcons)] = { std::move(imageList), std::move(indexes) };
}

void WinServerIconCache::onServerAdded(const std::string& name) {
    std::lock_guard lk(imageListsMutex_);
    for (auto& [k, v] : imageLists_) {
        const auto dpi = k.first;
        bool smallIcons = k.second;
        [[maybe_unused]] auto icon = getIconForServer(name, dpi, smallIcons);
        int iconIndex = v.first->AddIcon(icon);
        v.second[name] = iconIndex;
    }
}

std::optional<WinServerIconCache::ImageListWithIndexes> WinServerIconCache::getCachedImageList(int dpi, bool smallIcons /*= true*/) {
    std::lock_guard lk(cacheMutex_);
    auto it = imageLists_.find({ dpi, smallIcons });
    if (it != imageLists_.end()) {
        /*const std::vector<std::string>& serverNames = it->second.second;
        std::map<int, int> outIndexes;
        for (int i = 0; i < serverNames.size(); i++) {
            int serverIndex = engineList_->getUploadEngineIndex(serverNames[i]);
            outIndexes[serverIndex] = i;
        }*/
        return std::make_pair(it->second.first->m_hImageList, it->second.second);
    }
    return {};
}

void WinServerIconCache::preLoadIcons(int dpi) {
    if (iconsPreload_) {
        throw std::logic_error("preLoadIcons() should not be called twice");
    }
    iconsPreload_ = true;

    future_ = std::async(std::launch::async, [this, dpi]() -> int {
        loadIcons(dpi, true);
        loadIcons(dpi, false);
        return 0;
    });
}

WinServerIconCache::ImageListWithIndexes WinServerIconCache::getImageList(int dpi, bool smallIcons) {
    auto imageList = getCachedImageList(dpi, smallIcons);
    if (imageList) {
        return *imageList;
    }
  
    loadIcons(dpi, smallIcons);
   
    imageList = getCachedImageList(dpi, smallIcons);
    if (imageList) {
        return *imageList;
    }
    
    return {};
}
