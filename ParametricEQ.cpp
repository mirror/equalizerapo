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

BiQuad::BiQuad(float dbGain, float freq, float srate, float bandwidthOrQ, bool isQ)
{
    float A = pow(10, dbGain / 40);
    float omega = 2 * (float)M_PI * freq / srate;
    float sn = sin(omega);
    float cs = cos(omega);
	float alpha;
	if(isQ)
		alpha = sn / (2 * bandwidthOrQ);
	else
		alpha = sn * sinh((float)M_LN2/2 * bandwidthOrQ * omega / sn);
    
    float temp = 1 + (alpha /A);
    a0 = (1 + (alpha * A)) / temp;
    a[0] = (-2 * cs) / temp;
    a[1] = (1 - (alpha * A)) / temp;
    a[2] = - (-2 * cs) / temp;
    a[3] = - (1 - (alpha /A)) / temp;

	x1 = 0;
	x2 = 0;
	y1 = 0;
	y2 = 0;
}

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
}

ParametricEQ::~ParametricEQ()
{
	// Make sure notification thread is terminated before cleaning up, otherwise deleted memory might be accessed in loadConfig
	if(threadHandle != NULL)
	{
		SetEvent(shutdownEvent);
		if(WaitForSingleObject(threadHandle, INFINITE) == WAIT_OBJECT_0)
		{
			TraceF(L"Successfully terminated directory change notification thread", configPath.c_str());
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

	try
	{
		configPath = RegistryHelper::readValue(APP_REGPATH, L"ConfigPath");
	}
	catch(RegistryException e)
	{
		LogF(L"Can't read config path because of: %s", e.getMessage());
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

	TraceF(L"%d filters loaded", loadFilterCount);
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

				TraceF(L"%satching device \"%s\" \"%s\" \"%s\" with pattern \"%s\"", matches ? L"M" : L"Not m", deviceName.c_str(), connectionName.c_str(), deviceGuid.c_str(), value.c_str());
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
					wchar_t c = towlower(value[i]);

					if(c == L' ')
					{
						if(currentWord.length() > 0)
						{
							int channelNr = -1;

							if(currentWord == L"all")
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
								if(currentWord == L"l")
									channelPos = SPEAKER_FRONT_LEFT;
								else if(currentWord == L"r")
									channelPos = SPEAKER_FRONT_RIGHT;
								else if(currentWord == L"c")
									channelPos = SPEAKER_FRONT_CENTER;
								else if(currentWord == L"sub")
									channelPos = SPEAKER_LOW_FREQUENCY;
								else if(currentWord == L"rl")
									channelPos = SPEAKER_BACK_LEFT;
								else if(currentWord == L"rr")
									channelPos = SPEAKER_BACK_RIGHT;
								else if(currentWord == L"rc")
									channelPos = SPEAKER_BACK_CENTER;
								else if(currentWord == L"sl")
									channelPos = SPEAKER_SIDE_LEFT;
								else if(currentWord == L"sr")
									channelPos = SPEAKER_SIDE_RIGHT;
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

				wstringstream channelStream;
				for(unsigned c=0; c<channelCount; c++)
				{
					if(selectedChannels[c])
					{
						if(channelStream.tellp() > 0)
							channelStream << L", ";
						channelStream << c+1;
					}
				}

				TraceF(L"Selecting channel(s) %s", channelStream.str().c_str());
			}
			else if(key.find(L"Filter") == 0)
			{
				//Conversion to period as decimal mark, if needed
				value = StringHelper::replaceCharacters(value, L",", L'.');

				wchar_t freqString[10];
				float freq, gain, bandwidth;

				int matched = swscanf_s(value.c_str(), L" ON PEQ Fc %9s Hz Gain %f dB BW Oct %f", &freqString, 10, &gain, &bandwidth);
				if(matched == 3 && (freq = getFreq(freqString)) != -1.0f)
				{
					for(unsigned c=0; c<channelCount; c++)
					{
						if(selectedChannels[c] && channelData[c].loadFilterCount < (sizeof(channelData[c].filters)/sizeof(BiQuad)))
						{
							channelData[c].filters[channelData[c].loadFilterCount++] = BiQuad(gain, freq, sampleRate, bandwidth, false);
						}
					}
					TraceF(L"Adding filter with center frequency %g Hz, gain %g dB and bandwidth %g octaves", freq, gain, bandwidth);
				}
				else
				{
					float q;
					matched = swscanf_s(value.c_str(), L" ON PK Fc %9s Hz Gain %f dB Q %f", &freqString, 10, &gain, &q);
					if(matched == 3 && (freq = getFreq(freqString)) != -1.0f)
					{
						for(unsigned c=0; c<channelCount; c++)
						{
							if(selectedChannels[c] && channelData[c].loadFilterCount < (sizeof(channelData[c].filters)/sizeof(BiQuad)))
							{
								channelData[c].filters[channelData[c].loadFilterCount++] = BiQuad(gain, freq, sampleRate, q, true);
							}
						}
						TraceF(L"Adding filter with center frequency %g Hz, gain %g dB and Q %g", freq, gain, q);
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
