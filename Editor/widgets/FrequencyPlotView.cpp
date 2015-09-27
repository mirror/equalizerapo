/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2015  Jonas Thedering

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <algorithm>
#include <QWheelEvent>
#include <QScrollBar>

#include "FrequencyPlotItem.h"
#include "FrequencyPlotView.h"

using namespace std;

FrequencyPlotView::FrequencyPlotView(QWidget *parent) :
	QGraphicsView(parent)
{
	setViewportMargins(20, 0, 0, 20);
	hRuler = new FrequencyPlotHRuler(this);
	vRuler = new FrequencyPlotVRuler(this);
}

FrequencyPlotScene* FrequencyPlotView::scene() const
{
	return qobject_cast<FrequencyPlotScene*>(QGraphicsView::scene());
}

void FrequencyPlotView::setScene(FrequencyPlotScene* scene)
{
	QGraphicsView::setScene(scene);
}

void FrequencyPlotView::drawBackground(QPainter* painter, const QRectF& drawRect)
{
	painter->setRenderHint(QPainter::Antialiasing, false);

	QRectF rect = drawRect;
	if(!sceneRect().contains(rect))
	{
		rect = rect.intersected(sceneRect());
		painter->setClipRect(rect);
	}

	FrequencyPlotScene* s = scene();
	QPointF topLeft = mapToScene(0, 0);
	QPointF bottomRight = mapToScene(viewport()->width(), viewport()->height());
	double dbHeight = abs(s->yToDb(topLeft.y()) - s->yToDb(bottomRight.y()));
	double dbStep = dbHeight / 4;

	double dbBase = pow(10, floor(log10(dbStep)));
	if(dbStep >= 5 * dbBase)
		dbStep = 5 * dbBase;
	else if(dbStep >= 2 * dbBase)
		dbStep = 2 * dbBase;
	else
		dbStep = dbBase;

	double fromDb = floor(s->yToDb(rect.top() + rect.height()) / dbStep) * dbStep;
	double toDb = ceil(s->yToDb(rect.top()) / dbStep) * dbStep;

	painter->setPen(QColor(200, 200, 200));
	for(double db = fromDb; db <= toDb; db += dbStep)
	{
		double y = floor(s->dbToY(db)) + 0.5;
		if(y != -1)
			painter->drawLine(topLeft.x(), y, bottomRight.x(), y);
	}

	double fromHz = s->xToHz(rect.left());
	double toHz = s->xToHz(rect.left() + rect.width());

	const vector<double>& bands = s->getBands();
	if(bands.empty())
	{
		double hzBase = pow(10, floor(log10(fromHz)));
		fromHz = floor(fromHz / hzBase) * hzBase;
		fromHz += hzBase;
		if(round(fromHz / hzBase) >= 10)
			hzBase *= 10;
		for(double hz = fromHz; hz <= toHz;)
		{
			double x = floor(s->hzToX(hz)) + 0.5;
			if(x >= 0)
				painter->drawLine(x, topLeft.y(), x, bottomRight.y());

			hz += hzBase;
			if(round(hz / hzBase) >= 10)
				hzBase *= 10;
		}
	}
	else
	{
		vector<double>::const_iterator it = lower_bound(bands.cbegin(), bands.cend(), fromHz);
		for(; it != bands.cend() && *it < toHz; it++)
		{
			double hz = *it;
			double x = floor(s->hzToX(hz)) + 0.5;
			if(x >= 0)
				painter->drawLine(x, topLeft.y(), x, bottomRight.y());
		}
	}
}

void FrequencyPlotView::updateHRuler()
{
	hRuler->update();
}

void FrequencyPlotView::wheelEvent(QWheelEvent* event)
{
	FrequencyPlotScene* s = scene();
	QPointF scenePos = mapToScene(event->pos());
	double hz = s->xToHz(scenePos.x());
	double db = s->yToDb(scenePos.y());

	qreal zoomFactor = pow(1.001, event->angleDelta().y());
	s->setZoom(max(0.5, min(30, s->getZoom() * zoomFactor)));

	double x = s->hzToX(hz);
	if(x != -1)
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() + round(x - scenePos.x()));
	double y = s->dbToY(db);
	if(y != -1)
		verticalScrollBar()->setValue(verticalScrollBar()->value() + round(y - scenePos.y()));

	resetCachedContent();
	viewport()->update();
	hRuler->update();
	vRuler->update();
}

void FrequencyPlotView::scrollContentsBy(int dx, int dy)
{
	QGraphicsView::scrollContentsBy(dx, dy);

	if(dx != 0)
		hRuler->update();
	if(dy != 0)
		vRuler->update();
}

void FrequencyPlotView::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);

	const QRect rect = viewport()->geometry();
	hRuler->setGeometry(rect.x() - 20, rect.y() + rect.height(), rect.width() + 20, 20);
	vRuler->setGeometry(rect.x() - 20, rect.y(), 20, rect.height() + 20);
}

void FrequencyPlotView::mousePressEvent(QMouseEvent* event)
{
	QGraphicsView::mousePressEvent(event);

	lastMousePos = event->pos();
}

void FrequencyPlotView::mouseMoveEvent(QMouseEvent* event)
{
	if(event->buttons() & Qt::RightButton)
	{
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - lastMousePos.x()));
		verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - lastMousePos.y()));
	}
	else
	{
		QGraphicsView::mouseMoveEvent(event);
	}
	lastMousePos = event->pos();
}

void FrequencyPlotView::showEvent(QShowEvent* event)
{
	QGraphicsView::showEvent(event);

	horizontalScrollBar()->setValue(scene()->hzToX(20));
}
