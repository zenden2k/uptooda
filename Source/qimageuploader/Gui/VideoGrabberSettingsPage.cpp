#include "VideoGrabberSettingsPage.h"

#include <QAction>
#include <QColorDialog>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFontDialog>
#include <QIcon>
#include <QMenu>
#include <QMetaEnum>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>

#include <utility>

#include "Core/Settings/CommonGuiSettings.h"
#include "ui_VideoGrabberSettingsPage.h"

namespace {

QFontDatabase::WritingSystem writingSystemFromCharset(int charset) {
    switch (charset) {
    case 2:
        return QFontDatabase::Symbol;
    case 128:
        return QFontDatabase::Japanese;
    case 129:
    case 130:
        return QFontDatabase::Korean;
    case 134:
        return QFontDatabase::SimplifiedChinese;
    case 136:
        return QFontDatabase::TraditionalChinese;
    case 161:
        return QFontDatabase::Greek;
    case 163:
        return QFontDatabase::Vietnamese;
    case 177:
        return QFontDatabase::Hebrew;
    case 178:
        return QFontDatabase::Arabic;
    case 204:
        return QFontDatabase::Cyrillic;
    case 222:
        return QFontDatabase::Thai;
    case 0:
    case 162:
    case 186:
    case 238:
        return QFontDatabase::Latin;
    default:
        return QFontDatabase::Any;
    }
}

int charsetFromWritingSystem(QFontDatabase::WritingSystem writingSystem) {
    switch (writingSystem) {
    case QFontDatabase::Symbol:
        return 2;
    case QFontDatabase::Japanese:
        return 128;
    case QFontDatabase::Korean:
        return 129;
    case QFontDatabase::SimplifiedChinese:
        return 134;
    case QFontDatabase::TraditionalChinese:
        return 136;
    case QFontDatabase::Greek:
        return 161;
    case QFontDatabase::Vietnamese:
        return 163;
    case QFontDatabase::Hebrew:
        return 177;
    case QFontDatabase::Arabic:
        return 178;
    case QFontDatabase::Cyrillic:
        return 204;
    case QFontDatabase::Thai:
        return 222;
    case QFontDatabase::Latin:
        return 0;
    default:
        return 1;
    }
}

QFont deserializeFont(const std::string& serializedFont, int& charset, int& writingSystem) {
    const QStringList parts = QString::fromStdString(serializedFont).split(QLatin1Char(','));
    QFont font(parts.value(0, QStringLiteral("Tahoma")).trimmed());
    bool validSize = false;
    const int size = parts.value(1).trimmed().toInt(&validSize);
    font.setPointSize(validSize && size > 0 ? size : 12);

    const QString styles = parts.value(2).trimmed();
    font.setBold(styles.contains(QLatin1Char('b'), Qt::CaseInsensitive));
    font.setItalic(styles.contains(QLatin1Char('i'), Qt::CaseInsensitive));
    font.setUnderline(styles.contains(QLatin1Char('u'), Qt::CaseInsensitive));
    font.setStrikeOut(styles.contains(QLatin1Char('s'), Qt::CaseInsensitive));

    bool validCharset = false;
    const int parsedCharset = parts.value(3).trimmed().toInt(&validCharset);
    if (validCharset) {
        charset = parsedCharset;
    }

    QFontDatabase::WritingSystem parsedWritingSystem = writingSystemFromCharset(charset);
    if (parts.size() > 4) {
        const QByteArray writingSystemKey = parts.value(4).trimmed().toLatin1();
        bool validWritingSystem = false;
        const int value = QMetaEnum::fromType<QFontDatabase::WritingSystem>().keyToValue(writingSystemKey.constData(),
                                                                                         &validWritingSystem);
        if (validWritingSystem && value >= QFontDatabase::Any && value < QFontDatabase::WritingSystemsCount) {
            parsedWritingSystem = static_cast<QFontDatabase::WritingSystem>(value);
        }
    }
    writingSystem = parsedWritingSystem;
    return font;
}

std::string serializeFont(const QFont& font, int charset, int writingSystem) {
    QString styles;
    if (font.bold()) {
        styles += QLatin1Char('b');
    }
    if (font.italic()) {
        styles += QLatin1Char('i');
    }
    if (font.underline()) {
        styles += QLatin1Char('u');
    }
    if (font.strikeOut()) {
        styles += QLatin1Char('s');
    }
    const int pointSize = font.pointSize() > 0 ? font.pointSize() : 12;
    const QMetaEnum writingSystemMetaEnum = QMetaEnum::fromType<QFontDatabase::WritingSystem>();
    const char* writingSystemKey = writingSystemMetaEnum.valueToKey(writingSystem);
    return QStringLiteral("%1,%2,%3,%4,%5")
        .arg(font.family())
        .arg(pointSize)
        .arg(styles)
        .arg(charset)
        .arg(QString::fromLatin1(writingSystemKey ? writingSystemKey : "Any"))
        .toStdString();
}

QColor colorFromColorRef(uint32_t color) {
    return QColor::fromRgb(color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff);
}

uint32_t colorToColorRef(const QColor& color) {
    return static_cast<uint32_t>(color.red()) | (static_cast<uint32_t>(color.green()) << 8)
        | (static_cast<uint32_t>(color.blue()) << 16);
}

} // namespace

