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
#include <QPainter>

#include "FrequencyPlotView.h"
#include "FrequencyPlotHRuler.h"

using namespace std;

FrequencyPlotHRuler::FrequencyPlotHRuler(QWidget* parent) :
	QWidget(parent)
{
}

void FrequencyPlotHRuler::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	FrequencyPlotView* view = qobject_cast<FrequencyPlotView*>(parentWidget());
	FrequencyPlotScene* s = view->scene();
	QFontMetrics metrics = painter.fontMetrics();
	painter.setPen(QColor(50, 50, 50));

	QPointF topLeft = view->mapToScene(0, 0);
	QPointF bottomRight = view->mapToScene(view->viewport()->width(), view->viewport()->height());
	double fromHz = s->xToHz(topLeft.x());
	double toHz = s->xToHz(bottomRight.x());
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
			double x = s->hzToX(hz);
			if(x != -1)
			{
				QString text;
				if(hz < 1000)
					text = QString("%0").arg(hz);
				else
					text = QString("%0k").arg(hz / 1000);
				if(metrics.width(text) + 2 < s->hzToX(hz + hzBase) - x)
					painter.drawText(x - topLeft.x() + height() + 1, 0, 0, height(), Qt::TextDontClip | Qt::AlignCenter, text);
			}

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
			double x = s->hzToX(hz);
			if(x != -1)
			{
				QString text;
				if(hz < 1000)
					text = QString("%0").arg(hz);
				else
					text = QString("%0k").arg(hz / 1000);
				painter.drawText(x - topLeft.x() + height() + 1, 0, 0, height(), Qt::TextDontClip | Qt::AlignCenter, text);
			}
		}
	}
}
