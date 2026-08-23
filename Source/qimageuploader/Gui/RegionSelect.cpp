#include "RegionSelect.h"

#include <QGuiApplication>
#include <QScreen>

namespace {
constexpr int ZOOM_SIDE = 200;
constexpr int ZOOM_FACTOR = 8;
constexpr qreal CROSS_HALF_SIZE = 12.0;

QRect desktopGeometry() {
    QRect geometry;
    const auto screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        geometry = geometry.united(screen->geometry());
    }

    return geometry;
}

QRect toPixmapCoordinates(const QRect& rect, const QPixmap& pixmap) {
    const qreal devicePixelRatio = pixmap.devicePixelRatio();
    const int left = qRound(rect.x() * devicePixelRatio);
    const int top = qRound(rect.y() * devicePixelRatio);
    const int right = qRound((rect.x() + rect.width()) * devicePixelRatio);
    const int bottom = qRound((rect.y() + rect.height()) * devicePixelRatio);

    return QRect(left, top, right - left, bottom - top).intersected(pixmap.rect());
}

QRect centeredRect(const QPoint& center, const QSize& size, const QRect& bounds) {
    const QSize boundedSize(qMin(size.width(), bounds.width()), qMin(size.height(), bounds.height()));
    QRect rect(center - QPoint(boundedSize.width() / 2, boundedSize.height() / 2), boundedSize);

    if (rect.left() < bounds.left()) {
        rect.moveLeft(bounds.left());
    } else if (rect.right() > bounds.right()) {
        rect.moveRight(bounds.right());
    }
    if (rect.top() < bounds.top()) {
        rect.moveTop(bounds.top());
    } else if (rect.bottom() > bounds.bottom()) {
        rect.moveBottom(bounds.bottom());
    }

    return rect;
}
}

RegionSelect::RegionSelect(QWidget* parent, QPixmap* src) : QDialog(parent) {

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowState(Qt::WindowFullScreen);
    setCursor(Qt::CrossCursor);

    const QRect desktopRect = desktopGeometry();
    sizeDesktop = desktopRect.size();
    resize(sizeDesktop);

    desktopPixmapBkg = *src;
    desktopPixmapClr = desktopPixmapBkg;

    move(desktopRect.topLeft());
    drawBackground();
}

RegionSelect::~RegionSelect() = default;

bool RegionSelect::event(QEvent* event) {
    if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::KeyPress) {
        accept();
    }
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() != Qt::LeftButton)
            reject();

        selStartPoint = mouseEvent->pos();
        selectRect = QRect(selStartPoint, QSize());
    }

    return QDialog::event(event);
}

void RegionSelect::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    painter.drawPixmap(QPoint(0, 0), desktopPixmapBkg);

    drawRectSelection(painter);
}

void RegionSelect::mouseMoveEvent(QMouseEvent* event) {
    selectRect = QRect(selStartPoint, event->pos()).normalized();
    selEndPoint = event->pos();
    update();
}

void RegionSelect::drawBackground() {
    QPainter painter(&desktopPixmapBkg);

    painter.setBrush(QBrush(QColor(0, 0, 0, 85), Qt::SolidPattern));

    painter.drawRect(desktopPixmapBkg.rect());

    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    QRect txtRect = primaryScreen ? primaryScreen->geometry() : desktopPixmapBkg.rect();
    txtRect.translate(-desktopGeometry().topLeft());
    QString txtTip = QApplication::tr("Use your mouse to draw a rectangle to screenshot or  exit pressing\nany key or "
                                      "using the right or middle mouse buttons.");

    txtRect.setHeight(qRound(static_cast<double>(txtRect.height()) / 10));

    painter.setPen(QPen(Qt::red)); // ste message rect border color
    painter.setBrush(QBrush(QColor(255, 255, 255, 180), Qt::SolidPattern));
    QRect txtBgRect = painter.boundingRect(txtRect, Qt::AlignCenter, txtTip);

    txtBgRect.setX(txtBgRect.x() - 6);
    txtBgRect.setY(txtBgRect.y() - 4);
    txtBgRect.setWidth(txtBgRect.width() + 12);
    txtBgRect.setHeight(txtBgRect.height() + 8);

    painter.drawRect(txtBgRect);

    painter.setPen(QPen(Qt::black)); // black color pen
    painter.drawText(txtBgRect, Qt::AlignCenter, txtTip);
}

