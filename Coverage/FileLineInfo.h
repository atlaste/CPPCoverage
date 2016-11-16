#pragma once

#include <cstdint>

struct FileLineInfo
{
	FileLineInfo() :
		DebugCount(0),
		HitCount(0)
	{}

	uint16_t DebugCount;
	uint16_t HitCount;
};
