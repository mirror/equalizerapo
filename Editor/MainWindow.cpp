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

#include <sstream>
#include <QDrag>
#include <QElapsedTimer>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStringBuilder>
#include <QScrollArea>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

#define WIN32_LEAN_AND_MEAN
#include <QProcess>
#include <windows.h>

#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "helpers/ChannelHelper.h"
#include "Editor/helpers/GUIChannelHelper.h"
#include "version.h"
#include "FilterTable.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

using namespace std;

MainWindow::MainWindow(QDir configDir, QWidget *parent) :
	QMainWindow(parent), ui(new Ui::MainWindow), configDir(configDir)
{
	outputDevices = QList<DeviceAPOInfo>::fromVector(QVector<DeviceAPOInfo>::fromStdVector(DeviceAPOInfo::loadAllInfos(false)));
	inputDevices = QList<DeviceAPOInfo>::fromVector(QVector<DeviceAPOInfo>::fromStdVector(DeviceAPOInfo::loadAllInfos(true)));

	defaultOutputDevice = NULL;
	for(DeviceAPOInfo& apoInfo : outputDevices)
	{
		if(apoInfo.isDefaultDevice)
		{
			defaultOutputDevice = &apoInfo;
			break;
		}
	}

	ui->setupUi(this);

	QString version = QString("%0.%1").arg(MAJOR).arg(MINOR);
	if(REVISION != 0)
		version += QString(".%0").arg(REVISION);
	setWindowTitle(tr("Equalizer APO %0 Configuration Editor").arg(version));

	QWidget* spacer = new QWidget;
	spacer->setFixedWidth(10);
	ui->mainToolBar->addWidget(spacer);

	instantModeCheckBox = new QCheckBox(tr("Instant mode"));
	instantModeCheckBox->setChecked(true);
	instantModeCheckBox->setToolTip(tr("Changes are saved immediately"));
	connect(instantModeCheckBox, SIGNAL(toggled(bool)), this, SLOT(instantModeEnabled(bool)));
	ui->mainToolBar->addWidget(instantModeCheckBox);

	spacer = new QWidget;
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui->mainToolBar->addWidget(spacer);

	ui->mainToolBar->addWidget(new QLabel(tr("Device: ")));

	deviceComboBox = new QComboBox;
	connect(deviceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(deviceSelected(int)));
	ui->mainToolBar->addWidget(deviceComboBox);

	spacer = new QWidget;
	spacer->setFixedWidth(10);
	ui->mainToolBar->addWidget(spacer);

	ui->mainToolBar->addWidget(new QLabel(tr("Channel configuration: ")));

	channelConfigurationComboBox = new QComboBox;
	channelConfigurationComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	ui->mainToolBar->addWidget(channelConfigurationComboBox);

	QStandardItemModel* model = qobject_cast<QStandardItemModel*>(deviceComboBox->model());
	if(defaultOutputDevice != NULL)
		deviceComboBox->addItem(tr("Default") + " (" + QString::fromStdWString(defaultOutputDevice->connectionName) + " - " + QString::fromStdWString(defaultOutputDevice->deviceName) + ")", NULL);

	deviceComboBox->addItem(tr("Playback devices:"));
	QStandardItem* item = model->item(model->rowCount() - 1);
	QFont font = item->font();
	font.setBold(true);
	item->setFont(font);
	item->setSelectable(false);

	for(DeviceAPOInfo& apoInfo : outputDevices)
		if(apoInfo.isInstalled)
			deviceComboBox->addItem(QString::fromStdWString(apoInfo.connectionName) + " - " + QString::fromStdWString(apoInfo.deviceName), QVariant::fromValue(&apoInfo));

	deviceComboBox->addItem(tr("Capture devices:"));
	item = model->item(model->rowCount() - 1);
	item->setFont(font);
	item->setSelectable(false);

	for(DeviceAPOInfo& apoInfo : inputDevices)
		if(apoInfo.isInstalled)
			deviceComboBox->addItem(QString::fromStdWString(apoInfo.connectionName) + " - " + QString::fromStdWString(apoInfo.deviceName), QVariant::fromValue(&apoInfo));

	connect(channelConfigurationComboBox, SIGNAL(activated(int)), this, SLOT(channelConfigurationSelected(int)));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::doChecks()
{
	if(!DeviceAPOInfo::checkProtectedAudioDG(false))
	{
		if(QMessageBox::warning(this, tr("Registry problem"), tr("A registry value that is required for the operation of Equalizer APO is not set correctly.\nDo you want to run the Configurator application to fix the problem?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			QProcess::startDetached(QCoreApplication::applicationDirPath() + "/Configurator.exe", QStringList());
			return;
		}
	}

	if(!defaultOutputDevice->isInstalled)
	{
		if(QMessageBox::warning(this, tr("APO not installed to device"), tr("Equalizer APO has not been installed to the selected device.\nDo you want to run the Configurator application to fix the problem?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			QProcess::startDetached(QCoreApplication::applicationDirPath() + "/Configurator.exe", QStringList());
			return;
		}
	}

	DeviceAPOInfo* disabledApoInfo = NULL;
	for(DeviceAPOInfo& apoInfo : outputDevices)
	{
		if(apoInfo.isInstalled && apoInfo.isEnhancementsDisabled)
		{
			disabledApoInfo = &apoInfo;
			break;
		}
	}

	if(disabledApoInfo == NULL)
	{
		for(DeviceAPOInfo& apoInfo : inputDevices)
		{
			if(apoInfo.isInstalled && apoInfo.isEnhancementsDisabled)
			{
				disabledApoInfo = &apoInfo;
				break;
			}
		}
	}

	if(disabledApoInfo != NULL)
	{
		if(QMessageBox::warning(this, tr("Audio enhancements disabled"), tr("Audio enhancements are not enabled for the device\n%0 %1.\nDo you want to run the Configurator application to fix the problem?").arg(QString::fromStdWString(disabledApoInfo->connectionName)).arg(QString::fromStdWString(disabledApoInfo->deviceName)), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			QProcess::startDetached(QCoreApplication::applicationDirPath() + "/Configurator.exe", QStringList());
			return;
		}
	}
}

void MainWindow::load(QString path)
{
	for(int i = 0; i < ui->tabWidget->count(); i++)
	{
		QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->widget(i));
		FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());

		if(filterTable->getConfigPath() == path)
		{
			ui->tabWidget->setCurrentIndex(i);
			return;
		}
	}

	QElapsedTimer timer;
	timer.start();

	HANDLE hFile = INVALID_HANDLE_VALUE;
	while(hFile == INVALID_HANDLE_VALUE)
	{
		hFile = CreateFile(path.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			if(error != ERROR_SHARING_VIOLATION)
			{
				QMessageBox::critical(this, tr("Error"), tr("Error while reading configuration file: %0").arg(QString::fromStdWString(StringHelper::getSystemErrorString(error))));
				return;
			}

			// file is being written, so wait
			Sleep(1);
		}
	}

	stringstream inputStream;

	char buf[8192];
	unsigned long bytesRead = -1;
	while(ReadFile(hFile, buf, sizeof(buf), &bytesRead, NULL) && bytesRead != 0)
	{
		inputStream.write(buf, bytesRead);
	}

	CloseHandle(hFile);

	inputStream.seekg(0);

	QList<QString> lines;
	while(inputStream.good())
	{
		string encodedLine;
		getline(inputStream, encodedLine);
		if(encodedLine.size() > 0 && encodedLine[encodedLine.size() - 1] == '\r')
			encodedLine.resize(encodedLine.size() - 1);

		wstring line = StringHelper::toWString(encodedLine, CP_UTF8);
		if(line.find(L'\uFFFD') != -1)
			line = StringHelper::toWString(encodedLine, CP_ACP);

		lines.append(QString::fromStdWString(line));
	}

	QFileInfo fileInfo(path);
	FilterTable* filterTable = addTab(fileInfo.fileName(), QDir::toNativeSeparators(fileInfo.absoluteFilePath()));

	filterTable->setLines(path, lines);

	connect(filterTable, SIGNAL(linesChanged()), this, SLOT(linesChanged()));

	qDebug("Loading took %.1f ms", timer.nsecsElapsed() / 1e6);

	ui->tabWidget->setCurrentIndex(ui->tabWidget->count() - 1);
}

void MainWindow::save(FilterTable* filterTable, QString path)
{
	QElapsedTimer timer;
	timer.start();

	QList<QString> lines = filterTable->getLines();

	bool first = true;
	QByteArray byteArray;
	for(QString line : lines)
	{
		if(first)
			first = false;
		else
			byteArray.append("\r\n");
		byteArray.append(line.toUtf8());
	}

	HANDLE hFile = INVALID_HANDLE_VALUE;
	while(hFile == INVALID_HANDLE_VALUE)
	{
		hFile = CreateFile(path.toStdWString().c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			if(error != ERROR_SHARING_VIOLATION)
			{
				QMessageBox::critical(this, tr("Error"), tr("Error while writing configuration file: %0").arg(QString::fromStdWString(StringHelper::getSystemErrorString(error))));
				return;
			}

			// file is being written, so wait
			Sleep(1);
		}
	}

	unsigned long bytesWritten;
	WriteFile(hFile, byteArray.constData(), byteArray.length(), &bytesWritten, NULL);
	if(bytesWritten != byteArray.length())
	{
		// should never happen
		QMessageBox::critical(this, tr("Error"), tr("Only %0/%1 bytes have been written!").arg(bytesWritten).arg(byteArray.length()));
	}

	CloseHandle(hFile);

	qDebug("Saving took %.1f ms", timer.nsecsElapsed() / 1e6);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	while(ui->tabWidget->count() > 0)
	{
		ui->tabWidget->setCurrentIndex(0);

		if(!on_tabWidget_tabCloseRequested(0))
		{
			event->ignore();
			break;
		}
	}
}

void MainWindow::deviceSelected(int index)
{
	DeviceAPOInfo* apoInfo = deviceComboBox->itemData(index).value<DeviceAPOInfo*>();
	if(apoInfo == NULL)
		apoInfo = defaultOutputDevice;

	channelConfigurationComboBox->clear();

	const QList<GUIChannelHelper::ChannelConfigurationInfo>& infos = GUIChannelHelper::getInstance()->getChannelConfigurationInfos();

	if(apoInfo != NULL)
	{
		const GUIChannelHelper::ChannelConfigurationInfo* selectedInfo = NULL;
		for(const GUIChannelHelper::ChannelConfigurationInfo& info : infos)
		{
			if(info.channelMask == apoInfo->channelMask)
			{
				selectedInfo = &info;
				break;
			}
		}

		if(selectedInfo != NULL)
			channelConfigurationComboBox->addItem(tr("From device") + " (" + selectedInfo->name + ")", 0);
		else
			channelConfigurationComboBox->addItem(tr("From device") + " (" + apoInfo->channelCount + " channels)", 0);
	}

	for(const GUIChannelHelper::ChannelConfigurationInfo& info : infos)
		channelConfigurationComboBox->addItem(info.name, info.channelMask);

	channelConfigurationSelected(channelConfigurationComboBox->currentIndex());
}

void MainWindow::channelConfigurationSelected(int index)
{
	DeviceAPOInfo* selectedDevice;
	int channelMask;
	getDeviceAndChannelMask(&selectedDevice, &channelMask);

	for(int i = 0; i < ui->tabWidget->count(); i++)
	{
		QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->widget(i));
		FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());
		filterTable->updateDeviceAndChannelMask(selectedDevice, channelMask);
	}
}

void MainWindow::linesChanged()
{
	FilterTable* filterTable = qobject_cast<FilterTable*>(sender());

	if(instantModeCheckBox->isChecked())
	{
		QString configPath = filterTable->getConfigPath();
		if(configPath.length() > 0)
		{
			save(filterTable, configPath);
			return;
		}
	}

	int tabIndex = -1;
	for(int i = 0; i < ui->tabWidget->count(); i++)
	{
		QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->widget(i));
		if(scrollArea->widget() == filterTable)
		{
			tabIndex = i;
			break;
		}
	}
	QString tabText = ui->tabWidget->tabText(tabIndex);
	if(!tabText.endsWith('*'))
	{
		tabText += '*';
		ui->tabWidget->setTabText(tabIndex, tabText);
	}
}

void MainWindow::getDeviceAndChannelMask(DeviceAPOInfo** selectedDevice, int* channelMask)
{
	*selectedDevice = deviceComboBox->currentData().value<DeviceAPOInfo*>();
	if(*selectedDevice == NULL)
		*selectedDevice = defaultOutputDevice;

	*channelMask = channelConfigurationComboBox->currentData().toInt();
	if(*channelMask == 0 && selectedDevice != NULL)
	{
		*channelMask = (*selectedDevice)->channelMask;

		if(*channelMask == 0)
			*channelMask = ChannelHelper::getDefaultChannelMask((*selectedDevice)->channelCount);
	}
}

bool MainWindow::on_tabWidget_tabCloseRequested(int index)
{
	if(ui->tabWidget->tabText(index).endsWith('*'))
	{
		ui->tabWidget->setCurrentIndex(index);
		QString configPath = ui->tabWidget->tabToolTip(index);
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Unsaved changes"));
		messageBox.setText(tr("The configuration file %0 has unsaved changes.").arg(configPath));
		messageBox.setInformativeText(tr("Do you want to save the changes before closing the file?"));
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Save);
		messageBox.setEscapeButton(QMessageBox::Cancel);
		messageBox.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
		int result = messageBox.exec();

		switch(result)
		{
		case QMessageBox::Save:
			ui->actionSave->trigger();
			if(ui->tabWidget->tabText(index).endsWith('*'))
				// saving was canceled
				return false;
			break;
		case QMessageBox::Discard:
			break;
		case QMessageBox::Cancel:
			return false;
		}
	}

	ui->tabWidget->removeTab(index);
	return true;
}

