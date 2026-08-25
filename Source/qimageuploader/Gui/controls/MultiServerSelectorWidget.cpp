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
    updateStyle();

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 3, 6, 4);
    layout->setSpacing(6);

    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea_->setCursor(Qt::ArrowCursor);

    selectorContainer_ = new QWidget(scrollArea_);
    selectorLayout_ = new QVBoxLayout(selectorContainer_);
    selectorLayout_->setContentsMargins(0, 0, 0, 0);
    selectorLayout_->setSpacing(3);
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

QString MultiServerSelectorWidget::baseTitle() const { return baseTitle_; }

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
    container->setObjectName(QStringLiteral("multiServerSelectorRow"));
    container->setAttribute(Qt::WA_StyledBackground);
    container->setCursor(Qt::ArrowCursor);
    auto* rowLayout = new QHBoxLayout(container);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(4);

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
    deleteButton->setIcon(QIcon(QStringLiteral(":/res/cancel.svg")));
    deleteButton->setIconSize(QSize(20, 20));
    deleteButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    deleteButton->setToolTip(tr("Delete server"));
    deleteButton->setAutoRaise(true);
    deleteButton->setCursor(Qt::PointingHandCursor);
    deleteButton->setFixedSize(34, 34);
    deleteButton->setStyleSheet(QStringLiteral("QToolButton { background: transparent; border: 0; border-radius: 5px; "
                                               "padding: 0; }"
                                               "QToolButton:hover { background: #fde8e9; }"
                                               "QToolButton:pressed { background: #f8cfd1; }"
                                               "QToolButton:disabled { background: transparent; }"));
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
    row.container->setStyleSheet(hasError
                                     ? QStringLiteral("QWidget#multiServerSelectorRow { background-color: #fde7eb; "
                                                      "border: 1px solid #e8a2ae; border-radius: 6px; }")
                                     : QString());
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
    updateStyle();
    updateHeight();
}

void MultiServerSelectorWidget::updateStyle() {
    setStyleSheet(QStringLiteral("MultiServerSelectorWidget { margin-top: 10px; padding-top: 6px; }"
                                 "MultiServerSelectorWidget::title { font-weight: %1; }")
                      .arg(expanded_ ? QStringLiteral("bold") : QStringLiteral("normal")));
}
