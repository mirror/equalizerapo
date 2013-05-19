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
#include <tclap/CmdLine.h>

#include "../version.h"
#include "../ParametricEQ.h"
#include "../LogHelper.h"
#include "../StringHelper.h"
#include "PerformanceCounter.h"

using namespace std;

int main(int argc, char** argv)
{
	try
	{
		stringstream versionStream;
		versionStream << MAJOR << "." << MINOR;
		TCLAP::CmdLine cmd("Benchmark generates a linear sine sweep or reads from the given input file. "
			"It then filters the waveform using the Equalizer APO filter configuration "
			"and finally writes to the given file or into the user's temp directory.", ' ', versionStream.str());

		TCLAP::SwitchArg noPauseArg("", "nopause", "Do not wait for key press at the end", cmd);
		TCLAP::SwitchArg verboseArg("v", "verbose", "Print trace and error messages to console instead of logfile", cmd);
		TCLAP::ValueArg<string> guidArg("", "guid", "Endpoint GUID to use when parsing configuration (Default: <empty>)", false, "", "string", cmd);
		TCLAP::ValueArg<string> connectionnameArg("", "connectionname", "Connection name to use when parsing configuration (Default: File output)", false, "File output", "string", cmd);
		TCLAP::ValueArg<string> devicenameArg("", "devicename", "Device name to use when parsing configuration (Default: Benchmark)", false, "Benchmark", "string", cmd);
		TCLAP::ValueArg<unsigned> batchsizeArg("", "batchsize", "Number of frames processed in one batch (Default: 65536)", false, 65536, "integer", cmd);
		TCLAP::ValueArg<string> outputArg("o", "output", "File to write sound data to", false, "", "string", cmd);
		TCLAP::ValueArg<string> inputArg("i", "input", "File to load sound data from instead of generating sweep", false, "", "string", cmd);
		TCLAP::ValueArg<unsigned> rateArg("r", "rate", "Sample rate of generated sweep (Default: 44100)", false, 44100, "integer", cmd);
		TCLAP::ValueArg<float> toArg("t", "to", "End frequency of generated sweep in Hz (Default: 20000.0)", false, 20000.0f, "float", cmd);
		TCLAP::ValueArg<float> fromArg("f", "from", "Start frequency of generated sweep in Hz (Default: 0.1)", false, 0.1f, "float", cmd);
		TCLAP::ValueArg<float> lengthArg("l", "length", "Length of generated sweep in seconds (Default: 200.0)", false, 200.0f, "float", cmd);
		TCLAP::ValueArg<unsigned> channelArg("c", "channels", "Number of channels of generated sweep (Default: 2)", false, 2, "integer", cmd);

		cmd.parse(argc, argv);

		LogHelper::set(stderr, verboseArg.getValue(), true, true);

		unsigned sampleRate;
		unsigned channelCount;
		unsigned channelMask;
		unsigned frameCount;
		float length;
		float* buf;

		printf("Benchmark %d.%d\n", MAJOR, MINOR);
		printf("Run \"%s -h\" to show usage info\n", argv[0]);
		printf("\n");

		string input = inputArg.getValue();
		if(input != "")
		{
			SF_INFO info;
			printf("Reading sound data from %s\n", input.c_str());
			SNDFILE* inFile = sf_open(input.c_str(), SFM_READ, &info);
			if(inFile == NULL)
			{
				fprintf(stderr, "%s", sf_strerror(inFile));
				return 1;
			}

			sampleRate = info.samplerate;
			channelCount = info.channels;
			channelMask = 0;
			frameCount = (unsigned)info.frames;
			length = float(frameCount) / sampleRate;

			buf = new float[frameCount * channelCount];

			sf_count_t numRead = 0;
			while(numRead < frameCount)
				numRead += sf_readf_float(inFile, buf + numRead * channelCount, frameCount - numRead);

			sf_close(inFile);
			inFile = NULL;
		}
		else
		{
			sampleRate = rateArg.getValue();
			channelMask = 0;
			channelCount = channelArg.getValue();
			float sweepFrom = fromArg.getValue();
			float sweepTo = toArg.getValue();
			float sweepDiff = sweepTo - sweepFrom;
			length = lengthArg.getValue();
			frameCount = (unsigned)(length * sampleRate);

			printf("No input file given, so generating linear sine sweep from %g to %g Hz over %g seconds\n", sweepFrom, sweepTo, length);

			buf = new float[frameCount * channelCount];

			for(unsigned i=0;i<frameCount;i++)
				for(unsigned j=0;j<channelCount;j++)
				{
					float t=(i*1.0f / sampleRate);

					buf[i*channelCount + j] = sin(((sweepFrom + sweepDiff*(t/length)/2)*t)*2*(float)M_PI);
				}
		}

		printf("\nProcessing %d frames from %d channel(s)\n", frameCount, channelCount);

		ParametricEQ peq;
		peq.setDeviceInfo(StringHelper::toWString(devicenameArg.getValue(), CP_ACP),
			StringHelper::toWString(connectionnameArg.getValue(), CP_ACP),
			StringHelper::toWString(guidArg.getValue(), CP_ACP));
		peq.initialize((float)sampleRate, channelCount, channelMask);

		PrecisionTimer timer;
		timer.start();

		unsigned batchsize = batchsizeArg.getValue();
		for(unsigned i=0; i<frameCount; i+=batchsize)
		{
			peq.process(buf + i*channelCount, buf + i*channelCount, min(batchsize, frameCount - i));
		}

		double time = timer.stop();

		printf("%d samples processed in %f seconds\n", frameCount * channelCount, time);
		printf("This is equivalent to %.2f%% CPU load (one core) when processing in real time\n", 100.0f * time / length);

		unsigned clipCount = 0;
		float max = 0;
		for(unsigned i=0; i<frameCount*channelCount; i++)
		{
			float f = fabs(buf[i]);
			if(f > max)
				max = f;
			if(f > 1.0f)
				clipCount++;
		}

		printf("Max output level: %f (%f dB)", max, log10(max) * 20.0f);
		if(clipCount > 0)
			printf(" (%d samples clipped!)", clipCount);
		printf("\n");

		string output = outputArg.getValue();
		if(output == "")
		{
			char temp[255];
			GetTempPathA(sizeof(temp)/sizeof(wchar_t), temp);

			output = temp;
			output += "testout.wav";
		}

		printf("\nWriting output to %s\n", output.c_str());

		SF_INFO info = {frameCount, sampleRate, channelCount, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 0};
		SNDFILE* outFile = sf_open(output.c_str(), SFM_WRITE, &info);
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

		if(!noPauseArg.getValue())
			system("pause");

		return 0;
	}
	catch(TCLAP::ArgException e)
	{
		printf("Error: %s for arg %s\n", e.error(), e.argId());
		return -1;
	}
}
