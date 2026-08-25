#include "ServerListPopup.h"

#include <algorithm>

#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QScreen>
#include <QSizePolicy>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWindow>

#include "Core/ServiceLocator.h"
#include "Core/Settings/QtGuiSettings.h"
#include "Core/Upload/UploadEngine.h"
#include "Gui/models/ServerTableModel.h"

namespace {

constexpr int ALL_TYPES = 0;
constexpr int RESIZE_BORDER_WIDTH = 10;
bool IconModeEnabled = false;

} // namespace

ServerListPopup::ServerListPopup(int serversMask, const std::string& selectedServer, QWidget* parent) :
    QDialog(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), serversMask_(serversMask),
    selectedServer_(selectedServer),
    model_(std::make_unique<ServerTableModel>(ServiceLocator::instance()->engineList(), this)) {
    setObjectName(QStringLiteral("serverListPopup"));
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setWindowTitle(tr("Choose server"));
    setMinimumSize(720, 420);
    resize(860, 500);

    auto* layout = new QVBoxLayout(this);
    auto* typeLayout = new QHBoxLayout;
    typeButtonGroup_ = new QButtonGroup(this);

    const auto addTypeButton = [this, typeLayout](const QString& text, int mask, bool enabled) {
        auto* button = new QRadioButton(text, this);
        button->setEnabled(enabled);
        typeButtonGroup_->addButton(button, mask);
        typeLayout->addWidget(button);
        return button;
    };

    auto* allButton = addTypeButton(tr("All"), ALL_TYPES, true);
    addTypeButton(tr("Image"), CUploadEngineData::TypeImageServer, serversMask_ & CUploadEngineData::TypeImageServer);
    addTypeButton(tr("File"), CUploadEngineData::TypeFileServer, serversMask_ & CUploadEngineData::TypeFileServer);
    addTypeButton(tr("Video"), CUploadEngineData::TypeVideoServer, serversMask_ & CUploadEngineData::TypeVideoServer);
    allButton->setChecked(true);
    typeLayout->addStretch(1);
    layout->addLayout(typeLayout);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setObjectName(QStringLiteral("serverSearchEdit"));
    searchEdit_->setClearButtonEnabled(true);
    searchEdit_->setPlaceholderText(tr("Search servers"));
    searchEdit_->addAction(QIcon(QStringLiteral(":/res/search.svg")), QLineEdit::LeadingPosition);
    searchEdit_->setMinimumWidth(280);
    searchEdit_->setMaximumWidth(420);
    searchEdit_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    tableView_ = new QTableView(this);
    tableView_->setObjectName(QStringLiteral("serverTable"));
    tableView_->setModel(model_.get());
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView_->setContextMenuPolicy(Qt::CustomContextMenu);
    tableView_->setAlternatingRowColors(true);
    tableView_->setSortingEnabled(false);
    tableView_->verticalHeader()->hide();
    tableView_->horizontalHeader()->setStretchLastSection(true);
    tableView_->horizontalHeader()->setSectionResizeMode(ServerTableModel::SERVER, QHeaderView::Stretch);
    tableView_->horizontalHeader()->setSectionResizeMode(ServerTableModel::MAX_FILE_SIZE,
                                                         QHeaderView::ResizeToContents);
    tableView_->horizontalHeader()->setSectionResizeMode(ServerTableModel::STORAGE_TIME, QHeaderView::ResizeToContents);
    tableView_->horizontalHeader()->setSectionResizeMode(ServerTableModel::ACCOUNT, QHeaderView::ResizeToContents);
    tableView_->horizontalHeader()->setSectionResizeMode(ServerTableModel::FILE_FORMATS, QHeaderView::Stretch);
    layout->addWidget(tableView_, 1);

    iconView_ = new QListView(this);
    iconView_->setObjectName(QStringLiteral("serverIconView"));
    iconView_->setModel(model_.get());
    iconView_->setModelColumn(ServerTableModel::SERVER);
    iconView_->setViewMode(QListView::IconMode);
    iconView_->setResizeMode(QListView::Adjust);
    iconView_->setMovement(QListView::Static);
    iconView_->setWrapping(true);
    iconView_->setWordWrap(true);
    iconView_->setUniformItemSizes(false);
    iconView_->setIconSize(QSize(40, 40));
    iconView_->setGridSize(QSize(150, 86));
    iconView_->setSpacing(5);
    iconView_->setSelectionMode(QAbstractItemView::SingleSelection);
    iconView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    iconView_->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(iconView_, 1);
    setIconMode(IconModeEnabled);

    auto* bottomLayout = new QHBoxLayout;
    bottomLayout->setSpacing(12);
    bottomLayout->addWidget(searchEdit_);
    bottomLayout->addStretch(1);
    optionsButton_ = new QToolButton(this);
    optionsButton_->setObjectName(QStringLiteral("serverOptionsButton"));
    optionsButton_->setIcon(QIcon(QStringLiteral(":/res/options.svg")));
    optionsButton_->setToolTip(tr("Options"));
    optionsButton_->setCursor(Qt::PointingHandCursor);
    bottomLayout->addWidget(optionsButton_);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    if (auto* okButton = buttons->button(QDialogButtonBox::Ok)) {
        okButton->setProperty("class", QStringLiteral("highlighted"));
    }
    bottomLayout->addWidget(buttons);
    layout->addLayout(bottomLayout);

    connect(searchEdit_, &QLineEdit::textChanged, this, &ServerListPopup::applyFilter);
    connect(typeButtonGroup_, &QButtonGroup::idClicked, this, &ServerListPopup::applyFilter);
    connect(tableView_, &QTableView::doubleClicked, this, &ServerListPopup::acceptCurrent);
    connect(tableView_, &QTableView::customContextMenuRequested, this, &ServerListPopup::showServerContextMenu);
    connect(iconView_, &QListView::doubleClicked, this, &ServerListPopup::acceptCurrent);
    connect(iconView_, &QListView::customContextMenuRequested, this, &ServerListPopup::showServerContextMenu);
    connect(model_.get(), &ServerTableModel::dataChanged, this,
            [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
                if (roles.empty() || roles.contains(Qt::DecorationRole) || roles.contains(Qt::SizeHintRole)) {
                    iconView_->doItemsLayout();
                    iconView_->viewport()->update();
                }
            });
    connect(optionsButton_, &QToolButton::clicked, this, &ServerListPopup::showOptionsMenu);
    connect(buttons, &QDialogButtonBox::accepted, this, &ServerListPopup::acceptCurrent);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QApplication::instance()->installEventFilter(this);

    applyFilter();
    selectServer(selectedServer_);
    searchEdit_->setFocus();
}

