#ifndef UPTOODA_ADDEDFILESTABWIDGET_H
#define UPTOODA_ADDEDFILESTABWIDGET_H

#include <QWidget>

class QButtonGroup;
class QPushButton;
class ThumbnailListModel;
class ThumbnailListView;

class AddedFilesTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit AddedFilesTabWidget(QWidget* parent = nullptr);

    void setModel(ThumbnailListModel* model);
    void revealRow(int row);

    signals:
        void clearRequested();
    void removeRequested(const QList<int>& rows);
    void openRequested(int row);
    void nextRequested();

private:
    void updateState();

    ThumbnailListView* listView_ = nullptr;
    QPushButton* removeButton_ = nullptr;
    QPushButton* clearButton_ = nullptr;
    QPushButton* nextButton_ = nullptr;
    ThumbnailListModel* model_ = nullptr;
};

#endif //UPTOODA_ADDEDFILESTABWIDGET_H