#include "UploadSettingsTabWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "ServerSelectorWidget.h"


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
    uploadButton_->setProperty("class", "highlighted");
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