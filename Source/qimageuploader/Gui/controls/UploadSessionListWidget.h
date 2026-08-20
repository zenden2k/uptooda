#ifndef QIMAGEUPLOADER_GUI_CONTROLS_UPLOADSESSIONLISTWIDGET_H
#define QIMAGEUPLOADER_GUI_CONTROLS_UPLOADSESSIONLISTWIDGET_H

#include <QHash>
#include <QScrollArea>
#include <QSet>

#include <memory>

class QVBoxLayout;
class QLabel;
class QProgressBar;
class UploadManager;
class UploadSession;
class UploadTask;

class UploadSessionListWidget : public QScrollArea {
    Q_OBJECT

public:
    explicit UploadSessionListWidget(QWidget* parent = nullptr);
    ~UploadSessionListWidget() override;

    void setUploadManager(UploadManager* uploadManager);
    void detach();
    void refresh();
    void hideSession(UploadSession* session);
    void hideTask(UploadTask* task);
    void selectSession(UploadSession* session);

    std::shared_ptr<UploadSession> selectedSession() const;
    std::shared_ptr<UploadTask> selectedTask() const;

signals:
    void showCodesRequested(UploadSession* session, UploadTask* task);
    void imageViewerRequested(UploadSession* session, UploadTask* task);
    void removeTaskRequested(UploadSession* session, UploadTask* task);
    void contextMenuRequested(UploadSession* session, UploadTask* task, const QPoint& globalPosition);

private:
    struct TaskWidgets {
        QProgressBar* ProgressBar = nullptr;
        QLabel* PercentLabel = nullptr;
        QLabel* TransferredLabel = nullptr;
        QLabel* StatusLabel = nullptr;
    };

    void attachSession(UploadSession* session);
    void queueRefresh();
    void queueTaskUpdate(UploadTask* task);
    void flushTaskUpdates();
    void updateTask(UploadTask* task);
    std::shared_ptr<UploadSession> findSession(UploadSession* session) const;
    std::shared_ptr<UploadTask> findTask(UploadTask* task) const;

    UploadManager* uploadManager_ = nullptr;
    QWidget* contents_ = nullptr;
    QVBoxLayout* contentsLayout_ = nullptr;
    UploadSession* selectedSession_ = nullptr;
    UploadTask* selectedTask_ = nullptr;
    QSet<UploadSession*> hiddenSessions_;
    QSet<UploadTask*> hiddenTasks_;
    QHash<UploadTask*, TaskWidgets> taskWidgets_;
    QSet<UploadTask*> pendingTaskUpdates_;
    bool refreshQueued_ = false;
    bool taskUpdateQueued_ = false;
};

#endif
