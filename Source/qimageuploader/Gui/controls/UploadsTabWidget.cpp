#include "UploadsTabWidget.h"

#include <QStackedWidget>
#include <QVBoxLayout>

#include "MainWindowTabsWidget.h"
#include "UploadSessionListWidget.h"

UploadsTabWidget::UploadsTabWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    stack_ = new QStackedWidget(this);
    stack_->addWidget(new EmptyTabWidget(tr("Uploads"), tr("Your uploads will appear here"), stack_));
    sessionList_ = new UploadSessionListWidget(stack_);
    stack_->addWidget(sessionList_);
    layout->addWidget(stack_);

    connect(sessionList_, &UploadSessionListWidget::hasItemsChanged, this,
            [this](bool hasItems) { stack_->setCurrentWidget(hasItems ? sessionList_ : stack_->widget(0)); });
}

UploadSessionListWidget* UploadsTabWidget::sessionList() const {
    return sessionList_;
}