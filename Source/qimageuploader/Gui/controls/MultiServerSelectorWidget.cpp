#include "MultiServerSelectorWidget.h"

#include <algorithm>

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "Core/Upload/UploadEngine.h"
#include "ServerSelectorWidget.h"

MultiServerSelectorWidget::MultiServerSelectorWidget(UploadEngineManager* uploadEngineManager, QWidget* parent) :
    QGroupBox(parent), uploadEngineManager_(uploadEngineManager) {
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setStyleSheet(QStringLiteral("MultiServerSelectorWidget::title { font-weight: normal; }"));

    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(8);

    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea_->setCursor(Qt::ArrowCursor);

    selectorContainer_ = new QWidget(scrollArea_);
    selectorLayout_ = new QVBoxLayout(selectorContainer_);
    selectorLayout_->setContentsMargins(0, 0, 0, 0);
    selectorLayout_->setSpacing(6);
    selectorLayout_->addStretch(1);
    scrollArea_->setWidget(selectorContainer_);
    layout->addWidget(scrollArea_, 1);

    addButton_ = new QPushButton(QIcon(QStringLiteral(":/res/icon-plus.png")), tr("Add server"), this);
    addButton_->setCursor(Qt::ArrowCursor);
    layout->addWidget(addButton_, 0, Qt::AlignTop);
    connect(addButton_, &QPushButton::clicked, this, [this] {
        addSelector();
        setExpanded(true);
    });

    addSelector();
    QTimer::singleShot(0, this, &MultiServerSelectorWidget::updateHeight);
}

void MultiServerSelectorWidget::setTitle(const QString& title) {
    baseTitle_ = title;
    updateTitle();
}

void MultiServerSelectorWidget::setServerProfileGroup(ServerProfileGroup serverProfileGroup) {
    for (const SelectorRow& row : rows_) {
        delete row.container;
    }
    rows_.clear();

    for (const ServerProfile& profile : serverProfileGroup.getItems()) {
        addSelector(profile);
    }
    if (rows_.empty()) {
        addSelector();
    }
    updateDeleteButtons();
    updateTitle();
    QTimer::singleShot(0, this, &MultiServerSelectorWidget::updateHeight);
}

ServerProfileGroup MultiServerSelectorWidget::serverProfileGroup() const {
    ServerProfileGroup result;
    for (const SelectorRow& row : rows_) {
        const ServerProfile& profile = row.selector->serverProfile();
        if (!profile.isNull()) {
            result.addItem(profile);
        }
    }
    return result;
}

void MultiServerSelectorWidget::setServersMask(int mask) {
    serversMask_ = mask;
    for (const SelectorRow& row : rows_) {
        row.selector->setServersMask(mask);
    }
}

void MultiServerSelectorWidget::updateServerList() {
    for (const SelectorRow& row : rows_) {
        row.selector->updateServerList();
    }
}

void MultiServerSelectorWidget::fillServerIcons() {
    iconsLoaded_ = true;
    for (const SelectorRow& row : rows_) {
        row.selector->fillServerIcons();
    }
}

bool MultiServerSelectorWidget::validate(QString* firstInvalidServerName, bool focusFirstInvalid) {
    SelectorRow* firstInvalidRow = nullptr;
    bool hasSelectedServer = false;
    for (SelectorRow& row : rows_) {
        const ServerProfile& profile = row.selector->serverProfile();
        hasSelectedServer = hasSelectedServer || !profile.serverName().empty();
        const CUploadEngineData* server = profile.uploadEngineData();
        const bool hasError = !profile.serverName().empty() && server
            && server->NeedAuthorization == CUploadEngineData::naObligatory && profile.profileName().empty();
        setValidationError(row, hasError);
        if (hasError && !firstInvalidRow) {
            firstInvalidRow = &row;
        }
    }

    if (!hasSelectedServer && !rows_.empty()) {
        firstInvalidRow = &rows_.front();
        setValidationError(*firstInvalidRow, true);
    }

    if (!firstInvalidRow) {
        return hasSelectedServer;
    }
    if (firstInvalidServerName) {
        *firstInvalidServerName = QString::fromUtf8(firstInvalidRow->selector->serverProfile().serverName());
    }
    if (focusFirstInvalid) {
        setExpanded(true);
        scrollArea_->ensureWidgetVisible(firstInvalidRow->container, 0, selectorLayout_->spacing());
        firstInvalidRow->selector->focusServerSelection();
        for (QWidget* ancestor = parentWidget(); ancestor; ancestor = ancestor->parentWidget()) {
            if (auto* parentScrollArea = qobject_cast<QScrollArea*>(ancestor)) {
                parentScrollArea->ensureWidgetVisible(this, 0, selectorLayout_->spacing());
                break;
            }
        }
    }
    return false;
}

