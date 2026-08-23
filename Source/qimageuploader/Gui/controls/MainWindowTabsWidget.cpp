#include "MainWindowTabsWidget.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "AddedFilesTabWidget.h"
#include "Gui/controls/ServerSelectorWidget.h"
#include "Gui/controls/ThumbnailListView.h"
#include "Gui/models/ThumbnailListModel.h"
#include "UploadSessionListWidget.h"
#include "UploadSettingsTabWidget.h"
#include "UploadsTabWidget.h"

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

int TabSwitcherWidget::currentIndex() const {
    return currentIndex_;
}

void TabSwitcherWidget::setCurrentIndex(int index) {
    QPushButton* button = qobject_cast<QPushButton*>(buttonGroup_->button(index));
    if (!button || index == currentIndex_) {
        return;
    }
    currentIndex_ = index;
    button->setChecked(true);
    emit currentChanged(index);
}

void TabSwitcherWidget::setAddedFilesCount(int count) {
    addedFilesButton_->setText(tr("Added files (%1)").arg(count));
}

QPushButton* TabSwitcherWidget::createTabButton(const QString& text, int index) {
    auto* button = new QPushButton(text, this);
    button->setCheckable(true);
    button->setFlat(true);
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    buttonGroup_->addButton(button, index);
    return button;
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

UploadsTabWidget* MainWindowTabsWidget::uploadsTab() const {
    return uploadsTab_;
}

AddedFilesTabWidget* MainWindowTabsWidget::addedFilesTab() const {
    return addedFilesTab_;
}

UploadSettingsTabWidget* MainWindowTabsWidget::uploadSettingsTab() const {
    return uploadSettingsTab_;
}

void MainWindowTabsWidget::configureUploadSettings(UploadEngineManager* uploadEngineManager,
                                                   const ServerProfile& imageProfile,
                                                   const ServerProfile& fileProfile) {
    uploadSettingsTab_->configure(uploadEngineManager, imageProfile, fileProfile);
}

void MainWindowTabsWidget::setPendingFilesModel(ThumbnailListModel* model) {
    addedFilesTab_->setModel(model);
}

void MainWindowTabsWidget::setPendingFilesCount(int count) {
    tabSwitcher_->setAddedFilesCount(count);
    uploadSettingsTab_->setFileCount(count);
}

int MainWindowTabsWidget::currentIndex() const {
    return tabSwitcher_->currentIndex();
}

void MainWindowTabsWidget::setCurrentIndex(int index) {
    tabSwitcher_->setCurrentIndex(index);
}
