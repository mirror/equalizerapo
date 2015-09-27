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

#pragma once

#include <string>
#include <vector>
#include <QMainWindow>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>

#include "FilterTable.h"
#include "DeviceAPOInfo.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QDir configDir, QWidget *parent = 0);
	~MainWindow();
	void doChecks();
	void load(QString path);
	void save(FilterTable* filterTable, QString path);

protected:
	void closeEvent(QCloseEvent* event) override;

private slots:
	void deviceSelected(int index);
	void channelConfigurationSelected(int index);
	void linesChanged();

	bool on_tabWidget_tabCloseRequested(int index);
	void on_actionOpen_triggered();
	void on_actionSave_triggered();
	void on_actionSaveAs_triggered();
	void on_actionNew_triggered();

	void on_actionCut_triggered();
	void on_actionDelete_triggered();
	void on_actionPaste_triggered();
	void on_actionCopy_triggered();

	void instantModeEnabled(bool enabled);

private:
	FilterTable* addTab(QString title, QString tooltip);
	void getDeviceAndChannelMask(DeviceAPOInfo** selectedDevice, int* channelMask);

	Ui::MainWindow *ui;

	QDir configDir;
	QCheckBox* instantModeCheckBox;
	QComboBox* deviceComboBox;
	QComboBox* channelConfigurationComboBox;
	QList<DeviceAPOInfo> outputDevices;
	QList<DeviceAPOInfo> inputDevices;
	DeviceAPOInfo* defaultOutputDevice;
};

Q_DECLARE_METATYPE(DeviceAPOInfo*)
