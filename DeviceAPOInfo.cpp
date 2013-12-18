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

#include <mmdeviceapi.h>

#include "DeviceAPOInfo.h"

#include "EqualizerAPO.h"
#include "ParametricEQ.h"
#include "StringHelper.h"
#include "RegistryHelper.h"

using namespace std;

#define commonKeyPath L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio"
#define renderKeyPath commonKeyPath L"\\Render"
#define captureKeyPath commonKeyPath L"\\Capture"
static const wchar_t* connectionValueName = L"{a45c254e-df1c-4efd-8020-67d146a850e0},2";
static const wchar_t* deviceValueName = L"{b3f8fa53-0004-438e-9003-51a46e139bfc},6";
static const wchar_t* lfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},1";
static const wchar_t* gfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},2";
static const wchar_t* lfxGuidValueName2 = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},5";
static const wchar_t* gfxGuidValueName2 = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},6";
static const wchar_t* fxTitleValueName = L"{b725f130-47ef-101a-a5f1-02608c9eebac},10";
static const wchar_t* sfxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},5";
static const wchar_t* mfxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},6";
static const wchar_t* defaultProcessingModeValue = L"{C18E2F7E-933D-4965-B7D1-1EEF228D2AF3}";

vector<DeviceAPOInfo> DeviceAPOInfo::loadAllInfos(bool input)
{
	vector<DeviceAPOInfo> result;
	vector<wstring> deviceGuidStrings = RegistryHelper::enumSubKeys(input ? captureKeyPath : renderKeyPath);
	for(vector<wstring>::iterator it = deviceGuidStrings.begin(); it != deviceGuidStrings.end(); it++)
	{
		wstring deviceGuidString = *it;

		DeviceAPOInfo info;
		if(info.load(deviceGuidString))
			result.push_back(info);
	}

	return result;
}

bool DeviceAPOInfo::load(const wstring& deviceGuid)
{
	wstring keyPath;
	if(RegistryHelper::keyExists(renderKeyPath L"\\" + deviceGuid))
	{
		keyPath = renderKeyPath L"\\" + deviceGuid;
		isInput = false;
	}
	else
	{
		keyPath = captureKeyPath L"\\" + deviceGuid;
		isInput = true;
	}

	unsigned long deviceState = RegistryHelper::readDWORDValue(keyPath, L"DeviceState");
	if(deviceState & DEVICE_STATE_NOTPRESENT)
		return false;

	this->deviceGuid = deviceGuid;

	connectionName = RegistryHelper::readValue(keyPath + L"\\Properties", connectionValueName);
	deviceName = RegistryHelper::readValue(keyPath + L"\\Properties", deviceValueName);

	isInstalled = false;
	isLFX = false;
	isAPO2 = false;
	foundAPO2Values = false;

	if(!RegistryHelper::keyExists(keyPath + L"\\FxProperties"))
	{
		originalApoGuid = APOGUID_NOKEY;
	}
	else
	{
		originalApoGuid = APOGUID_NOVALUE;

		bool found = tryAPOGuid(keyPath, lfxGuidValueName, true, false);
		if(!found)
			found = tryAPOGuid(keyPath, gfxGuidValueName, false, false);
		if(!found)
			found = tryAPOGuid(keyPath, lfxGuidValueName2, true, true);
		if(!found)
			found = tryAPOGuid(keyPath, gfxGuidValueName2, false, true);

		foundAPO2Values = RegistryHelper::isWindowsVersionAtLeast(6, 3) // Windows 8.1
			&& (RegistryHelper::valueExists(keyPath + L"\\FxProperties", lfxGuidValueName2)
			|| RegistryHelper::valueExists(keyPath + L"\\FxProperties", gfxGuidValueName2));

		if(found)
		{
			if(RegistryHelper::valueExists(APP_REGPATH L"\\Child APOs", deviceGuid))
				originalApoGuid = RegistryHelper::readValue(APP_REGPATH L"\\Child APOs", deviceGuid);
		}
		else
		{
			wstring guidValueName;
			if(isInput)
			{
				guidValueName = lfxGuidValueName;
				if(foundAPO2Values)
					guidValueName = lfxGuidValueName2;
			}
			else
			{
				guidValueName = gfxGuidValueName;
				if(foundAPO2Values)
					guidValueName = gfxGuidValueName2;
			}

			if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", guidValueName))
				originalApoGuid = RegistryHelper::readValue(keyPath + L"\\FxProperties", guidValueName);
		}
	}

	return true;
}

