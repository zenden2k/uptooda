#include "MediaDlg.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFutureWatcher>
#include <QImageWriter>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QResizeEvent>
#include <QtConcurrentRun>

#include "../../MediaInfo/MediaInfoHelper.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/CommonDefs.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/QtGuiSettings.h"
#include "Core/Video/VideoUtils.h"
#include "Gui/QtImageGenerator.h"
#include "Gui/VirtualFileDrop.h"
#include "Gui/controls/ThumbnailListView.h"
#include "Gui/models/ThumbnailListModel.h"
#include "Helpers.h"
#include "Video/QtImage.h"
#include "Video/VideoGrabber.h"
#include "ui_MediaDlg.h"

Q_DECLARE_METATYPE(AbstractImage*)

class MediaDropHighlight final : public QWidget {
public:
    explicit MediaDropHighlight(QWidget* parent = nullptr) : QWidget(parent) { }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(232, 246, 255, 235));

        const QRectF borderRect = QRectF(rect()).adjusted(10.5, 10.5, -10.5, -10.5);
        QPen borderPen(QColor(QStringLiteral("#399bd8")), 3, Qt::DashLine);
        borderPen.setDashPattern({ 7, 5 });
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(borderRect, 12, 12);

        QFont font = painter.font();
        font.setPointSize(qMax(20, font.pointSize() + 10));
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(QColor(QStringLiteral("#197db8")));
        painter.drawText(rect().adjusted(24, 24, -24, -24), Qt::AlignCenter | Qt::TextWordWrap,
                         QCoreApplication::translate("MediaDlg", "Drop a media file"));
    }
};

namespace {

struct MediaInfoResult {
    std::string Summary;
    std::string FullInfo;
};

QStringList LocalFilesFromMimeData(const QMimeData* mimeData) {
    QStringList result;
    if (!mimeData || !mimeData->hasUrls()) {
        return result;
    }
    for (const QUrl& url : mimeData->urls()) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QString fileName = QFileInfo(url.toLocalFile()).absoluteFilePath();
        if (QFileInfo(fileName).isFile()) {
            result.append(fileName);
        }
    }
    return result;
}

bool IsThumbnailListInternalDrag(const QMimeData* mimeData) {
    return mimeData && mimeData->hasFormat(ThumbnailListModel::internalMimeType());
}

} // namespace

