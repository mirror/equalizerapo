/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2012  Jonas Thedering

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

#include "DeviceAPOInfo.h"

#include "../EqualizerAPO.h"
#include "../ParametricEQ.h"
#include "../RegistryHelper.h"

using namespace std;

#define renderKeyPath L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Render"
static const wchar_t* connectionValueName = L"{a45c254e-df1c-4efd-8020-67d146a850e0},2";
static const wchar_t* deviceValueName = L"{b3f8fa53-0004-438e-9003-51a46e139bfc},6";
static const wchar_t* apoGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},1";

vector<DeviceAPOInfo> DeviceAPOInfo::loadAllInfos()
{
	vector<DeviceAPOInfo> result;
	vector<wstring> deviceGuidStrings = RegistryHelper::enumSubKeys(renderKeyPath);
	for(vector<wstring>::iterator it = deviceGuidStrings.begin(); it != deviceGuidStrings.end(); it++)
	{
		wstring deviceGuidString = *it;

		if(!RegistryHelper::keyExists(renderKeyPath L"\\" + deviceGuidString + L"\\FxProperties"))
			continue;

		wstring connectionName = RegistryHelper::readValue(renderKeyPath L"\\" + deviceGuidString + L"\\Properties", connectionValueName);
		wstring deviceName = RegistryHelper::readValue(renderKeyPath L"\\" + deviceGuidString + L"\\Properties", deviceValueName);
		wstring apoGuidString = RegistryHelper::readValue(renderKeyPath L"\\" + deviceGuidString + L"\\FxProperties", apoGuidValueName);

		GUID apoGuid;
		if(!SUCCEEDED(CLSIDFromString(apoGuidString.c_str(), &apoGuid)))
			continue;

		bool installed = false;
		if(apoGuid == __uuidof(EqualizerAPO))
		{
			installed = true;
		}

		DeviceAPOInfo info = {deviceName, connectionName, deviceGuidString, apoGuidString, installed};
		result.push_back(info);
	}

	return result;
}

void DeviceAPOInfo::install()
{
	RegistryHelper::createKey(APP_REGPATH L"\\Child APOs");

	RegistryHelper::writeValue(APP_REGPATH L"\\Child APOs", deviceGuid, originalApoGuid);

	RegistryHelper::saveToFile(renderKeyPath L"\\" + deviceGuid + L"\\FxProperties", apoGuidValueName,
		L"backup_" + RegistryHelper::replaceIllegalCharacters(deviceName) + L"_" + RegistryHelper::replaceIllegalCharacters(connectionName) + L".reg");

	RegistryHelper::writeValue(renderKeyPath L"\\" + deviceGuid + L"\\FxProperties", apoGuidValueName, RegistryHelper::getGuidString(__uuidof(EqualizerAPO)));
}

void DeviceAPOInfo::uninstall()
{
	wstring originalChildApoGuid = RegistryHelper::readValue(APP_REGPATH L"\\Child APOs", deviceGuid);

	RegistryHelper::writeValue(renderKeyPath L"\\" + deviceGuid + L"\\FxProperties", apoGuidValueName, originalChildApoGuid);

	RegistryHelper::deleteValue(APP_REGPATH L"\\Child APOs", deviceGuid);

	if(RegistryHelper::valueCount(APP_REGPATH L"\\Child APOs") == 0)
		RegistryHelper::deleteKey(APP_REGPATH L"\\Child APOs");
}
