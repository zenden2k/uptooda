#include "ServerFolderSelectDialog.h"

#include <algorithm>
#include <functional>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QStyle>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "Core/CommonDefs.h"
#include "Core/Network/NetworkClientFactory.h"
#include "Core/ServiceLocator.h"
#include "Core/Upload/AdvancedUploadEngine.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Core/Upload/UploadManager.h"
#include "Core/Upload/UploadSession.h"

namespace {
constexpr int IS_PLACEHOLDER_ROLE = Qt::UserRole;

void AddPlaceholder(QTreeWidgetItem* item) {
    auto* placeholder = new QTreeWidgetItem(item, { QObject::tr("Loading...") });
    placeholder->setData(0, IS_PLACEHOLDER_ROLE, true);
}
}

ServerFolderSelectDialog::ServerFolderSelectDialog(ServerProfile& serverProfile,
                                                   UploadEngineManager* uploadEngineManager, QWidget* parent) :
    QDialog(parent), serverProfile_(serverProfile), uploadEngineManager_(uploadEngineManager) {
    setWindowTitle(tr("Folder list"));
    resize(560, 460);

    auto* layout = new QVBoxLayout(this);
    descriptionLabel_ = new QLabel(this);
    descriptionLabel_->setWordWrap(true);
    descriptionLabel_->setText(tr("Folder list on server %1 for account '%2':")
                                   .arg(U2Q(serverProfile_.serverName()), U2Q(serverProfile_.profileName())));
    layout->addWidget(descriptionLabel_);

    folderTree_ = new QTreeWidget(this);
    folderTree_->setObjectName(QStringLiteral("serverFolderTree"));
    folderTree_->setHeaderHidden(true);
    folderTree_->setAlternatingRowColors(true);
    folderTree_->setAnimated(true);
    folderTree_->setIndentation(22);
    folderTree_->setIconSize(QSize(20, 20));
    folderTree_->setSelectionBehavior(QAbstractItemView::SelectRows);
    folderTree_->setUniformRowHeights(true);
    folderTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(folderTree_, 1);

    statusLabel_ = new QLabel(this);
    statusLabel_->setVisible(false);
    layout->addWidget(statusLabel_);

    auto* buttonLayout = new QHBoxLayout;
    createFolderButton_ = new QPushButton(tr("Create folder"), this);
    buttonLayout->addWidget(createFolderButton_);
    buttonLayout->addStretch(1);
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    okButton_ = buttonBox->button(QDialogButtonBox::Ok);
    buttonLayout->addWidget(buttonBox);
    layout->addLayout(buttonLayout);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ServerFolderSelectDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ServerFolderSelectDialog::reject);
    connect(createFolderButton_, &QPushButton::clicked, this, [this] { createFolder({ }); });
    connect(folderTree_, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem* item) {
        CFolderItem* folder = folderForItem(item);
        if (folder && !loadedFolderIds_.count(folder->id)) {
            loadFolders(folder->id);
        }
    });
    connect(folderTree_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem*, int) { accept(); });
    connect(folderTree_, &QTreeWidget::customContextMenuRequested, this, &ServerFolderSelectDialog::showContextMenu);
    connect(folderTree_, &QTreeWidget::itemSelectionChanged, this,
            [this] { okButton_->setEnabled(!busy_ && folderForItem(folderTree_->currentItem())); });

    initialPath_ = serverProfile_.parentIds();
    initialPath_.erase(std::remove(initialPath_.begin(), initialPath_.end(), std::string()), initialPath_.end());
    if (!serverProfile_.folderId().empty()) {
        initialPath_.push_back(serverProfile_.folderId());
    }

    auto uploadEngine
        = std::dynamic_pointer_cast<CAdvancedUploadEngine>(uploadEngineManager_->getUploadEngine(serverProfile_));
    if (uploadEngine) {
        NetworkClientFactory factory;
        networkClient_ = factory.create();
        uploadEngine->setNetworkClient(networkClient_.get());
        uploadEngine->getAccessTypeList(accessTypeList_);
    }

    loadFolders({ });
}

