#include "crawl-button.h"


/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2011, Bernd Stramm
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QWidget>
#include <QPaintEvent>
#include <QPen>
#include <QColor>
#include <QPainter>
#include <QWidget>
#include <QPoint>
#include <QPointF>
#include <QCursor>
#include <QTimer>
#include <cmath>

namespace openmielke
{

CrawlButton::CrawlButton (QWidget * parent)
  :QPushButton (parent)
{
  lookTimer = new QTimer (this);
  connect (lookTimer, SIGNAL (timeout()), this, SLOT (Look()));
  lookTimer->start (1250);
}

void
CrawlButton::Look ()
{
  update ();
}

void
CrawlButton::paintEvent (QPaintEvent * event)
{
  QPushButton::paintEvent (event);
  ShowEyes ();
}

void
CrawlButton::ShowEyes ()
{
  double h = this->height();
  double w = this->width();
  QPoint midL (w*0.25, h*0.5);
  QPoint midR (w*0.75, h*0.5);
  double rx = w * 0.22;
  double ry = h * 0.4;
  double rmin = (rx > ry ? ry : rx);
  QPointF cursPoint (QCursor::pos());
  QPoint gloMidL(this->mapToGlobal(midL));
  QPoint gloMidR(this->mapToGlobal(midR));
  double dirLx = cursPoint.x() - double(gloMidL.x());
  double dirLy = cursPoint.y() - double(gloMidL.y());
  double thetaL = -atan2(dirLx,dirLy) * 180.0/M_PI;
  double dirRx = cursPoint.x() - double(gloMidR.x());
  double dirRy = cursPoint.y() - double(gloMidR.y());
  double thetaR = -atan2(dirRx,dirRy) * 180.0/M_PI;

  DrawOneEye (midL,thetaL,rmin*0.5,rmin*0.15);
  DrawOneEye (midR,thetaR,rmin*0.5,rmin*0.15);

}

void
CrawlButton::DrawOneEye (QPoint mid, double theta, double r, double wid)
{
  QColor eyecolor(QColor(20,100,200,255));

  QPainter paint(this);
  paint.setRenderHint(QPainter::HighQualityAntialiasing);
  paint.setBrush(Qt::NoBrush);
  QPen linePen;
  linePen.setColor(eyecolor);
  linePen.setWidth(wid);
  paint.setPen(linePen);
  paint.save();
  paint.translate(mid);
  paint.rotate(theta);
  paint.translate(0,r);
  paint.setBrush(eyecolor);
  paint.drawEllipse (QPointF(0,0),r*0.8,r*0.8);
  paint.restore();
  paint.drawEllipse (QPointF(mid),r*2.0,r*2.0);

}

} // namespace

