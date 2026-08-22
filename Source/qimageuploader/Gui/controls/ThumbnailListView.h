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
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private:
    int dropRow(const QPoint& position) const;
    bool openImage(const QModelIndex& index);

    QString emptyText_;
    QPoint dragStartPosition_ { -1, -1 };
    bool internalDragActive_ = false;
    std::unique_ptr<ImageViewerWindow> imageViewerWindow_;
};

#endif
