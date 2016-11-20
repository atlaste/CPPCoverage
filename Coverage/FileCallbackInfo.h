#pragma once

#include "FileLineInfo.h"
#include "FileInfo.h"
#include "BreakpointData.h"
#include "RuntimeOptions.h"
#include "CallbackInfo.h"
#include "ProfileNode.h"

#include <string>
#include <set>
#include <unordered_map>
#include <memory>
#include <Windows.h>

struct FileCallbackInfo
{
	FileCallbackInfo(const std::string& filename) :
		filename(filename)
	{
		auto& opts = RuntimeOptions::Instance();
		if (opts.CodePath.size() == 0)
		{
			auto idx = filename.find("x64");
			if (idx == std::string::npos)
			{
				idx = filename.find("Debug");
			}
			if (idx == std::string::npos)
			{
				idx = filename.find("Release");
			}
			if (idx == std::string::npos)
			{
				idx = filename.find('\\');
			}
			if (idx == std::string::npos)
			{
				throw "Cannot locate source file base for this executable";
			}
			sourcePath = filename.substr(0, idx);
		}
		else
		{
			sourcePath = opts.CodePath;
		}
	}

	std::string filename;
	std::string sourcePath;

	std::unordered_map<std::string, std::unique_ptr<FileInfo>> lineData;
	
	bool PathMatches(const char* filename)
	{
		const char* ptr = filename;
		const char* gt = sourcePath.data();
		const char* gte = gt + sourcePath.size();

		for (; *ptr && gt != gte; ++ptr, ++gt)
		{
			char lhs = tolower(*gt);
			char rhs = tolower(*ptr);
			if (lhs != rhs) { return false; }
		}

		return true;
	}

	FileLineInfo *LineInfo(const std::string& filename, DWORD64 lineNumber)
	{
		auto it = lineData.find(filename);
		if (it == lineData.end())
		{
			auto newLineData = new FileInfo(filename);
			lineData[filename] = std::unique_ptr<FileInfo>(newLineData);

			return newLineData->LineInfo(size_t(lineNumber));
		}
		else
		{
			return it->second->LineInfo(size_t(lineNumber));
		}
	}

	void WriteReport(RuntimeOptions::ExportFormatType exportFormat, std::unordered_map<std::string, std::unique_ptr<std::vector<ProfileInfo>>>& mergedProfileInfo, const std::string& filename)
	{
		switch (exportFormat)
		{
			case RuntimeOptions::Cobertura: WriteCobertura(filename); break;
			default: WriteNative(filename, mergedProfileInfo); break;
		}
	}

	void WriteCobertura(const std::string& filename)
	{
		std::string reportFilename = filename;
		std::ofstream ofs(reportFilename);

		double total = 0;
		double covered = 0;
		for (auto& it : lineData)
		{
			auto ptr = it.second.get();
			for (size_t i = 0; i < ptr->numberLines; ++i)
			{
				if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
				{
					++total;
					if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
					{
						++covered;
					}
				}
			}
		}

		double lineRate = covered / total;

		ofs << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
		ofs << "<coverage line-rate=\"" << lineRate << "\">" << std::endl;
		ofs << "<packages>" << std::endl;
		ofs << "<package name=\"Program.exe\" line-rate=\"" << lineRate << "\">" << std::endl;
		ofs << "<classes>" << std::endl;
		for (auto& it : lineData)
		{
			auto ptr = it.second.get();

			std::string name = it.first;
			auto idx = name.find_last_of('\\');
			if (idx != std::string::npos)
			{
				name = name.substr(idx + 1);
			}

			double total = 0;
			double covered = 0;
			for (size_t i = 0; i < ptr->numberLines; ++i)
			{
				if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
				{
					++total;
					if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
					{
						++covered;
					}
				}
			}

			double lineRate = covered / total;

			ofs << "<class name=\"" << name << " filename=\"" << it.first.substr(2) << "\" line-rate=\"" << lineRate << "\">" << std::endl;
			ofs << "<lines>" << std::endl;

			for (size_t i = 0; i < ptr->numberLines; ++i)
			{
				if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
				{
					if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
					{
						ofs << "<line number=\"" << i << "\" hits=\"1\"/>" << std::endl;
					}
					else
					{
						ofs << "<line number=\"" << i << "\" hits=\"0\"/>" << std::endl;
					}
				}
			}

			ofs << "</lines>" << std::endl;
			ofs << "</class>" << std::endl;
		}

		ofs << "</classes>" << std::endl;
		ofs << "</package>" << std::endl;
		ofs << "</packages>" << std::endl;
		ofs << "<sources><source>c:</source></sources>" << std::endl;
		ofs << "</coverage>" << std::endl;
	}

	void WriteNative(const std::string& filename, std::unordered_map<std::string, std::unique_ptr<std::vector<ProfileInfo>>>& mergedProfileInfo)
	{
		std::string reportFilename = filename;
		std::ofstream ofs(reportFilename);

		for (auto& it : lineData)
		{
			ofs << "FILE: " << it.first << std::endl;
			auto ptr = it.second.get();

			std::string result;
			result.reserve(ptr->numberLines + 1);

			for (size_t i = 0; i < ptr->numberLines; ++i)
			{
				char state = 'i';
				if (ptr->relevant[i])
				{
					auto& line = ptr->lines[i];
					if (line.DebugCount == 0)
					{
						state = '_';
					}
					else if (line.DebugCount == line.HitCount)
					{
						state = 'c';
					}
					else if (line.HitCount == 0)
					{
						state = 'u';
					}
					else
					{
						state = 'p';
					}
				}
				result.push_back(state);
			}

			ofs << "RES: " << result << std::endl;

			auto profInfo = mergedProfileInfo.find(it.first);

			if (profInfo == mergedProfileInfo.end())
			{
				ofs << "PROF: " << std::endl;
			}
			else
			{
				ofs << "PROF: ";
				for (auto& it : *(profInfo->second.get()))
				{
					ofs << int(it.Deep) << ',' << int(it.Shallow) << ',';
				}
				ofs << std::endl;
			}
		}
	}
};