ServerFolderSelectDialog::~ServerFolderSelectDialog() {
    if (busy_ && currentSession_) {
        if (auto* uploadManager = ServiceLocator::instance()->uploadManager()) {
            uploadManager->stopSession(currentSession_.get());
        }
    }
}

CFolderItem ServerFolderSelectDialog::selectedFolder() const { return selectedFolder_; }

void ServerFolderSelectDialog::accept() {
    if (busy_) {
        return;
    }
    QTreeWidgetItem* item = folderTree_->currentItem();
    CFolderItem* folder = folderForItem(item);
    if (!folder) {
        return;
    }

    selectedFolder_ = *folder;
    selectedFolder_.parentIds.clear();
    for (QTreeWidgetItem* parent = item->parent(); parent; parent = parent->parent()) {
        if (CFolderItem* parentFolder = folderForItem(parent)) {
            selectedFolder_.parentIds.push_back(parentFolder->id);
        }
    }
    std::reverse(selectedFolder_.parentIds.begin(), selectedFolder_.parentIds.end());
    QDialog::accept();
}

void ServerFolderSelectDialog::reject() {
    if (busy_ && currentSession_) {
        if (auto* uploadManager = ServiceLocator::instance()->uploadManager()) {
            uploadManager->stopSession(currentSession_.get());
        }
    }
    QDialog::reject();
}

void ServerFolderSelectDialog::loadFolders(const std::string& parentId) {
    CFolderItem parentFolder;
    parentFolder.setId(parentId);
    runFolderTask(FolderOperationType::foGetFolders, parentFolder, parentId);
}

void ServerFolderSelectDialog::runFolderTask(FolderOperationType operation, const CFolderItem& folder,
                                             const std::string& refreshParentId) {
    if (busy_) {
        return;
    }
    auto* uploadManager = ServiceLocator::instance()->uploadManager();
    if (!uploadManager) {
        statusLabel_->setText(tr("Upload manager is not available."));
        statusLabel_->setVisible(true);
        return;
    }

    auto task = std::make_shared<FolderTask>(operation);
    task->setServerProfile(serverProfile_);
    if (operation == FolderOperationType::foGetFolders) {
        task->folderList().setParentFolder(folder);
    } else {
        task->setFolder(folder);
    }

    const FolderAction action = operation == FolderOperationType::foGetFolders
        ? FolderAction::Load
        : (operation == FolderOperationType::foCreateFolder ? FolderAction::Create : FolderAction::Modify);
    QPointer<ServerFolderSelectDialog> guard(this);
    task->addTaskFinishedCallback([guard, action, refreshParentId](UploadTask* uploadTask, bool success) {
        auto* folderTask = dynamic_cast<FolderTask*>(uploadTask);
        if (!folderTask) {
            return;
        }
        const CFolderItem resultFolder = folderTask->folder();
        const std::vector<CFolderItem> resultFolders = folderTask->folderList().m_folderItems;
        QMetaObject::invokeMethod(
            qApp,
            [guard, action, refreshParentId, resultFolder, resultFolders, success] {
                if (guard) {
                    guard->folderTaskFinished(action, refreshParentId, resultFolder, resultFolders, success);
                }
            },
            Qt::QueuedConnection);
    });

    currentSession_ = std::make_shared<UploadSession>();
    currentSession_->setService(true);
    currentSession_->addTask(task);
    setBusy(true);
    uploadManager->addSession(currentSession_);
}

void ServerFolderSelectDialog::folderTaskFinished(FolderAction action, const std::string& parentId,
                                                  const CFolderItem& folder, const std::vector<CFolderItem>& folders,
                                                  bool success) {
    setBusy(false);
    if (!success) {
        statusLabel_->setText(tr("Failed to perform the folder operation."));
        statusLabel_->setVisible(true);
        return;
    }
    statusLabel_->setVisible(false);

    if (action == FolderAction::Load) {
        addFolders(parentId, folders);
        continueInitialSelection();
        if (!selectAfterRefreshId_.empty()) {
            if (QTreeWidgetItem* item = itemForId(selectAfterRefreshId_)) {
                folderTree_->setCurrentItem(item);
                selectAfterRefreshId_.clear();
            }
        }
        return;
    }

    refreshFolder(parentId, folder.id);
}

