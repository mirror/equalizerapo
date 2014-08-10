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

#include "helpers/MemoryHelper.h"
#include "BiQuadFilter.h"

using namespace std;

BiQuadFilter::BiQuadFilter(BiQuad::Type type, double dbGain, double freq, double bandwidthOrQOrS, bool isBandwidth)
	:type(type), dbGain(dbGain), freq(freq), bandwidthOrQOrS(bandwidthOrQOrS), isBandwidth(isBandwidth)
{
}

BiQuadFilter::~BiQuadFilter()
{
	if(biquads != NULL)
	{
		MemoryHelper::free(biquads);
		biquads = NULL;
	}
}

vector<wstring> BiQuadFilter::initialize(float sampleRate, unsigned maxFrameCount, vector<wstring> channelNames)
{
	this->channelCount = channelNames.size();
	biquads = (BiQuad*)MemoryHelper::alloc(channelCount * sizeof(BiQuad));

	for(unsigned i=0; i<channelCount; i++)
	{
		new (biquads + i) BiQuad(type, dbGain, freq, sampleRate, bandwidthOrQOrS, isBandwidth);
	}

	return channelNames;
}

#pragma AVRT_CODE_BEGIN
void BiQuadFilter::process(float** output, float** input, unsigned frameCount)
{
	for(unsigned i=0; i<channelCount; i++)
	{
		BiQuad bq = biquads[i];

		float* inputChannel = input[i];
		float* outputChannel = output[i];

		for(unsigned j=0; j<frameCount; j++)
			outputChannel[j] = bq.process(inputChannel[j]);

		bq.removeDenormals();
		biquads[i] = bq;
	}
}
#pragma AVRT_CODE_END