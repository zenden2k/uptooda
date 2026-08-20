#include "ImageViewerWindow.h"

#include <QAbstractButton>
#include <QCloseEvent>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImageReader>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QMovie>
#include <QPainter>
#include <QRegion>
#include <QResizeEvent>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QtConcurrentRun>

#include <algorithm>

class ViewerBusyIndicator final : public QWidget {
public:
    explicit ViewerBusyIndicator(QWidget* parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFixedSize(48, 48);
        connect(&timer_, &QTimer::timeout, this, [this] {
            angle_ = (angle_ + 30) % 360;
            update();
        });
    }

    void setRunning(bool running) {
        setVisible(running);
        if (running) {
            timer_.start(70);
        } else {
            timer_.stop();
        }
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(QRectF(rect()).center());
        painter.rotate(angle_);
        for (int segment = 0; segment < 12; ++segment) {
            QColor color(QStringLiteral("#f0f2f4"));
            color.setAlphaF(0.18 + segment * 0.07);
            painter.setPen(QPen(color, 3.2, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(QPointF(0, -11), QPointF(0, -17));
            painter.rotate(30);
        }
    }

private:
    QTimer timer_;
    int angle_ = 0;
};

namespace {
class ElidedLabel final : public QLabel {
public:
    explicit ElidedLabel(QWidget* parent) : QLabel(parent) { }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setPen(palette().color(QPalette::WindowText));
        painter.setFont(font());
        painter.drawText(rect(), alignment(), fontMetrics().elidedText(text(), Qt::ElideMiddle, width()));
    }
};

class ViewerButton final : public QAbstractButton {
public:
    enum class Symbol { Previous, Next, Minimize, FullScreen, Close };

    ViewerButton(Symbol symbol, QWidget* parent) : QAbstractButton(parent), symbol_(symbol) {
        const bool navigation = symbol_ == Symbol::Previous || symbol_ == Symbol::Next;
        setCursor(navigation ? Qt::PointingHandCursor : Qt::ArrowCursor);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const bool navigation = symbol_ == Symbol::Previous || symbol_ == Symbol::Next;
        QColor background = navigation ? QColor(38, 50, 56, 64) : QColor(48, 59, 69, 194);
        if (underMouse() && isEnabled()) {
            background = symbol_ == Symbol::Close ? QColor(217, 75, 85, 235)
                                                  : (navigation ? QColor(98, 106, 115, 79) : QColor(83, 97, 109, 235));
        }
        painter.setBrush(background);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect(), navigation ? 8 : 5, navigation ? 8 : 5);

        QColor stroke = Qt::white;
        if (!isEnabled()) {
            stroke.setAlphaF(0.35);
        } else if (!underMouse() && !navigation) {
            stroke.setAlphaF(0.82);
        }
        painter.setPen(QPen(stroke, navigation ? 3.0 : 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        const QPointF center = QRectF(rect()).center();
        if (symbol_ == Symbol::Previous) {
            painter.drawPolyline(QPolygonF({ { width() * 0.62, height() * 0.34 },
                                             { width() * 0.42, height() * 0.50 },
                                             { width() * 0.62, height() * 0.66 } }));
        } else if (symbol_ == Symbol::Next) {
            painter.drawPolyline(QPolygonF({ { width() * 0.38, height() * 0.34 },
                                             { width() * 0.58, height() * 0.50 },
                                             { width() * 0.38, height() * 0.66 } }));
        } else if (symbol_ == Symbol::Minimize) {
            painter.drawLine(center + QPointF(-6, 0), center + QPointF(6, 0));
        } else if (symbol_ == Symbol::FullScreen) {
            painter.drawRect(QRectF(center.x() - 6, center.y() - 5, 12, 10));
        } else {
            painter.drawLine(center + QPointF(-5, -5), center + QPointF(5, 5));
            painter.drawLine(center + QPointF(5, -5), center + QPointF(-5, 5));
        }
    }

private:
    Symbol symbol_;
};
}

ImageViewerWindow::ImageViewerWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle(tr("Image viewer"));
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
    setContentsMargins(0, 0, 0, 0);

    previousButton_ = new ViewerButton(ViewerButton::Symbol::Previous, this);
    previousButton_->setFixedSize(54, 86);
    nextButton_ = new ViewerButton(ViewerButton::Symbol::Next, this);
    nextButton_->setFixedSize(54, 86);
    minimizeButton_ = new ViewerButton(ViewerButton::Symbol::Minimize, this);
    minimizeButton_->setFixedSize(42, 36);
    minimizeButton_->setToolTip(tr("Minimize"));
    fullScreenButton_ = new ViewerButton(ViewerButton::Symbol::FullScreen, this);
    fullScreenButton_->setFixedSize(42, 36);
    fullScreenButton_->setToolTip(tr("Full screen"));
    closeButton_ = new ViewerButton(ViewerButton::Symbol::Close, this);
    closeButton_->setFixedSize(42, 36);
    closeButton_->setToolTip(tr("Close"));

    errorLabel_ = new QLabel(this);
    errorLabel_->setAlignment(Qt::AlignCenter);
    errorLabel_->setStyleSheet(QStringLiteral("color: #f0f2f4; font-size: 16px; background: transparent;"));
    fileNameLabel_ = new ElidedLabel(this);
    fileNameLabel_->setAlignment(Qt::AlignCenter);
    fileNameLabel_->setStyleSheet(QStringLiteral("color: #f2f4f6; font-size: 13px; background: transparent;"));

    loadingIndicator_ = new ViewerBusyIndicator(this);
    loadingIndicatorDelayTimer_ = new QTimer(this);
    loadingIndicatorDelayTimer_->setSingleShot(true);
    loadingIndicatorDelayTimer_->setInterval(400);
    connect(loadingIndicatorDelayTimer_, &QTimer::timeout, this, [this] {
        if (loading_) {
            loadingIndicator_->setRunning(true);
        }
    });

    connect(previousButton_, &QAbstractButton::clicked, this, &ImageViewerWindow::showPrevious);
    connect(nextButton_, &QAbstractButton::clicked, this, &ImageViewerWindow::showNext);
    connect(minimizeButton_, &QAbstractButton::clicked, this, &ImageViewerWindow::minimizeViewer);
    connect(fullScreenButton_, &QAbstractButton::clicked, this, &ImageViewerWindow::toggleFullScreen);
    connect(closeButton_, &QAbstractButton::clicked, this, &ImageViewerWindow::closeViewer);
    updateControls();
}

ImageViewerWindow::~ImageViewerWindow() = default;

void ImageViewerWindow::setImageViewerSource(std::unique_ptr<IImageViewerSource> source) {
    hide();
    setUpdatesEnabled(false);
    openWhenReady_ = false;
    image_ = { };
    source_ = std::move(source);
    loadFile(source_ ? source_->currentFile() : QString { });
}

void ImageViewerWindow::setTransientParent(QWindow* parent) {
    winId();
    windowHandle()->setTransientParent(parent);
}

void ImageViewerWindow::open() {
    openWhenReady_ = true;
    if (!image_.isNull()) {
        showViewer();
    }
}

void ImageViewerWindow::showNext() {
    if (source_ && source_->hasNext()) {
        loadFile(source_->nextFile());
    }
}

void ImageViewerWindow::showPrevious() {
    if (source_ && source_->hasPrevious()) {
        loadFile(source_->previousFile());
    }
}

void ImageViewerWindow::minimizeViewer() { showMinimized(); }

void ImageViewerWindow::toggleFullScreen() {
    QScreen* targetScreen = viewerScreen();
    if (!targetScreen) {
        return;
    }
    fillsScreen_ = !fillsScreen_;
    setViewerGeometry(fillsScreen_ ? targetScreen->geometry() : targetScreen->availableGeometry());
}

void ImageViewerWindow::closeViewer() {
    hide();
    openWhenReady_ = false;
    ++loadRequest_;
    movie_.reset();
    loading_ = false;
    loadingIndicatorDelayTimer_->stop();
    loadingIndicator_->setRunning(false);
    image_ = { };
    fileName_.clear();
    errorLabel_->clear();
    updateControls();
    update();
}

void ImageViewerWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
    closeViewer();
}

void ImageViewerWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Left) {
        showPrevious();
    } else if (event->key() == Qt::Key_Right) {
        showNext();
    } else if (event->key() == Qt::Key_Escape) {
        closeViewer();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void ImageViewerWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !imageRect().contains(event->position().toPoint())) {
        closeViewer();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ImageViewerWindow::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(18, 21, 25, 181));
    if (!image_.isNull()) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(imageRect(), image_);
    }
}

void ImageViewerWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    previousButton_->move(20, (height() - previousButton_->height()) / 2);
    nextButton_->move(width() - 20 - nextButton_->width(), (height() - nextButton_->height()) / 2);
    closeButton_->move(width() - 12 - closeButton_->width(), 12);
    fullScreenButton_->move(closeButton_->x() - 4 - fullScreenButton_->width(), 12);
    minimizeButton_->move(fullScreenButton_->x() - 4 - minimizeButton_->width(), 12);
    errorLabel_->setGeometry(92, (height() - 40) / 2, std::max(0, width() - 184), 40);
    fileNameLabel_->setGeometry(90, height() - 50, std::max(0, width() - 180), 28);
    loadingIndicator_->move((width() - loadingIndicator_->width()) / 2, (height() - loadingIndicator_->height()) / 2);
}

void ImageViewerWindow::loadFile(const QString& fileName) {
    const quint64 request = ++loadRequest_;
    movie_.reset();
    loadingIndicatorDelayTimer_->stop();
    loadingIndicator_->setRunning(false);
    fileName_ = QFileInfo(fileName).fileName();
    loading_ = !fileName.isEmpty();
    errorLabel_->clear();
    updateControls();
    update();

    if (fileName.isEmpty()) {
        loading_ = false;
        errorLabel_->setText(tr("Image is unavailable"));
        updateControls();
        return;
    }

    QImageReader animationReader(fileName);
    if (animationReader.supportsAnimation()) {
        auto movie = std::make_unique<QMovie>(fileName);
        if (movie->isValid()) {
            auto frameLoaded = std::make_shared<bool>(false);
            connect(movie.get(), &QMovie::frameChanged, this, [this, request, frameLoaded](int) {
                if (request != loadRequest_ || !movie_) {
                    return;
                }
                const QImage frame = movie_->currentImage();
                if (frame.isNull()) {
                    return;
                }
                *frameLoaded = true;
                image_ = frame;
                if (loading_) {
                    loading_ = false;
                    updateControls();
                    if (openWhenReady_) {
                        showViewer();
                    }
                }
                update();
            });
            connect(movie.get(), &QMovie::error, this, [this, request, frameLoaded](QImageReader::ImageReaderError) {
                if (request != loadRequest_ || *frameLoaded) {
                    return;
                }
                loading_ = false;
                errorLabel_->setText(tr("Unable to load image"));
                updateControls();
                if (openWhenReady_) {
                    showViewer();
                }
            });
            movie_ = std::move(movie);
            movie_->start();
            return;
        }
    }

    auto* watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, request] {
        const QImage image = watcher->result();
        watcher->deleteLater();
        if (request != loadRequest_) {
            return;
        }
        loading_ = false;
        if (image.isNull()) {
            errorLabel_->setText(tr("Unable to load image"));
        } else {
            image_ = image;
        }
        updateControls();
        update();
        if (openWhenReady_) {
            showViewer();
        }
    });
    watcher->setFuture(QtConcurrent::run([fileName] {
        QImageReader reader(fileName);
        reader.setAutoTransform(true);
        return reader.read();
    }));
}

