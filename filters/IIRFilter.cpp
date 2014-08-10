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
#include "IIRFilter.h"

using namespace std;

#define IS_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) == 0)

IIRFilter::IIRFilter(const vector<double>& coefficients)
{
	order = (unsigned)coefficients.size() / 2 - 1;
	a = (float*)MemoryHelper::alloc(order * sizeof(float));
	b = (float*)MemoryHelper::alloc(order * sizeof(float));
	x = NULL;
	y = NULL;

	double a0 = coefficients[order+1];
	b0 = float(coefficients[0] / a0);
	for(unsigned i=0; i<order; i++)
	{
		b[i] = float(coefficients[i+1] / a0);
		a[i] = float(-coefficients[i+order+2] / a0);
	}
}

IIRFilter::~IIRFilter()
{
	MemoryHelper::free(a);
	MemoryHelper::free(b);

	if(x != NULL)
		MemoryHelper::free(x);
	if(y != NULL)
		MemoryHelper::free(y);
}

vector<wstring> IIRFilter::initialize(float sampleRate, unsigned maxFrameCount, vector<wstring> channelNames)
{
	channelCount = (unsigned)channelNames.size();

	if(x != NULL)
		MemoryHelper::free(x);
	if(y != NULL)
		MemoryHelper::free(y);

	x = (float*)MemoryHelper::alloc(order * channelCount * sizeof(float));
	y = (float*)MemoryHelper::alloc(order * channelCount * sizeof(float));
	memset(x, 0, order * channelCount * sizeof(float));
	memset(y, 0, order * channelCount * sizeof(float));

	return channelNames;
}

#pragma AVRT_CODE_BEGIN
void IIRFilter::process(float** output, float** input, unsigned frameCount)
{
	for(unsigned i=0; i<channelCount; i++)
	{
		float* inputChannel = input[i];
		float* outputChannel = output[i];

		unsigned channelOffset = i*order;
		float* xo = x+channelOffset;
		float* yo = y+channelOffset;
		for(unsigned j=0; j<frameCount; j++)
		{
			float sample = inputChannel[j];
			float sum = b0 * sample;

			for(unsigned k=order-1; k>0; k--)
			{
				sum += b[k] * xo[k];
			}

			sum += b[0] * xo[0];

			for(unsigned k=order-1; k>0; k--)
			{
				sum += a[k] * yo[k];
			}

			sum += a[0] * yo[0];

			for(unsigned k=order-1; k>0; k--)
			{
				xo[k] = xo[k-1];
				yo[k] = yo[k-1];
			}

			xo[0] = sample;
			yo[0] = sum;

			outputChannel[j] = sum;
		}
	}

	for(unsigned i=0; i<channelCount*order; i++)
	{
		if(IS_DENORMAL(x[i]))
			x[i] = 0.0f;
		if(IS_DENORMAL(y[i]))
			y[i] = 0.0f;
	}
}
#pragma AVRT_CODE_END