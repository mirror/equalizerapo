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
#include "helpers/StringHelper.h"
#include "helpers/RegistryHelper.h"
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
	categoryTabCtrl = GetDlgItem(hDlg, IDC_CATEGORY_TAB_CTRL);
	deviceLists[0] = GetDlgItem(hDlg, IDC_PLAYBACK_LIST);
	deviceLists[1] = GetDlgItem(hDlg, IDC_CAPTURE_LIST);
	okButton = GetDlgItem(hDlg, IDOK);
	cancelButton = GetDlgItem(hDlg, IDCANCEL);
	requestLabel = GetDlgItem(hDlg, IDC_REQUEST);
	copyDeviceCommandButton = GetDlgItem(hDlg, IDC_COPY_DEVICE_COMMAND);

	wchar_t stringBuf[255];

	TCITEM tci;
	tci.pszText = stringBuf;
	tci.mask = TCIF_TEXT;

	LoadStringW(hInstance, IDS_PLAYBACK_DEVICES, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	TabCtrl_InsertItem(categoryTabCtrl, 0, &tci);

	LoadStringW(hInstance, IDS_CAPTURE_DEVICES, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	TabCtrl_InsertItem(categoryTabCtrl, 1, &tci);

	LoadStringW(hInstance, IDS_REQUEST, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	SetWindowTextW(requestLabel, stringBuf);

	LoadStringW(hInstance, IDS_COPY_DEVICE_COMMAND, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	SetWindowTextW(copyDeviceCommandButton, stringBuf);

	LoadStringW(hInstance, IDS_OK, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	SetWindowTextW(okButton, stringBuf);

	LoadStringW(hInstance, IDS_CANCEL, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
	SetWindowTextW(cancelButton, stringBuf);

	for(int i=0; i<=1; i++)
	{
		HWND deviceList = deviceLists[i];
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

		try
		{
			apoInfos[i] = DeviceAPOInfo::loadAllInfos(i==1);

			int itemCount=0;
			for(vector<DeviceAPOInfo>::iterator it = apoInfos[i].begin(); it != apoInfos[i].end(); it++)
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
					if(it->canBeUpgraded())
						LoadStringW(hInstance, IDS_WILL_BE_UPGRADED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
					else
						LoadStringW(hInstance, IDS_ALREADY_INSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
					ListView_SetItemText(deviceList, itemCount, 2, stringBuf);
				}
				else if(it->isExperimental())
				{
					LoadStringW(hInstance, IDS_CAN_BE_INSTALLED_EXPERIMENTAL, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
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
}

void Configurator::onLvnItemChanged(unsigned sourceId, LPNMLISTVIEW info)
{
	int index = (sourceId == IDC_PLAYBACK_LIST ? 0 : 1);
	HWND deviceList = deviceLists[index];

	int itemCount = ListView_GetItemCount(deviceList);

	if(apoInfos[index].size() == itemCount)
	{
		DeviceAPOInfo apoInfo = apoInfos[index][info->lParam];
		bool checked = ListView_GetCheckState(deviceList,info->iItem) != 0;
		wchar_t stringBuf[255];
		if(checked && !apoInfo.isInstalled)
			LoadStringW(hInstance, IDS_WILL_BE_INSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		else if(!checked && apoInfo.isInstalled)
			LoadStringW(hInstance, IDS_WILL_BE_UNINSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		else if(apoInfo.isInstalled && apoInfo.canBeUpgraded())
			LoadStringW(hInstance, IDS_WILL_BE_UPGRADED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		else if(apoInfo.isInstalled)
			LoadStringW(hInstance, IDS_ALREADY_INSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		else if(apoInfo.isExperimental())
			LoadStringW(hInstance, IDS_CAN_BE_INSTALLED_EXPERIMENTAL, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		else
			LoadStringW(hInstance, IDS_CAN_BE_INSTALLED, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));

		ListView_SetItemText(deviceList, info->iItem, 2, stringBuf);

		EnableWindow(okButton, isChanged());

		EnableWindow(copyDeviceCommandButton, ListView_GetSelectedCount(deviceList) > 0);
	}
}

bool Configurator::onButtonClicked(unsigned sourceId)
{
	if(sourceId == IDOK)
	{
		for(int index = 0; index <= 1; index++)
		{
			HWND deviceList = deviceLists[index];

			for(int i = 0; i < ListView_GetItemCount(deviceList); i++)
			{
				LVITEM item;
				item.iItem = i;
				item.iSubItem = 0;
				item.mask = LVIF_PARAM;
				ListView_GetItem(deviceList, &item);

				try
				{
					DeviceAPOInfo info = apoInfos[index][item.lParam];
					if(ListView_GetCheckState(deviceList, i) && !info.isInstalled)
						info.install();
					else if(!ListView_GetCheckState(deviceList, i) && info.isInstalled)
						info.uninstall();
					else if(ListView_GetCheckState(deviceList, i) && info.canBeUpgraded())
					{
						info.uninstall();
						info.load(info.deviceGuid);
						info.install();
					}
				}
				catch(RegistryException e)
				{
					MessageBoxW(hDlg, e.getMessage().c_str(), L"Error while accessing the registry", MB_ICONERROR | MB_OK);
				}
			}
		}
	}
	else if(sourceId == IDC_COPY_DEVICE_COMMAND)
	{
		wstring command = L"Device: ";

		int index = TabCtrl_GetCurSel(categoryTabCtrl);
		HWND deviceList = deviceLists[index];

		bool first = true;
		for(int i = 0; i < ListView_GetItemCount(deviceList); i++)
		{
			LVITEM item;
			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_PARAM | LVIF_STATE;
			item.stateMask = LVIS_SELECTED;
			ListView_GetItem(deviceList, &item);

			if(item.state & LVIS_SELECTED)
			{
				if(first)
					first = false;
				else
					command += L"; ";

				DeviceAPOInfo info = apoInfos[index][item.lParam];
				command += StringHelper::replaceCharacters(info.deviceName + L" " + info.connectionName + L" " + info.deviceGuid, L";", L" ");
			}
		}

		OpenClipboard(hDlg);

		HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, (command.length() + 1) * sizeof(wchar_t));
		wchar_t* data = (wchar_t*)GlobalLock(hData);
		memcpy(data, command.c_str(), (command.length() + 1) * sizeof(wchar_t));
		GlobalUnlock(data);

		SetClipboardData(CF_UNICODETEXT, hData);

		CloseClipboard();
	}

	if(sourceId == IDCANCEL && hasUpgrades())
	{
		wchar_t captionBuf[255];
		LoadStringW(hInstance, IDS_CANCEL_UPGRADES_CAPTION, captionBuf, sizeof(captionBuf)/sizeof(wchar_t));
		wchar_t stringBuf[255];
		LoadStringW(hInstance, IDS_CANCEL_UPGRADES, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
		if(MessageBoxW(hDlg, stringBuf, captionBuf, MB_ICONWARNING | MB_YESNO) == IDNO)
			return false;
	}

	if(sourceId == IDOK || sourceId == IDCANCEL)
	{
		if(cmdLine == L"/i")
		{
			wchar_t stringBuf[255];
			LoadStringW(hInstance, IDS_AFTERINSTALL, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
			MessageBoxW(hDlg, stringBuf, L"Info", MB_ICONINFORMATION | MB_OK);
		}
		else if(sourceId == IDOK && RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
		{
			wchar_t captionBuf[255];
			LoadStringW(hInstance, IDS_SHOULD_REBOOT_CAPTION, captionBuf, sizeof(captionBuf)/sizeof(wchar_t));
			wchar_t stringBuf[255];
			LoadStringW(hInstance, IDS_SHOULD_REBOOT, stringBuf, sizeof(stringBuf)/sizeof(wchar_t));
			if(MessageBoxW(hDlg, stringBuf, captionBuf, MB_ICONQUESTION | MB_YESNO) == IDYES)
			{
				HANDLE tokenHandle;
				if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
				{
					LUID luid;
					if(LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid))
					{
						TOKEN_PRIVILEGES tp;
						tp.PrivilegeCount = 1;
						tp.Privileges[0].Luid = luid;
						tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

						if(AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
							InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_RESTART | SHUTDOWN_GRACE_OVERRIDE, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE);
					}

					CloseHandle(tokenHandle);
				}
			}
		}

		return true;
	}

	return false;
}

void Configurator::onTcnSelChange(unsigned sourceId)
{
	int index = TabCtrl_GetCurSel(categoryTabCtrl);

	ShowWindow(deviceLists[index], SW_SHOW);
	ShowWindow(deviceLists[1 - index], SW_HIDE);

	EnableWindow(copyDeviceCommandButton, ListView_GetSelectedCount(deviceLists[index]) > 0);

	SetFocus(deviceLists[index]);
}

bool Configurator::isChanged()
{
	bool changed = false;

	for(int index = 0; index <= 1; index++)
	{
		HWND deviceList = deviceLists[index];

		for(int i = 0; i < ListView_GetItemCount(deviceList); i++)
		{
			LVITEM item;
			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_PARAM;
			ListView_GetItem(deviceList, &item);
			if((ListView_GetCheckState(deviceList, i) != 0) != apoInfos[index][item.lParam].isInstalled
				|| ListView_GetCheckState(deviceList, i) && apoInfos[index][item.lParam].isInstalled && apoInfos[index][item.lParam].canBeUpgraded())
			{
				changed = true;
				break;
			}
		}
	}

	return changed;
}

bool Configurator::hasUpgrades()
{
	bool hasUpgrades = false;

	for(int index = 0; index <= 1; index++)
	{
		HWND deviceList = deviceLists[index];

		for(int i = 0; i < ListView_GetItemCount(deviceList); i++)
		{
			LVITEM item;
			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_PARAM;
			ListView_GetItem(deviceList, &item);
			if(ListView_GetCheckState(deviceList, i) && apoInfos[index][item.lParam].isInstalled && apoInfos[index][item.lParam].canBeUpgraded())
			{
				hasUpgrades = true;
				break;
			}
		}
	}

	return hasUpgrades;
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
		for(int index = 0; index <= 1; index++)
		{
			vector<DeviceAPOInfo> apoInfos = DeviceAPOInfo::loadAllInfos(index == 1);

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
		if(configurator->onButtonClicked(LOWORD(wParam)))
		{
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
		case TCN_SELCHANGE:
			configurator->onTcnSelChange((unsigned)((LPNMHDR)lParam)->idFrom);
			break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}