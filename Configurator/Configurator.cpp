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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "../RegistryHelper.h"
#include "Configurator.h"

using namespace std;

Configurator* configurator;

Configurator::Configurator(HINSTANCE hInstance, const wchar_t* cmdLine)
	:cmdLine(cmdLine)
{
	this->hInstance = hInstance;
	hDlg = NULL;
}

void Configurator::onInitDialog(HWND hDlg)
{
	this->hDlg = hDlg;
	deviceList = GetDlgItem(hDlg, IDC_DEVICE_LIST);
	okButton = GetDlgItem(hDlg, IDOK);
	cancelButton = GetDlgItem(hDlg, IDCANCEL);
	requestLabel = GetDlgItem(hDlg, IDC_REQUEST);

	wchar_t stringBuf[255];
	LoadStringW(hInstance, IDS_REQUEST, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	SetWindowTextW(requestLabel, stringBuf);

	ListView_SetExtendedListViewStyle(deviceList, ListView_GetExtendedListViewStyle(deviceList) | LVS_EX_CHECKBOXES);
	LVCOLUMN column;
	column.pszText = stringBuf;
	column.mask = LVCF_TEXT;

	LoadStringW(hInstance, IDS_CONNECTOR, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	ListView_InsertColumn(deviceList, 0, &column);

	LoadStringW(hInstance, IDS_DEVICE, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	ListView_InsertColumn(deviceList, 1, &column);

	LoadStringW(hInstance, IDS_STATUS, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	ListView_InsertColumn(deviceList, 2, &column);

	LVITEM item;
	item.iSubItem = 0;
	item.pszText = stringBuf;
	item.mask = LVIF_TEXT | LVIF_PARAM;

	try {
		apoInfos = DeviceAPOInfo::loadAllInfos();

		int itemCount=0;
		for(vector<DeviceAPOInfo>::iterator it = apoInfos.begin(); it != apoInfos.end(); it++)
		{
			wcsncpy_s(stringBuf, sizeof(stringBuf)/sizeof(wchar_t), it->connectionName.c_str(), _TRUNCATE);
			item.iItem = itemCount;
			item.lParam = itemCount;
			ListView_InsertItem(deviceList, &item);
			wcsncpy_s(stringBuf, sizeof(stringBuf)/sizeof(wchar_t), it->deviceName.c_str(), _TRUNCATE);
			ListView_SetItemText(deviceList, itemCount, 1, stringBuf);

			if(it->isInstalled)
			{
				ListView_SetCheckState(deviceList, itemCount, TRUE);
				LoadStringW(hInstance, IDS_ALREADY_INSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
				ListView_SetItemText(deviceList, itemCount, 2, stringBuf);
			}
			else
			{
				LoadStringW(hInstance, IDS_CAN_BE_INSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
				ListView_SetItemText(deviceList, itemCount, 2, stringBuf);
			}

			itemCount++;
		}
	}
	catch(RegistryException e)
	{
		MessageBoxW(hDlg, e.getMessage().c_str(), L"Error while accessing the registry", MB_ICONERROR | MB_OK);
	}

	ListView_SetColumnWidth(deviceList, 0, LVSCW_AUTOSIZE);
	ListView_SetColumnWidth(deviceList, 1, LVSCW_AUTOSIZE);
	ListView_SetColumnWidth(deviceList, 2, LVSCW_AUTOSIZE);
}

void Configurator::onLvnItemChanged(unsigned sourceId, LPNMLISTVIEW info)
{
	int itemCount = ListView_GetItemCount(deviceList);

	if(apoInfos.size() == itemCount)
	{
		DeviceAPOInfo apoInfo = apoInfos[info->lParam];
		bool checked = ListView_GetCheckState(deviceList,info->iItem) != 0;
		wchar_t stringBuf[255];
		if(checked && !apoInfo.isInstalled)
			LoadStringW(hInstance, IDS_WILL_BE_INSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		else if(!checked && apoInfo.isInstalled)
			LoadStringW(hInstance, IDS_WILL_BE_UNINSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		else if(apoInfo.isInstalled)
			LoadStringW(hInstance, IDS_ALREADY_INSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		else
			LoadStringW(hInstance, IDS_CAN_BE_INSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));

		ListView_SetItemText(deviceList, info->iItem, 2, stringBuf);

		bool changed = false;
		for(int i = 0; i < itemCount; i++)
		{
			LVITEM item;
			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_PARAM;
			ListView_GetItem(deviceList, &item);
			if((ListView_GetCheckState(deviceList, i) != 0) != apoInfos[item.lParam].isInstalled)
			{
				changed = true;
				break;
			}
		}

		EnableWindow(okButton, changed);
	}
}

void Configurator::onButtonClicked(unsigned sourceId)
{
	if(sourceId == IDOK)
	{
		try
		{
			for(int i = 0; i < ListView_GetItemCount(deviceList); i++)
			{
				LVITEM item;
				item.iItem = i;
				item.iSubItem = 0;
				item.mask = LVIF_PARAM;
				ListView_GetItem(deviceList, &item);

				DeviceAPOInfo info = apoInfos[item.lParam];
				if(ListView_GetCheckState(deviceList, i) && !info.isInstalled)
					info.install();
				else if(!ListView_GetCheckState(deviceList, i) && info.isInstalled)
					info.uninstall();
			}
		}
		catch(RegistryException e)
		{
			MessageBoxW(hDlg, e.getMessage().c_str(), L"Error while accessing the registry", MB_ICONERROR | MB_OK);
		}
	}

	if(cmdLine == L"/i")
	{
		wchar_t stringBuf[255];
		LoadStringW(hInstance, IDS_AFTERINSTALL, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		MessageBoxW(hDlg, stringBuf, L"Info", MB_ICONINFORMATION | MB_OK);
	}
}

INT_PTR CALLBACK dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE /* hPrevInstance */,
                     LPWSTR    lpCmdLine,
                     int       nCmdShow)
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	int result = 0;

	if(wcscmp(lpCmdLine, L"/u") == 0)
	{
		vector<DeviceAPOInfo> apoInfos = DeviceAPOInfo::loadAllInfos();

		for(vector<DeviceAPOInfo>::iterator it = apoInfos.begin(); it != apoInfos.end(); it++)
		{
			try
			{
				if(it->isInstalled)
					it->uninstall();
			}
			catch (RegistryException e)
			{
				MessageBoxW(NULL, e.getMessage().c_str(), NULL, MB_ICONERROR | MB_OK);
				result = -1;
			}
		}
	}
	else
	{
		configurator = new Configurator(hInstance, lpCmdLine);
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINWINDOW), GetDesktopWindow(), dlgProc);
	}

	return result;
}

INT_PTR CALLBACK dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		configurator->onInitDialog(hDlg);

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			configurator->onButtonClicked(LOWORD(wParam));

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case LVN_ITEMCHANGED:
			configurator->onLvnItemChanged((unsigned)((LPNMHDR)lParam)->idFrom, (LPNMLISTVIEW) lParam);
			break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
