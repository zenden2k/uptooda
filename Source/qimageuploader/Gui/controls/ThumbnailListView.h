#ifndef QIMAGEUPLOADER_GUI_CONTROLS_THUMBNAILLISTVIEW_H
#define QIMAGEUPLOADER_GUI_CONTROLS_THUMBNAILLISTVIEW_H

#include <QListView>

#include <memory>

class ImageViewerWindow;

class ThumbnailListView : public QListView {
    Q_OBJECT

public:
    explicit ThumbnailListView(QWidget* parent = nullptr);
    ~ThumbnailListView() override;

    void setEmptyText(const QString& text);
    QList<int> selectedRows() const;
    void revealRow(int row);

signals:
    void removeRequested(const QList<int>& rows);
    void openRequested(int row);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void openImage(const QModelIndex& index);

    QString emptyText_;
    std::unique_ptr<ImageViewerWindow> imageViewerWindow_;
};

#endif
