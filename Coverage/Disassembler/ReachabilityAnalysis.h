#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <iostream>

struct ReachabilityAnalysis
{
	ReachabilityAnalysis(const ReachabilityAnalysis&) = delete;
	ReachabilityAnalysis& operator=(const ReachabilityAnalysis&) = delete;

	ReachabilityAnalysis(ReachabilityAnalysis&& o) noexcept
	{
		state = 0;
		methodStart = 0;
		numberBytes = 0;

		std::swap(state, o.state);
		std::swap(methodStart, o.methodStart);
		std::swap(numberBytes, o.numberBytes);
	}

	ReachabilityAnalysis& operator=(ReachabilityAnalysis&& o) noexcept
	{
		if (this != &o)
		{
			delete state;

			state = 0;
			methodStart = 0;
			numberBytes = 0;

			std::swap(state, o.state);
			std::swap(methodStart, o.methodStart);
			std::swap(numberBytes, o.numberBytes);
		}
		return *this;
	}

	ReachabilityAnalysis(HANDLE processHandle, DWORD64 methodStart, SIZE_T numberBytes);

	static size_t FirstInstructionSize(HANDLE processHandle, DWORD64 methodStart);

	DWORD64 methodStart;
	size_t numberBytes;
	uint8_t* state;

	bool operator<(const ReachabilityAnalysis& o) const
	{
		return methodStart < o.methodStart;
	}

	friend void swap(ReachabilityAnalysis& lhs, ReachabilityAnalysis& rhs)
	{
		std::swap(lhs.state, rhs.state);
		std::swap(lhs.methodStart, rhs.methodStart);
		std::swap(lhs.numberBytes, rhs.numberBytes);
	}

	~ReachabilityAnalysis()
	{
		delete[] state;
		state = 0;
	}
};