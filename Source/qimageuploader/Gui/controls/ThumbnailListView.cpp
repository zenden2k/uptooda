#include "ThumbnailListView.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QUrl>

#include "Gui/ImageViewerWindow.h"
#include "Gui/models/ThumbnailListModel.h"

namespace {

constexpr int GRID_WIDTH = 163;
constexpr int GRID_HEIGHT = 132;
constexpr int CARD_MARGIN = 4;
constexpr int CARD_PADDING = 6;
constexpr int THUMBNAIL_HEIGHT = 86;
constexpr int TEXT_SPACING = 7;

class ThumbnailItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
        return { GRID_WIDTH, GRID_HEIGHT };
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        const bool selected = option.state.testFlag(QStyle::State_Selected);
        const bool hovered = option.state.testFlag(QStyle::State_MouseOver);
        const QRect cardRect = option.rect.adjusted(CARD_MARGIN, CARD_MARGIN, -CARD_MARGIN, -CARD_MARGIN);

        QColor backgroundColor(QStringLiteral("#ffffff"));
        QColor borderColor(QStringLiteral("#d6e1ea"));
        int borderWidth = 1;
        if (selected) {
            backgroundColor = QColor(QStringLiteral("#dff1fd"));
            borderColor = QColor(QStringLiteral("#399bd8"));
            borderWidth = 2;
        } else if (hovered) {
            backgroundColor = QColor(QStringLiteral("#e8f4fb"));
            borderColor = QColor(QStringLiteral("#9fcce8"));
        }

        painter->setBrush(backgroundColor);
        painter->setPen(QPen(borderColor, borderWidth));
        painter->drawRoundedRect(cardRect, 8, 8);

        const QRect contentRect = cardRect.adjusted(CARD_PADDING, CARD_PADDING, -CARD_PADDING, -CARD_PADDING);
        const QRect thumbnailRect(contentRect.left(), contentRect.top(), contentRect.width(), THUMBNAIL_HEIGHT);
        const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (!icon.isNull()) {
            const QPixmap pixmap = icon.pixmap(thumbnailRect.size(), QIcon::Normal, QIcon::Off);
            const QSize pixmapSize = pixmap.deviceIndependentSize().toSize();
            const QPoint pixmapPosition(thumbnailRect.center().x() - pixmapSize.width() / 2,
                                        thumbnailRect.center().y() - pixmapSize.height() / 2);
            painter->drawPixmap(pixmapPosition, pixmap);
        }

        const int textTop = thumbnailRect.bottom() + 1 + TEXT_SPACING;
        const QRect textRect(contentRect.left(), textTop, contentRect.width(), contentRect.bottom() - textTop + 1);
        painter->setPen(QColor(QStringLiteral("#40566b")));
        painter->setFont(option.font);
        const QString text
            = option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideMiddle, textRect.width());
        painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, text);
        painter->restore();
    }
};

} // namespace

ThumbnailListView::ThumbnailListView(QWidget* parent) : QListView(parent) {
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setUniformItemSizes(true);
    setWrapping(true);
    setWordWrap(false);
    setIconSize(QSize(135, 86));
    setGridSize(QSize(GRID_WIDTH, GRID_HEIGHT));
    setSpacing(0);
    setItemDelegate(new ThumbnailItemDelegate(this));
    setStyleSheet(QStringLiteral("QListView { background: #f3f7fa; border: 0; outline: 0; color: #40566b; }"));
    connect(this, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (openImage(index)) {
            emit openRequested(index.row());
        }
    });
}

ThumbnailListView::~ThumbnailListView() = default;

void ThumbnailListView::setEmptyText(const QString& text) {
    emptyText_ = text;
    viewport()->update();
}

QList<int> ThumbnailListView::selectedRows() const {
    QList<int> result;
    if (!selectionModel()) {
        return result;
    }
    for (const QModelIndex& index : selectionModel()->selectedIndexes()) {
        result.append(index.row());
    }
    return result;
}

void ThumbnailListView::revealRow(int row) {
    if (!model() || row < 0 || row >= model()->rowCount()) {
        return;
    }
    const QModelIndex index = model()->index(row, 0);
    setCurrentIndex(index);
    scrollTo(index, QAbstractItemView::EnsureVisible);
}

void ThumbnailListView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat(ThumbnailListModel::internalMimeType())) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void ThumbnailListView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat(ThumbnailListModel::internalMimeType())) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void ThumbnailListView::dropEvent(QDropEvent* event) {
    if (!model() || !event->mimeData()->hasFormat(ThumbnailListModel::internalMimeType())) {
        event->ignore();
        return;
    }

    if (model()->dropMimeData(event->mimeData(), Qt::MoveAction, dropRow(event->position().toPoint()), 0, { })) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ThumbnailListView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        const QList<int> rows = selectedRows();
        if (!rows.isEmpty()) {
            emit removeRequested(rows);
        }
        event->accept();
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && currentIndex().isValid()) {
        if (openImage(currentIndex())) {
            emit openRequested(currentIndex().row());
        }
        event->accept();
        return;
    }
    QListView::keyPressEvent(event);
}

