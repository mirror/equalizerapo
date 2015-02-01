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
#include "helpers/StringHelper.h"
#include "helpers/RegistryHelper.h"

using namespace std;

#define commonKeyPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio"
#define renderKeyPath commonKeyPath L"\\Render"
#define captureKeyPath commonKeyPath L"\\Capture"
static const wchar_t* connectionValueName = L"{a45c254e-df1c-4efd-8020-67d146a850e0},2";
static const wchar_t* deviceValueName = L"{b3f8fa53-0004-438e-9003-51a46e139bfc},6";
static const wchar_t* lfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},1";
static const wchar_t* gfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},2";
static const wchar_t* sfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},5";
static const wchar_t* mfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},6";
static const wchar_t* efxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},7";
static const wchar_t* fxTitleValueName = L"{b725f130-47ef-101a-a5f1-02608c9eebac},10";
static const wchar_t* sfxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},5";
static const wchar_t* mfxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},6";
static const wchar_t* efxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},7";
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

	if(!RegistryHelper::keyExists(keyPath + L"\\FxProperties"))
	{
		originalLfxGuid = APOGUID_NOKEY;
		originalGfxGuid = APOGUID_NOKEY;
	}
	else
	{
		originalLfxGuid = APOGUID_NOVALUE;
		originalGfxGuid = APOGUID_NOVALUE;
		originalSfxGuid = APOGUID_NOVALUE;
		originalMfxGuid = APOGUID_NOVALUE;
		originalEfxGuid = APOGUID_NOVALUE;

		bool found = false;
		if(isInput)
			found = tryAPOGuid(keyPath, lfxGuidValueName, EQUALIZERAPO_LFX_GUID, true);
		else
			found = tryAPOGuid(keyPath, lfxGuidValueName, EQUALIZERAPO_LFX_GUID, true) && tryAPOGuid(keyPath, gfxGuidValueName, EQUALIZERAPO_GFX_GUID, false);
		found = tryAPOGuid(keyPath, lfxGuidValueName, EQUALIZERAPO_LFX_GUID, true) && tryAPOGuid(keyPath, gfxGuidValueName, EQUALIZERAPO_GFX_GUID, false);
		isDual = found;

		if(!found)
			found = tryAPOGuid(keyPath, lfxGuidValueName, EQUALIZERAPO_GFX_GUID, true);
		if(!found)
			found = tryAPOGuid(keyPath, gfxGuidValueName, EQUALIZERAPO_GFX_GUID, false);
		isInstalled = found;

		if(found)
		{
			if(RegistryHelper::keyExists(APP_REGPATH L"\\Child APOs\\" + deviceGuid))
			{
				originalLfxGuid = RegistryHelper::readValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, lfxGuidValueName);
				originalGfxGuid = RegistryHelper::readValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, gfxGuidValueName);
				originalSfxGuid = RegistryHelper::readValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, sfxGuidValueName);
				originalMfxGuid = RegistryHelper::readValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, mfxGuidValueName);
				originalEfxGuid = RegistryHelper::readValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, efxGuidValueName);
			}
			else if(RegistryHelper::valueExists(APP_REGPATH L"\\Child APOs", deviceGuid))
			{
				if(isLFX)
				{
					originalLfxGuid = RegistryHelper::readValue(APP_REGPATH L"\\Child APOs", deviceGuid);
					originalGfxGuid = L"";
				}
				else
				{
					originalLfxGuid = L"";
					originalGfxGuid = RegistryHelper::readValue(APP_REGPATH L"\\Child APOs", deviceGuid);
				}
			}
		}
		else
		{
			if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", lfxGuidValueName))
				originalLfxGuid = RegistryHelper::readValue(keyPath + L"\\FxProperties", lfxGuidValueName);
			if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", gfxGuidValueName))
				originalGfxGuid = RegistryHelper::readValue(keyPath + L"\\FxProperties", gfxGuidValueName);
		}
	}

	return true;
}

bool DeviceAPOInfo::canBeUpgraded()
{
	return isInstalled && !isDual;
}

bool DeviceAPOInfo::isExperimental()
{
	return !isInstalled && originalGfxGuid == APOGUID_NOKEY;
}

