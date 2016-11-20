#pragma once

#include "BreakpointData.h"

#include <unordered_map>
#include <Windows.h>

struct ProcessInfo
{
	ProcessInfo(DWORD pid, HANDLE handle) :
		ProcessId(pid),
		Handle(handle)
	{}

	DWORD ProcessId;
	HANDLE Handle;

	std::unordered_map<PVOID, DWORD64> LoadedModules;
	std::unordered_map<DWORD, HANDLE> Threads;
	std::unordered_map<PVOID, BreakpointData> breakPoints;
};