VideoGrabberSettingsPage::VideoGrabberSettingsPage(CommonGuiSettings* settings, QWidget* parent) :
    SettingsPage(parent), ui_(std::make_unique<Ui::VideoGrabberSettingsPage>()), settings_(settings) {
    ui_->setupUi(this);

    connect(ui_->showMediaInfoCheckBox, &QCheckBox::toggled, this, &VideoGrabberSettingsPage::updateMediaInfoControls);
    connect(ui_->fontButton, &QPushButton::clicked, this, &VideoGrabberSettingsPage::chooseFont);
    connect(ui_->textColorButton, &QPushButton::clicked, this, &VideoGrabberSettingsPage::chooseTextColor);
    connect(ui_->folderBrowseButton, &QPushButton::clicked, this, &VideoGrabberSettingsPage::browseForFolder);
    connect(ui_->macrosButton, &QPushButton::clicked, this, &VideoGrabberSettingsPage::showMacrosMenu);

    load();
}

VideoGrabberSettingsPage::~VideoGrabberSettingsPage() = default;

void VideoGrabberSettingsPage::load() {
    const VideoSettingsStruct& video = settings_->VideoSettings;
    ui_->columnsSpin->setValue(video.Columns);
    ui_->tileWidthSpin->setValue(video.TileWidth);
    ui_->horizontalGapSpin->setValue(video.GapWidth);
    ui_->verticalGapSpin->setValue(video.GapHeight);
    ui_->showMediaInfoCheckBox->setChecked(video.ShowMediaInfo);
    ui_->folderEdit->setText(QString::fromUtf8(video.SnapshotsFolder.c_str()));
    ui_->filenameTemplateEdit->setText(QString::fromUtf8(video.SnapshotFileTemplate.c_str()));
    mediaInfoFont_ = deserializeFont(video.Font, fontCharset_, fontWritingSystem_);
    textColor_ = colorFromColorRef(video.TextColor);
    updateFontButton();
    updateColorButton();
    updateMediaInfoControls(video.ShowMediaInfo);
}

bool VideoGrabberSettingsPage::validate(QString& error) const {
    const QString filenameTemplate = ui_->filenameTemplateEdit->text().trimmed();
    if (filenameTemplate.isEmpty()) {
        error = tr("The filename template cannot be empty!");
        ui_->filenameTemplateEdit->setFocus();
        return false;
    }
    if (filenameTemplate.contains(QRegularExpression(QStringLiteral("[:*?\"<>|]")))) {
        error = tr("The filename template contains forbidden characters!");
        ui_->filenameTemplateEdit->setFocus();
        ui_->filenameTemplateEdit->selectAll();
        return false;
    }
    return true;
}

void VideoGrabberSettingsPage::apply() {
    VideoSettingsStruct& video = settings_->VideoSettings;
    video.Columns = ui_->columnsSpin->value();
    video.TileWidth = ui_->tileWidthSpin->value();
    video.GapWidth = ui_->horizontalGapSpin->value();
    video.GapHeight = ui_->verticalGapSpin->value();
    video.ShowMediaInfo = ui_->showMediaInfoCheckBox->isChecked();
    video.Font = serializeFont(mediaInfoFont_, fontCharset_, fontWritingSystem_);
    video.TextColor = colorToColorRef(textColor_);
    video.SnapshotsFolder = ui_->folderEdit->text().trimmed().toUtf8().toStdString();
    video.SnapshotFileTemplate = ui_->filenameTemplateEdit->text().trimmed().toUtf8().toStdString();
}