MediaDlg::MediaDlg(QString fileName, QWidget* parent, bool showMediaInfo) : QDialog(parent), ui(new Ui::MediaDlg) {
    qRegisterMetaType<AbstractImage*>("AbstractImage*");
    auto settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    setAcceptDrops(true);

    dropHighlightOverlay_ = new MediaDropHighlight(this);
    dropHighlightOverlay_->setAttribute(Qt::WA_TransparentForMouseEvents);
    dropHighlightOverlay_->setGeometry(rect());
    dropHighlightOverlay_->hide();

    resize(820, 560);
    setMinimumSize(720, 500);
    ui->verticalLayout->setContentsMargins(20, 20, 20, 18);
    ui->verticalLayout->setSpacing(14);
    ui->horizontalLayout->setSpacing(8);
    ui->horizontalLayout_2->setSpacing(10);
    ui->lineEdit->setMinimumHeight(40);
    ui->browseButton->setFixedSize(42, 40);
    ui->grabButton->setMinimumHeight(40);
    ui->numOfFramesSpinBox->setFixedSize(122, 40);
    ui->numOfFramesSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->numOfFramesSpinBox->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    ui->comboBox->setFixedSize(190, 40);
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    ui->numOfFramesSpinBox->setValue(settings->VideoSettings.NumOfFrames);
    ui->stopButton->setVisible(false);
    ui->cancelMosaicButton->setIcon(QIcon(QStringLiteral(":/res/cancel.svg")));
    ui->cancelMosaicButton->setIconSize(QSize(16, 16));
    ui->mosaicProgressBar->setVisible(false);
    ui->cancelMosaicButton->setVisible(false);

    ui->tabBar->setDrawBase(false);
    ui->tabBar->setExpanding(false);
    ui->tabBar->setUsesScrollButtons(false);
    ui->tabBar->setElideMode(Qt::ElideNone);
    ui->tabBar->addTab(tr("Extract frames"));
    ui->tabBar->addTab(tr("Media information"));
    ui->mediaInfoEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    for (const auto& engine : CommonGuiSettings::VideoEngines) {
        QString name = QString::fromStdString(engine);
        ui->comboBox->addItem(name, QVariant(name));
    }

    int index = ui->comboBox->findData(QString::fromStdString(settings->VideoSettings.Engine));
    if (index != -1) {
        ui->comboBox->setCurrentIndex(index);
    }

    ui->progressRing->hide();
    frameModel_ = std::make_unique<ThumbnailListModel>(this);
    ui->listWidget->setModel(frameModel_.get());
    ui->listWidget->setEmptyText(tr("Extracted frames will appear here"));

    connect(ui->tabBar, &QTabBar::currentChanged, this, &MediaDlg::onCurrentTabChanged);
    connect(ui->lineEdit, &QLineEdit::editingFinished, this, [this] {
        const QString fileName = QDir::fromNativeSeparators(ui->lineEdit->text());
        if (fileName != fileName_) {
            setFileName(fileName);
        }
    });
    connect(ui->stopButton, &QPushButton::clicked, this, &MediaDlg::onStopButtonClicked);
    connect(ui->createMosaicButton, &QPushButton::clicked, this, &MediaDlg::createMosaic);
    connect(ui->cancelMosaicButton, &QToolButton::clicked, this, &MediaDlg::cancelMosaic);
    connect(ui->listWidget, &ThumbnailListView::removeRequested, frameModel_.get(), &ThumbnailListModel::removeItems);
    connect(frameModel_.get(), &QAbstractItemModel::rowsInserted, this, &MediaDlg::updateMosaicControls);
    connect(frameModel_.get(), &QAbstractItemModel::rowsRemoved, this, &MediaDlg::updateMosaicControls);
    connect(frameModel_.get(), &QAbstractItemModel::modelReset, this, &MediaDlg::updateMosaicControls);
    connect(ui->summaryRadioButton, &QRadioButton::toggled, this, &MediaDlg::updateMediaInfoText);
    connect(ui->fullInfoRadioButton, &QRadioButton::toggled, this, &MediaDlg::updateMediaInfoText);
    connect(ui->disableLocalizationCheckBox, &QCheckBox::toggled, this, &MediaDlg::reloadMediaInfo);
    connect(ui->copyMediaInfoButton, &QPushButton::clicked, this, &MediaDlg::copyMediaInfo);
    connect(this, &MediaDlg::finished, this, &MediaDlg::onFinished);
    updateMosaicControls();

    setFileName(fileName);
    if (showMediaInfo && isVideoFile(fileName_)) {
        ui->tabBar->setCurrentIndex(MEDIA_INFO_TAB);
    }
}

MediaDlg::~MediaDlg() {
    delete mosaicGenerator_;
    delete ui;
}

