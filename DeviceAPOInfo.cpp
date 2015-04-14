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
#define childApoPath APP_REGPATH L"\\Child APOs"
static const wchar_t* preMixChildGuidValueName = L"PreMixChild";
static const wchar_t* postMixChildGuidValueName = L"PostMixChild";
static const wchar_t* versionValueName = L"Version";
static const wchar_t* connectionValueName = L"{a45c254e-df1c-4efd-8020-67d146a850e0},2";
static const wchar_t* deviceValueName = L"{b3f8fa53-0004-438e-9003-51a46e139bfc},6";
static const wchar_t* lfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},1";
static const wchar_t* gfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},2";
static const wchar_t* sfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},5";
static const wchar_t* mfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},6";
static const wchar_t* efxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},7";
static const unsigned allGuidValueNameCount = 5;
static const wchar_t* allGuidValueNames[] = {lfxGuidValueName, gfxGuidValueName, sfxGuidValueName, mfxGuidValueName, efxGuidValueName};
static enum GuidValueIndices
{
	LFX_INDEX = 0,
	GFX_INDEX = 1,
	SFX_INDEX = 2,
	MFX_INDEX = 3,
	EFX_INDEX = 4
};
static const wchar_t* fxTitleValueName = L"{b725f130-47ef-101a-a5f1-02608c9eebac},10";
static const wchar_t* sfxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},5";
static const wchar_t* mfxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},6";
static const wchar_t* efxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},7";
static const wchar_t* defaultProcessingModeValue = L"{C18E2F7E-933D-4965-B7D1-1EEF228D2AF3}";
static const wchar_t* installVersion = L"2";

vector<DeviceAPOInfo> DeviceAPOInfo::loadAllInfos(bool input)
{
	vector<DeviceAPOInfo> result;
	vector<wstring> deviceGuidStrings = RegistryHelper::enumSubKeys(input ? captureKeyPath : renderKeyPath);
	for(vector<wstring>::iterator it = deviceGuidStrings.begin(); it != deviceGuidStrings.end(); it++)
	{
		wstring deviceGuidString = *it;

		DeviceAPOInfo info;
		if(info.load(deviceGuidString))
		{
			info.selectedInstallState = info.currentInstallState;
			result.push_back(info);
		}
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
	currentInstallState.installMode = INSTALL_LFX_GFX;
	version = L"0";
	preMixChildGuid = L"";
	postMixChildGuid = L"";
	currentInstallState.installPreMix = true;
	currentInstallState.installPostMix = !isInput;
	currentInstallState.useOriginalAPOPreMix = true;
	currentInstallState.useOriginalAPOPostMix = !isInput;

	if(!RegistryHelper::keyExists(keyPath + L"\\FxProperties"))
	{
		for(int i=0; i < allGuidValueNameCount; i++)
			originalApoGuids[i] = APOGUID_NOKEY;
	}
	else
	{
		for(int i=0; i < allGuidValueNameCount; i++)
		{
			if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", allGuidValueNames[i]))
				originalApoGuids[i] = RegistryHelper::readValue(keyPath + L"\\FxProperties", allGuidValueNames[i]);
			else
				originalApoGuids[i] = APOGUID_NOVALUE;
		}

		bool found = false;
		bool foundAt[allGuidValueNameCount];
		for(int i=0; i < allGuidValueNameCount; i++)
		{
			foundAt[i] = false;

			if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", allGuidValueNames[i]))
			{
				wstring apoGuidString = RegistryHelper::readValue(keyPath + L"\\FxProperties", allGuidValueNames[i]);

				GUID apoGuid;
				if(SUCCEEDED(CLSIDFromString(apoGuidString.c_str(), &apoGuid)))
				{
					if(apoGuid == EQUALIZERAPO_PRE_MIX_GUID || apoGuid == EQUALIZERAPO_POST_MIX_GUID)
					{
						foundAt[i] = true;
						found = true;
					}
				}
			}
		}

		if(found)
		{
			isInstalled = true;

			if(RegistryHelper::keyExists(childApoPath L"\\" + deviceGuid))
			{
				if(RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, versionValueName))
				{
					version = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, versionValueName);
					if(version != installVersion)
						throw RegistryException(L"Unsupported version of APO installation detected! Please uninstall newer Equalizer APO before using this version of Configurator.");
				}
				else
				{
					version = L"1";
				}

				for(int i=0; i < allGuidValueNameCount; i++)
				{
					if(RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, allGuidValueNames[i]))
						originalApoGuids[i] = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, allGuidValueNames[i]);
				}

				if(version == installVersion)
				{
					if(RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, preMixChildGuidValueName))
						preMixChildGuid = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, preMixChildGuidValueName);
					if(RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, postMixChildGuidValueName))
						postMixChildGuid = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, postMixChildGuidValueName);

					currentInstallState.installPreMix = foundAt[LFX_INDEX] || foundAt[SFX_INDEX];
					currentInstallState.installPostMix = foundAt[GFX_INDEX] || foundAt[MFX_INDEX] || foundAt[EFX_INDEX];

					if(preMixChildGuid == L"")
						currentInstallState.useOriginalAPOPreMix = false;
					if(postMixChildGuid == L"")
						currentInstallState.useOriginalAPOPostMix = false;

					if(foundAt[LFX_INDEX] || foundAt[GFX_INDEX])
						currentInstallState.installMode = INSTALL_LFX_GFX;
					else if(foundAt[EFX_INDEX])
						currentInstallState.installMode = INSTALL_SFX_EFX;
					else if(foundAt[SFX_INDEX] || foundAt[MFX_INDEX])
						currentInstallState.installMode = INSTALL_SFX_MFX;
				}
				else
				{
					if(RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, lfxGuidValueName))
					{
						preMixChildGuid = originalApoGuids[0];
						postMixChildGuid = originalApoGuids[1];
					}
					else
					{
						preMixChildGuid = originalApoGuids[2];
						postMixChildGuid = originalApoGuids[3];
					}
				}
			}
			else if(RegistryHelper::valueExists(childApoPath, deviceGuid))
			{
				for(int i=0; i < allGuidValueNameCount; i++)
				{
					if(foundAt[i])
					{
						originalApoGuids[i] = RegistryHelper::readValue(childApoPath, deviceGuid);
						if(i == LFX_INDEX || i == SFX_INDEX)
							preMixChildGuid = originalApoGuids[i];
						else
							postMixChildGuid = originalApoGuids[i];
						break;
					}
				}
			}
		}
	}

	return true;
}

