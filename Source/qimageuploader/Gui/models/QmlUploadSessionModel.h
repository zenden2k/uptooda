#ifndef QIMAGEUPLOADER_GUI_MODELS_QMLUPLOADSESSIONMODEL_H
#define QIMAGEUPLOADER_GUI_MODELS_QMLUPLOADSESSIONMODEL_H

#include <QAbstractListModel>
#include <QSet>

#include <memory>

class UploadManager;
class UploadSession;
class UploadTask;

class QmlUploadSessionModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { SessionIdRole = Qt::UserRole + 1, FileCountRole, ServerNamesRole, ServerIconRole, TasksRole };

    explicit QmlUploadSessionModel(UploadManager* uploadManager, QObject* parent = nullptr);
    ~QmlUploadSessionModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool moveTask(const QString& sessionId, int fromIndex, int toIndex);
    void detach();
    std::shared_ptr<UploadSession> findSession(const QString& sessionId) const;
    std::shared_ptr<UploadTask> findTask(const QString& sessionId, const QString& taskId) const;
    void hideSession(const QString& sessionId);
    void hideTask(const QString& taskId);
    void refresh();

private:
    static QString objectId(const void* object);
    QVariantList taskList(const std::shared_ptr<UploadSession>& session) const;
    QVariantMap taskData(const std::shared_ptr<UploadTask>& task) const;
    QString iconForServer(const std::string& serverName) const;
    void attachSession(UploadSession* session);
    void queueRefresh();
    void resetModel();

    UploadManager* UploadManager_;
    QSet<QString> HiddenSessions_;
    QSet<QString> HiddenTasks_;
    QHash<QString, QStringList> TaskOrders_;
};

#endif