void MediaDlg::frameGrabbed(const std::string& timeString, int64_t time, const std::shared_ptr<AbstractImage>& image) {
    Q_UNUSED(time);
    if (!image) {
        return;
    }
    QString timeStringQt = U2Q(timeString);
    auto qtImage = dynamic_cast<QtImage*>(image.get());
    if (!qtImage) {
        return;
    }
    QImage img = qtImage->toQImage();

    if (!img.isNull()) {
        const auto settings = ServiceLocator::instance()->settings<QtGuiSettings>();
        const QString templatePath
            = QDir::fromNativeSeparators(QString::fromUtf8(settings->VideoSettings.SnapshotFileTemplate));
        const QFileInfo templateInfo(templatePath);
        QString templateWithoutExtension = templateInfo.completeBaseName();
        if (templateInfo.path() != QStringLiteral(".")) {
            templateWithoutExtension = QDir(templateInfo.path()).filePath(templateWithoutExtension);
        }

        const QString relativeFileName
            = Helpers::GenerateFileNameFromTemplate(templateWithoutExtension, grabbedFramesCount_ + 1, img.size(),
                                                    fileName_, tr("Frame"))
            + QStringLiteral(".png");
        QString baseDirectory = QDir::fromNativeSeparators(QString::fromUtf8(settings->VideoSettings.SnapshotsFolder));
        if (baseDirectory.isEmpty()) {
            baseDirectory = U2Q(AppRuntimeInfo::instance()->tempDirectory());
        }

        QString outputFileName = QDir(baseDirectory).filePath(relativeFileName);
        if (!QDir().mkpath(QFileInfo(outputFileName).absolutePath())) {
            baseDirectory = U2Q(AppRuntimeInfo::instance()->tempDirectory());
            outputFileName = QDir(baseDirectory).filePath(relativeFileName);
        }

        if (QDir().mkpath(QFileInfo(outputFileName).absolutePath())) {
            outputFileName = Helpers::MakeUniqueFileName(outputFileName);
            QImageWriter writer(outputFileName, "png");
            writer.setCompression(1);
            writer.setQuality(10);

            if (writer.write(img)) {
                ++grabbedFramesCount_;
                QMetaObject::invokeMethod(this, "frameGrabbedSlot", Qt::BlockingQueuedConnection,
                                          Q_ARG(QString, timeStringQt), Q_ARG(QString, outputFileName),
                                          Q_ARG(QImage, img));
            }
        }
    }
}

void MediaDlg::frameGrabbedSlot(const QString& timeString, const QString& fileName, const QImage& image) {
    QIcon icon(QPixmap::fromImage(image));
    frameModel_->addFile(fileName, timeString, icon);
}

void MediaDlg::on_grabButton_clicked() {
    grabInProgress_ = true;
    grabCanceled_ = false;
    grabbedFramesCount_ = 0;
    ui->grabButton->setEnabled(false);
    ui->buttonBox->setEnabled(false);
    ui->stopButton->setVisible(true);
    ui->browseButton->setEnabled(false);
    ui->lineEdit->setEnabled(false);
    ui->numOfFramesSpinBox->setEnabled(false);
    ui->comboBox->setEnabled(false);
    updateMosaicControls();

    grabber_ = std::make_unique<VideoGrabber>();
    grabber_->setVideoEngine(getVideoEngine());

    using namespace std::placeholders;
    grabber_->setOnFrameGrabbed([this](auto&& timeString, auto&& time, auto&& image) {
        frameGrabbed(std::forward<decltype(timeString)>(timeString), time, std::forward<decltype(image)>(image));
    });
    grabber_->setOnFinished([this](bool success) { onGrabFinished(success); });

    int frameCount = ui->numOfFramesSpinBox->value();
    if (frameCount < 1) {
        frameCount = 10;
    }
    grabber_->setFrameCount(frameCount);
    ui->progressRing->show();
    grabber_->grab(Q2U(fileName_));
}

void MediaDlg::on_browseButton_clicked() {
    const QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) {
        setFileName(fileName);
    }
}

void MediaDlg::onGrabFinished(bool success) {
    QMetaObject::invokeMethod(this, "grabFinishedSlot", Qt::QueuedConnection, Q_ARG(bool, success));
}

void MediaDlg::grabFinishedSlot(bool success) {
    grabInProgress_ = false;
    ui->grabButton->setEnabled(true);
    ui->buttonBox->setEnabled(true);
    ui->stopButton->setVisible(false);
    ui->browseButton->setEnabled(true);
    ui->lineEdit->setEnabled(true);
    ui->numOfFramesSpinBox->setEnabled(true);
    ui->comboBox->setEnabled(true);
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    ui->progressRing->hide();
    updateMosaicControls();

    if (!success && !grabCanceled_) {
        QMessageBox::warning(this, tr("Frame extraction failed"),
                             tr("Failed to extract frames from the selected file."));
    }
}

void MediaDlg::onStopButtonClicked() {
    if (grabber_) {
        grabCanceled_ = true;
        grabber_->abort();
    }
}