bool DeviceAPOInfo::canBeUpgraded()
{
	return isInstalled && version != installVersion;
}

bool DeviceAPOInfo::hasChanges()
{
	return isInstalled && selectedInstallState != currentInstallState;
}

bool DeviceAPOInfo::isExperimental()
{
	return !isInstalled && originalApoGuids[0] == APOGUID_NOKEY;
}

wstring DeviceAPOInfo::getOriginalAPOPreMix()
{
	wstring guid;
	switch(selectedInstallState.installMode)
	{
	case INSTALL_LFX_GFX:
		guid = originalApoGuids[LFX_INDEX];
		if(RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
		{
			if(originalApoGuids[LFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[GFX_INDEX] == APOGUID_NOVALUE)
				guid = originalApoGuids[SFX_INDEX];
		}
		break;
	case INSTALL_SFX_MFX:
		guid = originalApoGuids[SFX_INDEX];
		if(originalApoGuids[SFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[MFX_INDEX] == APOGUID_NOVALUE)
			guid = originalApoGuids[LFX_INDEX];
		break;
	case INSTALL_SFX_EFX:
		guid = originalApoGuids[SFX_INDEX];
		if(originalApoGuids[SFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[EFX_INDEX] == APOGUID_NOVALUE)
			guid = originalApoGuids[LFX_INDEX];
		break;
	}

	if(guid == APOGUID_NOKEY || guid == APOGUID_NOVALUE)
		guid = L"";

	return guid;
}

wstring DeviceAPOInfo::getOriginalAPOPostMix()
{
	wstring guid;
	switch(selectedInstallState.installMode)
	{
	case INSTALL_LFX_GFX:
		guid = originalApoGuids[GFX_INDEX];
		if(RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
		{
			if(originalApoGuids[LFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[GFX_INDEX] == APOGUID_NOVALUE)
				guid = originalApoGuids[MFX_INDEX];
		}
		break;
	case INSTALL_SFX_MFX:
		guid = originalApoGuids[MFX_INDEX];
		if(originalApoGuids[SFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[MFX_INDEX] == APOGUID_NOVALUE)
			guid = originalApoGuids[GFX_INDEX];
		break;
	case INSTALL_SFX_EFX:
		guid = originalApoGuids[EFX_INDEX];
		if(originalApoGuids[SFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[EFX_INDEX] == APOGUID_NOVALUE)
			guid = originalApoGuids[GFX_INDEX];
		break;
	}

	if(guid == APOGUID_NOKEY || guid == APOGUID_NOVALUE)
		guid = L"";

	return guid;
}

void DeviceAPOInfo::install()
{
	if(!selectedInstallState.installPreMix && !selectedInstallState.installPostMix)
		return;

	RegistryHelper::createKey(childApoPath);
	RegistryHelper::createKey(childApoPath L"\\" + deviceGuid);

	wstring keyPath;
	if(!isInput)
		keyPath = renderKeyPath L"\\" + deviceGuid;
	else
		keyPath = captureKeyPath L"\\" + deviceGuid;

	if(!RegistryHelper::keyExists(keyPath + L"\\FxProperties"))
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

		for(int i=0; i < allGuidValueNameCount; i++)
		{
			RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, allGuidValueNames[i], APOGUID_NOKEY);
		}
	}
	else
	{
		vector<wstring> valuenames;

		for(int i=0; i < allGuidValueNameCount; i++)
		{
			wstring apoGuidString = APOGUID_NOVALUE;
			if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", allGuidValueNames[i]))
			{
				apoGuidString = RegistryHelper::readValue(keyPath + L"\\FxProperties", allGuidValueNames[i]);
				valuenames.push_back(allGuidValueNames[i]);
			}

			RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, allGuidValueNames[i], apoGuidString);
		}

		if(!valuenames.empty())
			RegistryHelper::saveToFile(keyPath + L"\\FxProperties", valuenames,
				L"backup_" + StringHelper::replaceIllegalCharacters(deviceName) + L"_" + StringHelper::replaceIllegalCharacters(connectionName) + L".reg");
	}

	wstring preMixValue;
	wstring postMixValue;
	if(selectedInstallState.useOriginalAPOPreMix)
		preMixValue = getOriginalAPOPreMix();
	if(selectedInstallState.useOriginalAPOPostMix)
		postMixValue = getOriginalAPOPostMix();
	RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, preMixChildGuidValueName, preMixValue);
	RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, postMixChildGuidValueName, postMixValue);

	RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, versionValueName, installVersion);

	if(selectedInstallState.installMode == INSTALL_LFX_GFX)
	{
		if(selectedInstallState.installPreMix)
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", lfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID));
		if(selectedInstallState.installPostMix && !isInput)
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", gfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID));
		if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", sfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", sfxGuidValueName);
		if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", mfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", mfxGuidValueName);
		if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", efxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", efxGuidValueName);
	}
	else if(selectedInstallState.installMode == INSTALL_SFX_MFX)
	{
		if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", lfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", lfxGuidValueName);
		if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", gfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", gfxGuidValueName);
		if(selectedInstallState.installPreMix)
		{
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", sfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID));
			if(!RegistryHelper::valueExists(keyPath + L"\\FxProperties", sfxProcessingModesValueName))
				RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", sfxProcessingModesValueName, defaultProcessingModeValue);
		}
		if(selectedInstallState.installPostMix && !isInput)
		{
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", mfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID));
			if(!RegistryHelper::valueExists(keyPath + L"\\FxProperties", mfxProcessingModesValueName))
				RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", mfxProcessingModesValueName, defaultProcessingModeValue);
		}
		// don't change efx
	}
	else if(selectedInstallState.installMode == INSTALL_SFX_EFX)
	{
		if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", lfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", lfxGuidValueName);
		if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", gfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", gfxGuidValueName);
		if(selectedInstallState.installPreMix)
		{
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", sfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID));
			if(!RegistryHelper::valueExists(keyPath + L"\\FxProperties", sfxProcessingModesValueName))
				RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", sfxProcessingModesValueName, defaultProcessingModeValue);
		}
		// don't change mfx
		if(selectedInstallState.installPostMix && !isInput)
		{
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", efxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID));
			if(!RegistryHelper::valueExists(keyPath + L"\\FxProperties", efxProcessingModesValueName))
				RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", efxProcessingModesValueName, defaultProcessingModeValue);
		}
	}
}

