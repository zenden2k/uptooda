#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QIcon>
#include <QSet>

#include <string>
#include <vector>

class CUploadEngineData;
class CUploadEngineListBase;

class ServerTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { SERVER, MAX_FILE_SIZE, STORAGE_TIME, ACCOUNT, FILE_FORMATS, COLUMN_COUNT };

    enum Role { ServerNameRole = Qt::UserRole + 1 };

    explicit ServerTableModel(CUploadEngineListBase* engineList, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void refresh();
    void setFilter(const QString& query, int typeMask);
    int rowForServer(const std::string& serverName) const;
    const CUploadEngineData* serverAt(int row) const;
    std::string serverNameAt(int row) const;

private:
    bool matchesFilter(const CUploadEngineData* server) const;
    QString displayText(const CUploadEngineData* server, int column) const;
    void requestIcon(const std::string& serverName) const;

    CUploadEngineListBase* engineList_;
    std::vector<const CUploadEngineData*> servers_;
    std::vector<const CUploadEngineData*> filteredServers_;
    QString query_;
    int typeMask_;
    mutable QHash<QString, QIcon> icons_;
    mutable QSet<QString> pendingIcons_;
    QIcon defaultIcon_;
};