ServerListPopup::~ServerListPopup() = default;

std::string ServerListPopup::selectedServer() const { return selectedServer_; }

void ServerListPopup::showPopup(const QRect& anchorRect) {
    const QRect availableGeometry = QGuiApplication::screenAt(anchorRect.center())
        ? QGuiApplication::screenAt(anchorRect.center())->availableGeometry()
        : QApplication::primaryScreen()->availableGeometry();

    QPoint position = anchorRect.bottomLeft() + QPoint(0, 2);
    if (position.x() + width() > availableGeometry.right()) {
        position.setX(anchorRect.right() - width());
    }
    if (position.y() + height() > availableGeometry.bottom()) {
        position.setY(anchorRect.top() - height() - 2);
    }
    const int maxX = std::max(availableGeometry.left(), availableGeometry.right() - width() + 1);
    const int maxY = std::max(availableGeometry.top(), availableGeometry.bottom() - height() + 1);
    position.setX(std::clamp(position.x(), availableGeometry.left(), maxX));
    position.setY(std::clamp(position.y(), availableGeometry.top(), maxY));
    move(position);
    setWindowModality(Qt::NonModal);
    show();
    raise();
    activateWindow();
}

bool ServerListPopup::event(QEvent* event) {
    if (event->type() == QEvent::WindowDeactivate) {
        QTimer::singleShot(0, this, [this] {
            QWidget* activePopup = QApplication::activePopupWidget();
            const bool childMenuIsActive = activePopup && activePopup != this && isAncestorOf(activePopup);
            if (isVisible() && !isActiveWindow() && !childMenuIsActive) {
                reject();
            }
        });
    }
    return QDialog::event(event);
}

bool ServerListPopup::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress && isVisible()) {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const QPoint globalPosition = mouseEvent->globalPosition().toPoint();
        bool insidePopup = frameGeometry().contains(globalPosition);
        if (!insidePopup) {
            const auto childWindows = findChildren<QWidget*>();
            insidePopup = std::any_of(childWindows.cbegin(), childWindows.cend(), [&globalPosition](QWidget* child) {
                return child->isWindow() && child->isVisible() && child->frameGeometry().contains(globalPosition);
            });
        }
        if (!insidePopup) {
            reject();
            return false;
        }
    }

    const bool redirectsToSearch = watched == tableView_ || watched == tableView_->viewport() || watched == iconView_
        || watched == iconView_->viewport() || (qobject_cast<QRadioButton*>(watched) && watched->parent() == this);
    if (redirectsToSearch && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        const QString text = keyEvent->text();
        const bool containsPrintableCharacter
            = std::any_of(text.cbegin(), text.cend(), [](QChar character) { return character.isPrint(); });
        if (containsPrintableCharacter) {
            searchEdit_->setFocus(Qt::ShortcutFocusReason);
            QCoreApplication::sendEvent(searchEdit_, event);
            return true;
        }
    } else if (redirectsToSearch && event->type() == QEvent::InputMethod) {
        searchEdit_->setFocus(Qt::ShortcutFocusReason);
        QCoreApplication::sendEvent(searchEdit_, event);
        return true;
    }
    return QDialog::eventFilter(watched, event);
}

