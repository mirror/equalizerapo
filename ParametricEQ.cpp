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

#include <cstdio>
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Shlwapi.h>
#include <Ks.h>
#include <KsMedia.h>

#include "StringHelper.h"
#include "RegistryHelper.h"
#include "LogHelper.h"
#include "ParametricEQ.h"

using namespace std;
using namespace stdext;

static wregex regexType(L"^\\s*ON\\s+([A-Za-z]+)");
static wregex regexFreq(L"\\s+Fc\\s*([0-9.]+)\\s*H\\s*z");
static wregex regexGain(L"\\s+Gain\\s*([-+0-9.]+)\\s*dB");
static wregex regexQ(L"\\s+Q\\s*([0-9.]+)");
static wregex regexBW(L"\\s+BW\\s+Oct\\s*([0-9.]+)");
static wregex regexSlope(L"^\\s*([0-9.]+)\\s*dB");

ChannelData::ChannelData()
{
	preamp = 1.0f;
	filterCount = 0;
}

ParametricEQ::ParametricEQ()
{
	channelCount = 0;
	channelData = NULL;
	lastInputWasSilent = false;
	threadHandle = NULL;

	channelNameToPosMap[L"L"] = SPEAKER_FRONT_LEFT;
	channelNameToPosMap[L"R"] = SPEAKER_FRONT_RIGHT;
	channelNameToPosMap[L"C"] = SPEAKER_FRONT_CENTER;
	channelNameToPosMap[L"SUB"] = SPEAKER_LOW_FREQUENCY;
	channelNameToPosMap[L"RL"] = SPEAKER_BACK_LEFT;
	channelNameToPosMap[L"RR"] = SPEAKER_BACK_RIGHT;
	channelNameToPosMap[L"RC"] = SPEAKER_BACK_CENTER;
	channelNameToPosMap[L"SL"] = SPEAKER_SIDE_LEFT;
	channelNameToPosMap[L"SR"] = SPEAKER_SIDE_RIGHT;

	for(hash_map<wstring, int>::iterator it=channelNameToPosMap.begin(); it!=channelNameToPosMap.end(); it++)
		channelPosToNameMap[it->second] = it->first;

	filterNameToTypeMap[L"PK"] = BiQuad::PEAKING;
	filterNameToTypeMap[L"PEQ"] = BiQuad::PEAKING;
	filterNameToTypeMap[L"Modal"] = BiQuad::PEAKING;
	filterNameToTypeMap[L"LP"] = BiQuad::LOW_PASS;
	filterNameToTypeMap[L"HP"] = BiQuad::HIGH_PASS;
	filterNameToTypeMap[L"LPQ"] = BiQuad::LOW_PASS;
	filterNameToTypeMap[L"HPQ"] = BiQuad::HIGH_PASS;
	filterNameToTypeMap[L"BP"] = BiQuad::BAND_PASS;
	filterNameToTypeMap[L"LS"] = BiQuad::LOW_SHELF;
	filterNameToTypeMap[L"HS"] = BiQuad::HIGH_SHELF;
	filterNameToTypeMap[L"NO"] = BiQuad::NOTCH;
	filterNameToTypeMap[L"AP"] = BiQuad::ALL_PASS;

	filterTypeToDescriptionMap[BiQuad::PEAKING] = L"peaking";
	filterTypeToDescriptionMap[BiQuad::LOW_PASS] = L"low-pass";
	filterTypeToDescriptionMap[BiQuad::HIGH_PASS] = L"high-pass";
	filterTypeToDescriptionMap[BiQuad::BAND_PASS] = L"band-pass";
	filterTypeToDescriptionMap[BiQuad::LOW_SHELF] = L"low-shelf";
	filterTypeToDescriptionMap[BiQuad::HIGH_SHELF] = L"high-shelf";
	filterTypeToDescriptionMap[BiQuad::NOTCH] = L"notch";
	filterTypeToDescriptionMap[BiQuad::ALL_PASS] = L"all-pass";
}

ParametricEQ::~ParametricEQ()
{
	// Make sure notification thread is terminated before cleaning up, otherwise deleted memory might be accessed in loadConfig
	if(threadHandle != NULL)
	{
		SetEvent(shutdownEvent);
		if(WaitForSingleObject(threadHandle, INFINITE) == WAIT_OBJECT_0)
		{
			TraceF(L"Successfully terminated directory change notification thread");
		}
		CloseHandle(shutdownEvent);
		CloseHandle(threadHandle);
		threadHandle = NULL;
	}

	if(channelData != NULL)
	{
		delete[] channelData;
		channelData = NULL;
	}
}

