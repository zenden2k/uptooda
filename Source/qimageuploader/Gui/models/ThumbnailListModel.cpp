#include "ThumbnailListModel.h"

#include <QFileInfo>
#include <QPixmap>

#include <algorithm>

#include "Gui/FileThumbnailCache.h"

ThumbnailListModel::ThumbnailListModel(QObject* parent) : QAbstractListModel(parent) { }

int ThumbnailListModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : items_.size(); }

QVariant ThumbnailListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return { };
    }
    const Item& item = items_[index.row()];
    switch (role) {
    case Qt::DisplayRole:
        return item.DisplayText;
    case Qt::DecorationRole:
        return thumbnail(item);
    case Qt::ToolTipRole:
    case FILE_PATH_ROLE:
        return item.FilePath;
    default:
        return { };
    }
}

QHash<int, QByteArray> ThumbnailListModel::roleNames() const {
    auto roles = QAbstractListModel::roleNames();
    roles[FILE_PATH_ROLE] = "filePath";
    return roles;
}

int ThumbnailListModel::addFiles(const QStringList& fileNames) {
    int firstRow = -1;
    for (const QString& fileName : fileNames) {
        const int row = addFile(fileName);
        if (firstRow < 0 && row >= 0) {
            firstRow = row;
        }
    }
    return firstRow;
}

int ThumbnailListModel::addFile(const QString& fileName, const QString& displayText, const QIcon& icon) {
    return addItem(fileName, displayText, icon, false);
}

int ThumbnailListModel::addGeneratedMosaic(const QString& fileName, const QString& displayText, const QIcon& icon) {
    return addItem(fileName, displayText, icon, true);
}

int ThumbnailListModel::addItem(const QString& fileName, const QString& displayText, const QIcon& icon,
                                bool generatedMosaic) {
    const QString absoluteFileName = QFileInfo(fileName).absoluteFilePath();
    if (absoluteFileName.isEmpty()) {
        return -1;
    }
    for (const Item& item : items_) {
        if (item.FilePath == absoluteFileName) {
            return -1;
        }
    }

    const int row = items_.size();
    beginInsertRows({ }, row, row);
    items_.append({ absoluteFileName, displayText.isEmpty() ? QFileInfo(absoluteFileName).fileName() : displayText,
                    icon, generatedMosaic });
    endInsertRows();
    return row;
}

void ThumbnailListModel::removeItems(const QList<int>& rows) {
    QList<int> sortedRows = rows;
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());
    sortedRows.erase(std::unique(sortedRows.begin(), sortedRows.end()), sortedRows.end());
    for (int row : sortedRows) {
        if (row < 0 || row >= items_.size()) {
            continue;
        }
        beginRemoveRows({ }, row, row);
        pendingThumbnails_.remove(items_[row].FilePath);
        items_.removeAt(row);
        endRemoveRows();
    }
}

void ThumbnailListModel::clear() {
    if (items_.isEmpty()) {
        return;
    }
    beginResetModel();
    items_.clear();
    pendingThumbnails_.clear();
    endResetModel();
}

QString ThumbnailListModel::filePath(int row) const {
    return row >= 0 && row < items_.size() ? items_[row].FilePath : QString { };
}

QString ThumbnailListModel::displayText(int row) const {
    return row >= 0 && row < items_.size() ? items_[row].DisplayText : QString { };
}

bool ThumbnailListModel::isGeneratedMosaic(int row) const {
    return row >= 0 && row < items_.size() && items_[row].generatedMosaic_;
}

QStringList ThumbnailListModel::filePaths() const {
    QStringList result;
    result.reserve(items_.size());
    for (const Item& item : items_) {
        result.append(item.FilePath);
    }
    return result;
}

QIcon ThumbnailListModel::thumbnail(const Item& item) const {
    if (!item.Icon.isNull()) {
        return item.Icon;
    }
    FileThumbnailCache& cache = FileThumbnailCache::instance();
    if (const auto cached = cache.cached(item.FilePath)) {
        return cached->Image.isNull() ? QIcon { } : QIcon(QPixmap::fromImage(cached->Image));
    }

    if (!pendingThumbnails_.contains(item.FilePath)) {
        const quint64 request = ++nextThumbnailRequest_;
        pendingThumbnails_.insert(item.FilePath, request);
        auto* model = const_cast<ThumbnailListModel*>(this);
        const QString filePath = item.FilePath;
        cache.request(filePath, model, [model, filePath, request](FileThumbnailCache::Thumbnail) {
            if (model->pendingThumbnails_.value(filePath) != request) {
                return;
            }
            model->pendingThumbnails_.remove(filePath);

            int row = -1;
            for (int i = 0; i < model->items_.size(); ++i) {
                if (model->items_[i].FilePath == filePath) {
                    row = i;
                    break;
                }
            }
            if (row < 0) {
                return;
            }

            const QModelIndex itemIndex = model->index(row, 0);
            emit model->dataChanged(itemIndex, itemIndex, { Qt::DecorationRole });
        });
    }

    return { };
}
