#ifndef QIMAGEUPLOADER_GUI_SERVERFOLDERSELECTDIALOG_H
#define QIMAGEUPLOADER_GUI_SERVERFOLDERSELECTDIALOG_H

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <QDialog>

#include "Core/Upload/FolderItem.h"
#include "Core/Upload/FolderTask.h"
#include "Core/Upload/ServerProfile.h"

class QLabel;
class INetworkClient;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class UploadEngineManager;
class UploadSession;

class ServerFolderSelectDialog : public QDialog {
    Q_OBJECT

public:
    explicit ServerFolderSelectDialog(ServerProfile& serverProfile, UploadEngineManager* uploadEngineManager,
                                      QWidget* parent = nullptr);
    ~ServerFolderSelectDialog() override;

    CFolderItem selectedFolder() const;

private:
    enum class FolderAction { Load, Create, Modify };

    void accept() override;
    void reject() override;
    void loadFolders(const std::string& parentId);
    void runFolderTask(FolderOperationType operation, const CFolderItem& folder, const std::string& refreshParentId);
    void folderTaskFinished(FolderAction action, const std::string& parentId, const CFolderItem& folder,
                            const std::vector<CFolderItem>& folders, bool success);
    void addFolders(const std::string& parentId, const std::vector<CFolderItem>& folders);
    void continueInitialSelection();
    void refreshFolder(const std::string& parentId, const std::string& folderId = { });
    void createFolder(const CFolderItem& parentFolder);
    void editSelectedFolder();
    bool editFolderProperties(CFolderItem& folder, bool createNew);
    void showContextMenu(const QPoint& position);
    void setBusy(bool busy);
    QTreeWidgetItem* itemForId(const std::string& id) const;
    CFolderItem* folderForItem(QTreeWidgetItem* item);

    ServerProfile& serverProfile_;
    UploadEngineManager* uploadEngineManager_;
    QLabel* descriptionLabel_;
    QLabel* statusLabel_;
    QTreeWidget* folderTree_;
    QPushButton* createFolderButton_;
    QPushButton* okButton_;
    std::unique_ptr<INetworkClient> networkClient_;
    std::shared_ptr<UploadSession> currentSession_;
    std::unordered_map<QTreeWidgetItem*, CFolderItem> folders_;
    std::unordered_map<std::string, QTreeWidgetItem*> itemsById_;
    std::unordered_set<std::string> loadedFolderIds_;
    std::vector<std::string> initialPath_;
    std::vector<std::string> accessTypeList_;
    std::string selectAfterRefreshId_;
    CFolderItem selectedFolder_;
    bool busy_ = false;
};

#endif