void ParametricEQ::setDeviceInfo(const wstring& deviceName, const wstring& connectionName, const wstring& deviceGuid)
{
	this->deviceName = deviceName;
	this->connectionName = connectionName;
	this->deviceGuid = deviceGuid;
}

void ParametricEQ::initialize(float sampleRate, unsigned channelCount, unsigned channelMask)
{
	this->sampleRate = sampleRate;
    this->channelCount = channelCount;
	channelData = new ChannelData[channelCount];

	if(channelMask == 0)
	{
		switch(channelCount)
		{
		case 1:
			channelMask = KSAUDIO_SPEAKER_MONO;
			break;
		case 2:
			channelMask = KSAUDIO_SPEAKER_STEREO;
			break;
		case 4:
			channelMask = KSAUDIO_SPEAKER_QUAD;
			break;
		case 6:
			channelMask = KSAUDIO_SPEAKER_5POINT1_SURROUND;
			break;
		case 8:
			channelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
			break;
		}
	}
	this->channelMask = channelMask;

	wstringstream channelNames;
	unsigned c=0;
	for(int i=0; i<31; i++)
	{
		int channelPos = 1<<i;
		if(channelMask & channelPos)
		{
			c++;
			if(channelNames.tellp() > 0)
				channelNames << L" ";
			if(channelPosToNameMap.find(channelPos) != channelPosToNameMap.end())
				channelNames << channelPosToNameMap[channelPos];
			else
				channelNames << c;
		}
	}
	TraceF(L"%d channels for this device: %s", channelCount, channelNames.str().c_str());

	try
	{
		configPath = RegistryHelper::readValue(APP_REGPATH, L"ConfigPath");
	}
	catch(RegistryException e)
	{
		LogF(L"Can't read config path because of: %s", e.getMessage().c_str());
		return;
	}
	if(configPath != L"")
	{
		loadConfig();
		
		if(threadHandle == NULL)
		{
			shutdownEvent = CreateEventW(NULL, true, false, NULL);
			threadHandle = CreateThread(NULL, 0, notificationThread, this, 0, NULL);
			if(threadHandle == INVALID_HANDLE_VALUE)
				threadHandle = NULL;
			else
			{
				TraceF(L"Successfully created directory change notification thread for %s and its subtree", configPath.c_str());
			}
		}
	}
}

void ParametricEQ::loadConfig()
{
	for(unsigned c=0; c<channelCount; c++)
	{
		channelData[c].loadPreamp = 1.0f;
		channelData[c].loadFilterCount = 0;
	}

	vector<bool> selectedChannels(channelCount, true);
	loadConfig(configPath + L"\\config.txt", selectedChannels);

	unsigned loadFilterCount = 0;
	for(unsigned c=0; c<channelCount; c++)
	{
		channelData[c].preamp = channelData[c].loadPreamp;
		channelData[c].filterCount = channelData[c].loadFilterCount;
		loadFilterCount += channelData[c].loadFilterCount;
	}

	wstringstream channelFilterCounts;
	unsigned c=0;
	for(int i=0; i<31; i++)
	{
		int channelPos = 1<<i;
		if(channelMask & channelPos)
		{
			c++;
			if(channelFilterCounts.tellp() > 0)
				channelFilterCounts << L" ";
			if(channelPosToNameMap.find(channelPos) != channelPosToNameMap.end())
				channelFilterCounts << channelPosToNameMap[channelPos];
			else
				channelFilterCounts << c;

			channelFilterCounts << ":" << channelData[c-1].loadFilterCount;
		}
	}

	TraceF(L"%d filters loaded: %s", loadFilterCount, channelFilterCounts.str().c_str());
}

