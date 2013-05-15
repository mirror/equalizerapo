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

#define APOGUID_NULL L"{00000000-0000-0000-0000-000000000000}"
#define APOGUID_NOKEY L"!KEY"
#define APOGUID_NOVALUE L"!VALUE"

class DeviceAPOInfo
{
public:
	static std::vector<DeviceAPOInfo> loadAllInfos(bool input);
	bool load(const std::wstring& deviceGuid);
	void install();
	void uninstall();

	std::wstring deviceName;
	std::wstring connectionName;
	std::wstring deviceGuid;
	std::wstring originalApoGuid;
	bool isInput;
	bool isInstalled;
	bool isLFX;
};
