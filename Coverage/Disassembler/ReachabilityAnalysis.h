#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>

struct ReachabilityAnalysis
{
private:
	ReachabilityAnalysis(const ReachabilityAnalysis& o);
	ReachabilityAnalysis(ReachabilityAnalysis&& o);

public:
	ReachabilityAnalysis(PHANDLE processHandle, DWORD64 baseAddress, DWORD64 methodStart, SIZE_T numberBytes);

	uint8_t* state;
	size_t numberBytes;

	~ReachabilityAnalysis()
	{
		delete[] state;
	}
};