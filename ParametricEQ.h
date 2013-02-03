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

#pragma once

#include <string>

struct BiQuad
{
    BiQuad() {}
    BiQuad(float dbGain, float freq, float srate, float bandwidthOrQ, bool isQ);
        
    __declspec(align(16)) float a[4];
	float a0;
};

class ParametricEQ
{
public:
	ParametricEQ();
	~ParametricEQ();

	void setDeviceInfo(const std::wstring& deviceName, const std::wstring& connectionName, const std::wstring& deviceGuid);
	void initialize(float sampleRate, unsigned channelCount);
	void loadConfig();
	void process(float *output, float *input, unsigned frameCount);

private:
	void loadConfig(std::wstring path, unsigned& loadFilterCount, float& loadPreamp);
	float getFreq(const std::wstring& freqString);
	static unsigned long __stdcall notificationThread(void* parameter);

	std::wstring deviceName;
	std::wstring connectionName;
	std::wstring deviceGuid;
	std::wstring configPath;
	float sampleRate;
	unsigned channelCount;
	float preamp;
    BiQuad filters[100];
    unsigned filterCount;
    float sampleData[2000];
	bool lastInputWasSilent;
	void* threadHandle;
	void* shutdownEvent;
};
