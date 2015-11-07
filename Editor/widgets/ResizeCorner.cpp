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

#include <QMouseEvent>

#include "ResizeCorner.h"

ResizeCorner::ResizeCorner(FilterTable* filterTable, QSize minimumSize, QSize maximumSize, std::function<QSize()> getFunc, std::function<void(QSize)> setFunc, QWidget *parent)
	: QLabel(parent), minimumSize(minimumSize), maximumSize(maximumSize), getFunc(getFunc), setFunc(setFunc), filterTable(filterTable)
{
	setPixmap(QPixmap(":/icons/resize_corner.png"));
	setCursor(Qt::SizeFDiagCursor);
}

void ResizeCorner::mousePressEvent(QMouseEvent* event)
{
	filterTable->setMinimumHeightHint(filterTable->height());
	QSize size = getFunc();
	offsetX = size.width() - event->globalX();
	offsetY = size.height() - event->globalY();
}

void ResizeCorner::mouseMoveEvent(QMouseEvent* event)
{
	QSize size;
	size.setWidth(offsetX + event->globalPos().x());
	size.setHeight(offsetY + event->globalPos().y());
	size = size.expandedTo(minimumSize);
	size = size.boundedTo(maximumSize);
	setFunc(size);
}

void ResizeCorner::mouseReleaseEvent(QMouseEvent*)
{
	filterTable->setMinimumHeightHint(0);
}
