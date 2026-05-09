/***************************************************************************
 *   Copyright (C) 2009 by Artem 'DOOMer' Galichkin                        *
 *   doomer3d@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef REGIONSELECT_H
#define REGIONSELECT_H

#include <QDialog>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

#include "ScreenCapture/ScreenCaptureQt.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class RegionSelect : public QDialog        
{
public:
	 RegionSelect(QWidget *parent = nullptr, QPixmap* src = nullptr);
    ~RegionSelect();
    QPixmap getSelection();
	 CScreenshotRegion* selectedRegion();

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QRect selectRect;
    QSize sizeDesktop;

    QPoint selStartPoint;
    QPoint selEndPoint;

    bool palBackground;

    QPixmap desktopPixmapBkg;
    QPixmap desktopPixmapClr;

    void drawBackGround();
    void drawRectSelection(QPainter &painter);


};

#endif // REGIONSELECT_H