void DeviceAPOInfo::install()
{
	RegistryHelper::createKey(APP_REGPATH L"\\Child APOs");
	RegistryHelper::createKey(APP_REGPATH L"\\Child APOs\\" + deviceGuid);

	RegistryHelper::writeValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, lfxGuidValueName, originalLfxGuid);
	if(isInput)
		RegistryHelper::writeValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, gfxGuidValueName, L"");
	else
		RegistryHelper::writeValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, gfxGuidValueName, originalGfxGuid);
	RegistryHelper::writeValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, sfxGuidValueName, originalSfxGuid);
	RegistryHelper::writeValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, mfxGuidValueName, originalMfxGuid);
	RegistryHelper::writeValue(APP_REGPATH L"\\Child APOs\\" + deviceGuid, efxGuidValueName, originalEfxGuid);

	wstring keyPath;
	if(isInput)
		keyPath = captureKeyPath L"\\" + deviceGuid;
	else
		keyPath = renderKeyPath L"\\" + deviceGuid;

	if(originalGfxGuid == APOGUID_NOKEY)
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
	}
	else
	{
		vector<wstring> valuenames;
		if(originalLfxGuid != APOGUID_NOVALUE)
			valuenames.push_back(lfxGuidValueName);
		if(!isInput && originalGfxGuid != APOGUID_NOVALUE)
			valuenames.push_back(gfxGuidValueName);
		if(originalSfxGuid != APOGUID_NOVALUE)
			valuenames.push_back(sfxGuidValueName);
		if(originalMfxGuid != APOGUID_NOVALUE)
			valuenames.push_back(mfxGuidValueName);
		if(originalEfxGuid != APOGUID_NOVALUE)
			valuenames.push_back(efxGuidValueName);

		if(!valuenames.empty())
			RegistryHelper::saveToFile(keyPath + L"\\FxProperties", valuenames,
				L"backup_" + StringHelper::replaceIllegalCharacters(deviceName) + L"_" + StringHelper::replaceIllegalCharacters(connectionName) + L".reg");
	}

	RegistryHelper::writeValue(keyPath + L"\\FxProperties", lfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_LFX_GUID));
	if(!isInput)
		RegistryHelper::writeValue(keyPath + L"\\FxProperties", gfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_GFX_GUID));
}

void DeviceAPOInfo::uninstall()
{
	wstring keyPath;
	if(!isInput)
		keyPath = renderKeyPath L"\\" + deviceGuid;
	else
		keyPath = captureKeyPath L"\\" + deviceGuid;

	if(originalLfxGuid == APOGUID_NOKEY || originalGfxGuid == APOGUID_NOKEY)
	{
		RegistryHelper::deleteKey(keyPath + L"\\FxProperties");
	}
	else
	{
		if(originalLfxGuid == APOGUID_NOVALUE)
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", lfxGuidValueName);
		else if(originalLfxGuid != L"")
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", lfxGuidValueName, originalLfxGuid);

		if(originalGfxGuid == APOGUID_NOVALUE)
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", gfxGuidValueName);
		else if(originalGfxGuid != L"")
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", gfxGuidValueName, originalGfxGuid);

		if(originalSfxGuid != APOGUID_NOVALUE)
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", sfxGuidValueName, originalSfxGuid);
		if(originalMfxGuid != APOGUID_NOVALUE)
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", mfxGuidValueName, originalMfxGuid);
		if(originalEfxGuid != APOGUID_NOVALUE)
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", efxGuidValueName, originalEfxGuid);
	}

	if(RegistryHelper::valueExists(APP_REGPATH L"\\Child APOs", deviceGuid))
		RegistryHelper::deleteValue(APP_REGPATH L"\\Child APOs", deviceGuid);

	if(RegistryHelper::keyExists(APP_REGPATH L"\\Child APOs\\" + deviceGuid))
		RegistryHelper::deleteKey(APP_REGPATH L"\\Child APOs\\" + deviceGuid);

	if(RegistryHelper::keyEmpty(APP_REGPATH L"\\Child APOs"))
		RegistryHelper::deleteKey(APP_REGPATH L"\\Child APOs");
}

bool DeviceAPOInfo::tryAPOGuid(const wstring& keyPath, const wstring& valueName, GUID guid, bool lfx)
{
	if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", valueName))
	{
		wstring apoGuidString = RegistryHelper::readValue(keyPath + L"\\FxProperties", valueName);

		GUID apoGuid;
		if(SUCCEEDED(CLSIDFromString(apoGuidString.c_str(), &apoGuid)))
		{
			if(apoGuid == guid)
			{
				isLFX = lfx;

				return true;
			}
		}
	}

	return false;
}