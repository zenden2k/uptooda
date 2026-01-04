#include "QtServerIconCache.h"

#include <QFile>

#include "Core/Upload/UploadEngine.h"

QtServerIconCache::QtServerIconCache(CUploadEngineListBase *engineList, std::string iconsDir):
    AbstractServerIconCache(engineList, std::move(iconsDir))

{
    defaultServerIcon_ = QIcon(":/res/server.png");
}

NativeIcon QtServerIconCache::getIconForServer(const std::string &name, int dpi, bool smallIcon) {
    return tryIconLoad(name).icon;
}

NativeIcon QtServerIconCache::getBigIconForServer(const std::string &name, int dpi) {
    return tryIconLoad(name).icon;
}

NativeBitmap QtServerIconCache::getIconBitmapForServer(const std::string &name, int dpi, bool smallIcon) {
    return tryIconLoad(name).bm;
}

void QtServerIconCache::preLoadIcons(int dpi) {
    if (iconsPreload_) {
        throw std::logic_error("preLoadIcons() should not be called twice");
    }
    iconsPreload_ = true;

    for (int i = 0; i < engineList_->count(); i++) {
        CUploadEngineData* ued = engineList_->byIndex(i);
        [[maybe_unused]] auto icon = getIconForServer(ued->Name, dpi);
    }
}

QtServerIconCache::CacheItem QtServerIconCache::tryIconLoad(const std::string &name) {
    std::lock_guard lk(cacheMutex_);
    const auto iconIt = serverIcons_.find(name);
    if (iconIt != serverIcons_.end()) {
        return iconIt->second;
    }

    QString iconFileName = QString::fromStdString(getIconNameForServer(name, true));

    if (!QFile::exists(iconFileName)) {
        serverIcons_[name] = {};
        return {};
    }

    QIcon ico;
    if (QFile::exists(iconFileName)) {
        ico = QIcon(iconFileName);

    }
    if (ico.isNull()) {
        ico = defaultServerIcon_;
    }

    CacheItem item(ico);
    serverIcons_[name] = item;
    return item;
}
