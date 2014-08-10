/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2014  Jonas Thedering

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

#include <algorithm>

#include "helpers/LogHelper.h"
#include "IFilter.h"

using namespace std;

int IFilter::getChannelIndex(std::wstring word, const std::vector<std::wstring>& channelNames, bool allowNew)
{
	int channelIndex = -1;

	if(iswdigit(word[0]))
	{
		channelIndex = wcstol(word.c_str(), NULL, 10) - 1;

		if(channelIndex < 0 || channelIndex >= (int)channelNames.size())
		{
			LogF(L"Channel number %s out of range (1 - %d)", word.c_str(), channelNames.size());
			channelIndex = -1;
		}
	}
	else
	{
		vector<wstring>::const_iterator pos = find(channelNames.begin(), channelNames.end(), word);

		if(pos == channelNames.end())
		{
			//Special handling to accept "wrong", but unambiguous positions
			if(word == L"SL")
				pos = find(channelNames.begin(), channelNames.end(), L"RL");
			else if(word == L"SR")
				pos = find(channelNames.begin(), channelNames.end(), L"RR");
			else if(word == L"RL")
				pos = find(channelNames.begin(), channelNames.end(), L"SL");
			else if(word == L"RR")
				pos = find(channelNames.begin(), channelNames.end(), L"SR");
		}

		if(pos != channelNames.end())
			channelIndex = (int)(pos - channelNames.begin());
		else if(!allowNew)
			LogF(L"Invalid channel position %s", word.c_str());
	}

	return channelIndex;
}
