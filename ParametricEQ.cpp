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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Shlwapi.h>

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
}

ParametricEQ::ParametricEQ()
{
    filterCount = 0;
	channelCount = 0;
	preamp = 1.0f;
    memset(sampleData, 0, 1000 * sizeof(float));
	lastInputWasSilent = false;
	threadHandle = NULL;
}

ParametricEQ::~ParametricEQ()
{
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
}

void ParametricEQ::initialize(float sampleRate, unsigned channelCount)
{
	this->sampleRate = sampleRate;
    this->channelCount = channelCount;
	memset(sampleData, 0, 1000 * sizeof(float));
    filterCount = 0;
	preamp = 1.0f;

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
	unsigned loadFilterCount = 0;
	float loadPreamp = 1.0f;

	loadConfig(configPath + L"\\config.txt", loadFilterCount, loadPreamp);

	TraceF(L"%d filters loaded", loadFilterCount);
	preamp = loadPreamp;
	filterCount = loadFilterCount;
}

void ParametricEQ::loadConfig(wstring path, unsigned& loadFilterCount, float& loadPreamp)
{
	TraceF(L"Loading configuration from %s", path.c_str());

	ifstream inputStream(path);
	if(!inputStream.good())
	{
		LogF(L"Config file not found at %s!", path.c_str());
		return;
	}

	while(!inputStream.eof())
	{
		string line;
		getline(inputStream, line);

		size_t pos = line.find(':');
		if(pos != -1)
		{
			string key = line.substr(0, pos);
			string value = line.substr(pos + 1);

			//Conversion to period as decimal mark, if needed
			for(unsigned i=0;i<value.length();i++)
			{
				if(value[i] == ',')
					value[i] = '.';
			}

			if(key.find("Filter") == 0)
			{
				if(loadFilterCount < (sizeof(filters)/sizeof(BiQuad)))
				{
					char freqString[10];
					float freq, gain, bandwidth;
				
					int matched = sscanf_s(value.c_str(), " ON PEQ Fc %9s Hz Gain %f dB BW Oct %f", &freqString, 10, &gain, &bandwidth);
					if(matched == 3 && (freq = getFreq(freqString)) != -1.0f)
					{
						filters[loadFilterCount++] = BiQuad(gain, freq, sampleRate, bandwidth, false);
						TraceF(L"Adding filter with center frequency %g Hz, gain %g dB and bandwidth %g octaves", freq, gain, bandwidth);
					}
					else
					{
						float q;
						matched = sscanf_s(value.c_str(), " ON PK Fc %9s Hz Gain %f dB Q %f", &freqString, 10, &gain, &q);
						if(matched == 3 && (freq = getFreq(freqString)) != -1.0f)
						{
							filters[loadFilterCount++] = BiQuad(gain, freq, sampleRate, q, true);
							TraceF(L"Adding filter with center frequency %g Hz, gain %g dB and Q %g", freq, gain, q);
						}
					}
				}
			}
			else if(key == "Preamp")
			{
				float preamp_dB;
				int matched = sscanf_s(value.c_str(), " %f dB", &preamp_dB);
				if(matched == 1)
				{
					loadPreamp = pow(10.0f, preamp_dB / 20.0f);
					TraceF(L"Setting preamp to %g dB", preamp_dB);
				}
			}
			else if(key == "Include")
			{
				while(value.length() > 0 && iswspace(value[0]))
					value = value.substr(1);

				int length = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, NULL, 0);
				wchar_t* charBuf = new wchar_t[length];
				MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, charBuf, length);

				wstring includePath;
				if(PathIsRelativeA(value.c_str()))
				{
					wchar_t filePath[MAX_PATH];
					path._Copy_s(filePath, sizeof(filePath), MAX_PATH);
					if(path.size() < MAX_PATH)
						filePath[path.size()]=L'\0';
					else
						filePath[MAX_PATH - 1]=L'\0';
					PathRemoveFileSpecW(filePath);
					PathAppendW(filePath, charBuf);
					includePath = filePath;
				}
				else
					includePath = charBuf;

				delete charBuf;

				loadConfig(includePath, loadFilterCount, loadPreamp);
			}
		}
	}
}

#define IS_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) == 0)

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
	for(unsigned i = 0; i < (filterCount+1)*channelCount*2; i++)
		if(IS_DENORMAL(sampleData[i]))
			sampleData[i] = 0;

    for (unsigned i = 0; i < frameCount * channelCount; i+=channelCount)
        for (unsigned c=0; c<channelCount; c++)
        {
            float sample = input[i+c];
            
            for(unsigned f=0; f<filterCount; f++)
            {
                unsigned dataIndex = (c*(filterCount+1) + f)*2;
                float result = filters[f].a0 * sample + filters[f].a[0] * sampleData[dataIndex] + filters[f].a[1] * sampleData[dataIndex+1] +
                        filters[f].a[2] * sampleData[dataIndex+2] + filters[f].a[3] * sampleData[dataIndex+3];
                
                sampleData[dataIndex+1] = sampleData[dataIndex];
                sampleData[dataIndex] = sample;
                
                //Input for next stage
                sample = result;
            }
            unsigned lastIndex = (c*(filterCount+1) + filterCount)*2;
            sampleData[lastIndex + 1] = sampleData[lastIndex];
            sampleData[lastIndex] = sample;
            
            output[i+c] = sample * preamp;
        }
}

float ParametricEQ::getFreq(char* freqString)
{
	size_t len = strlen(freqString);
	float result;
	int matched = sscanf_s(freqString, "%f", &result);
	if(matched == 1)
	{
		if(len >= 5)
		{
			if(freqString[len - 4] == '.')
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
