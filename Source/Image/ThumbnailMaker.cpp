#include "ThumbnailMaker.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickGraphicsDevice>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QUrl>

namespace {

class ThumbnailImageProvider final : public QQuickImageProvider {
public:
    explicit ThumbnailImageProvider(QImage image)
        : QQuickImageProvider(QQuickImageProvider::Image)
        , image_(std::move(image)) {
    }

    QImage requestImage(const QString&, QSize* size, const QSize& requestedSize) override {
        if (size) {
            *size = image_.size();
        }

        if (requestedSize.isValid()) {
            return image_.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        return image_;
    }

private:
    QImage image_;
};

QString humanFileSize(qsizetype bytes) {
    static constexpr double kStep = 1024.0;
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    double value = static_cast<double>(bytes);
    int unit = 0;

    while (value >= kStep && unit < 4) {
        value /= kStep;
        ++unit;
    }

    return unit == 0
        ? QStringLiteral("%1 %2").arg(bytes).arg(units[unit])
        : QStringLiteral("%1 %2").arg(value, 0, 'f', value < 10.0 ? 1 : 0).arg(units[unit]);
}

QSize readTemplateAdditions(QQuickItem* rootItem) {
    return QSize(rootItem->property("addWidth").toInt(), rootItem->property("addHeight").toInt());
}

} // namespace

ThumbnailMaker::ThumbnailMaker() = default;

ThumbnailMaker::~ThumbnailMaker() = default;

QImage ThumbnailMaker::createThumbnail(const QImage& source, const Options& options) {
    if (source.isNull() || options.templateFile.isEmpty() || !qGuiApp) {
        return {};
    }

    QQmlEngine probeEngine;
    probeEngine.addImageProvider(QStringLiteral("thumbnail"), new ThumbnailImageProvider(source));
    QQmlComponent probeComponent(&probeEngine, QUrl::fromLocalFile(options.templateFile));
    auto probeObject = std::unique_ptr<QObject>(probeComponent.create());
    auto* probeItem = qobject_cast<QQuickItem*>(probeObject.get());

    if (!probeItem) {
        return {};
    }

    probeItem->setProperty("drawText", options.drawText);
    probeItem->setProperty("drawFrame", options.drawFrame);
    const QSize additions = readTemplateAdditions(probeItem);
    const QSize imageSize = calculateImageSize(source, options, additions);
    const QSize outputSize(imageSize.width() + additions.width(), imageSize.height() + additions.height());

    if (outputSize.isEmpty()) {
        return {};
    }

    return renderQml(source, options, imageSize, outputSize);
}

QSize ThumbnailMaker::calculateImageSize(const QImage& source, const Options& options, const QSize& additions) const {
    const int sourceWidth = source.width();
    const int sourceHeight = source.height();
    int width = std::max(1, options.targetWidth);
    int height = std::max(1, options.targetHeight);

    if (options.resizeMode == ResizeMode::ByWidth) {
        width = std::max(10, width - additions.width());
        height = std::max(1, qRound(static_cast<double>(width) / sourceWidth * sourceHeight));
    } else if (options.resizeMode == ResizeMode::ByHeight) {
        height = std::max(10, height - additions.height());
        width = std::max(1, qRound(static_cast<double>(height) / sourceHeight * sourceWidth));
    } else {
        width = std::max(10, width - additions.width());
        height = std::max(10, height - additions.height());
    }

    return QSize(width, height);
}

QImage ThumbnailMaker::renderQml(const QImage& source, const Options& options, const QSize& imageSize,
    const QSize& outputSize) {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QOpenGLContext context;
    if (!context.create()) {
        return {};
    }

    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();

    if (!surface.isValid() || !context.makeCurrent(&surface)) {
        return {};
    }

    QQuickRenderControl renderControl;
    QQuickWindow window(&renderControl);
    window.setColor(Qt::transparent);
    window.setGeometry(0, 0, outputSize.width(), outputSize.height());
    window.setGraphicsDevice(QQuickGraphicsDevice::fromOpenGLContext(&context));

    renderControl.initialize();

    QQmlEngine engine;
    engine.addImageProvider(QStringLiteral("thumbnail"), new ThumbnailImageProvider(source));
    QQmlComponent component(&engine, QUrl::fromLocalFile(options.templateFile));
    auto rootObject = std::unique_ptr<QObject>(component.create());
    auto* rootItem = qobject_cast<QQuickItem*>(rootObject.get());

    if (!rootItem) {
        context.doneCurrent();
        return {};
    }

    QString text = options.text;
    text.replace(QStringLiteral("%width%"), QString::number(source.width()));
    text.replace(QStringLiteral("%height%"), QString::number(source.height()));
    text.replace(QStringLiteral("%size%"), humanFileSize(options.fileSize));

    rootItem->setProperty("sourceWidth", source.width());
    rootItem->setProperty("sourceHeight", source.height());
    rootItem->setProperty("thumbnailImageWidth", imageSize.width());
    rootItem->setProperty("thumbnailImageHeight", imageSize.height());
    rootItem->setProperty("userText", text);
    rootItem->setProperty("drawText", options.drawText);
    rootItem->setProperty("drawFrame", options.drawFrame);
    rootItem->setProperty("backgroundColor", options.backgroundColor);
    rootItem->setSize(outputSize);
    rootItem->setParentItem(window.contentItem());

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    QOpenGLFramebufferObject fbo(outputSize, fboFormat);
    fbo.bind();
    context.functions()->glViewport(0, 0, outputSize.width(), outputSize.height());
    context.functions()->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    context.functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    window.setRenderTarget(QQuickRenderTarget::fromOpenGLTexture(fbo.texture(), outputSize));

    renderControl.polishItems();
    renderControl.beginFrame();
    renderControl.sync();
    renderControl.render();
    renderControl.endFrame();

    context.functions()->glFlush();
    QImage result = fbo.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);

    context.doneCurrent();
    return result;
}
