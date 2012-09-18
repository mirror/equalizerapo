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

#include <string>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "EqualizerAPO.h"
#include "ClassFactory.h"
#include "RegistryHelper.h"
#include "LogHelper.h"

using namespace std;

static HINSTANCE hModule;

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, void* lpReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH)
		::hModule = hModule;

	return TRUE;
}

STDAPI DllCanUnloadNow()
{
	if(EqualizerAPO::instCount == 0 && ClassFactory::lockCount == 0)
		return S_OK;
	else
		return S_FALSE;
}

STDAPI DllGetClassObject(const CLSID& clsid, const IID& iid, void** ppv)
{
	if(clsid != __uuidof(EqualizerAPO))
		return CLASS_E_CLASSNOTAVAILABLE;

	ClassFactory* factory = new ClassFactory();
	if(factory == NULL)
		return E_OUTOFMEMORY;

	HRESULT hr = factory->QueryInterface(iid, ppv);
	factory->Release();

	return hr;
}

STDAPI DllRegisterServer()
{
	wchar_t filename[1024];
	GetModuleFileNameW(hModule, filename, sizeof(filename)/sizeof(wchar_t));

	HRESULT hr = RegisterAPO(EqualizerAPO::regProperties);
	if(FAILED(hr))
	{
		UnregisterAPO(__uuidof(EqualizerAPO));
		return hr;
	}

	try
	{
		wstring apoClsidString = RegistryHelper::getGuidString(__uuidof(EqualizerAPO));

		RegistryHelper::createKey(L"SOFTWARE\\Classes\\CLSID\\" + apoClsidString);
		RegistryHelper::writeValue(L"SOFTWARE\\Classes\\CLSID\\" + apoClsidString, L"", L"EqualizerAPO Class");
		RegistryHelper::createKey(L"SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32");
		RegistryHelper::writeValue(L"SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32", L"", filename);
		RegistryHelper::writeValue(L"SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32", L"ThreadingModel", L"Both");
	}
	catch(RegistryException e)
	{
		UnregisterAPO(__uuidof(EqualizerAPO));
		return E_FAIL;
	}

	return S_OK;
}

STDAPI DllUnregisterServer()
{
	try
	{
		wstring apoClsidString = RegistryHelper::getGuidString(__uuidof(EqualizerAPO));

		RegistryHelper::deleteKey(L"SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32");
		RegistryHelper::deleteKey(L"SOFTWARE\\Classes\\CLSID\\" + apoClsidString);
	}
	catch(RegistryException e)
	{
		return E_FAIL;
	}

	HRESULT hr = UnregisterAPO(__uuidof(EqualizerAPO));

	return hr;
}
