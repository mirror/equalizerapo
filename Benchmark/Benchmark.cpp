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

#include <cstdio>
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <sndfile.h>

#include "../ParametricEQ.h"
#include "PerformanceCounter.h"

using namespace std;

const int BATCH_SIZE = 65536;

int main(int argc, char** argv)
{
	unsigned sampleRate;
	unsigned channelCount;
	unsigned frameCount;
	float* buf;

	if(argc >= 2 && strlen(argv[1]) > 0)
	{
		SF_INFO info;
		printf("Reading sound data from %s\n", argv[1]);
		SNDFILE* inFile = sf_open(argv[1], SFM_READ, &info);
		if(inFile == NULL)
		{
			fprintf(stderr, "%s", sf_strerror(inFile));
			return 1;
		}

		sampleRate = info.samplerate;
		channelCount = info.channels;
		frameCount = (unsigned)info.frames;

		buf = new float[frameCount * channelCount];

		sf_count_t numRead = 0;
		while(numRead < frameCount)
			numRead += sf_readf_float(inFile, buf + numRead * channelCount, frameCount - numRead);

		sf_close(inFile);
		inFile = NULL;
	}
	else
	{
		sampleRate = 44100;
		channelCount = 1;
		frameCount = 8820000;
		float sweepFrom = 0.1f;
		float sweepTo = 20000;
		float sweepDiff = sweepTo - sweepFrom;
		float length = 200;

		printf("No input file given, so generating linear sine sweep from %g to %g Hz over %g seconds\n", sweepFrom, sweepTo, length);

		buf = new float[frameCount * channelCount];

		for(unsigned i=0;i<frameCount;i++)
			for(unsigned j=0;j<channelCount;j++)
			{
				float t=(i*1.0f / sampleRate);

				buf[i*channelCount + j] = sin(((sweepFrom + sweepDiff*(t/length)/2)*t)*2*(float)M_PI);
			}
	}

	printf("Processing %d frames from %d channel(s)\n", frameCount, channelCount);

	ParametricEQ peq;
	peq.setDeviceInfo(L"Benchmark", L"File output", L"");
	peq.initialize((float)sampleRate, channelCount);

	PrecisionTimer timer;
	timer.start();

	for(unsigned i=0; i<frameCount; i+=BATCH_SIZE)
	{
		peq.process(buf + i*channelCount, buf + i*channelCount, min(BATCH_SIZE, frameCount - i));
	}

	double time = timer.stop();

	printf("%d samples processed in %f seconds\n", frameCount * channelCount, time);

	string outPath;
	if(argc >= 3 && strlen(argv[2]) > 0)
		outPath = argv[2];
	else
	{
		char temp[255];
		GetTempPathA(sizeof(temp)/sizeof(wchar_t), temp);

		outPath = temp;
		outPath += "testout.wav";
	}

	printf("Writing output to %s\n", outPath.c_str());

	SF_INFO info = {frameCount, sampleRate, channelCount, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 0};
	SNDFILE* outFile = sf_open(outPath.c_str(), SFM_WRITE, &info);
	if(outFile == NULL)
	{
		fprintf(stderr, "%s", sf_strerror(outFile));
		return 1;
	}

	sf_count_t numWritten = 0;
	while(numWritten < frameCount)
		numWritten += sf_writef_float(outFile, buf + numWritten * channelCount, frameCount - numWritten);

	sf_close(outFile);
	outFile = NULL;

	delete[] buf;

	if(argc < 3)
		system("pause");

	return 0;
}