void MainWindow::on_actionOpen_triggered()
{
	QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->currentWidget());
	QString path;
	if(scrollArea != NULL)
	{
		FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());
		if(filterTable->getConfigPath().length() > 0)
		{
			QFileInfo fileInfo(filterTable->getConfigPath());
			path = fileInfo.absolutePath();
		}
	}
	if(path.length() == 0)
		path = configDir.absolutePath();

	QFileDialog dialog(this, tr("Open file"), path, "*.txt");
	dialog.setFileMode(QFileDialog::ExistingFiles);
	dialog.setNameFilter(tr("E-APO configurations (*.txt)"));

	if(dialog.exec() == QDialog::Accepted)
	{
		for(QString file : dialog.selectedFiles())
			load(file);
	}
}

void MainWindow::on_actionSave_triggered()
{
	QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->currentWidget());
	if(scrollArea == NULL)
		return;

	FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());

	if(filterTable->getConfigPath().length() == 0)
	{
		ui->actionSaveAs->trigger();
	}
	else
	{
		save(filterTable, filterTable->getConfigPath());

		QString tabText = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
		if(tabText.endsWith('*'))
			ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), tabText.left(tabText.length() - 1));
	}
}

void MainWindow::on_actionSaveAs_triggered()
{
	QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->currentWidget());
	if(scrollArea == NULL)
		return;

	FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());
	QString path;
	QString filename;
	if(filterTable->getConfigPath().length() == 0)
	{
		path = configDir.absolutePath();
	}
	else
	{
		QFileInfo fileInfo(filterTable->getConfigPath());
		path = fileInfo.absolutePath();
		filename = fileInfo.fileName();
	}
	QFileDialog dialog(this, tr("Save file as"), path, "*.txt");
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setNameFilter(tr("E-APO configurations (*.txt)"));
	dialog.setDefaultSuffix(".txt");
	if(filename.length() > 0)
		dialog.selectFile(filename);

	if(dialog.exec() == QDialog::Accepted)
	{
		QString savePath = dialog.selectedFiles().first();
		save(filterTable, savePath);
		filterTable->setConfigPath(savePath);

		QFileInfo fileInfo(savePath);
		ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), fileInfo.fileName());
		ui->tabWidget->setTabToolTip(ui->tabWidget->currentIndex(), QDir::toNativeSeparators(fileInfo.absoluteFilePath()));
	}
}

