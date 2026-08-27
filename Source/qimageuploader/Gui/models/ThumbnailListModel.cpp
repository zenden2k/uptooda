#include "ThumbnailListModel.h"

#include <QDataStream>
#include <QFileInfo>
#include <QMimeData>
#include <QPixmap>
#include <QUrl>

#include <algorithm>

#include "Gui/FileThumbnailCache.h"

ThumbnailListModel::ThumbnailListModel(QObject* parent) : QAbstractListModel(parent) { }

QString ThumbnailListModel::internalMimeType() {
    return QStringLiteral("application/x-uptooda-thumbnail-rows");
}

int ThumbnailListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : items_.size();
}

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

Qt::ItemFlags ThumbnailListModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags result = QAbstractListModel::flags(index);
    if (index.isValid()) {
        result |= Qt::ItemIsDragEnabled;
    }
    return result | Qt::ItemIsDropEnabled;
}

QStringList ThumbnailListModel::mimeTypes() const { return { internalMimeType(), QStringLiteral("text/uri-list") }; }

QMimeData* ThumbnailListModel::mimeData(const QModelIndexList& indexes) const {
    QList<int> rows;
    for (const QModelIndex& index : indexes) {
        if (index.isValid() && index.column() == 0 && !rows.contains(index.row())) {
            rows.append(index.row());
        }
    }
    std::sort(rows.begin(), rows.end());

    QList<QUrl> urls;
    QByteArray encodedRows;
    QDataStream stream(&encodedRows, QIODevice::WriteOnly);
    for (int row : rows) {
        stream << row;
        urls.append(QUrl::fromLocalFile(items_[row].FilePath));
    }

    auto* result = new QMimeData;
    result->setData(internalMimeType(), encodedRows);
    result->setUrls(urls);
    return result;
}

bool ThumbnailListModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                                      const QModelIndex& parent) {
    if (action == Qt::IgnoreAction) {
        return true;
    }
    if ((action != Qt::CopyAction && action != Qt::MoveAction) || !data || !data->hasFormat(internalMimeType())
        || column > 0) {
        return false;
    }

    QList<int> rows;
    QByteArray encodedRows = data->data(internalMimeType());
    QDataStream stream(&encodedRows, QIODevice::ReadOnly);
    while (!stream.atEnd()) {
        int sourceRow = -1;
        stream >> sourceRow;
        if (sourceRow >= 0 && sourceRow < items_.size() && !rows.contains(sourceRow)) {
            rows.append(sourceRow);
        }
    }
    if (rows.isEmpty()) {
        return false;
    }
    std::sort(rows.begin(), rows.end());

    int destinationRow = row;
    if (destinationRow < 0) {
        destinationRow = parent.isValid() ? parent.row() : items_.size();
    }
    destinationRow = std::clamp(destinationRow, 0, static_cast<int>(items_.size()));

    QList<Item> movedItems;
    movedItems.reserve(rows.size());
    for (int sourceRow : rows) {
        movedItems.append(items_[sourceRow]);
        if (sourceRow < destinationRow) {
            --destinationRow;
        }
    }

    beginResetModel();
    for (auto it = rows.crbegin(); it != rows.crend(); ++it) {
        items_.removeAt(*it);
    }
    for (int i = 0; i < movedItems.size(); ++i) {
        items_.insert(destinationRow + i, movedItems[i]);
    }
    endResetModel();
    return true;
}

Qt::DropActions ThumbnailListModel::supportedDragActions() const {
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions ThumbnailListModel::supportedDropActions() const {
    return Qt::CopyAction | Qt::MoveAction;
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
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater());
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
