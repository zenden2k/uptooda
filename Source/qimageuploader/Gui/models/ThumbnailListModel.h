#ifndef QIMAGEUPLOADER_GUI_MODELS_THUMBNAILLISTMODEL_H
#define QIMAGEUPLOADER_GUI_MODELS_THUMBNAILLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QIcon>
#include <QStringList>

class ThumbnailListModel final : public QAbstractListModel {
public:
    enum Roles { FILE_PATH_ROLE = Qt::UserRole + 1 };

    explicit ThumbnailListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = { }) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int addFiles(const QStringList& fileNames);
    int addFile(const QString& fileName, const QString& displayText = { }, const QIcon& icon = { });
    int addGeneratedMosaic(const QString& fileName, const QString& displayText, const QIcon& icon = { });
    void removeItems(const QList<int>& rows);
    void clear();
    QString filePath(int row) const;
    QString displayText(int row) const;
    bool isGeneratedMosaic(int row) const;
    QStringList filePaths() const;

private:
    struct Item {
        QString FilePath;
        QString DisplayText;
        QIcon Icon;
        bool generatedMosaic_ = false;
    };

    int addItem(const QString& fileName, const QString& displayText, const QIcon& icon, bool generatedMosaic);
    QIcon thumbnail(const Item& item) const;

    QList<Item> items_;
    mutable QHash<QString, quint64> pendingThumbnails_;
    mutable quint64 nextThumbnailRequest_ = 0;
};

#endif