void MultiServerSelectorWidget::mousePressEvent(QMouseEvent* event) {
    QGroupBox::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        setExpanded(!expanded_);
    }
}

void MultiServerSelectorWidget::addSelector(const ServerProfile& serverProfile) {
    auto* container = new QWidget(selectorContainer_);
    container->setCursor(Qt::ArrowCursor);
    auto* rowLayout = new QHBoxLayout(container);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(6);

    auto* selector = new ServerSelectorWidget(uploadEngineManager_, false, container);
    selector->setServersMask(serversMask_);
    selector->setServerProfile(serverProfile);
    selector->setCursor(Qt::ArrowCursor);
    for (QWidget* child : selector->findChildren<QWidget*>()) {
        child->setCursor(Qt::ArrowCursor);
    }
    if (iconsLoaded_) {
        selector->fillServerIcons();
    }
    rowLayout->addWidget(selector, 1);

    auto* deleteButton = new QToolButton(container);
    deleteButton->setText(QStringLiteral("\u00d7"));
    deleteButton->setToolTip(tr("Delete server"));
    deleteButton->setAutoRaise(true);
    deleteButton->setCursor(Qt::ArrowCursor);
    deleteButton->setFixedSize(34, 34);
    deleteButton->setStyleSheet(QStringLiteral(
        "QToolButton { color: #d9363e; border: 0; border-radius: 5px; font-size: 26px; font-weight: bold; }"
        "QToolButton:hover { background: #fde8e9; color: #bd2028; }"
        "QToolButton:pressed { background: #f8cfd1; }"
        "QToolButton:disabled { color: #e7a6aa; }"));
    rowLayout->addWidget(deleteButton, 0, Qt::AlignVCenter);

    selectorLayout_->insertWidget(selectorLayout_->count() - 1, container);
    rows_.push_back({ container, selector, deleteButton });
    connect(deleteButton, &QToolButton::clicked, this, [this, container] { removeSelector(container); });
    connect(selector, &ServerSelectorWidget::serverProfileChanged, this, &MultiServerSelectorWidget::updateTitle);
    updateDeleteButtons();
    updateTitle();
    QTimer::singleShot(0, this, &MultiServerSelectorWidget::updateHeight);
}

void MultiServerSelectorWidget::removeSelector(QWidget* container) {
    if (rows_.size() <= 1) {
        return;
    }
    const auto it = std::find_if(rows_.begin(), rows_.end(),
                                 [container](const SelectorRow& row) { return row.container == container; });
    if (it == rows_.end()) {
        return;
    }
    delete it->container;
    rows_.erase(it);
    updateDeleteButtons();
    updateTitle();
    QTimer::singleShot(0, this, &MultiServerSelectorWidget::updateHeight);
}

void MultiServerSelectorWidget::updateDeleteButtons() {
    const bool canDelete = rows_.size() > 1;
    for (const SelectorRow& row : rows_) {
        row.deleteButton->setEnabled(canDelete);
    }
}

void MultiServerSelectorWidget::updateTitle() {
    const int selectedServerCount
        = static_cast<int>(std::count_if(rows_.begin(), rows_.end(), [](const SelectorRow& row) {
              return !row.selector->serverProfile().serverName().empty();
          }));
    QGroupBox::setTitle(baseTitle_.isEmpty() ? QString()
                                             : QStringLiteral("%1 (%2)").arg(baseTitle_).arg(selectedServerCount));
}

void MultiServerSelectorWidget::setValidationError(SelectorRow& row, bool hasError) {
    row.selector->setStyleSheet(
        hasError ? QStringLiteral("ServerSelectorWidget { background-color: #fde7eb; border: 1px solid #e8a2ae; "
                                  "border-radius: 6px; font-weight: bold; }")
                 : QStringLiteral("QGroupBox { font-weight: bold; }"));
}

void MultiServerSelectorWidget::updateHeight() {
    if (rows_.empty()) {
        return;
    }
    const int visibleRows = expanded_ ? std::min<int>(3, rows_.size()) : 1;
    const int rowHeight = rows_.front().container->sizeHint().height();
    const int spacing = selectorLayout_->spacing();
    scrollArea_->setFixedHeight(rowHeight * visibleRows + spacing * (visibleRows - 1));
    layout()->activate();
    setFixedHeight(sizeHint().height());
}

void MultiServerSelectorWidget::setExpanded(bool expanded) {
    if (expanded_ == expanded) {
        return;
    }
    expanded_ = expanded;
    setStyleSheet(expanded_ ? QStringLiteral("MultiServerSelectorWidget::title { font-weight: bold; }")
                            : QStringLiteral("MultiServerSelectorWidget::title { font-weight: normal; }"));
    updateHeight();
}