void MainWindow::on_actionNew_triggered()
{
	FilterTable* filterTable = addTab(tr("Unsaved"), "");
	filterTable->setLines("", QList<QString>());

	connect(filterTable, SIGNAL(linesChanged()), this, SLOT(linesChanged()));
	ui->tabWidget->setCurrentIndex(ui->tabWidget->count() - 1);
}

FilterTable* MainWindow::addTab(QString title, QString tooltip)
{
	QScrollArea* scrollArea = new QScrollArea(ui->tabWidget);
	scrollArea->setWidgetResizable(true);
	FilterTable* filterTable = new FilterTable(this);
	scrollArea->setWidget(filterTable);
	filterTable->setAcceptDrops(true);
	filterTable->setFocusPolicy(Qt::WheelFocus);

	int tabIndex = ui->tabWidget->addTab(scrollArea, title);
	ui->tabWidget->setTabToolTip(tabIndex, tooltip);

	DeviceAPOInfo* selectedDevice;
	int channelMask;
	getDeviceAndChannelMask(&selectedDevice, &channelMask);
	filterTable->updateDeviceAndChannelMask(selectedDevice, channelMask);
	filterTable->initialize(scrollArea, outputDevices, inputDevices);

	return filterTable;
}

void MainWindow::on_actionCut_triggered()
{
	QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->currentWidget());
	if(scrollArea == NULL)
		return;

	FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());
	filterTable->cut();
}

void MainWindow::on_actionCopy_triggered()
{
	QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->currentWidget());
	if(scrollArea == NULL)
		return;

	FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());
	filterTable->copy();
}

void MainWindow::on_actionPaste_triggered()
{
	QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->currentWidget());
	if(scrollArea == NULL)
		return;

	FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());
	filterTable->paste();
}

void MainWindow::on_actionDelete_triggered()
{
	QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->currentWidget());
	if(scrollArea == NULL)
		return;

	FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());
	filterTable->deleteSelectedLines();
}

void MainWindow::instantModeEnabled(bool enabled)
{
	if(enabled)
	{
		for(int i = 0; i < ui->tabWidget->count(); i++)
		{
			QScrollArea* scrollArea = qobject_cast<QScrollArea*>(ui->tabWidget->widget(i));
			FilterTable* filterTable = qobject_cast<FilterTable*>(scrollArea->widget());

			if(filterTable->getConfigPath().length() > 0)
			{
				save(filterTable, filterTable->getConfigPath());

				QString tabText = ui->tabWidget->tabText(i);
				if(tabText.endsWith('*'))
					ui->tabWidget->setTabText(i, tabText.left(tabText.length() - 1));
			}
		}
	}
}
