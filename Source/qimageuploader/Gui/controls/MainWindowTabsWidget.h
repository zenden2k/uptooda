#ifndef QIMAGEUPLOADER_GUI_CONTROLS_MAINWINDOWTABSWIDGET_H
#define QIMAGEUPLOADER_GUI_CONTROLS_MAINWINDOWTABSWIDGET_H

#include <QWidget>

#include "Core/Upload/ServerProfile.h"

class QButtonGroup;
class QLabel;
class QPushButton;
class QStackedWidget;
class ServerSelectorWidget;
class ThumbnailListModel;
class ThumbnailListView;
class UploadEngineManager;
class UploadSessionListWidget;

class TabSwitcherWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabSwitcherWidget(QWidget* parent = nullptr);

    int currentIndex() const;
    void setCurrentIndex(int index);
    void setAddedFilesCount(int count);

signals:
    void currentChanged(int index);

private:
    QPushButton* createTabButton(const QString& text, int index);

    QButtonGroup* buttonGroup_ = nullptr;
    QPushButton* addedFilesButton_ = nullptr;
    int currentIndex_ = 0;
};

class AddedFilesTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit AddedFilesTabWidget(QWidget* parent = nullptr);

    void setModel(ThumbnailListModel* model);
    void revealRow(int row);

signals:
    void clearRequested();
    void removeRequested(const QList<int>& rows);
    void openRequested(int row);
    void nextRequested();

private:
    void updateState();

    ThumbnailListView* listView_ = nullptr;
    QPushButton* removeButton_ = nullptr;
    QPushButton* clearButton_ = nullptr;
    QPushButton* nextButton_ = nullptr;
    ThumbnailListModel* model_ = nullptr;
};

class UploadSettingsTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit UploadSettingsTabWidget(QWidget* parent = nullptr);

    void configure(UploadEngineManager* uploadEngineManager, const ServerProfile& imageProfile,
                   const ServerProfile& fileProfile);
    void setFileCount(int count);
    ServerProfile imageServerProfile() const;
    ServerProfile fileServerProfile() const;
    void fillServerIcons();

signals:
    void backRequested();
    void uploadRequested();

private:
    QWidget* form_ = nullptr;
    QLabel* fileCountLabel_ = nullptr;
    ServerSelectorWidget* imageServerWidget_ = nullptr;
    ServerSelectorWidget* fileServerWidget_ = nullptr;
    QPushButton* uploadButton_ = nullptr;
};

class UploadsTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit UploadsTabWidget(QWidget* parent = nullptr);

    UploadSessionListWidget* sessionList() const;

private:
    QStackedWidget* stack_ = nullptr;
    UploadSessionListWidget* sessionList_ = nullptr;
};

class EmptyTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit EmptyTabWidget(const QString& title, const QString& description, QWidget* parent = nullptr);
};

class MainWindowTabsWidget : public QWidget {
    Q_OBJECT

public:
    explicit MainWindowTabsWidget(QWidget* parent = nullptr);

    UploadsTabWidget* uploadsTab() const;
    AddedFilesTabWidget* addedFilesTab() const;
    UploadSettingsTabWidget* uploadSettingsTab() const;
    void configureUploadSettings(UploadEngineManager* uploadEngineManager, const ServerProfile& imageProfile,
                                 const ServerProfile& fileProfile);
    void setPendingFilesModel(ThumbnailListModel* model);
    void setPendingFilesCount(int count);
    int currentIndex() const;
    void setCurrentIndex(int index);

signals:
    void currentChanged(int index);

private:
    TabSwitcherWidget* tabSwitcher_ = nullptr;
    QStackedWidget* pageStack_ = nullptr;
    UploadsTabWidget* uploadsTab_ = nullptr;
    AddedFilesTabWidget* addedFilesTab_ = nullptr;
    UploadSettingsTabWidget* uploadSettingsTab_ = nullptr;
};

#endif
