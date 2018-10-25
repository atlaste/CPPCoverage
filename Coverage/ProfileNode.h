#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include <Windows.h>

struct ProfileInfo
{
	ProfileInfo() :
		Shallow(0),
		Deep(0)
	{}

	float Shallow;
	float Deep;
};

struct ProfileFrame
{
	ProfileFrame(const std::string& filename, DWORD64 lineNumber, bool shallow) :
		filename(filename)
	{
		Update(lineNumber, shallow);
	}

	std::string filename;
	std::unordered_map<DWORD64, ProfileInfo> lineHitCount;

	void Update(DWORD64 lineNumber, bool shallow)
	{
		if (lineNumber < 0xf00000)
		{
			auto it = lineHitCount.find(lineNumber);
			if (it == lineHitCount.end())
			{
				ProfileInfo pi;
				pi.Deep++;
				pi.Shallow += shallow ? 1 : 0;

				lineHitCount[lineNumber] = pi;
			}
			else
			{
				it->second.Deep++;
				it->second.Shallow += shallow ? 1 : 0;
			}
		}
	}
};