void DeviceAPOInfo::uninstall()
{
	wstring keyPath;
	if(!isInput)
		keyPath = renderKeyPath L"\\" + deviceGuid;
	else
		keyPath = captureKeyPath L"\\" + deviceGuid;

	if(originalApoGuids[0] == APOGUID_NOKEY)
	{
		RegistryHelper::deleteKey(keyPath + L"\\FxProperties");
	}
	else
	{
		for(int i=0; i < allGuidValueNameCount; i++)
		{
			if(originalApoGuids[i] == APOGUID_NOVALUE)
			{
				if(RegistryHelper::valueExists(keyPath + L"\\FxProperties", allGuidValueNames[i]))
					RegistryHelper::deleteValue(keyPath + L"\\FxProperties", allGuidValueNames[i]);
			}
			else if(originalApoGuids[i] != L"")
			{
				RegistryHelper::writeValue(keyPath + L"\\FxProperties", allGuidValueNames[i], originalApoGuids[i]);
			}
		}
	}

	if(RegistryHelper::valueExists(childApoPath, deviceGuid))
		RegistryHelper::deleteValue(childApoPath, deviceGuid);

	if(RegistryHelper::keyExists(childApoPath L"\\" + deviceGuid))
		RegistryHelper::deleteKey(childApoPath L"\\" + deviceGuid);

	if(RegistryHelper::keyEmpty(childApoPath))
		RegistryHelper::deleteKey(childApoPath);
}
