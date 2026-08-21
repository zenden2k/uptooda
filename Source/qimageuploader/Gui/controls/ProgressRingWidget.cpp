#include "ProgressRingWidget.h"

#include <QPainter>
#include <QtGlobal>

ProgressRingWidget::ProgressRingWidget(QWidget* parent) : QWidget(parent), current_(0) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(32, 32);
    startTimer(20);
}

void ProgressRingWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);

    QPen pen;
    pen.setWidth(3);
    pen.setColor(QColor(0, 191, 255)); // DeepSkyBlue
    p.setPen(pen);

    p.setRenderHint(QPainter::Antialiasing);

    const qreal diameter = qMin(width(), height()) - 4.0;
    const QRectF rectangle((width() - diameter) / 2.0, (height() - diameter) / 2.0, diameter, diameter);

    // int startAngle = (int)(qMin(0.0, current_) * 360);
    int startAngle = (int)(current_ * 360 * -16);
    // int spanAngle = (int)(qMin(0.2, current_+0.2) * 360);
    int spanAngle = (int)((0.5) * 360 * -16);


    p.drawArc(rectangle, startAngle, spanAngle);
}

void ProgressRingWidget::timerEvent(QTimerEvent*) {
    if (isVisible()) {
        current_ += 0.025;
        if (current_ > 1.0) {
            current_ = 0.0;
        }
        update();
    }
}
