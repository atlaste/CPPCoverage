#pragma once

#include "ProcessInfo.h"
#include <set>

struct FileCallbackInfo;

struct CallbackInfo
{
	CallbackInfo(FileCallbackInfo* fileInfo, ProcessInfo* processInfo, bool registerLines) :
		fileInfo(fileInfo),
		processInfo(processInfo),
		registerLines(registerLines)
	{}

	FileCallbackInfo* fileInfo;
	ProcessInfo* processInfo;
	bool registerLines;
	std::set<PVOID> breakpointsToSet;

	void SetBreakpoints(PVOID baseAddress, HANDLE process)
	{
		for (auto addr : breakpointsToSet)
		{
			BYTE instruction;
			SIZE_T readBytes;

			// Read the first instruction    
			ReadProcessMemory(process, addr, &instruction, 1, &readBytes);

			// Save breakpoint data
			processInfo->breakPoints.find(addr)->second.originalData = instruction;

			// Replace it with Breakpoint
			instruction = 0xCC;
			WriteProcessMemory(process, addr, &instruction, 1, &readBytes);

			// Flush to process -- we might want to do this another way
			FlushInstructionCache(process, addr, 1);

			// std::cout << "Set breakpoint at instruction " << std::hex << addr << std::dec << std::endl;
		}

		// std::cout << "Set " << breakpointsToSet.size() << " breakpoints." << std::endl;

		// Clear for next module that's loaded
		breakpointsToSet.clear();
	}
};
