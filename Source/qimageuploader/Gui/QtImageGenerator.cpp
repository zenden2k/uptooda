#include "QtImageGenerator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QPen>
#include <QtConcurrentRun>

#include <limits>
#include <utility>

#include "../../MediaInfo/MediaInfoHelper.h"
#include "Core/CommonDefs.h"
#include "Helpers.h"

namespace {

constexpr int LABEL_FONT_SIZE = 14;
constexpr int INFO_FONT_SIZE = 12;
constexpr int INFO_TO_FRAMES_GAP = 3;

QFont mediaInfoFont(const std::string& serializedFont) {
    const QStringList parts = QString::fromStdString(serializedFont).split(QLatin1Char(','));
    QFont font(parts.value(0, QStringLiteral("Tahoma")));
    bool validSize = false;
    const int size = parts.value(1).toInt(&validSize);
    font.setPixelSize(validSize && size > 0 ? size : INFO_FONT_SIZE);
    font.setBold(parts.value(2).contains(QLatin1Char('b'), Qt::CaseInsensitive));
    return font;
}

QColor colorFromRgbValue(uint32_t color) {
    return QColor::fromRgb(color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff);
}

QImage loadImage(const QString& fileName) {
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    return reader.read();
}

void drawImageTitle(QPainter& painter, const QRect& tileRect, const QString& title) {
    if (title.isEmpty()) {
        return;
    }

    QFont font(QStringLiteral("Tahoma"));
    font.setPixelSize(LABEL_FONT_SIZE);
    font.setBold(true);

    const QFontMetrics metrics(font);
    const int x = tileRect.right() - metrics.horizontalAdvance(title) - 2;
    const int baseline = tileRect.bottom() - metrics.descent() - 2;

    painter.save();
    painter.setFont(font);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setPen(QColor(0, 0, 0, 200));
    painter.drawText(x + 1, baseline + 1, title);
    painter.setPen(Qt::white);
    painter.drawText(x, baseline, title);
    painter.restore();
}

} // namespace

QtImageGenerator::QtImageGenerator(QVector<FileItem> files, QString mediaFile, VideoSettingsStruct videoSettings,
                                   Options options, QObject* parent) :
    QObject(parent), files_(std::move(files)), mediaFile_(std::move(mediaFile)),
    videoSettings_(std::move(videoSettings)), options_(std::move(options)) {
    connect(&watcher_, &QFutureWatcher<GenerationResult>::finished, this, [this] {
        const GenerationResult result = watcher_.result();
        emit finished(result.Success, result.Canceled, result.OutputFileName, result.ErrorMessage);
    });
}

QtImageGenerator::~QtImageGenerator() {
    cancel();
    watcher_.waitForFinished();
}

void QtImageGenerator::start() {
    if (watcher_.isRunning()) {
        return;
    }
    canceled_.storeRelease(0);
    watcher_.setFuture(QtConcurrent::run([this] { return generate(); }));
}

void QtImageGenerator::cancel() { canceled_.storeRelease(1); }