bool DeviceAPOInfo::canBeUpgraded()
{
	return isInstalled && (!isInput && isLFX || !isAPO2 && foundAPO2Values);
}

bool DeviceAPOInfo::isExperimental()
{
	return !isInstalled && originalApoGuid == APOGUID_NOKEY;
}

void DeviceAPOInfo::install()
{
	RegistryHelper::createKey(APP_REGPATH L"\\Child APOs");

	RegistryHelper::writeValue(APP_REGPATH L"\\Child APOs", deviceGuid, originalApoGuid);

	wstring keyPath;
	wstring guidValueName;
	if(isInput)
	{
		keyPath = captureKeyPath L"\\" + deviceGuid;
		guidValueName = lfxGuidValueName;
		if(foundAPO2Values)
			guidValueName = lfxGuidValueName2;
	}
	else
	{
		keyPath = renderKeyPath L"\\" + deviceGuid;
		guidValueName = gfxGuidValueName;
		if(foundAPO2Values)
			guidValueName = gfxGuidValueName2;
	}

	if(originalApoGuid == APOGUID_NOKEY)
	{
		try
		{
			RegistryHelper::createKey(keyPath + L"\\FxProperties");
		}
		catch(RegistryException e)
		{
			// Permissions were not sufficient, so change them
			RegistryHelper::takeOwnership(keyPath);
			RegistryHelper::makeWritable(keyPath);

			RegistryHelper::createKey(keyPath + L"\\FxProperties");
		}

		RegistryHelper::writeValue(keyPath + L"\\FxProperties", fxTitleValueName, L"Equalizer APO");
		RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", sfxProcessingModesValueName, defaultProcessingModeValue);
		RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", mfxProcessingModesValueName, defaultProcessingModeValue);
	}
	else if(originalApoGuid != APOGUID_NOVALUE)
	{
		RegistryHelper::saveToFile(keyPath + L"\\FxProperties", guidValueName,
			L"backup_" + StringHelper::replaceIllegalCharacters(deviceName) + L"_" + StringHelper::replaceIllegalCharacters(connectionName) + L".reg");
	}

	RegistryHelper::writeValue(keyPath + L"\\FxProperties", guidValueName, RegistryHelper::getGuidString(__uuidof(EqualizerAPO)));
}

void DeviceAPOInfo::uninstall()
{
	wstring keyPath;
	if(!isInput)
		keyPath = renderKeyPath L"\\" + deviceGuid;
	else
		keyPath = captureKeyPath L"\\" + deviceGuid;

	if(originalApoGuid == APOGUID_NOKEY)
	{
		RegistryHelper::deleteKey(keyPath + L"\\FxProperties");
	}
	else
	{
		wstring guidValueName;
		if(isLFX)
		{
			guidValueName = lfxGuidValueName;
			if(isAPO2)
				guidValueName = lfxGuidValueName2;
		}
		else
		{
			guidValueName = gfxGuidValueName;
			if(isAPO2)
				guidValueName = gfxGuidValueName2;
		}

		if(originalApoGuid == APOGUID_NOVALUE)
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", guidValueName);
		else
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", guidValueName, originalApoGuid);
	}

	if(RegistryHelper::valueExists(APP_REGPATH L"\\Child APOs", deviceGuid))
		RegistryHelper::deleteValue(APP_REGPATH L"\\Child APOs", deviceGuid);

	if(RegistryHelper::valueCount(APP_REGPATH L"\\Child APOs") == 0)
		RegistryHelper::deleteKey(APP_REGPATH L"\\Child APOs");
}

bool DeviceAPOInfo::tryAPOGuid(const wstring& keyPath, const wchar_t* valueName, bool lfx, bool apo2)
{
	if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", valueName))
	{
		wstring apoGuidString = RegistryHelper::readValue(keyPath + L"\\FxProperties", valueName);

		GUID apoGuid;
		if(SUCCEEDED(CLSIDFromString(apoGuidString.c_str(), &apoGuid)))
		{
			if(apoGuid == __uuidof(EqualizerAPO))
			{
				isInstalled = true;
				isLFX = lfx;
				isAPO2 = apo2;

				return true;
			}
		}
	}

	return false;
}