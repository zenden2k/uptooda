#ifndef QIMAGEUPLOADER_GUI_QTIMAGEGENERATOR_H
#define QIMAGEUPLOADER_GUI_QTIMAGEGENERATOR_H

#include <QAtomicInteger>
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QVector>

class QtImageGenerator final : public QObject {
    Q_OBJECT

public:
    struct FileItem {
        QString FileName;
        QString Title;
    };

    struct Options {
        int Columns = 3;
        int TileWidth = 200;
        int GapWidth = 5;
        int GapHeight = 7;
        bool ShowMediaInfo = true;
        bool EnableMediaInfoLocalization = true;
        QString OutputDirectory;
    };

    explicit QtImageGenerator(QVector<FileItem> files, QString mediaFile, Options options, QObject* parent = nullptr);
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
    Options options_;
    QAtomicInt canceled_ = 0;
    QFutureWatcher<GenerationResult> watcher_;
};

#endif
