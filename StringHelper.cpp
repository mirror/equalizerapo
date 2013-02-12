/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2013  Jonas Thedering

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
#include <sstream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "StringHelper.h"

using namespace std;

wstring StringHelper::replaceCharacters(std::wstring s, std::wstring chars, wchar_t replacement)
{
	wstring result;
	result.reserve(s.length());

	for(unsigned i=0; i<s.length(); i++)
	{
		wchar_t c = s[i];
		if(chars.find(c) != -1)
			c = replacement;

		result += c;
	}

	return result;
}

wstring StringHelper::replaceIllegalCharacters(wstring filename)
{
	return replaceCharacters(filename, L"<>:\"/\\|?*", '_');
}

wstring StringHelper::toWString(string s, unsigned codepage)
{
	int length = MultiByteToWideChar(codepage, 0, s.c_str(), -1, NULL, 0);
	wchar_t* charBuf = new wchar_t[length];
	MultiByteToWideChar(codepage, 0, s.c_str(), -1, charBuf, length);
	wstring result = charBuf;
	delete charBuf;

	return result;
}

wstring StringHelper::toLowerCase(wstring s)
{
	wchar_t* charBuf = new wchar_t[s.length() + 1];
	memcpy(charBuf, s.c_str(), (s.length() + 1) * sizeof(wchar_t));
	errno_t err = _wcslwr_s(charBuf, s.length() + 1);

	wstring result = charBuf;
	delete charBuf;

	if(err == 0)
		return result;
	else
		return s;
}