void ParametricEQ::loadConfig(const wstring& path, vector<bool> selectedChannels)
{
	TraceF(L"Loading configuration from %s", path.c_str());

	ifstream inputStream(path);
	if(!inputStream.good())
	{
		LogF(L"Config file not found at %s!", path.c_str());
		return;
	}

	boolean deviceMatches = true;
	wstring deviceString = StringHelper::toLowerCase(deviceName + L" " + connectionName + L" " + deviceGuid);

	while(!inputStream.eof())
	{
		string encodedLine;
		getline(inputStream, encodedLine);

		wstring line = StringHelper::toWString(encodedLine, CP_UTF8);
		if(line.find(L'\uFFFD') != -1)
			line = StringHelper::toWString(encodedLine, CP_ACP);

		size_t pos = line.find(L':');
		if(pos != -1)
		{
			wstring key = line.substr(0, pos);
			wstring value = line.substr(pos + 1);

			if(key == L"Device")
			{
				value = value + L";";

				vector<vector<wstring>> fullList;
				vector<wstring> currentList;
				wstring currentWord;

				for(unsigned i=0; i<value.length(); i++)
				{
					wchar_t c = value[i];
					if(c == L' ' || c == L';')
					{
						if(currentWord.length() > 0)
						{
							currentList.push_back(currentWord);
							currentWord.clear();
						}
						if(c == L';' && currentList.size() > 0)
						{
							fullList.push_back(currentList);
							currentList.clear();
						}
					}
					else
					{
						currentWord += c;
					}
				}

				boolean matches = false;

				for(unsigned i=0; i<fullList.size(); i++)
				{
					matches = true;

					if(fullList[i].size() == 1 && StringHelper::toLowerCase(fullList[i][0]) == L"all")
						break;

					for(unsigned j=0; j<fullList[i].size(); j++)
					{
						wstring word = StringHelper::toLowerCase(fullList[i][j]);
						if(deviceString.find(word) == -1)
						{
							matches = false;
							break;
						}
					}

					if(matches)
						break;
				}

				TraceF(L"%satching pattern \"%s\" with device \"%s\" \"%s\" \"%s\"", matches ? L"M" : L"Not m", value.substr(0, value.length()-1).c_str(), deviceName.c_str(), connectionName.c_str(), deviceGuid.c_str());
				deviceMatches = matches;
			}

			if(!deviceMatches)
				continue;

			if(key == L"Channel")
			{
				selectedChannels = vector<bool>(channelCount, false);

				value = value + L" ";

				wstring currentWord;
				for(unsigned i=0; i<value.length(); i++)
				{
					wchar_t c = towupper(value[i]);

					if(c == L' ')
					{
						if(currentWord.length() > 0)
						{
							int channelNr = -1;

							if(currentWord == L"ALL")
							{
								selectedChannels = vector<bool>(channelCount, true);
							}
							else if(iswdigit(currentWord[0]))
							{
								channelNr = wcstol(currentWord.c_str(), NULL, 10) - 1;
							}
							else
							{
								int channelPos = -1;
								if(channelNameToPosMap.find(currentWord) != channelNameToPosMap.end())
									channelPos = channelNameToPosMap[currentWord];
								else
									LogF(L"Invalid channel position %s", currentWord.c_str());

								if(channelPos != -1)
									channelNr = getChannelNumber(channelPos);
							}

							if(channelNr != -1 && channelNr < (int)channelCount)
							{
								selectedChannels[channelNr] = true;
							}

							currentWord.clear();
						}
					}
					else
					{
						currentWord += c;
					}
				}

				wstringstream channelNumbers;
				for(unsigned c=0; c<channelCount; c++)
				{
					if(selectedChannels[c])
					{
						if(channelNumbers.tellp() > 0)
							channelNumbers << L", ";

						channelNumbers << c+1;
					}
				}

				TraceF(L"Selecting channel(s) number %s", channelNumbers.str().c_str());
			}
			else if(key.find(L"Filter") == 0)
			{
				//Conversion to period as decimal mark, if needed
				value = StringHelper::replaceCharacters(value, L",", L'.');

				wsmatch match;
				wstring typeString;

				bool found = regex_search(value, match, regexType);
				if(found)
				{
					typeString = match.str(1);
					if(filterNameToTypeMap.find(typeString) != filterNameToTypeMap.end())
					{
						BiQuad::Type type = filterNameToTypeMap[typeString];
						wstring typeDescription = filterTypeToDescriptionMap[type];
						value = match.suffix().str();

						wstringstream stream;
						stream << L"Adding " << typeDescription << L" filter";

						float freq = 0;
						float gain = 0;
						float bandwidthOrQOrS = 0;
						bool isBandwidth = false;
						bool error = false;

						found = regex_search(value, match, regexFreq);
						if(found)
						{
							wstring freqString = match.str(1);
							freq = getFreq(freqString);
							stream << " with frequency " << freq << " Hz";
						}
						else
						{
							LogF(L"No frequency given in filter string %s%s", typeString.c_str(), value.c_str());
							error = true;
						}

						found = regex_search(value, match, regexGain);
						if(found)
						{
							if(type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::NOTCH || type == BiQuad::ALL_PASS)
								TraceF(L"Ignoring gain for filter of type %s", typeDescription.c_str());
							else
							{
								wstring gainString = match.str(1);
								gain = (float)wcstod(gainString.c_str(), NULL);
								if(type == BiQuad::PEAKING)
									stream << ", gain " << gain << " dB";
								else
									stream << " and gain " << gain << " dB";
							}
						}
						else if(type == BiQuad::PEAKING || type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
						{
							LogF(L"No gain given in filter string %s%s", typeString.c_str(), value.c_str());
							error = true;
						}

						found = regex_search(value, match, regexQ);
						if(found)
						{
							if(type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
								TraceF(L"Ignoring Q for filter of type %s", typeDescription.c_str());
							else
							{
								wstring qString = match.str(1);
								bandwidthOrQOrS = (float)wcstod(qString.c_str(), NULL);
								stream << " and Q " << bandwidthOrQOrS;
							}
						}

						found = regex_search(value, match, regexBW);
						if(found)
						{
							if(type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
								TraceF(L"Ignoring bandwidth for filter of type %s", typeDescription.c_str());
							else
							{
								wstring bwString = match.str(1);
								bandwidthOrQOrS = (float)wcstod(bwString.c_str(), NULL);
								isBandwidth = true;
								stream << " and bandwidth " << bandwidthOrQOrS << " octaves";
							}
						}

						found = regex_search(value, match, regexSlope);
						if(found)
						{
							if(!(type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF))
								TraceF(L"Ignoring slope for filter of type %s", typeDescription.c_str());
							else
							{
								wstring slopeString = match.str(1);
								bandwidthOrQOrS = (float)wcstod(slopeString.c_str(), NULL);
								stream << " and slope " << bandwidthOrQOrS << " dB";
							}
						}

						if(bandwidthOrQOrS == 0)
						{
							if(type == BiQuad::PEAKING || type == BiQuad::ALL_PASS)
							{
								LogF(L"No Q or bandwidth given in filter string %s%s", typeString.c_str(), value.c_str());
								error = true;
							}
							else if(type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::BAND_PASS)
							{
								bandwidthOrQOrS = (float)M_SQRT1_2;
							}
							else if(type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
							{
								bandwidthOrQOrS = 0.9f; // found out by experimentation with RoomEQWizard
							}
							else if(type == BiQuad::NOTCH)
							{
								bandwidthOrQOrS = 30.0f; // found out by experimentation with RoomEQWizard
							}
						}
						else if(type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
						{
							// Maximum S is 1 for 12 dB
							bandwidthOrQOrS /= 12.0f;
							// frequency adjustment for DCX2496
							float centerFreqFactor = pow(10.0f, abs(gain) / 80.0f / bandwidthOrQOrS);
							if(type == BiQuad::LOW_SHELF)
								freq *= centerFreqFactor;
							else
								freq /= centerFreqFactor;
						}

						if(!error)
						{
							TraceF(L"%s", stream.str().c_str());

							for(unsigned c=0; c<channelCount; c++)
							{
								if(selectedChannels[c] && channelData[c].loadFilterCount < (sizeof(channelData[c].filters)/sizeof(BiQuad)))
								{
									channelData[c].filters[channelData[c].loadFilterCount++] = BiQuad(type, gain, freq, sampleRate, bandwidthOrQOrS, isBandwidth);
								}
							}
						}
					}
					else if(typeString != L"None")
					{
						LogF(L"Invalid filter type %s", typeString.c_str());
					}
				}
			}
			else if(key == L"Preamp")
			{
				//Conversion to period as decimal mark, if needed
				value = StringHelper::replaceCharacters(value, L",", L'.');

				float preamp_dB;
				int matched = swscanf_s(value.c_str(), L" %f dB", &preamp_dB);
				if(matched == 1)
				{
					for(unsigned c=0; c<channelCount; c++)
					{
						if(selectedChannels[c])
						{
							channelData[c].loadPreamp *= pow(10.0f, preamp_dB / 20.0f);
						}
					}

					TraceF(L"Adjusting preamp by %g dB", preamp_dB);
				}
			}
			else if(key == L"Include")
			{
				while(value.length() > 0 && iswspace(value[0]))
					value = value.substr(1);

				wstring includePath;
				if(PathIsRelativeW(value.c_str()))
				{
					wchar_t filePath[MAX_PATH];
					path._Copy_s(filePath, sizeof(filePath), MAX_PATH);
					if(path.size() < MAX_PATH)
						filePath[path.size()] = L'\0';
					else
						filePath[MAX_PATH - 1] = L'\0';
					PathRemoveFileSpecW(filePath);
					PathAppendW(filePath, value.c_str());
					includePath = filePath;
				}
				else
					includePath = value;

				loadConfig(includePath, selectedChannels);
			}
		}
	}
}

void ParametricEQ::process(float *output, float *input, unsigned frameCount)
{
	bool inputSilent = true;

    for (unsigned i = 0; i < frameCount * channelCount; i++)
	{
		if(input[i] != 0)
		{
			inputSilent = false;
			break;
		}
	}

	if(inputSilent)
	{
		if(lastInputWasSilent)
		{
			//Avoid processing cost if silence would be output anyway
			if(input != output)
				memset(output, 0, frameCount * channelCount * sizeof(float));

			return;
		}
		else
			lastInputWasSilent = true;
	}
	else
		lastInputWasSilent = false;

	//Avoid denormals
    for (unsigned c=0; c<channelCount; c++)
    {
        for(unsigned f=0; f<channelData[c].filterCount; f++)
		{
			channelData[c].filters[f].removeDenormals();
		}
	}

    for (unsigned i = 0; i < frameCount * channelCount; i+=channelCount)
        for (unsigned c=0; c<channelCount; c++)
        {
            float sample = input[i+c];
            
            for(unsigned f=0; f<channelData[c].filterCount; f++)
            {
				sample = channelData[c].filters[f].process(sample);
            }
            
            output[i+c] = sample * channelData[c].preamp;
        }
}

float ParametricEQ::getFreq(const wstring& freqString)
{
	float result;
	int matched = swscanf_s(freqString.c_str(), L"%f", &result);
	if(matched == 1)
	{
		if(freqString.length() >= 5)
		{
			if(freqString[freqString.length() - 4] == L'.')
			{
				//Interpret as thousands separator because of Room EQ Wizard
				result *= 1000.0f;
			}
		}

		return result;
	}
	else
		return -1.0f;
}

unsigned ParametricEQ::getChannelNumber(unsigned position)
{
	//Special handling to accept "wrong", but unambiguous positions
	if(channelMask == KSAUDIO_SPEAKER_5POINT1_SURROUND)
	{
		if(position == SPEAKER_BACK_LEFT)
			return 4;
		else if(position == SPEAKER_BACK_RIGHT)
			return 5;
	}
	else if(channelMask == KSAUDIO_SPEAKER_5POINT1)
	{
		if(position == SPEAKER_SIDE_LEFT)
			return 4;
		else if(position == SPEAKER_SIDE_RIGHT)
			return 5;
	}

	if((channelMask & position) == 0)
		return -1;

	int channelNr = 0;
	for(unsigned i=1; i<position; i<<=1)
	{
		if(channelMask & i)
			channelNr++;
	}

	return channelNr;
}

unsigned long __stdcall ParametricEQ::notificationThread(void* parameter)
{
	ParametricEQ* peq = (ParametricEQ*)parameter;

	HANDLE notificationHandle = FindFirstChangeNotificationW(peq->configPath.c_str(), true, FILE_NOTIFY_CHANGE_LAST_WRITE);
	if(notificationHandle == INVALID_HANDLE_VALUE)
		notificationHandle = NULL;

	HANDLE handles[2] = {peq->shutdownEvent, notificationHandle};
	while(true)
	{
		DWORD which = WaitForMultipleObjects(2, handles, false, INFINITE);

		if(which == WAIT_OBJECT_0)
		{
			//Shutdown
			break;
		}
		else
		{
			FindNextChangeNotification(notificationHandle);
			//Wait for second event within 100 milliseconds to avoid loading twice
			WaitForMultipleObjects(1, &notificationHandle, false, 100);
			peq->loadConfig();
			FindNextChangeNotification(notificationHandle);
		}
	}

	FindCloseChangeNotification(notificationHandle);

	return 0;
}
