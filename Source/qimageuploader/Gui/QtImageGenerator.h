#ifndef QIMAGEUPLOADER_GUI_QTIMAGEGENERATOR_H
#define QIMAGEUPLOADER_GUI_QTIMAGEGENERATOR_H

#include <QAtomicInteger>
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QVector>

#include "Core/Settings/CommonGuiSettings.h"

class QtImageGenerator final : public QObject {
    Q_OBJECT

public:
    struct FileItem {
        QString FileName;
        QString Title;
    };

    struct Options {
        bool EnableMediaInfoLocalization = true;
        QString OutputDirectory;
    };

    explicit QtImageGenerator(QVector<FileItem> files, QString mediaFile, VideoSettingsStruct videoSettings,
                              Options options, QObject* parent = nullptr);
    ~QtImageGenerator() override;

    void start();
    void cancel();

signals:
    void progressChanged(int value, int maximum);
    void finished(bool success, bool canceled, const QString& outputFileName, const QString& errorMessage);

private:
    struct GenerationResult {
        bool Success = false;
        bool Canceled = false;
        QString OutputFileName;
        QString ErrorMessage;
    };

    GenerationResult generate();
    bool isCanceled() const;

    QVector<FileItem> files_;
    QString mediaFile_;
    VideoSettingsStruct videoSettings_;
    Options options_;
    QAtomicInt canceled_ = 0;
    QFutureWatcher<GenerationResult> watcher_;
};

#endif
