#include "FrameGrabberDlg.h"

#include <QFileDialog>
#include <QTemporaryFile>
#include <QDesktopServices>
#include <QDebug>
#include <QImageWriter>

#include "ui_FrameGrabberDlg.h"
#include "Video/VideoGrabber.h"
#include "Core/CommonDefs.h"
#include "Video/QtImage.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/QtGuiSettings.h"

Q_DECLARE_METATYPE(AbstractImage*)

FrameGrabberDlg::FrameGrabberDlg(QString fileName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FrameGrabberDlg)
{
    qRegisterMetaType<AbstractImage*>("AbstractImage*");
    auto settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    ui->numOfFramesSpinBox->setValue(settings->VideoSettings.NumOfFrames);
    ui->stopButton->setVisible(false);
    ui->lineEdit->setText(fileName);

    for (const auto& engine: CommonGuiSettings::VideoEngines) {
        QString name = QString::fromStdString(engine);
        ui->comboBox->addItem(name, QVariant(name));
    }

    int index = ui->comboBox->findData(QString::fromStdString(settings->VideoSettings.Engine));
    if ( index != -1 ) {
        ui->comboBox->setCurrentIndex(index);
    }

    ui->progressRing->hide();
	connect(ui->stopButton, &QPushButton::clicked, this, &FrameGrabberDlg::onStopButtonClicked);
    connect(ui->listWidget, &QListWidget::doubleClicked, this, &FrameGrabberDlg::itemDoubleClicked);
    connect(this, &FrameGrabberDlg::finished, this, &FrameGrabberDlg::onFinished);
}

FrameGrabberDlg::~FrameGrabberDlg()
{
    delete ui;
}

void FrameGrabberDlg::frameGrabbed(const std::string& timeStr, int64_t time, std::shared_ptr<AbstractImage> image) {
	if (!image) {
		return;
	}
    QString timeString = U2Q(timeStr);
	auto qtImage = dynamic_cast<QtImage*>(image.get());
	if (!qtImage) {
	    return;
	}
    QImage img = qtImage->toQImage();

    if (!img.isNull()) {
        QString tempDirectory = U2Q(AppRuntimeInfo::instance()->tempDirectory());
        QTemporaryFile f(tempDirectory + "/grab_XXXXXX.png");
        f.setAutoRemove(false);
        QString uniqueFileName;
        if (f.open()) {
            uniqueFileName = f.fileName();
            f.close();
        }
        if (!uniqueFileName.isEmpty()) {
            QImageWriter writer(uniqueFileName);
            writer.setCompression(1);
            writer.setQuality(10);

            if (writer.write(img)) {
                QIcon ico(QPixmap::fromImage(img/*.scaledToWidth(150, Qt::SmoothTransformation)*/));
                
                QMetaObject::invokeMethod(this, "frameGrabbedSlot", Qt::BlockingQueuedConnection,
                    Q_ARG(QString, timeString), Q_ARG(QString, uniqueFileName), Q_ARG(QIcon, ico));
            }
        }
    }
}

void FrameGrabberDlg::frameGrabbedSlot(QString timeStr, QString fileName, QIcon ico) {
    QListWidgetItem * item = new QListWidgetItem(ui->listWidget);
    item->setText(timeStr);
    item->setData(Qt::UserRole, fileName);
    item->setIcon(ico);
    ui->listWidget->addItem(item);
}

void FrameGrabberDlg::on_grabButton_clicked()
{
	ui->grabButton->setEnabled(false);
	ui->buttonBox->setEnabled(false);
	ui->stopButton->setVisible(true);
	ui->browseButton->setEnabled(false);

	grabber_ = std::make_unique<VideoGrabber>();


    grabber_->setVideoEngine(getVideoEngine());

    using namespace std::placeholders;
    grabber_->setOnFrameGrabbed(std::bind(&FrameGrabberDlg::frameGrabbed, this, _1, _2, _3));
    grabber_->setOnFinished(std::bind(&FrameGrabberDlg::onGrabFinished, this));

    int frameCount = ui->numOfFramesSpinBox->value();
    if (frameCount < 1) {
        frameCount = 10;
    }
    grabber_->setFrameCount(frameCount);
    ui->progressRing->show();
    grabber_->grab(Q2U(ui->lineEdit->text()));
}

void FrameGrabberDlg::on_browseButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if ( fileName.length() ) {
        ui->lineEdit->setText( fileName );
    }

}

void FrameGrabberDlg::onGrabFinished() {
	QMetaObject::invokeMethod(this, "grabFinishedSlot", Qt::BlockingQueuedConnection);
}

void FrameGrabberDlg::grabFinishedSlot() {
    ui->grabButton->setEnabled(true);
    ui->buttonBox->setEnabled(true);
    ui->stopButton->setVisible(false);
    ui->browseButton->setEnabled(true);
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    ui->progressRing->hide();
}

void FrameGrabberDlg::onStopButtonClicked() {
	if (grabber_) {
		grabber_->abort();
	}
}

void FrameGrabberDlg::getGrabbedFrames(QStringList& fileNames) const {
    int itemCount = ui->listWidget->count();
    for ( int i = 0; i< itemCount; i++) {
        auto item = ui->listWidget->item(i);
        fileNames.push_back(item->data(Qt::UserRole).toString());
    }
}

void FrameGrabberDlg::itemDoubleClicked(const QModelIndex& index) {
    QListWidgetItem* item = ui->listWidget->item(index.row());
    if (item) {
        QString fileName = item->data(Qt::UserRole).toString();
        QDesktopServices::openUrl("file:///" + fileName);
    }
}

void FrameGrabberDlg::onFinished()
{
    auto settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    settings->VideoSettings.NumOfFrames = ui->numOfFramesSpinBox->value();
    settings->VideoSettings.Engine = ui->comboBox->currentData().toString().toStdString();
}

void FrameGrabberDlg::closeEvent(QCloseEvent *event) {
    event->accept();
    reject();
}

VideoGrabber::VideoEngine FrameGrabberDlg::getVideoEngine() const {
    auto settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    std::string videoEngine = ui->comboBox->currentData().toString().toStdString();

    if (videoEngine == QtGuiSettings::VideoEngineAuto) {
        if ( !settings->IsFFmpegAvailable() ) {
            videoEngine = QtGuiSettings::VideoEngineDirectshow;
        } else {
            videoEngine = QtGuiSettings::VideoEngineFFmpeg;
            QString fileName = ui->lineEdit->text();
            QFileInfo info(fileName);
            const QString fileExt = info.suffix().toLower();
            if (fileExt == "wmv" || fileExt == "asf") {
                videoEngine = QtGuiSettings::VideoEngineDirectshow;
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