void MediaDlg::createMosaic() {
    if (mosaicInProgress_) {
        return;
    }

    QVector<QtImageGenerator::FileItem> files;
    for (int row = 0; row < frameModel_->rowCount(); ++row) {
        if (!frameModel_->isGeneratedMosaic(row)) {
            files.append({ frameModel_->filePath(row), frameModel_->displayText(row) });
        }
    }
    if (files.isEmpty()) {
        return;
    }

    QtImageGenerator::Options options;
    options.EnableMediaInfoLocalization = !ui->disableLocalizationCheckBox->isChecked();
    options.OutputDirectory = U2Q(AppRuntimeInfo::instance()->tempDirectory());

    const int sourceCount = files.size();
    auto settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    mosaicGenerator_
        = new QtImageGenerator(std::move(files), fileName_, settings->VideoSettings, std::move(options), this);
    connect(mosaicGenerator_, &QtImageGenerator::progressChanged, this, [this](int value, int maximum) {
        ui->mosaicProgressBar->setRange(0, maximum);
        ui->mosaicProgressBar->setValue(value);
    });
    connect(mosaicGenerator_, &QtImageGenerator::finished, this, &MediaDlg::mosaicFinished);

    mosaicInProgress_ = true;
    ui->mosaicProgressBar->setRange(0, sourceCount);
    ui->mosaicProgressBar->setValue(0);
    ui->mosaicProgressBar->setVisible(true);
    ui->cancelMosaicButton->setEnabled(true);
    ui->cancelMosaicButton->setVisible(true);
    ui->buttonBox->setEnabled(false);
    ui->grabButton->setEnabled(false);
    ui->browseButton->setEnabled(false);
    ui->lineEdit->setEnabled(false);
    ui->numOfFramesSpinBox->setEnabled(false);
    ui->comboBox->setEnabled(false);
    ui->listWidget->setEnabled(false);
    updateMosaicControls();
    mosaicGenerator_->start();
}

void MediaDlg::cancelMosaic() {
    if (mosaicGenerator_) {
        ui->cancelMosaicButton->setEnabled(false);
        mosaicGenerator_->cancel();
    }
}

void MediaDlg::mosaicFinished(bool success, bool canceled, const QString& outputFileName, const QString& errorMessage) {
    QtImageGenerator* generator = mosaicGenerator_;
    mosaicGenerator_ = nullptr;
    mosaicInProgress_ = false;
    ui->mosaicProgressBar->setVisible(false);
    ui->cancelMosaicButton->setVisible(false);
    ui->buttonBox->setEnabled(true);
    ui->grabButton->setEnabled(true);
    ui->browseButton->setEnabled(true);
    ui->lineEdit->setEnabled(true);
    ui->numOfFramesSpinBox->setEnabled(true);
    ui->comboBox->setEnabled(true);
    ui->listWidget->setEnabled(true);

    if (success && !outputFileName.isEmpty()) {
        const int mosaicRow = frameModel_->addGeneratedMosaic(outputFileName, tr("Mosaic"), QIcon(outputFileName));
        if (mosaicRow >= 0) {
            const QModelIndex mosaicIndex = frameModel_->index(mosaicRow, 0);
            ui->listWidget->setCurrentIndex(mosaicIndex);
            ui->listWidget->selectionModel()->select(mosaicIndex, QItemSelectionModel::ClearAndSelect);
            ui->listWidget->scrollTo(mosaicIndex, QAbstractItemView::PositionAtCenter);
        }
    } else if (!canceled) {
        QMessageBox::warning(this, tr("Mosaic creation failed"), errorMessage);
    }
    updateMosaicControls();
    generator->deleteLater();
}

void MediaDlg::updateMosaicControls() {
    bool hasSourceFrames = false;
    for (int row = 0; row < frameModel_->rowCount(); ++row) {
        if (!frameModel_->isGeneratedMosaic(row)) {
            hasSourceFrames = true;
            break;
        }
    }
    const bool mediaInfoPage = ui->stackedWidget->currentIndex() == MEDIA_INFO_TAB;
    ui->createMosaicButton->setVisible(hasSourceFrames && !grabInProgress_ && !mediaInfoPage);
    ui->createMosaicButton->setEnabled(hasSourceFrames && !grabInProgress_ && !mosaicInProgress_);
    ui->copyMediaInfoButton->setVisible(mediaInfoPage);
}

