
#include "AddedFilesTabWidget.h"

#include <QAbstractItemModel>
#include <QPushButton>
#include <QVBoxLayout>

#include "ThumbnailListView.h"
#include "../Models/ThumbnailListModel.h"

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
    nextButton_->setProperty("class", "highlighted");
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