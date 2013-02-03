
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