void MediaDlg::getGrabbedFrames(QStringList& fileNames) const { fileNames.append(frameModel_->filePaths()); }

void MediaDlg::onFinished() {
    auto settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    settings->VideoSettings.NumOfFrames = ui->numOfFramesSpinBox->value();
    settings->VideoSettings.Engine = ui->comboBox->currentData().toString().toStdString();
}

void MediaDlg::onCurrentTabChanged(int index) {
    ui->stackedWidget->setCurrentIndex(index);
    updateMosaicControls();
    if (index == MEDIA_INFO_TAB) {
        startMediaInfoLoad();
    }
}

void MediaDlg::updateMediaInfoText() {
    if (!mediaInfoLoaded_) {
        return;
    }
    ui->mediaInfoEdit->setPlainText(ui->fullInfoRadioButton->isChecked() ? mediaInfoFull_ : mediaInfoSummary_);
}

void MediaDlg::reloadMediaInfo() {
    ++mediaInfoRequest_;
    mediaInfoLoading_ = false;
    mediaInfoLoaded_ = false;
    mediaInfoSummary_.clear();
    mediaInfoFull_.clear();
    ui->mediaInfoEdit->setPlainText(tr("Loading..."));
    startMediaInfoLoad();
}

void MediaDlg::copyMediaInfo() { QApplication::clipboard()->setText(ui->mediaInfoEdit->toPlainText()); }

void MediaDlg::closeEvent(QCloseEvent* event) {
    if (grabInProgress_ || mosaicInProgress_) {
        event->ignore();
        return;
    }
    event->accept();
    reject();
}

void MediaDlg::dragEnterEvent(QDragEnterEvent* event) {
    if (grabInProgress_ || mosaicInProgress_ || IsThumbnailListInternalDrag(event->mimeData())) {
        dragContainsFiles_ = false;
        dropHighlightOverlay_->hide();
        event->ignore();
        return;
    }

    dragContainsFiles_
        = !LocalFilesFromMimeData(event->mimeData()).isEmpty() || VirtualFileDrop::hasFiles(event->mimeData());
    if (!dragContainsFiles_) {
        event->ignore();
        return;
    }

    dropHighlightOverlay_->setGeometry(rect());
    dropHighlightOverlay_->raise();
    dropHighlightOverlay_->show();
    event->acceptProposedAction();
}

void MediaDlg::dragMoveEvent(QDragMoveEvent* event) {
    if (dragContainsFiles_) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MediaDlg::dragLeaveEvent(QDragLeaveEvent* event) {
    dragContainsFiles_ = false;
    dropHighlightOverlay_->hide();
    event->accept();
}

void MediaDlg::dropEvent(QDropEvent* event) {
    dragContainsFiles_ = false;
    dropHighlightOverlay_->hide();
    if (grabInProgress_ || mosaicInProgress_ || IsThumbnailListInternalDrag(event->mimeData())) {
        event->ignore();
        return;
    }

    QStringList fileNames = LocalFilesFromMimeData(event->mimeData());
    if (fileNames.isEmpty()) {
        fileNames = VirtualFileDrop::materializeFiles(event->mimeData());
    }
    if (fileNames.isEmpty()) {
        event->ignore();
        return;
    }

    setFileName(fileNames.first());
    event->acceptProposedAction();
}

void MediaDlg::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    if (dropHighlightOverlay_) {
        dropHighlightOverlay_->setGeometry(rect());
    }
}

void MediaDlg::reject() {
    if (!grabInProgress_ && !mosaicInProgress_) {
        QDialog::reject();
    }
}

bool MediaDlg::isVideoFile(const QString& fileName) {
    const std::string extension = QFileInfo(fileName).suffix().toLower().toStdString();
    return VideoUtils::videoFilesExtensions.find(extension) != VideoUtils::videoFilesExtensions.end();
}