void ImageViewerWindow::showViewer() {
    if (image_.isNull()) {
        return;
    }
    openWhenReady_ = false;
    fillsScreen_ = true;
    setWindowState(Qt::WindowNoState);
    if (QScreen* targetScreen = viewerScreen()) {
        setViewerGeometry(targetScreen->geometry());
    }
    setUpdatesEnabled(true);
    show();
    repaint();
    raise();
    activateWindow();
    setFocus(Qt::ActiveWindowFocusReason);
}

void ImageViewerWindow::setViewerGeometry(const QRect& geometry) {
    if (!geometry.isValid()) {
        return;
    }
    setMask(QRegion(QRect(QPoint(), geometry.size())));
    setGeometry(geometry);
}

QScreen* ImageViewerWindow::viewerScreen() const {
    if (windowHandle() && windowHandle()->transientParent() && windowHandle()->transientParent()->screen()) {
        return windowHandle()->transientParent()->screen();
    }
    return screen();
}

void ImageViewerWindow::updateControls() {
    previousButton_->setEnabled(source_ && source_->hasPrevious());
    nextButton_->setEnabled(source_ && source_->hasNext());
    if (loading_) {
        if (loadingIndicator_->isHidden() && !loadingIndicatorDelayTimer_->isActive()) {
            loadingIndicatorDelayTimer_->start();
        }
    } else {
        loadingIndicatorDelayTimer_->stop();
        loadingIndicator_->setRunning(false);
    }
    errorLabel_->setVisible(!loading_ && !errorLabel_->text().isEmpty());
    fileNameLabel_->setText(fileName_);
}

QRect ImageViewerWindow::imageRect() const {
    if (image_.isNull()) {
        return { };
    }
    const QSize availableSize(std::max(0, width() - 184), std::max(0, height() - 130));
    QSize displayedSize = image_.size();
    if (displayedSize.width() > availableSize.width() || displayedSize.height() > availableSize.height()) {
        displayedSize.scale(availableSize, Qt::KeepAspectRatio);
    }
    return QRect(QPoint((width() - displayedSize.width()) / 2, (height() - displayedSize.height()) / 2), displayedSize);
}
