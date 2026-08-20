#include "MainWindowTabsWidget.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Gui/controls/ServerSelectorWidget.h"
#include "Gui/controls/ThumbnailListView.h"
#include "Gui/models/ThumbnailListModel.h"
#include "UploadSessionListWidget.h"

TabSwitcherWidget::TabSwitcherWidget(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("tabSwitcher"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(52);

    buttonGroup_ = new QButtonGroup(this);
    buttonGroup_->setExclusive(true);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(22, 6, 22, 0);
    layout->setSpacing(8);
    layout->addWidget(createTabButton(tr("Uploads"), 0));
    layout->addWidget(createTabButton(tr("Screenshots"), 1));
    layout->addWidget(createTabButton(tr("Screen recordings"), 2));
    layout->addStretch(1);
    addedFilesButton_ = createTabButton(tr("Added files (0)"), 3);
    layout->addWidget(addedFilesButton_);
    layout->addWidget(createTabButton(tr("Upload settings"), 4));

    setStyleSheet(
        QStringLiteral("QPushButton { background: transparent; border: 0; border-bottom: 3px solid transparent;"
                       " border-radius: 0;"
                       " color: #5b697b; padding: 0 18px; }"
                       "QPushButton:hover { background: #edf6fc; }"
                       "QPushButton:checked { color: #2789c9; border-bottom-color: #55afe8; }"));

    buttonGroup_->button(0)->setChecked(true);
    connect(buttonGroup_, &QButtonGroup::idClicked, this, &TabSwitcherWidget::setCurrentIndex);
}

int TabSwitcherWidget::currentIndex() const { return currentIndex_; }

void TabSwitcherWidget::setCurrentIndex(int index) {
    QPushButton* button = qobject_cast<QPushButton*>(buttonGroup_->button(index));
    if (!button || index == currentIndex_) {
        return;
    }
    currentIndex_ = index;
    button->setChecked(true);
    emit currentChanged(index);
}

void TabSwitcherWidget::setAddedFilesCount(int count) { addedFilesButton_->setText(tr("Added files (%1)").arg(count)); }

QPushButton* TabSwitcherWidget::createTabButton(const QString& text, int index) {
    auto* button = new QPushButton(text, this);
    button->setCheckable(true);
    button->setFlat(true);
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    buttonGroup_->addButton(button, index);
    return button;
}

UploadsTabWidget::UploadsTabWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    sessionList_ = new UploadSessionListWidget(this);
    layout->addWidget(sessionList_);
}

UploadSessionListWidget* UploadsTabWidget::sessionList() const { return sessionList_; }

AddedFilesTabWidget::AddedFilesTabWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    listView_ = new ThumbnailListView(this);
    listView_->setEmptyText(tr("The added files list is empty"));
    layout->addWidget(listView_, 1);

    auto* buttonLayout = new QHBoxLayout;
    clearButton_ = new QPushButton(tr("Clear list"), this);
    removeButton_ = new QPushButton(tr("Remove selected"), this);
    nextButton_ = new QPushButton(tr("Next >"), this);
    buttonLayout->addWidget(clearButton_);
    buttonLayout->addWidget(removeButton_);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(nextButton_);
    layout->addLayout(buttonLayout);

    connect(clearButton_, &QPushButton::clicked, this, &AddedFilesTabWidget::clearRequested);
    connect(removeButton_, &QPushButton::clicked, this, [this] { emit removeRequested(listView_->selectedRows()); });
    connect(nextButton_, &QPushButton::clicked, this, &AddedFilesTabWidget::nextRequested);
    connect(listView_, &ThumbnailListView::removeRequested, this, &AddedFilesTabWidget::removeRequested);
    connect(listView_, &ThumbnailListView::openRequested, this, &AddedFilesTabWidget::openRequested);
}

void AddedFilesTabWidget::setModel(ThumbnailListModel* model) {
    if (model_ == model) {
        return;
    }
    model_ = model;
    listView_->setModel(model_);
    if (model_) {
        connect(model_, &QAbstractItemModel::rowsInserted, this, &AddedFilesTabWidget::updateState);
        connect(model_, &QAbstractItemModel::rowsRemoved, this, &AddedFilesTabWidget::updateState);
        connect(model_, &QAbstractItemModel::modelReset, this, &AddedFilesTabWidget::updateState);
        connect(listView_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                &AddedFilesTabWidget::updateState);
    }
    updateState();
}

void AddedFilesTabWidget::revealRow(int row) {
    if (!model_ || row < 0 || row >= model_->rowCount()) {
        return;
    }
    listView_->revealRow(row);
}

void AddedFilesTabWidget::updateState() {
    const bool hasFiles = model_ && model_->rowCount() > 0;
    clearButton_->setEnabled(hasFiles);
    nextButton_->setEnabled(hasFiles);
    removeButton_->setEnabled(hasFiles && listView_->selectionModel() && listView_->selectionModel()->hasSelection());
}

