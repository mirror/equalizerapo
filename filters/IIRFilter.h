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

#pragma once

#include "IFilter.h"

#pragma AVRT_VTABLES_BEGIN
class IIRFilter : public IFilter
{
public:
	IIRFilter(const std::vector<double>& coefficients);
	virtual ~IIRFilter();
	virtual bool getInPlace() {return true;}
	virtual std::vector<std::wstring> initialize(float sampleRate, unsigned maxFrameCount, std::vector<std::wstring> channelNames);
	virtual void process(float** output, float** input, unsigned frameCount);

private:
	unsigned order;
	float b0;
	float* a;
	float* b;
	unsigned channelCount;
	float* x;
	float* y;
};
#pragma AVRT_VTABLES_END
