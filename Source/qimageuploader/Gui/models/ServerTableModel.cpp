#include "ServerTableModel.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <vector>

#include <QColor>
#include <QFutureWatcher>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QStringList>
#include <QThreadPool>
#include <QtConcurrentRun>

#include "Core/AbstractServerIconCache.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/QtGuiSettings.h"
#include "Core/Upload/UploadEngine.h"
#include "Core/Utils/CoreUtils.h"

namespace {

class ServerIconThreadPool final : public QThreadPool {
public:
    ServerIconThreadPool() { setMaxThreadCount(1); }
};

QThreadPool* IconThreadPool() {
    static ServerIconThreadPool threadPool;
    return &threadPool;
}

QString MaxFileSizeText(const CUploadEngineData* server) {
    std::vector<std::optional<int64_t>> fileSizes(server->userTypes.size());
    for (const auto& formatGroup : server->SupportedFormatGroups) {
        if (!formatGroup.MaxFileSize) {
            continue;
        }

        const auto& userTypeIds = formatGroup.UserTypeIds.empty() ? server->getUserTypesIds() : formatGroup.UserTypeIds;
        for (const auto userTypeId : userTypeIds) {
            if (userTypeId >= fileSizes.size()) {
                continue;
            }
            if (!fileSizes[userTypeId].has_value()
                || formatGroup.MaxFileSize == CUploadEngineData::MAX_FILE_SIZE_UNLIMITED
                || formatGroup.MaxFileSize > *fileSizes[userTypeId]) {
                fileSizes[userTypeId] = formatGroup.MaxFileSize;
            }
        }
    }

    while (!fileSizes.empty() && !fileSizes.back().has_value()) {
        fileSizes.pop_back();
    }

    QStringList values;
    int valueCount = 0;
    for (const auto fileSize : fileSizes) {
        if (!fileSize.has_value()) {
            values.append(QStringLiteral("-"));
            continue;
        }
        values.append(*fileSize == CUploadEngineData::MAX_FILE_SIZE_UNLIMITED
                          ? QString::fromUtf8("\xE2\x88\x9E")
                          : QString::fromStdString(IuCoreUtils::FileSizeToString(*fileSize)));
        ++valueCount;
    }

    if (values.empty() && server->MaxFileSize) {
        values.append(server->MaxFileSize == CUploadEngineData::MAX_FILE_SIZE_UNLIMITED
                          ? QString::fromUtf8("\xE2\x88\x9E")
                          : QString::fromStdString(IuCoreUtils::FileSizeToString(server->MaxFileSize)));
        ++valueCount;
    }
    return valueCount ? values.join(QStringLiteral(" / ")) : QString();
}

QString StorageTimeText(const CUploadEngineData* server) {
    std::vector<std::optional<StorageTime>> storageTimes(server->userTypes.size());
    for (const auto& storageTime : server->StorageTimeInfo) {
        const auto& userTypeIds = storageTime.UserTypeIds.empty() ? server->getUserTypesIds() : storageTime.UserTypeIds;
        for (const auto userTypeId : userTypeIds) {
            if (userTypeId < storageTimes.size()) {
                storageTimes[userTypeId] = storageTime;
            }
        }
    }

    while (!storageTimes.empty() && !storageTimes.back().has_value()) {
        storageTimes.pop_back();
    }

    QStringList values;
    int valueCount = 0;
    for (const auto& storageTime : storageTimes) {
        if (!storageTime.has_value()) {
            values.append(QStringLiteral("-"));
            continue;
        }
        if (!storageTime->Time) {
            values.append(QStringLiteral("?"));
            continue;
        }

        QString value = storageTime->Time == StorageTime::TIME_INFINITE ? QString::fromUtf8("\xE2\x88\x9E")
                                                                        : QString::number(storageTime->Time);
        if (storageTime->AfterLastDownload) {
            value += QString::fromUtf8("\xE2\xA4\x93");
        }
        values.append(value);
        ++valueCount;
    }
    return valueCount ? values.join(QStringLiteral(" / ")) : QString();
}

QString FileFormatsText(const CUploadEngineData* server) {
    const auto extensions = server->getSupportedExtensions();
    QStringList result;
    for (const auto& extension : extensions) {
        result.append(QString::fromStdString(extension));
    }
    return result.join(QStringLiteral(", "));
}

} // namespace

ServerTableModel::ServerTableModel(CUploadEngineListBase* engineList, QObject* parent) :
    QAbstractTableModel(parent), engineList_(engineList), typeMask_(CUploadEngineListBase::ALL_SERVERS) {
    defaultIcon_ = QIcon(QStringLiteral(":/res/server.png"));
    refresh();
}

int ServerTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(filteredServers_.size());
}

int ServerTableModel::columnCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : COLUMN_COUNT; }

QVariant ServerTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return { };
    }

    const CUploadEngineData* server = filteredServers_[index.row()];
    if (role == Qt::DisplayRole) {
        return displayText(server, index.column());
    }
    if (role == ServerNameRole) {
        return QString::fromStdString(server->Name);
    }
    if (role == Qt::DecorationRole && index.column() == SERVER) {
        const QString serverName = QString::fromStdString(server->Name);
        const auto icon = icons_.constFind(serverName);
        if (icon != icons_.cend()) {
            return *icon;
        }
        requestIcon(server->Name);
        return QIcon(QStringLiteral(":/res/server.png"));
    }
    if (role == Qt::ForegroundRole) {
        if (auto* settings = ServiceLocator::instance()->settings<QtGuiSettings>()) {
            if (settings->ServerListSettings.isServerBlacklisted(server->Name)) {
                return QColor(193, 18, 31);
            }
            if (settings->ServerListSettings.isServerFavorite(server->Name)) {
                return QColor(40, 145, 50);
            }
        }
    }
    if (role == Qt::TextAlignmentRole && index.column() != SERVER && index.column() != FILE_FORMATS) {
        return Qt::AlignCenter;
    }
    return { };
}

