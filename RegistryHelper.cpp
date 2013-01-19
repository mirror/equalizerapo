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

#include <fstream>
#include <sstream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ObjBase.h>

#include "RegistryHelper.h"

using namespace std;

wstring RegistryHelper::readValue(wstring key, wstring valuename)
{
	wstring result;

	LSTATUS status;
	HKEY keyHandle;
	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	DWORD type;
	DWORD bufSize;
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
	if(status != ERROR_SUCCESS)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Error while reading registry value HKEY_LOCAL_MACHINE\\" + key + L"\\" + valuename + L": " + getSystemErrorString(status));
	}

	if(type != REG_SZ)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Registry value HKEY_LOCAL_MACHINE\\" + key + L"\\" + valuename + L" has wrong type");
	}

	wchar_t* buf = new wchar_t[bufSize / sizeof(wchar_t) + 1];
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, NULL, (LPBYTE)buf, &bufSize);

	RegCloseKey(keyHandle);

	if(status != ERROR_SUCCESS)
	{
		delete buf;
		throw RegistryException(L"Error while reading registry value HKEY_LOCAL_MACHINE\\" + key + L"\\" + valuename + L": " + getSystemErrorString(status));
	}

	//Remove zero-termination
	if(buf[bufSize / sizeof(wchar_t) - 1] == L'\0')
		bufSize -= sizeof(wchar_t);
	result = wstring((wchar_t*)buf, (wstring::size_type)bufSize/sizeof(wchar_t));
	delete buf;

	return result;
}

void RegistryHelper::writeValue(wstring key, wstring valuename, wstring value)
{
	LSTATUS status;
	HKEY keyHandle;
	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &keyHandle);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	status = RegSetValueExW(keyHandle, valuename.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), (DWORD)((value.size()+1)*sizeof(wchar_t)));

	RegCloseKey(keyHandle);

	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while writing to registry value HKEY_LOCAL_MACHINE\\" + key + L"\\" + valuename + L": " + getSystemErrorString(status));
}

void RegistryHelper::deleteValue(wstring key, wstring valuename)
{
	LSTATUS status;
	HKEY keyHandle;
	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &keyHandle);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	status = RegDeleteValueW(keyHandle, valuename.c_str());

	RegCloseKey(keyHandle);

	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while deleting registry value HKEY_LOCAL_MACHINE\\" + key + L"\\" + valuename + L": " + getSystemErrorString(status));
}

void RegistryHelper::createKey(wstring key)
{
	HKEY keyHandle;
	LSTATUS status;
	status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, NULL, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &keyHandle, NULL);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while creating registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	RegCloseKey(keyHandle);
}

void RegistryHelper::deleteKey(wstring key)
{
	LSTATUS status;
	status = RegDeleteKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), KEY_WOW64_64KEY, 0);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while deleting registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));
}

bool RegistryHelper::keyExists(wstring key)
{
	bool result;

	HKEY keyHandle;
	result = (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle) == ERROR_SUCCESS);

	if(result)
		RegCloseKey(keyHandle);

	return result;
}

vector<wstring> RegistryHelper::enumSubKeys(wstring key)
{
	vector<wstring> result;

	LSTATUS status;
	HKEY keyHandle;
	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY, &keyHandle);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	wchar_t keyName[256];
	DWORD keyLength = sizeof(keyName) / sizeof(wchar_t);
	int i=0;
	int itemCount=0;

	while((status = RegEnumKeyExW(keyHandle, i++, keyName, &keyLength, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
	{
		keyLength = sizeof(keyName) / sizeof(wchar_t);

		result.push_back(keyName);
	}

	RegCloseKey(keyHandle);

	if(status != ERROR_NO_MORE_ITEMS)
		throw RegistryException(L"Error while enumerating sub keys of registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	return result;
}

unsigned long RegistryHelper::valueCount(wstring key)
{
	LSTATUS status;
	HKEY keyHandle;
	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	DWORD valueCount;
	status = RegQueryInfoKeyW(keyHandle, NULL, NULL, NULL, NULL, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL);

	RegCloseKey(keyHandle);

	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while reading info for registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	return valueCount;
}

void RegistryHelper::saveToFile(wstring key, wstring valuename, wstring filepath)
{
	wstring value = readValue(key, valuename);

	wofstream stream(filepath);
	if(!stream.good())
		throw RegistryException(L"Error while opening file " + filepath + L" for writing");

	stream << L"Windows Registry Editor Version 5.00\n" << endl;
	stream << L"[HKEY_LOCAL_MACHINE\\" << key << L"]" << endl;
	stream << L"\"" << valuename << L"\"=\"" << value << L"\"" << endl;
	stream << endl;

	stream.close();
}

wstring RegistryHelper::getGuidString(GUID guid)
{
	wchar_t* temp;
	if(FAILED(StringFromCLSID(guid, &temp)))
		throw RegistryException(L"Could not convert GUID to string");
	std::wstring result(temp);
	CoTaskMemFree(temp);

	return result;
}

wstring RegistryHelper::replaceIllegalCharacters(wstring filename)
{
	wstringstream stream;

	for(unsigned i=0; i<filename.length(); i++)
	{
		wchar_t c = filename[i];
		switch(c)
		{
		case L'<':
		case L'>':
		case L':':
		case L'"':
		case L'/':
		case L'\\':
		case L'|':
		case L'?':
		case L'*':
			stream.put(L'_');
			break;
		default:
			stream.put(c);
		}
	}

	return stream.str();
}

wstring RegistryHelper::getSystemErrorString(long status)
{
	wchar_t* buf;

	if(FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, (LPTSTR)&buf, 0, NULL) != 0)
	{
		wstring result(buf);
		LocalFree(buf);

		return result;
	}
	else
		return L"";
}
