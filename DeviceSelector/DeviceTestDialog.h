#pragma once

#include <DeviceAPOInfo.h>
#include <DeviceTestThread.h>
#include <OpacityIconEngine.h>
#include <QtWidgets/QDialog>
#include "ui_DeviceTestDialog.h"

class DeviceTestDialog : public QDialog
{
	Q_OBJECT

public:
	DeviceTestDialog(QWidget* parent = nullptr);
	std::vector<std::shared_ptr<DeviceAPOInfo>> filterDevices(const std::vector<std::shared_ptr<AbstractAPOInfo>>& devices);
	void addDevices(std::vector<std::shared_ptr<DeviceAPOInfo>>& devices, QTreeWidgetItem* parentNode);

protected:
	__override void closeEvent(QCloseEvent* event);

private:
	Ui::DeviceTestDialogClass ui;

	QHash<QString, QTreeWidgetItem*> itemMap;
	QIcon animatedIcon;
	OpacityIconEngine* animatedIconEngine;
	DeviceTestThread* thread = nullptr;

	void log(const QString& message);
	void logError(const QString& message);
	void showErrorDialog(const QString& message);
	void setItemStatus(const QString& guid, bool postMix, ItemStatusType statusType);
	void animateIcon(const QVariant& value);
	void onThreadFinished();
	void onAbort(const QString& message, int code);
};