void ServerFolderSelectDialog::addFolders(const std::string& parentId, const std::vector<CFolderItem>& folders) {
    QTreeWidgetItem* parentItem = itemForId(parentId);
    if (parentItem) {
        while (parentItem->childCount()) {
            QTreeWidgetItem* child = parentItem->takeChild(0);
            std::vector<QTreeWidgetItem*> itemsToRemove { child };
            while (!itemsToRemove.empty()) {
                QTreeWidgetItem* current = itemsToRemove.back();
                itemsToRemove.pop_back();
                for (int index = 0; index < current->childCount(); ++index) {
                    itemsToRemove.push_back(current->child(index));
                }
                folders_.erase(current);
                itemsById_.erase(Q2U(current->data(0, Qt::UserRole + 1).toString()));
            }
            delete child;
        }
    } else if (parentId.empty()) {
        folderTree_->clear();
        folders_.clear();
        itemsById_.clear();
    }

    std::function<void(const std::string&, QTreeWidgetItem*)> addChildren = [&](const std::string& currentParentId,
                                                                                QTreeWidgetItem* currentParentItem) {
        bool hasReturnedChildren = false;
        for (const CFolderItem& folder : folders) {
            if (folder.parentId != currentParentId) {
                continue;
            }
            hasReturnedChildren = true;
            auto* item = currentParentItem ? new QTreeWidgetItem(currentParentItem) : new QTreeWidgetItem(folderTree_);
            item->setText(0, U2Q(folder.title));
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
            item->setData(0, Qt::UserRole + 1, U2Q(folder.id));
            folders_[item] = folder;
            itemsById_[folder.id] = item;

            const bool hasNestedFolders = std::any_of(folders.begin(), folders.end(),
                                                      [&](const auto& child) { return child.parentId == folder.id; });
            if (hasNestedFolders) {
                addChildren(folder.id, item);
            } else if (folder.itemCount == CFolderItem::icUnknown || folder.itemCount > 0) {
                AddPlaceholder(item);
            }
        }
        if (hasReturnedChildren) {
            loadedFolderIds_.insert(currentParentId);
        }
    };
    addChildren(parentId, parentItem);
    if (parentItem) {
        parentItem->sortChildren(0, Qt::AscendingOrder);
    } else {
        folderTree_->sortItems(0, Qt::AscendingOrder);
    }
    loadedFolderIds_.insert(parentId);
}

void ServerFolderSelectDialog::continueInitialSelection() {
    while (!initialPath_.empty()) {
        const std::string nextId = initialPath_.front();
        QTreeWidgetItem* item = itemForId(nextId);
        if (!item) {
            initialPath_.clear();
            return;
        }
        initialPath_.erase(initialPath_.begin());
        folderTree_->setCurrentItem(item);
        folderTree_->scrollToItem(item);
        if (!initialPath_.empty()) {
            item->setExpanded(true);
            if (!loadedFolderIds_.count(nextId)) {
                if (!busy_) {
                    loadFolders(nextId);
                }
                return;
            }
        }
    }
}

void ServerFolderSelectDialog::refreshFolder(const std::string& parentId, const std::string& folderId) {
    selectAfterRefreshId_ = folderId;
    loadedFolderIds_.erase(parentId);
    if (QTreeWidgetItem* parentItem = itemForId(parentId)) {
        parentItem->setExpanded(true);
    }
    loadFolders(parentId);
}

void ServerFolderSelectDialog::createFolder(const CFolderItem& parentFolder) {
    CFolderItem folder;
    if (!editFolderProperties(folder, true)) {
        return;
    }
    folder.parentId = parentFolder.id;
    runFolderTask(FolderOperationType::foCreateFolder, folder, parentFolder.id);
}