void ThumbnailListView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            emit removeRequested({ index.row() });
        }
        event->accept();
        return;
    }
    QListView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton && indexAt(event->pos()).isValid()) {
        dragStartPosition_ = event->pos();
    } else {
        dragStartPosition_ = { -1, -1 };
    }
}

void ThumbnailListView::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton) || dragStartPosition_.x() < 0) {
        QListView::mouseMoveEvent(event);
        return;
    }
    if (!internalDragActive_
        && (event->pos() - dragStartPosition_).manhattanLength() < QApplication::startDragDistance()) {
        QListView::mouseMoveEvent(event);
        return;
    }

    internalDragActive_ = true;
    if (viewport()->rect().contains(event->pos())) {
        viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    internalDragActive_ = false;
    dragStartPosition_ = { -1, -1 };
    viewport()->unsetCursor();
    startDrag(model() ? model()->supportedDragActions() : Qt::CopyAction);
    event->accept();
}

void ThumbnailListView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && internalDragActive_ && model()) {
        std::unique_ptr<QMimeData> mimeData(model()->mimeData(selectedIndexes()));
        if (mimeData) {
            model()->dropMimeData(mimeData.get(), Qt::MoveAction, dropRow(event->pos()), 0, { });
        }
        event->accept();
    } else {
        QListView::mouseReleaseEvent(event);
    }
    internalDragActive_ = false;
    dragStartPosition_ = { -1, -1 };
    viewport()->unsetCursor();
}

void ThumbnailListView::paintEvent(QPaintEvent* event) {
    QListView::paintEvent(event);
    if (!emptyText_.isEmpty() && (!model() || model()->rowCount() == 0)) {
        QPainter painter(viewport());
        painter.setPen(QColor(QStringLiteral("#8190a2")));
        QFont font = painter.font();
        font.setPointSize(font.pointSize() + 1);
        painter.setFont(font);
        painter.drawText(viewport()->rect(), Qt::AlignCenter, emptyText_);
    }
}

void ThumbnailListView::startDrag(Qt::DropActions supportedActions) {
    const QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty() || !model()) {
        return;
    }

    QMimeData* mimeData = model()->mimeData(indexes);
    if (!mimeData) {
        return;
    }

    auto* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    const QIcon icon = qvariant_cast<QIcon>(indexes.front().data(Qt::DecorationRole));
    if (!icon.isNull()) {
        drag->setPixmap(icon.pixmap(iconSize()));
    }

    // Copy is the safe default for other applications. This view requests MoveAction
    // for its own drops through defaultDropAction().
    drag->exec(supportedActions, Qt::CopyAction);
}

int ThumbnailListView::dropRow(const QPoint& position) const {
    if (!model() || model()->rowCount() == 0) {
        return 0;
    }

    const QModelIndex target = indexAt(position);
    if (!target.isValid()) {
        const QModelIndex first = model()->index(0, 0);
        return position.y() < visualRect(first).top() ? 0 : model()->rowCount();
    }

    const QRect targetRect = visualRect(target);
    const bool insertAfter = flow() == QListView::LeftToRight ? position.x() >= targetRect.center().x()
                                                              : position.y() >= targetRect.center().y();
    return target.row() + (insertAfter ? 1 : 0);
}

bool ThumbnailListView::openImage(const QModelIndex& index) {
    if (!index.isValid() || !model()) {
        return false;
    }

    const QString selectedFileName
        = model()->data(model()->index(index.row(), 0), ThumbnailListModel::FILE_PATH_ROLE).toString();
    if (selectedFileName.isEmpty()) {
        return false;
    }
    if (!ImageViewerWindow::isSupportedImageFile(selectedFileName)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(selectedFileName));
        return false;
    }

    QStringList fileNames;
    fileNames.reserve(model()->rowCount());
    for (int row = 0; row < model()->rowCount(); ++row) {
        fileNames.append(model()->data(model()->index(row, 0), ThumbnailListModel::FILE_PATH_ROLE).toString());
    }
    if (index.row() >= fileNames.size() || fileNames[index.row()].isEmpty()) {
        return false;
    }

    if (!imageViewerWindow_) {
        imageViewerWindow_ = std::make_unique<ImageViewerWindow>();
        if (window()->windowHandle()) {
            imageViewerWindow_->setTransientParent(window()->windowHandle());
        }
    }
    imageViewerWindow_->setImageViewerSource(
        std::make_unique<FileListImageViewerSource>(std::move(fileNames), index.row()));
    imageViewerWindow_->open();
    return true;
}
