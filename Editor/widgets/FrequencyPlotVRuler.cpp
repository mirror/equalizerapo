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

#include "FrequencyPlotView.h"
#include "FrequencyPlotScene.h"
#include "FrequencyPlotVRuler.h"

FrequencyPlotVRuler::FrequencyPlotVRuler(QWidget *parent) :
	QWidget(parent)
{
}

void FrequencyPlotVRuler::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	FrequencyPlotView* view = qobject_cast<FrequencyPlotView*>(parentWidget());
	FrequencyPlotScene* scene = view->scene();

	QPointF topLeft = view->mapToScene(0, 0);
	QPointF bottomRight = view->mapToScene(view->viewport()->width(), view->viewport()->height());
	double dbHeight = abs(scene->yToDb(topLeft.y()) - scene->yToDb(bottomRight.y()));
	double dbStep = dbHeight / 4;

	double dbBase = pow(10, floor(log10(dbStep)));
	if(dbStep >= 5 * dbBase)
		dbStep = 5 * dbBase;
	else if(dbStep >= 2 * dbBase)
		dbStep = 2 * dbBase;
	else
		dbStep = dbBase;

	double fromDb = floor(scene->yToDb(topLeft.y()) / dbStep) * dbStep;
	double toDb = ceil(scene->yToDb(bottomRight.y()) / dbStep) * dbStep;

	painter.setPen(QColor(50, 50, 50));
	for(double db = toDb; db <= fromDb; db += dbStep)
	{
		if(abs(db) < 1e-6)
			db = 0;
		double y = scene->dbToY(db);
		if(y != -1)
			painter.drawText(0, y - topLeft.y() - 1, width(), 0, Qt::TextDontClip | Qt::AlignCenter, QString("%0").arg(db));
	}
}