QVariant ServerTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    switch (section) {
    case SERVER:
        return tr("Server");
    case MAX_FILE_SIZE:
        return tr("Max. file size");
    case STORAGE_TIME:
        return tr("Storage time");
    case ACCOUNT:
        return tr("Account");
    case FILE_FORMATS:
        return tr("File formats");
    default:
        return { };
    }
}

void ServerTableModel::refresh() {
    beginResetModel();
    servers_.clear();
    if (engineList_) {
        for (int i = 0; i < engineList_->count(); ++i) {
            servers_.push_back(engineList_->byIndex(i));
        }
    }

    auto* settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    std::sort(servers_.begin(), servers_.end(), [settings](const auto* left, const auto* right) {
        if (settings) {
            const bool leftFavorite = settings->ServerListSettings.isServerFavorite(left->Name);
            const bool rightFavorite = settings->ServerListSettings.isServerFavorite(right->Name);
            if (leftFavorite != rightFavorite) {
                return leftFavorite;
            }
        }
        return QString::fromStdString(CUploadEngineListBase::getServerDisplayName(left))
                   .compare(QString::fromStdString(CUploadEngineListBase::getServerDisplayName(right)),
                            Qt::CaseInsensitive)
            < 0;
    });

    filteredServers_.clear();
    std::copy_if(servers_.begin(), servers_.end(), std::back_inserter(filteredServers_),
                 [this](const auto* server) { return matchesFilter(server); });
    endResetModel();
}

void ServerTableModel::setFilter(const QString& query, int typeMask) {
    query_ = query.trimmed();
    typeMask_ = typeMask;
    refresh();
}

int ServerTableModel::rowForServer(const std::string& serverName) const {
    const auto it = std::find_if(filteredServers_.begin(), filteredServers_.end(),
                                 [&serverName](const auto* server) { return server->Name == serverName; });
    return it == filteredServers_.end() ? -1 : static_cast<int>(std::distance(filteredServers_.begin(), it));
}

const CUploadEngineData* ServerTableModel::serverAt(int row) const {
    if (row < 0 || row >= rowCount()) {
        return nullptr;
    }
    return filteredServers_[row];
}

std::string ServerTableModel::serverNameAt(int row) const {
    const CUploadEngineData* server = serverAt(row);
    return server ? server->Name : std::string();
}

bool ServerTableModel::matchesFilter(const CUploadEngineData* server) const {
    if (!server || !(server->TypeMask & typeMask_)) {
        return false;
    }
    if (!query_.isEmpty()
        && !QString::fromStdString(CUploadEngineListBase::getServerDisplayName(server))
                .contains(query_, Qt::CaseInsensitive)) {
        return false;
    }

    if (auto* settings = ServiceLocator::instance()->settings<QtGuiSettings>()) {
        if (settings->ServerListSettings.HideBlackListed
            && settings->ServerListSettings.isServerBlacklisted(server->Name)) {
            return false;
        }
        if (settings->ServerListSettings.ShowFavoritesOnly
            && !settings->ServerListSettings.isServerFavorite(server->Name)) {
            return false;
        }
    }
    return true;
}

QString ServerTableModel::displayText(const CUploadEngineData* server, int column) const {
    switch (column) {
    case SERVER:
        return QString::fromStdString(CUploadEngineListBase::getServerDisplayName(server));
    case MAX_FILE_SIZE:
        return MaxFileSizeText(server);
    case STORAGE_TIME:
        return StorageTimeText(server);
    case ACCOUNT:
        if (server->NeedAuthorization == CUploadEngineData::naNotAvailable) {
            return QStringLiteral("-");
        }
        return server->NeedAuthorization == CUploadEngineData::naObligatory ? tr("required") : QStringLiteral("+");
    case FILE_FORMATS:
        return FileFormatsText(server);
    default:
        return { };
    }
}

void ServerTableModel::requestIcon(const std::string& serverName) const {
    const QString key = QString::fromStdString(serverName);
    if (pendingIcons_.contains(key)) {
        return;
    }

    auto* iconCache = ServiceLocator::instance()->serverIconCache();
    if (!iconCache) {
        return;
    }

    pendingIcons_.insert(key);
    auto* model = const_cast<ServerTableModel*>(this);
    auto* watcher = new QFutureWatcher<QImage>(model);
    connect(watcher, &QFutureWatcher<QImage>::finished, model, [this, model, watcher, key, serverName] {
        const QImage image = watcher->result();
        const QIcon icon = image.isNull() ? defaultIcon_ : QIcon(QPixmap::fromImage(image));
        model->icons_.insert(key, icon);
        model->pendingIcons_.remove(key);

        const int row = model->rowForServer(serverName);
        if (row >= 0) {
            const QModelIndex index = model->index(row, SERVER);
            emit model->dataChanged(index, index, { Qt::DecorationRole, Qt::SizeHintRole });
        }
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run(IconThreadPool(), [iconCache, serverName] {
        QImage image;
        image.load(QString::fromStdString(iconCache->getIconNameForServer(serverName, true)));
        return image;
    }));
}