void MediaDlg::setFileName(const QString& fileName) {
    const bool mediaInfoActive = ui->stackedWidget->currentIndex() == MEDIA_INFO_TAB;
    fileName_ = fileName;
    ui->lineEdit->setText(QDir::toNativeSeparators(fileName_));

    ++mediaInfoRequest_;
    mediaInfoLoading_ = false;
    mediaInfoLoaded_ = false;
    mediaInfoSummary_.clear();
    mediaInfoFull_.clear();
    ui->mediaInfoEdit->clear();

    const bool videoFile = isVideoFile(fileName_);
    ui->tabBar->setVisible(videoFile);
    if (videoFile) {
        const int targetTab = mediaInfoActive ? MEDIA_INFO_TAB : EXTRACT_FRAMES_TAB;
        ui->tabBar->setCurrentIndex(targetTab);
        ui->stackedWidget->setCurrentIndex(targetTab);
        if (mediaInfoActive) {
            startMediaInfoLoad();
        }
    } else {
        ui->stackedWidget->setCurrentIndex(MEDIA_INFO_TAB);
        startMediaInfoLoad();
    }
    updateMosaicControls();
}

void MediaDlg::startMediaInfoLoad() {
    if (ui->stackedWidget->currentIndex() != MEDIA_INFO_TAB || mediaInfoLoading_ || mediaInfoLoaded_) {
        return;
    }

    mediaInfoLoading_ = true;
    ui->mediaInfoEdit->setPlainText(tr("Loading..."));
    const uint64_t request = mediaInfoRequest_;
    const std::string fileName = Q2U(fileName_);
    const bool enableLocalization = !ui->disableLocalizationCheckBox->isChecked();
    auto* watcher = new QFutureWatcher<MediaInfoResult>(this);
    connect(watcher, &QFutureWatcher<MediaInfoResult>::finished, this, [this, watcher, request] {
        const MediaInfoResult result = watcher->result();
        watcher->deleteLater();
        if (request != mediaInfoRequest_) {
            return;
        }

        mediaInfoLoading_ = false;
        mediaInfoLoaded_ = true;
        mediaInfoSummary_ = U2Q(result.Summary);
        mediaInfoFull_ = U2Q(result.FullInfo);
        updateMediaInfoText();
    });
    watcher->setFuture(QtConcurrent::run([fileName, enableLocalization] {
        MediaInfoResult result;
        MediaInfoHelper::GetMediaFileInfo(fileName, result.Summary, result.FullInfo, enableLocalization);
        return result;
    }));
}

VideoGrabber::VideoEngine MediaDlg::getVideoEngine() const {
    std::string videoEngine = ui->comboBox->currentData().toString().toStdString();

    if (videoEngine == QtGuiSettings::VideoEngineAuto) {
        if (!QtGuiSettings::IsFFmpegAvailable()) {
            videoEngine = QtGuiSettings::VideoEngineDirectshow2;
        } else {
            videoEngine = QtGuiSettings::VideoEngineFFmpeg;
            QFileInfo info(ui->lineEdit->text());
            const QString fileExtension = info.suffix().toLower();
            if (fileExtension == "wmv" || fileExtension == "asf") {
                videoEngine = QtGuiSettings::VideoEngineDirectshow2;
            }
        }
    }
    VideoGrabber::VideoEngine engine = VideoGrabber::veAuto;
#ifdef IU_ENABLE_FFMPEG
    if (videoEngine == QtGuiSettings::VideoEngineFFmpeg) {
        engine = VideoGrabber::veAvcodec;
    } else
#endif
        if (videoEngine == QtGuiSettings::VideoEngineDirectshow) {
        engine = VideoGrabber::veDirectShow;
    } else if (videoEngine == QtGuiSettings::VideoEngineDirectshow2) {
        engine = VideoGrabber::veDirectShow2;
    } else if (videoEngine == QtGuiSettings::VideoEngineMediaFoundation) {
        engine = VideoGrabber::veMediaFoundation;
    }
    return engine;
}
