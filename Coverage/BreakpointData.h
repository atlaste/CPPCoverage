#pragma once

#include "FileLineInfo.h"

#include <Windows.h>

struct BreakpointData
{
	BreakpointData() {}
	BreakpointData(char originalData, FileLineInfo* lineInfo) :
		originalData(originalData),
		lineInfo(lineInfo)
	{}

	BYTE originalData;
	FileLineInfo* lineInfo;
};