void ServerFolderSelectDialog::editSelectedFolder() {
    CFolderItem* selected = folderForItem(folderTree_->currentItem());
    if (!selected) {
        return;
    }
    CFolderItem folder = *selected;
    if (editFolderProperties(folder, false)) {
        runFolderTask(FolderOperationType::foModifyFolder, folder, folder.parentId);
    }
}

bool ServerFolderSelectDialog::editFolderProperties(CFolderItem& folder, bool createNew) {
    QDialog dialog(this);
    dialog.setWindowTitle(createNew ? tr("Create folder (album)") : tr("Edit folder"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* formLayout = new QFormLayout;
    auto* titleEdit = new QLineEdit(U2Q(folder.title), &dialog);
    auto* summaryEdit = new QPlainTextEdit(U2Q(folder.summary), &dialog);
    auto* accessCombo = new QComboBox(&dialog);
    for (const std::string& accessType : accessTypeList_) {
        accessCombo->addItem(U2Q(accessType));
    }
    accessCombo->setCurrentIndex(folder.accessType);
    formLayout->addRow(tr("Folder name:"), titleEdit);
    formLayout->addRow(tr("Summary:"), summaryEdit);
    if (!accessTypeList_.empty()) {
        formLayout->addRow(tr("Access:"), accessCombo);
    }
    layout->addLayout(formLayout);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    folder.title = Q2U(titleEdit->text());
    folder.summary = Q2U(summaryEdit->toPlainText());
    if (accessCombo->currentIndex() >= 0) {
        folder.accessType = accessCombo->currentIndex();
    }
    return !folder.title.empty();
}

void ServerFolderSelectDialog::showContextMenu(const QPoint& position) {
    QTreeWidgetItem* item = folderTree_->itemAt(position);
    CFolderItem* folder = folderForItem(item);
    if (!folder || busy_) {
        return;
    }
    folderTree_->setCurrentItem(item);
    QMenu menu(this);
    QAction* openAction = nullptr;
    if (!folder->viewUrl.empty()) {
        openAction = menu.addAction(tr("Open in Web Browser"));
    }
    QAction* editAction = menu.addAction(tr("Edit"));
    QAction* createAction = menu.addAction(tr("Create nested folder"));
    QAction* copyIdAction = menu.addAction(tr("Copy folder's ID"));
    copyIdAction->setEnabled(!folder->id.empty() && folder->id != CFolderItem::NewFolderMark);
    QAction* selectedAction = menu.exec(folderTree_->viewport()->mapToGlobal(position));
    if (selectedAction == openAction) {
        QDesktopServices::openUrl(QUrl(U2Q(folder->viewUrl)));
    } else if (selectedAction == editAction) {
        editSelectedFolder();
    } else if (selectedAction == createAction) {
        createFolder(*folder);
    } else if (selectedAction == copyIdAction) {
        QApplication::clipboard()->setText(U2Q(folder->id));
    }
}

void ServerFolderSelectDialog::setBusy(bool busy) {
    busy_ = busy;
    folderTree_->setEnabled(!busy);
    createFolderButton_->setEnabled(!busy);
    okButton_->setEnabled(!busy && folderForItem(folderTree_->currentItem()));
    if (busy) {
        statusLabel_->setText(tr("Loading..."));
        statusLabel_->setVisible(true);
    }
}

QTreeWidgetItem* ServerFolderSelectDialog::itemForId(const std::string& id) const {
    auto it = itemsById_.find(id);
    return it == itemsById_.end() ? nullptr : it->second;
}

CFolderItem* ServerFolderSelectDialog::folderForItem(QTreeWidgetItem* item) {
    if (!item || item->data(0, IS_PLACEHOLDER_ROLE).toBool()) {
        return nullptr;
    }
    auto it = folders_.find(item);
    return it == folders_.end() ? nullptr : &it->second;
}
