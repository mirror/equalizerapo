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
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define APP_REGPATH L"HKEY_LOCAL_MACHINE\\SOFTWARE\\EqualizerAPO"

class RegistryHelper
{
public:
	static std::wstring readValue(std::wstring key, std::wstring valuename);
	static unsigned long readDWORDValue(std::wstring key, std::wstring valuename);
	static void writeValue(std::wstring key, std::wstring valuename, std::wstring value);
	static void writeMultiValue(std::wstring key, std::wstring valuename, std::wstring value);
	static void deleteValue(std::wstring key, std::wstring valuename);
	static void createKey(std::wstring key);
	static void deleteKey(std::wstring key);
	static void makeWritable(std::wstring key);
	static void takeOwnership(std::wstring key);
	static std::vector<std::wstring> enumSubKeys(std::wstring key);
	static bool keyExists(std::wstring key);
	static bool valueExists(std::wstring key, std::wstring valuename);
	static bool keyEmpty(std::wstring key);
	static void saveToFile(std::wstring key, std::vector<std::wstring> valuenames, std::wstring filepath);
	static std::wstring getGuidString(GUID guid);
	static bool isWindowsVersionAtLeast(unsigned major, unsigned minor);
	static HKEY openKey(const std::wstring& key, REGSAM samDesired);

private:
	static std::wstring splitKey(const std::wstring& key, HKEY* rootKey);

	static unsigned long windowsVersion;
};

class RegistryException
{
public:
	RegistryException(const std::wstring& message):message(message) {}

	std::wstring getMessage()
	{
		return message;
	}

private:
	std::wstring message;
};
