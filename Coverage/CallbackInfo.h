#pragma once

#include "ProcessInfo.h"
#include "Disassembler/ReachabilityAnalysis.h"
#include <set>
#include <vector>

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

	std::vector<ReachabilityAnalysis> reachableCode;

	void SetBreakpoints(PVOID baseAddress, HANDLE process)
	{
		BYTE buffer[4096];

		auto it = breakpointsToSet.begin();
		while (it != breakpointsToSet.end())
		{
			auto jt = breakpointsToSet.lower_bound(reinterpret_cast<BYTE*>(*it) + 4096);
			if (jt == breakpointsToSet.end() || jt == it)
			{
				for (; it != breakpointsToSet.end(); ++it)
				{
					auto addr = *it;

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
				}
			}
			else
			{
				auto diff = reinterpret_cast<BYTE*>(*jt) - reinterpret_cast<BYTE*>(*it);
				if (diff > 4096) { diff = 4096; }

				SIZE_T readBytes;

				auto startAddr = reinterpret_cast<BYTE*>(*it);

				// Read the instructions
				ReadProcessMemory(process, startAddr, buffer, diff, &readBytes);

				auto limit = startAddr + readBytes;

				for (; it != breakpointsToSet.end() && reinterpret_cast<BYTE*>(*it) < limit; ++it)
				{
					auto addr = *it;
					auto idx = reinterpret_cast<BYTE*>(addr) - startAddr;

					// Save breakpoint data
					processInfo->breakPoints.find(addr)->second.originalData = buffer[idx];

					// Replace it with Breakpoint
					buffer[idx] = 0xCC;
				}

				WriteProcessMemory(process, startAddr, buffer, readBytes, &readBytes);

				// Flush to process -- we might want to do this another way
				FlushInstructionCache(process, startAddr, readBytes);
			}
		}

		// Clear for next module that's loaded
		breakpointsToSet.clear();
	}
};