QtImageGenerator::GenerationResult QtImageGenerator::generate() {
    if (files_.isEmpty()) {
        return { false, false, { }, tr("There are no frames to include in the mosaic.") };
    }

    const int maximum = files_.size();
    emit progressChanged(0, maximum);

    QString mediaInfo;
#ifdef IU_ENABLE_MEDIAINFO
    if (videoSettings_.ShowMediaInfo && !mediaFile_.isEmpty()) {
        std::string summary;
        std::string fullInfo;
        MediaInfoHelper::GetMediaFileInfo(Q2U(mediaFile_), summary, fullInfo, options_.EnableMediaInfoLocalization);
        mediaInfo = U2Q(summary);
    }
#endif
    if (isCanceled()) {
        return { false, true, { }, { } };
    }

    QImage referenceImage;
    for (const FileItem& file : files_) {
        referenceImage = loadImage(file.FileName);
        if (!referenceImage.isNull()) {
            break;
        }
    }
    if (referenceImage.isNull()) {
        return { false, false, { }, tr("None of the selected frame images could be loaded.") };
    }

    const int columns = qMax(1, qMin(videoSettings_.Columns, maximum));
    const int rows = (maximum + columns - 1) / columns;
    const int tileWidth = qMax(1, videoSettings_.TileWidth);
    const int tileHeight
        = qMax(1, qRound(static_cast<qreal>(tileWidth) * referenceImage.height() / referenceImage.width()));
    const int gapWidth = qMax(0, videoSettings_.GapWidth);
    const int gapHeight = qMax(0, videoSettings_.GapHeight);
    const int mosaicWidth = gapWidth + columns * (tileWidth + gapWidth);

    const QFont infoFont = mediaInfoFont(videoSettings_.Font);
    const QFontMetrics infoMetrics(infoFont);
    const int infoTextWidth = qMax(1, mosaicWidth - gapWidth * 2);
    const int infoTextHeight = mediaInfo.isEmpty()
        ? 0
        : infoMetrics.boundingRect(QRect(0, 0, infoTextWidth, 100000), Qt::TextWordWrap, mediaInfo).height();
    const int framesTop = infoTextHeight > 0 ? gapHeight + infoTextHeight + INFO_TO_FRAMES_GAP : gapHeight;
    const qint64 mosaicHeight64 = static_cast<qint64>(framesTop) + static_cast<qint64>(rows) * (tileHeight + gapHeight);
    if (mosaicWidth <= 0 || mosaicHeight64 <= 0 || mosaicHeight64 > std::numeric_limits<int>::max()) {
        return { false, false, { }, tr("The resulting mosaic is too large.") };
    }

    QImage mosaic(QSize(mosaicWidth, static_cast<int>(mosaicHeight64)), QImage::Format_ARGB32_Premultiplied);
    if (mosaic.isNull()) {
        return { false, false, { }, tr("Unable to allocate the resulting mosaic image.") };
    }

    QPainter painter(&mosaic);
    QLinearGradient background(mosaic.rect().topRight(), mosaic.rect().bottomLeft());
    background.setColorAt(0.0, QColor(224, 224, 224));
    background.setColorAt(1.0, QColor(243, 243, 243));
    painter.fillRect(mosaic.rect(), background);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!mediaInfo.isEmpty()) {
        painter.save();
        painter.setFont(infoFont);
        painter.setPen(colorFromRgbValue(videoSettings_.TextColor));
        painter.drawText(QRect(gapWidth, gapHeight, infoTextWidth, infoTextHeight), Qt::TextWordWrap, mediaInfo);
        painter.restore();
    }

    const QPen framePen(QColor(90, 90, 90));
    for (int index = 0; index < maximum; ++index) {
        if (isCanceled()) {
            painter.end();
            return { false, true, { }, { } };
        }

        const QImage image = loadImage(files_[index].FileName);
        if (image.isNull()) {
            painter.end();
            return { false, false, { }, tr("Failed to load frame: %1").arg(files_[index].FileName) };
        }

        const int x = gapWidth + (index % columns) * (tileWidth + gapWidth);
        const int y = framesTop + (index / columns) * (tileHeight + gapHeight);
        const QRect tileRect(x, y, tileWidth, tileHeight);
        painter.drawImage(tileRect, image);
        drawImageTitle(painter, tileRect, files_[index].Title);
        painter.setPen(framePen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(tileRect.adjusted(0, 0, -1, -1));
        emit progressChanged(index + 1, maximum);
    }
    painter.end();

    if (isCanceled()) {
        return { false, true, { }, { } };
    }

    const QString templatePath = QDir::fromNativeSeparators(QString::fromUtf8(videoSettings_.SnapshotFileTemplate));
    const QFileInfo templateInfo(templatePath);
    QString templateWithoutExtension = templateInfo.completeBaseName();
    if (templateInfo.path() != QStringLiteral(".")) {
        templateWithoutExtension = QDir(templateInfo.path()).filePath(templateWithoutExtension);
    }

    const QString relativeFileName = Helpers::GenerateFileNameFromTemplate(
                                         templateWithoutExtension, 1, referenceImage.size(), mediaFile_, tr("Mosaic"))
        + QStringLiteral(".png");
    QString baseDirectory = QDir::fromNativeSeparators(QString::fromUtf8(videoSettings_.SnapshotsFolder));
    if (baseDirectory.isEmpty()) {
        baseDirectory = options_.OutputDirectory;
    }

    QString outputFileName = QDir(baseDirectory).filePath(relativeFileName);
    if (!QDir().mkpath(QFileInfo(outputFileName).absolutePath()) && baseDirectory != options_.OutputDirectory) {
        baseDirectory = options_.OutputDirectory;
        outputFileName = QDir(baseDirectory).filePath(relativeFileName);
    }
    if (!QDir().mkpath(QFileInfo(outputFileName).absolutePath())) {
        return { false, false, { }, tr("Unable to create the output directory.") };
    }
    outputFileName = Helpers::MakeUniqueFileName(outputFileName);

    if (!mosaic.save(outputFileName, "PNG")) {
        QFile::remove(outputFileName);
        return { false, false, { }, tr("Unable to save the mosaic image.") };
    }
    if (isCanceled()) {
        QFile::remove(outputFileName);
        return { false, true, { }, { } };
    }
    return { true, false, outputFileName, { } };
}

bool QtImageGenerator::isCanceled() const { return canceled_.loadAcquire() != 0; }
