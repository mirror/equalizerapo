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
#include <aclapi.h>

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

unsigned long RegistryHelper::readDWORDValue(wstring key, wstring valuename)
{
	unsigned long result;

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

	if(type != REG_DWORD)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Registry value HKEY_LOCAL_MACHINE\\" + key + L"\\" + valuename + L" has wrong type");
	}

	BYTE* buf = new BYTE[bufSize];
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, NULL, buf, &bufSize);

	RegCloseKey(keyHandle);

	if(status != ERROR_SUCCESS)
	{
		delete buf;
		throw RegistryException(L"Error while reading registry value HKEY_LOCAL_MACHINE\\" + key + L"\\" + valuename + L": " + getSystemErrorString(status));
	}

	result = ((unsigned long*)buf)[0];
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

void RegistryHelper::writeMultiValue(wstring key, wstring valuename, wstring value)
{
	LSTATUS status;
	HKEY keyHandle;
	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &keyHandle);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	wchar_t* data = new wchar_t[value.size() + 2];
	value._Copy_s(data, (value.size()+2)*sizeof(wchar_t), value.size());
	data[value.size()] = L'\0';
	data[value.size()+1] = L'\0';

	status = RegSetValueExW(keyHandle, valuename.c_str(), 0, REG_MULTI_SZ, (const BYTE*)data, (DWORD)((value.size()+2)*sizeof(wchar_t)));

	delete data;

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

void RegistryHelper::makeWritable(wstring key)
{
	LSTATUS status;
	HKEY keyHandle;
	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, READ_CONTROL | WRITE_DAC | KEY_WOW64_64KEY, &keyHandle);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	DWORD descriptorSize = 0;
	RegGetKeySecurity(keyHandle, DACL_SECURITY_INFORMATION, NULL, &descriptorSize);

	PSECURITY_DESCRIPTOR oldSd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, descriptorSize);
	status = RegGetKeySecurity(keyHandle, DACL_SECURITY_INFORMATION, oldSd, &descriptorSize);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while getting security information for registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	BOOL aclPresent, aclDefaulted;
	PACL oldAcl = NULL;
	if(!GetSecurityDescriptorDacl(oldSd, &aclPresent, &oldAcl, &aclDefaulted))
		throw RegistryException(L"Error in GetSecurityDescriptorDacl while ensuring writability");

	PSID sid = NULL;
	SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
	if(!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &sid)) 
		throw RegistryException(L"Error in AllocateAndInitializeSid while ensuring writability");

	EXPLICIT_ACCESS ea;
	ea.grfAccessPermissions = KEY_ALL_ACCESS;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea.Trustee.ptstrName = (LPWSTR)sid;

	PACL acl = NULL;
	if(ERROR_SUCCESS != SetEntriesInAcl(1, &ea, oldAcl, &acl)) 
		throw RegistryException(L"Error in SetEntriesInAcl while ensuring writability");

	PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH); 
	if(NULL == sd) 
		throw RegistryException(L"Error in LocalAlloc while ensuring writability");

	if(!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION)) 
		throw RegistryException(L"Error in InitializeSecurityDescriptor while ensuring writability");

	if(!SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE))
		throw RegistryException(L"Error in SetSecurityDescriptorDacl while ensuring writability");

	status = RegSetKeySecurity(keyHandle, DACL_SECURITY_INFORMATION, sd);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while setting security information for registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	FreeSid(sid);
	LocalFree(acl);
	HeapFree(GetProcessHeap(), 0, oldSd);
	LocalFree(sd);
}

void RegistryHelper::takeOwnership(wstring key)
{
	HANDLE tokenHandle;
	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
		throw RegistryException(L"Error in OpenProcessToken while taking ownership");

	LUID luid;
	if(!LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME, &luid))
		throw RegistryException(L"Error in LookupPrivilegeValue while taking ownership");

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if(!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

	LSTATUS status;
	HKEY keyHandle;
	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, WRITE_OWNER | KEY_WOW64_64KEY, &keyHandle);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH); 
	if(NULL == sd) 
		throw RegistryException(L"Error in SetPrivilege while taking ownership");

	if(!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION)) 
		throw RegistryException(L"Error in InitializeSecurityDescriptor while taking ownership");

	PSID sid = NULL;
	SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
	if(!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &sid)) 
		throw RegistryException(L"Error in AllocateAndInitializeSid while taking ownership");

	if(!SetSecurityDescriptorOwner(sd, sid, FALSE))
		throw RegistryException(L"Error in SetSecurityDescriptorOwner while taking ownership");

	status = RegSetKeySecurity(keyHandle, OWNER_SECURITY_INFORMATION, sd);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while setting security information for registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	tp.Privileges[0].Attributes = 0;

	if(!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

	FreeSid(sid);
	LocalFree(sd);
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

bool RegistryHelper::valueExists(wstring key, wstring valuename)
{
	LSTATUS status;
	HKEY keyHandle;
	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle);
	if(status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key HKEY_LOCAL_MACHINE\\" + key + L": " + getSystemErrorString(status));

	DWORD type;
	DWORD bufSize;
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
	RegCloseKey(keyHandle);
	return status == ERROR_SUCCESS;
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
