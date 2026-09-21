#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <future>
#include <vector>
#include <optional>

#include "AbstractServerIconCache.h"
#include "Core/Utils/CoreUtils.h"

class IconBitmapUtils;


/*struct PairHash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1> {}(p.first);
        auto hash2 = std::hash<T2> {}(p.second);
        return hash1 ^ (hash2 << 1); // Combine hashes
    }
};*/

class ImageListDeleter {
public:
    void operator()(CImageList* ptr) const {
        ptr->Destroy();
        delete ptr;
    }
};

class WinServerIconCache : public AbstractServerIconCache {
public:
    struct WinIcon {
        HICON icon {};
        HBITMAP bm {};
        WinIcon() = default;
        WinIcon(HICON i, HBITMAP b = NULL)
            : icon(i)
            , bm(b)
        {
        }
    };

    WinServerIconCache(CUploadEngineListBase* engineList, std::string iconsDir);
    ~WinServerIconCache() override;

    NativeIcon getIconForServer(const std::string& name, unsigned int dpi, bool smallIcon) override;

    /**
    * The caller of this function is responsible for destroying
    * the icon when it is no longer needed.
    */
    [[nodiscard]] NativeIcon getBigIconForServer(const std::string& name, unsigned int dpi) override;

    NativeBitmap getIconBitmapForServer(const std::string& name, unsigned int dpi, bool smallIcon) override;

    /**
    * @throws std::logic_error 
    */
    void preLoadIcons(unsigned int dpi) override;

    using ImageListWithIndexes = std::pair<HIMAGELIST, std::unordered_map<std::string,int>>;
    ImageListWithIndexes getImageList(unsigned int dpi, bool smallIcons = true);

private :
    std::unordered_map<std::pair<int, std::string>, WinIcon> serverIcons_;
    std::unique_ptr<IconBitmapUtils> iconBitmapUtils_;
    std::mutex cacheMutex_;
    std::future<int> future_;
    bool iconsPreload_ = false;
    std::mutex imageListsMutex_;
    std::unordered_map<std::pair<unsigned int,bool>, std::pair<std::unique_ptr<CImageList, ImageListDeleter>, std::unordered_map<std::string, int>>> imageLists_;
    WinIcon tryIconLoad(const std::string& name, unsigned int dpi, bool smallIcon = true);
    void loadIcons(unsigned int dpi, bool smallIcons);
    void onServerAdded(const std::string& name);
    std::optional<ImageListWithIndexes> getCachedImageList(unsigned int dpi, bool smallIcons = true);
};
