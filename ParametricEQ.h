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
#include <vector>
#include <hash_map>

#include "BiQuad.h"

struct ChannelData
{
	ChannelData();

	float preamp;
	unsigned filterCount;
	BiQuad filters[100];

	// used while loading instead of overwriting immediately
	float loadPreamp;
	unsigned loadFilterCount;
};

class ParametricEQ
{
public:
	ParametricEQ();
	~ParametricEQ();

	void setDeviceInfo(const std::wstring& deviceName, const std::wstring& connectionName, const std::wstring& deviceGuid);
	void initialize(float sampleRate, unsigned channelCount, unsigned channelMask);
	void loadConfig();
	void process(float *output, float *input, unsigned frameCount);

private:
	void loadConfig(const std::wstring& path, std::vector<bool> selectedChannels);
	float getFreq(const std::wstring& freqString);
	unsigned getChannelNumber(unsigned position);
	static unsigned long __stdcall notificationThread(void* parameter);

	std::wstring deviceName;
	std::wstring connectionName;
	std::wstring deviceGuid;
	std::wstring configPath;
	float sampleRate;
	unsigned channelCount;
	unsigned channelMask;
	ChannelData* channelData;
	bool lastInputWasSilent;
	void* threadHandle;
	void* shutdownEvent;
	stdext::hash_map<std::wstring, int> channelNameToPosMap;
	stdext::hash_map<int, std::wstring> channelPosToNameMap;
	stdext::hash_map<std::wstring, BiQuad::Type> filterNameToTypeMap;
	stdext::hash_map<BiQuad::Type, std::wstring> filterTypeToDescriptionMap;
};
