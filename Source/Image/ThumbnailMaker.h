#pragma once

#include <QColor>
#include <QImage>
#include <QSize>
#include <QString>

class ThumbnailMaker {
public:
    enum class ResizeMode {
        ByWidth,
        ByHeight,
        ByBoth
    };

    struct Options {
        QString templateFile;
        QString text;
        qsizetype fileSize = 0;
        int targetWidth = 180;
        int targetHeight = 0;
        ResizeMode resizeMode = ResizeMode::ByWidth;
        bool drawText = true;
        bool drawFrame = true;
        QColor backgroundColor = Qt::transparent;
    };

    ThumbnailMaker();
    ~ThumbnailMaker();

    QImage createThumbnail(const QImage& source, const Options& options);

private:
    QSize calculateImageSize(const QImage& source, const Options& options, const QSize& additions) const;
    QImage renderQml(const QImage& source, const Options& options, const QSize& imageSize,
        const QSize& outputSize);
};
