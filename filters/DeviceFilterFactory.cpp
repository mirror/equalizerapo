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

#include "helpers/LogHelper.h"
#include "helpers/StringHelper.h"
#include "FilterEngine.h"
#include "DeviceFilterFactory.h"

using namespace std;

void DeviceFilterFactory::initialize(FilterEngine* engine)
{
	deviceString = engine->getDeviceName() + L" " + engine->getConnectionName() + L" " + engine->getDeviceGuid();

	mup::ParserX* parser = engine->getParser();
	parser->DefineConst(L"deviceName", engine->getDeviceName());
	parser->DefineConst(L"connectionName", engine->getConnectionName());
	parser->DefineConst(L"deviceGuid", engine->getDeviceGuid());
}

vector<IFilter*> DeviceFilterFactory::startOfConfiguration()
{
	deviceMatches = true;

	return vector<IFilter*>();
}

vector<IFilter*> DeviceFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	if(command == L"Device")
	{
		wstring value = StringHelper::trim(parameters) + L";";

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

		bool matches = false;

		for(unsigned i=0; i<fullList.size(); i++)
		{
			matches = true;

			if(fullList[i].size() == 1 && StringHelper::toLowerCase(fullList[i][0]) == L"all")
				break;

			for(unsigned j=0; j<fullList[i].size(); j++)
			{
				wstring word = StringHelper::toLowerCase(fullList[i][j]);
				if(StringHelper::toLowerCase(deviceString).find(word) == -1)
				{
					matches = false;
					break;
				}
			}

			if(matches)
				break;
		}

		TraceF(L"%satching pattern \"%s\" with device \"%s\"", matches ? L"M" : L"Not m", value.substr(0, value.length()-1).c_str(), deviceString.c_str());
		deviceMatches = matches;
	}

	if(!deviceMatches)
		// skip line for further factories
		command = L"";

	return vector<IFilter*>();
}

std::vector<IFilter*> DeviceFilterFactory::endOfFile(const std::wstring& configPath)
{
	// in outer file, the device must have matched, otherwise the inner file would not have been included
	deviceMatches = true;

	return vector<IFilter*>();
}