UploadSettingsTabWidget::UploadSettingsTabWidget(QWidget* parent) : QWidget(parent) {
    auto* outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(24, 28, 24, 24);
    outerLayout->addStretch(1);
    form_ = new QWidget(this);
    form_->setMaximumWidth(960);
    auto* formLayout = new QVBoxLayout(form_);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(14);

    auto* titleLabel = new QLabel(tr("Upload settings"), form_);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QStringLiteral("color: #263548;"));
    formLayout->addWidget(titleLabel);

    fileCountLabel_ = new QLabel(form_);
    fileCountLabel_->setStyleSheet(QStringLiteral("color: #6a7889;"));
    formLayout->addWidget(fileCountLabel_);

    auto* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    auto* backButton = new QPushButton(tr("< Back"), form_);
    uploadButton_ = new QPushButton(tr("Upload"), form_);
    buttonLayout->addWidget(backButton);
    buttonLayout->addWidget(uploadButton_);
    formLayout->addLayout(buttonLayout);
    formLayout->addStretch(1);
    outerLayout->addWidget(form_, 6);
    outerLayout->addStretch(1);

    connect(backButton, &QPushButton::clicked, this, &UploadSettingsTabWidget::backRequested);
    connect(uploadButton_, &QPushButton::clicked, this, &UploadSettingsTabWidget::uploadRequested);
    setFileCount(0);
}

void UploadSettingsTabWidget::configure(UploadEngineManager* uploadEngineManager, const ServerProfile& imageProfile,
                                        const ServerProfile& fileProfile) {
    if (imageServerWidget_ || !uploadEngineManager) {
        return;
    }
    auto* formLayout = qobject_cast<QVBoxLayout*>(form_->layout());
    imageServerWidget_ = new ServerSelectorWidget(uploadEngineManager, false, form_);
    imageServerWidget_->setTitle(tr("Image server"));
    imageServerWidget_->setServersMask(ServerSelectorWidget::smImageServers);
    imageServerWidget_->updateServerList();
    imageServerWidget_->setServerProfile(imageProfile);
    formLayout->insertWidget(2, imageServerWidget_);

    fileServerWidget_ = new ServerSelectorWidget(uploadEngineManager, false, form_);
    fileServerWidget_->setTitle(tr("Server for other files"));
    fileServerWidget_->setServersMask(ServerSelectorWidget::smFileServers);
    fileServerWidget_->updateServerList();
    fileServerWidget_->setServerProfile(fileProfile);
    formLayout->insertWidget(3, fileServerWidget_);
}

void UploadSettingsTabWidget::setFileCount(int count) {
    fileCountLabel_->setText(tr("Selected files: %1").arg(count));
    uploadButton_->setEnabled(count > 0);
}

ServerProfile UploadSettingsTabWidget::imageServerProfile() const {
    return imageServerWidget_ ? imageServerWidget_->serverProfile() : ServerProfile();
}

ServerProfile UploadSettingsTabWidget::fileServerProfile() const {
    return fileServerWidget_ ? fileServerWidget_->serverProfile() : ServerProfile();
}

void UploadSettingsTabWidget::fillServerIcons() {
    if (imageServerWidget_) {
        imageServerWidget_->fillServerIcons();
    }
    if (fileServerWidget_) {
        fileServerWidget_->fillServerIcons();
    }
}

EmptyTabWidget::EmptyTabWidget(const QString& title, const QString& description, QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->addStretch(1);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 3);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QStringLiteral("color: #526273;"));
    layout->addWidget(titleLabel);

    auto* descriptionLabel = new QLabel(description, this);
    descriptionLabel->setAlignment(Qt::AlignCenter);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet(QStringLiteral("color: #8996a6;"));
    layout->addWidget(descriptionLabel);
    layout->addStretch(1);
    setStyleSheet(QStringLiteral("background: #f3f7fa;"));
}

MainWindowTabsWidget::MainWindowTabsWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabSwitcher_ = new TabSwitcherWidget(this);
    layout->addWidget(tabSwitcher_);

    pageStack_ = new QStackedWidget(this);
    uploadsTab_ = new UploadsTabWidget(pageStack_);
    pageStack_->addWidget(uploadsTab_);
    pageStack_->addWidget(
        new EmptyTabWidget(tr("Screenshots"), tr("This section will display screenshot history."), pageStack_));
    pageStack_->addWidget(
        new EmptyTabWidget(tr("Screen recordings"), tr("This section will display screen recordings."), pageStack_));
    addedFilesTab_ = new AddedFilesTabWidget(pageStack_);
    pageStack_->addWidget(addedFilesTab_);
    uploadSettingsTab_ = new UploadSettingsTabWidget(pageStack_);
    pageStack_->addWidget(uploadSettingsTab_);
    layout->addWidget(pageStack_, 1);

    connect(tabSwitcher_, &TabSwitcherWidget::currentChanged, pageStack_, &QStackedWidget::setCurrentIndex);
    connect(tabSwitcher_, &TabSwitcherWidget::currentChanged, this, &MainWindowTabsWidget::currentChanged);
}

UploadsTabWidget* MainWindowTabsWidget::uploadsTab() const { return uploadsTab_; }

AddedFilesTabWidget* MainWindowTabsWidget::addedFilesTab() const { return addedFilesTab_; }

UploadSettingsTabWidget* MainWindowTabsWidget::uploadSettingsTab() const { return uploadSettingsTab_; }

void MainWindowTabsWidget::configureUploadSettings(UploadEngineManager* uploadEngineManager,
                                                   const ServerProfile& imageProfile,
                                                   const ServerProfile& fileProfile) {
    uploadSettingsTab_->configure(uploadEngineManager, imageProfile, fileProfile);
}

void MainWindowTabsWidget::setPendingFilesModel(ThumbnailListModel* model) { addedFilesTab_->setModel(model); }

void MainWindowTabsWidget::setPendingFilesCount(int count) {
    tabSwitcher_->setAddedFilesCount(count);
    uploadSettingsTab_->setFileCount(count);
}

int MainWindowTabsWidget::currentIndex() const { return tabSwitcher_->currentIndex(); }

void MainWindowTabsWidget::setCurrentIndex(int index) { tabSwitcher_->setCurrentIndex(index); }
