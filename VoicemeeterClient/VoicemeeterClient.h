#pragma once

#include <string>
#include <vector>
#include "FilterEngine.h"
#include "VoicemeeterRemote.h"

class VoicemeeterClient
{
public:
	VoicemeeterClient(const std::vector<std::wstring>& outputs);
	~VoicemeeterClient();
	void start();
	void stop();

	void handle(long nCommand, void* lpData, long nnn);
	bool restart = false;

private:
	bool isBufferSilent(float** sampleData, long sampleCount);

	unsigned long mainThreadId;
	T_VBVMR_INTERFACE vmr;
	std::vector<FilterEngine*> engines;
	std::vector<int> idleSampleCounts;
	bool loggedIn = false;
	bool banana = false;
	std::vector<std::wstring> outputs;
};