void RegionSelect::drawRectSelection(QPainter& painter) {
    const QRect sourceRect = toPixmapCoordinates(selectRect, desktopPixmapClr);
    painter.drawPixmap(selectRect, desktopPixmapClr, sourceRect);
    painter.setPen(QPen(QBrush(QColor(0, 0, 0, 255)), 2));
    painter.drawRect(selectRect);

    QString txtSize = QApplication::tr("%1 x %2 pixels ").arg(sourceRect.width()).arg(sourceRect.height());
    painter.drawText(selectRect, Qt::AlignBottom | Qt::AlignRight, txtSize);

    if (!selEndPoint.isNull()) {
        const qreal devicePixelRatio = desktopPixmapClr.devicePixelRatio();
        const QSize zoomPixmapSize(qRound(ZOOM_SIDE * devicePixelRatio), qRound(ZOOM_SIDE * devicePixelRatio));
        const QSize zoomSourceSize((zoomPixmapSize.width() + ZOOM_FACTOR - 1) / ZOOM_FACTOR,
                                   (zoomPixmapSize.height() + ZOOM_FACTOR - 1) / ZOOM_FACTOR);
        const QPoint mousePixel(qBound(desktopPixmapClr.rect().left(), qRound(selEndPoint.x() * devicePixelRatio),
                                       desktopPixmapClr.rect().right()),
                                qBound(desktopPixmapClr.rect().top(), qRound(selEndPoint.y() * devicePixelRatio),
                                       desktopPixmapClr.rect().bottom()));
        const QRect zoomSourceRect = centeredRect(mousePixel, zoomSourceSize, desktopPixmapClr.rect());

        QPixmap zoomPixmap
            = desktopPixmapClr.copy(zoomSourceRect)
                  .scaled(zoomSourceRect.size() * ZOOM_FACTOR, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        const QRect zoomCropRect = centeredRect(zoomPixmap.rect().center(), zoomPixmapSize, zoomPixmap.rect());
        const qreal zoomPixelCenterOffset = (ZOOM_FACTOR - 1) / 2.0;
        const QPointF zoomCursor = QPointF(mousePixel - zoomSourceRect.topLeft()) * ZOOM_FACTOR
            + QPointF(zoomPixelCenterOffset, zoomPixelCenterOffset) - zoomCropRect.topLeft();
        zoomPixmap = zoomPixmap.copy(zoomCropRect);
        zoomPixmap.setDevicePixelRatio(devicePixelRatio);
        const QRect zoomPixmapRect(QPoint(), zoomPixmap.deviceIndependentSize().toSize());

        QPainter zoomPainter(&zoomPixmap);
        zoomPainter.setPen(QPen(QBrush(QColor(255, 0, 0, 180)), 2));
        zoomPainter.drawRect(zoomPixmapRect); // draw

        const QPointF crossCenter = zoomCursor / devicePixelRatio;
        zoomPainter.setCompositionMode(QPainter::CompositionMode_Difference);
        QPen crossPen(Qt::white);
        crossPen.setWidthF(ZOOM_FACTOR / devicePixelRatio);
        crossPen.setCapStyle(Qt::FlatCap);
        zoomPainter.setPen(crossPen);
        zoomPainter.drawLine(crossCenter - QPointF(CROSS_HALF_SIZE, 0), crossCenter + QPointF(CROSS_HALF_SIZE, 0));
        zoomPainter.drawLine(crossCenter - QPointF(0, CROSS_HALF_SIZE), crossCenter + QPointF(0, CROSS_HALF_SIZE));

        // position for drawing preview
        QPoint zoomCenter = selectRect.bottomRight();

        if (zoomCenter.x() + ZOOM_SIDE > width() || zoomCenter.y() + ZOOM_SIDE > height()) {
            zoomCenter -= QPoint(ZOOM_SIDE, ZOOM_SIDE);
        }
        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawPixmap(zoomCenter, zoomPixmap);
        painter.restore();
    }
}

QPixmap RegionSelect::getSelection() const {
    return desktopPixmapClr.copy(toPixmapCoordinates(selectRect, desktopPixmapClr));
}

CScreenshotRegion* RegionSelect::selectedRegion() const {
    const QRect sourceRect = toPixmapCoordinates(selectRect, desktopPixmapClr);
    return new CRectRegion(sourceRect.x(), sourceRect.y(), sourceRect.width(), sourceRect.height());
}