void VideoGrabberSettingsPage::browseForFolder() {
    const QString folder = QFileDialog::getExistingDirectory(this, tr("Select folder"), ui_->folderEdit->text());
    if (!folder.isEmpty()) {
        ui_->folderEdit->setText(QDir::toNativeSeparators(folder));
    }
}

void VideoGrabberSettingsPage::chooseFont() {
    QFontDialog dialog(mediaInfoFont_, this);
    dialog.setWindowTitle(tr("Select font"));
    dialog.setOption(QFontDialog::DontUseNativeDialog);

    QComboBox* writingSystemCombo = dialog.findChild<QComboBox*>();
    if (writingSystemCombo && fontWritingSystem_ >= 0 && fontWritingSystem_ < writingSystemCombo->count()) {
        writingSystemCombo->setCurrentIndex(fontWritingSystem_);
        writingSystemCombo->activated(fontWritingSystem_);
    }

    if (dialog.exec() == QDialog::Accepted) {
        mediaInfoFont_ = dialog.selectedFont();
        if (writingSystemCombo) {
            fontWritingSystem_ = writingSystemCombo->currentIndex();
            fontCharset_ = charsetFromWritingSystem(static_cast<QFontDatabase::WritingSystem>(fontWritingSystem_));
        }
        updateFontButton();
    }
}

void VideoGrabberSettingsPage::chooseTextColor() {
    const QColor color = QColorDialog::getColor(textColor_, this, tr("Select text color"));
    if (color.isValid()) {
        textColor_ = color;
        updateColorButton();
    }
}

void VideoGrabberSettingsPage::showMacrosMenu() {
    const std::pair<const char*, const char*> macros[] = {
        { "%f%", QT_TR_NOOP("video file name without extension") },
        { "%fe%", QT_TR_NOOP("video file name") },
        { "%ext%", QT_TR_NOOP("video file extension") },
        { "%y%", QT_TR_NOOP("year") },
        { "%m%", QT_TR_NOOP("month") },
        { "%d%", QT_TR_NOOP("day") },
        { "%h%", QT_TR_NOOP("hour") },
        { "%n%", QT_TR_NOOP("minute") },
        { "%s%", QT_TR_NOOP("second") },
        { "%i%", QT_TR_NOOP("index") },
        { "%cx%", QT_TR_NOOP("video width") },
        { "%cy%", QT_TR_NOOP("video height") },
        { "%random(N)%", QT_TR_NOOP("random string of N characters") },
        { "%type%", QT_TR_NOOP("object type") },
    };

    QMenu menu(this);
    for (const auto& [macro, description] : macros) {
        QAction* action = menu.addAction(QString::fromLatin1(macro) + QStringLiteral(" — ") + tr(description));
        action->setData(qstrcmp(macro, "%random(N)%") == 0 ? QStringLiteral("%random(6)%")
                                                           : QString::fromLatin1(macro));
    }
    if (QAction* selected = menu.exec(ui_->macrosButton->mapToGlobal(QPoint(0, ui_->macrosButton->height())))) {
        ui_->filenameTemplateEdit->insert(selected->data().toString());
    }
}

void VideoGrabberSettingsPage::updateMediaInfoControls(bool enabled) {
    ui_->fontLabel->setEnabled(enabled);
    ui_->fontButton->setEnabled(enabled);
    ui_->textColorLabel->setEnabled(enabled);
    ui_->textColorButton->setEnabled(enabled);
}

void VideoGrabberSettingsPage::updateColorButton() {
    QPixmap swatch(18, 18);
    swatch.fill(textColor_);
    ui_->textColorButton->setIcon(QIcon(swatch));
    ui_->textColorButton->setText(textColor_.name(QColor::HexRgb).toUpper());
}

void VideoGrabberSettingsPage::updateFontButton() {
    ui_->fontButton->setText(tr("%1, %2 pt").arg(mediaInfoFont_.family()).arg(mediaInfoFont_.pointSize()));
}
