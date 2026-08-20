#include "ThumbnailListView.h"

#include <QDesktopServices>
#include <QKeyEvent>
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
