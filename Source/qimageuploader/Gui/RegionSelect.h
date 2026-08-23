#ifndef REGIONSELECT_H
#define REGIONSELECT_H

#include <QApplication>
#include <QDialog>
#include <QMouseEvent>
#include <QPainter>

#include "ScreenCapture/ScreenCaptureQt.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class RegionSelect : public QDialog {
public:
    RegionSelect(QWidget* parent = nullptr, QPixmap* src = nullptr);
    ~RegionSelect() override;
    QPixmap getSelection() const;
    CScreenshotRegion* selectedRegion() const;

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QRect selectRect;
    QSize sizeDesktop;

    QPoint selStartPoint;
    QPoint selEndPoint;

    QPixmap desktopPixmapBkg;
    QPixmap desktopPixmapClr;

    void drawBackground();
    void drawRectSelection(QPainter& painter);
};

#endif // REGIONSELECT_H
