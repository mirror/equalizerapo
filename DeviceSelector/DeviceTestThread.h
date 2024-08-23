#pragma once

#include <DeviceAPOInfo.h>
#include <QThread>
#include <ReceiveThread.h>

enum class ItemStatusType
{
	waiting,
	success,
	warning,
	error
};

Q_DECLARE_METATYPE(ItemStatusType)

class DeviceTestThread : public QThread
{
	Q_OBJECT

public:
	DeviceTestThread(QObject* parent, const QVector<std::shared_ptr<DeviceAPOInfo>>& devices);

signals:
	void log(const QString& message);
	void logError(const QString& message);
	void showErrorDialog(const QString& message);
	void abort(const QString& message, int code);
	void setItemStatus(const QString& guid, bool postMix, ItemStatusType statusType);
	void finished();

protected:
	__override void run();

private:
	class TestResult
	{
	public:
		bool preMixOk = false;
		bool postMixOk = false;
		bool childAPOPreMixOk = true;
		bool childAPOPostMixOk = true;

		int getScore()
		{
			int score = 0;
			if (preMixOk)
				score += 20;
			if (postMixOk)
				score += 10;
			if (!childAPOPreMixOk)
				score -= 2;
			if (!childAPOPostMixOk)
				score -= 1;
			return score;
		}
	};

	class DeviceTestInfo
	{
	public:
		DeviceTestInfo(std::shared_ptr<DeviceAPOInfo> deviceInfo)
			:deviceInfo(deviceInfo)
		{
		}

		std::shared_ptr<DeviceAPOInfo> deviceInfo;
		TestResult currentResult;
		QVector<DeviceAPOInfo::InstallMode> remainingInstallModes;
		DeviceAPOInfo::InstallMode bestInstallMode;
		TestResult bestResult;
		bool wantsOriginalApoPreMix;
		bool wantsOriginalApoPostMix;
	};

	QHash<QString, DeviceTestInfo> infoMap;
};