void ServerListPopup::leaveEvent(QEvent* event) {
    unsetCursor();
    QDialog::leaveEvent(event);
}

void ServerListPopup::mouseMoveEvent(QMouseEvent* event) {
    updateResizeCursor(resizeEdges(event->position()));
    QDialog::mouseMoveEvent(event);
}

void ServerListPopup::mousePressEvent(QMouseEvent* event) {
    const Qt::Edges edges = resizeEdges(event->position());
    if (event->button() == Qt::LeftButton && edges != Qt::Edges() && windowHandle()
        && windowHandle()->startSystemResize(edges)) {
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void ServerListPopup::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    const bool shrinking = event->oldSize().isValid()
        && (event->size().width() < event->oldSize().width() || event->size().height() < event->oldSize().height());
    if (shrinking) {
        repaint();
    } else {
        update();
    }
}

void ServerListPopup::applyFilter() {
    const std::string previousSelection = selectedServer_;
    model_->setFilter(searchEdit_->text(), selectedTypeMask());
    selectServer(previousSelection);
}

void ServerListPopup::acceptCurrent() {
    const QModelIndex index = activeView()->currentIndex();
    if (!index.isValid()) {
        return;
    }
    selectedServer_ = model_->serverNameAt(index.row());
    accept();
}

void ServerListPopup::showOptionsMenu() {
    auto* settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    if (!settings) {
        return;
    }

    QMenu menu(this);
    QAction* favoritesOnlyAction = menu.addAction(tr("Show favorites only"));
    favoritesOnlyAction->setCheckable(true);
    favoritesOnlyAction->setChecked(settings->ServerListSettings.ShowFavoritesOnly);

    QAction* hideBlacklistedAction = menu.addAction(tr("Hide blacklisted"));
    hideBlacklistedAction->setCheckable(true);
    hideBlacklistedAction->setChecked(settings->ServerListSettings.HideBlackListed);

    menu.addSeparator();
    QMenu* viewModeMenu = menu.addMenu(tr("View mode"));
    QAction* tableModeAction = viewModeMenu->addAction(tr("Table"));
    tableModeAction->setCheckable(true);
    tableModeAction->setChecked(!IconModeEnabled);
    QAction* iconModeAction = viewModeMenu->addAction(tr("Icons"));
    iconModeAction->setCheckable(true);
    iconModeAction->setChecked(IconModeEnabled);
    auto* viewModeGroup = new QActionGroup(&menu);
    viewModeGroup->setExclusive(true);
    viewModeGroup->addAction(tableModeAction);
    viewModeGroup->addAction(iconModeAction);

    QAction* selectedAction = menu.exec(optionsButton_->mapToGlobal(QPoint(0, optionsButton_->height())));
    if (selectedAction == favoritesOnlyAction) {
        settings->ServerListSettings.ShowFavoritesOnly = favoritesOnlyAction->isChecked();
    } else if (selectedAction == hideBlacklistedAction) {
        settings->ServerListSettings.HideBlackListed = hideBlacklistedAction->isChecked();
    } else if (selectedAction == tableModeAction) {
        setIconMode(false);
        return;
    } else if (selectedAction == iconModeAction) {
        setIconMode(true);
        return;
    } else {
        return;
    }

    settings->notifyChange();
    applyFilter();
}

void ServerListPopup::showServerContextMenu(const QPoint& position) {
    auto* view = qobject_cast<QAbstractItemView*>(sender());
    if (!view) {
        return;
    }
    const QModelIndex index = view->indexAt(position);
    const CUploadEngineData* server = model_->serverAt(index.row());
    auto* settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    if (!index.isValid() || !server || !settings) {
        return;
    }

    view->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    view->setCurrentIndex(index);
    const std::string serverName = server->Name;
    const bool favorite = settings->ServerListSettings.isServerFavorite(serverName);
    const bool blacklisted = settings->ServerListSettings.isServerBlacklisted(serverName);

    QMenu menu(this);
    QAction* favoriteAction = menu.addAction(favorite ? tr("Remove from favorites") : tr("Add to favorites"));
    QAction* blacklistAction = menu.addAction(blacklisted ? tr("Remove from blacklist") : tr("Add to blacklist"));
    menu.addSeparator();
    QAction* websiteAction = menu.addAction(tr("Open the website"));
    websiteAction->setEnabled(!server->WebsiteUrl.empty());
    QAction* registrationAction = menu.addAction(tr("Go to signup page"));
    registrationAction->setEnabled(!server->RegistrationUrl.empty());

    QAction* selectedAction = menu.exec(view->viewport()->mapToGlobal(position));
    if (selectedAction == favoriteAction) {
        if (favorite) {
            settings->ServerListSettings.removeServerFromFavorites(serverName);
        } else {
            settings->ServerListSettings.addServerToFavorites(serverName);
        }
    } else if (selectedAction == blacklistAction) {
        if (blacklisted) {
            settings->ServerListSettings.removeServerFromBlacklist(serverName);
        } else {
            settings->ServerListSettings.addServerToBlacklist(serverName);
        }
    } else if (selectedAction == websiteAction) {
        QDesktopServices::openUrl(QUrl(QString::fromStdString(server->WebsiteUrl)));
        return;
    } else if (selectedAction == registrationAction) {
        QDesktopServices::openUrl(QUrl(QString::fromStdString(server->RegistrationUrl)));
        return;
    } else {
        return;
    }

    settings->notifyChange();
    model_->setFilter(searchEdit_->text(), selectedTypeMask());
    selectServer(serverName);
}

int ServerListPopup::selectedTypeMask() const {
    const int selectedMask = typeButtonGroup_->checkedId();
    return selectedMask == ALL_TYPES ? serversMask_ : selectedMask & serversMask_;
}

QAbstractItemView* ServerListPopup::activeView() const {
    return IconModeEnabled ? static_cast<QAbstractItemView*>(iconView_) : static_cast<QAbstractItemView*>(tableView_);
}

Qt::Edges ServerListPopup::resizeEdges(const QPointF& position) const {
    Qt::Edges edges;
    if (position.x() <= RESIZE_BORDER_WIDTH) {
        edges |= Qt::LeftEdge;
    } else if (position.x() >= width() - RESIZE_BORDER_WIDTH) {
        edges |= Qt::RightEdge;
    }
    if (position.y() <= RESIZE_BORDER_WIDTH) {
        edges |= Qt::TopEdge;
    } else if (position.y() >= height() - RESIZE_BORDER_WIDTH) {
        edges |= Qt::BottomEdge;
    }
    return edges;
}

void ServerListPopup::setIconMode(bool enabled) {
    const QModelIndex currentIndex = activeView() ? activeView()->currentIndex() : QModelIndex();
    IconModeEnabled = enabled;
    tableView_->setVisible(!enabled);
    iconView_->setVisible(enabled);
    if (currentIndex.isValid()) {
        activeView()->setCurrentIndex(model_->index(currentIndex.row(), ServerTableModel::SERVER));
        activeView()->scrollTo(activeView()->currentIndex());
    }
}

void ServerListPopup::updateResizeCursor(Qt::Edges edges) {
    if (edges == (Qt::LeftEdge | Qt::TopEdge) || edges == (Qt::RightEdge | Qt::BottomEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (edges == (Qt::RightEdge | Qt::TopEdge) || edges == (Qt::LeftEdge | Qt::BottomEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (edges.testFlag(Qt::LeftEdge) || edges.testFlag(Qt::RightEdge)) {
        setCursor(Qt::SizeHorCursor);
    } else if (edges.testFlag(Qt::TopEdge) || edges.testFlag(Qt::BottomEdge)) {
        setCursor(Qt::SizeVerCursor);
    } else {
        unsetCursor();
    }
}

void ServerListPopup::selectServer(const std::string& serverName) {
    const int row = model_->rowForServer(serverName);
    if (row >= 0) {
        tableView_->selectRow(row);
        tableView_->scrollTo(model_->index(row, ServerTableModel::SERVER));
        iconView_->setCurrentIndex(model_->index(row, ServerTableModel::SERVER));
        iconView_->scrollTo(model_->index(row, ServerTableModel::SERVER));
    } else if (model_->rowCount() > 0) {
        tableView_->selectRow(0);
        iconView_->setCurrentIndex(model_->index(0, ServerTableModel::SERVER));
    }
}
