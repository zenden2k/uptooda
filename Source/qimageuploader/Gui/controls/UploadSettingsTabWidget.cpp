#include "UploadSettingsTabWidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "MultiServerSelectorWidget.h"
#include "ServerSelectorWidget.h"

UploadSettingsTabWidget::UploadSettingsTabWidget(QWidget* parent) : QWidget(parent) {
    auto* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* scrollContents = new QWidget(scrollArea);
    auto* outerLayout = new QHBoxLayout(scrollContents);
    outerLayout->setContentsMargins(24, 28, 24, 12);
    outerLayout->addStretch(1);

    form_ = new QWidget(scrollContents);
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
    formLayout->addStretch(1);

    outerLayout->addWidget(form_, 6);
    outerLayout->addStretch(1);
    scrollArea->setWidget(scrollContents);
    pageLayout->addWidget(scrollArea, 1);

    auto* footer = new QWidget(this);
    auto* footerOuterLayout = new QHBoxLayout(footer);
    footerOuterLayout->setContentsMargins(24, 10, 24, 24);
    footerOuterLayout->addStretch(1);
    auto* buttonContainer = new QWidget(footer);
    buttonContainer->setMaximumWidth(960);
    auto* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch(1);
    auto* backButton = new QPushButton(tr("< Back"), buttonContainer);
    uploadButton_ = new QPushButton(tr("Upload"), buttonContainer);
    uploadButton_->setProperty("class", "highlighted");
    buttonLayout->addWidget(backButton);
    buttonLayout->addWidget(uploadButton_);
    footerOuterLayout->addWidget(buttonContainer, 6);
    footerOuterLayout->addStretch(1);
    pageLayout->addWidget(footer);

    connect(backButton, &QPushButton::clicked, this, &UploadSettingsTabWidget::backRequested);
    connect(uploadButton_, &QPushButton::clicked, this, [this] {
        if (validateServerGroups()) {
            emit uploadRequested();
        }
    });
    setFileCount(0);
}

void UploadSettingsTabWidget::configure(UploadEngineManager* uploadEngineManager,
                                        const ServerProfileGroup& imageProfiles,
                                        const ServerProfileGroup& fileProfiles) {
    if (imageServerWidget_ || !uploadEngineManager) {
        return;
    }
    auto* formLayout = qobject_cast<QVBoxLayout*>(form_->layout());
    imageServerWidget_ = new MultiServerSelectorWidget(uploadEngineManager, form_);
    imageServerWidget_->setTitle(tr("Image servers"));
    imageServerWidget_->setServersMask(ServerSelectorWidget::smImageServers | ServerSelectorWidget::smFileServers);
    imageServerWidget_->setServerProfileGroup(imageProfiles);
    formLayout->insertWidget(2, imageServerWidget_);

    fileServerWidget_ = new MultiServerSelectorWidget(uploadEngineManager, form_);
    fileServerWidget_->setTitle(tr("Servers for other files"));
    fileServerWidget_->setServersMask(ServerSelectorWidget::smFileServers);
    fileServerWidget_->setServerProfileGroup(fileProfiles);
    formLayout->insertWidget(3, fileServerWidget_);
}

void UploadSettingsTabWidget::setFileCount(int count) {
    fileCountLabel_->setText(tr("Selected files: %1").arg(count));
    uploadButton_->setEnabled(count > 0);
}

ServerProfileGroup UploadSettingsTabWidget::imageServerProfileGroup() const {
    return imageServerWidget_ ? imageServerWidget_->serverProfileGroup() : ServerProfileGroup();
}

ServerProfileGroup UploadSettingsTabWidget::fileServerProfileGroup() const {
    return fileServerWidget_ ? fileServerWidget_->serverProfileGroup() : ServerProfileGroup();
}

void UploadSettingsTabWidget::fillServerIcons() {
    if (imageServerWidget_) {
        imageServerWidget_->fillServerIcons();
    }
    if (fileServerWidget_) {
        fileServerWidget_->fillServerIcons();
    }
}

bool UploadSettingsTabWidget::validateServerGroups() {
    QString imageServerName;
    const bool imageServersValid = imageServerWidget_ && imageServerWidget_->validate(&imageServerName);

    QString fileServerName;
    const bool fileServersValid = fileServerWidget_ && fileServerWidget_->validate(&fileServerName, imageServersValid);
    if (imageServersValid && fileServersValid) {
        return true;
    }

    const bool imageGroupHasError = !imageServersValid;
    const QString& serverName = imageGroupHasError ? imageServerName : fileServerName;
    QString message;
    if (serverName.isEmpty()) {
        const MultiServerSelectorWidget* invalidGroup = imageGroupHasError ? imageServerWidget_ : fileServerWidget_;
        message = tr("You have not selected a server for \"%1\"").arg(invalidGroup->baseTitle());
    } else {
        message = tr("You have not selected account for server \"%1\"").arg(serverName);
    }
    QMessageBox::critical(this, tr("Error"), message);
    return false;
}
