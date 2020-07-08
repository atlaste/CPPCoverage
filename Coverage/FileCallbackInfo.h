#pragma once

#include "FileLineInfo.h"
#include "FileInfo.h"
#include "BreakpointData.h"
#include "RuntimeOptions.h"
#include "CallbackInfo.h"
#include "ProfileNode.h"
#include "RuntimeNotifications.h"

#include <string>
#include <set>
#include <unordered_map>
#include <memory>
#include <Windows.h>
#include <ctime>

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

		packageName = opts.PackageName;
	}

	std::string filename;
	std::string sourcePath;
	std::string packageName;

	std::unordered_map<std::string, std::unique_ptr<FileInfo>> lineData;

	void Filter(RuntimeNotifications& notifications)
	{
		std::unordered_map<std::string, std::unique_ptr<FileInfo>> newLineData;
		for (auto& it : lineData)
		{
			if (!notifications.IgnoreFile(it.first))
			{
				std::unique_ptr<FileInfo> tmp(nullptr);
				std::swap(tmp, it.second);
				newLineData[it.first] = std::move(tmp);
			}
			else
			{
				std::cout << "Removing file " << it.first << std::endl;
			}
		}
		std::swap(lineData, newLineData);
	}

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
			case RuntimeOptions::Clover: WriteClover(filename); break;
			case RuntimeOptions::Cobertura: WriteCobertura(filename); break;
			default: WriteNative(filename, mergedProfileInfo); break;
		}
	}

	void WriteClover(const std::string& filename)
	{
		std::string reportFilename = filename;
		std::ofstream ofs(reportFilename);

		size_t totalFiles = 0;
		size_t coveredFiles = 0;
		size_t totalLines = 0;
		size_t coveredLines = 0;
		for (auto& it : lineData)
		{
			auto ptr = it.second.get();
			for (size_t i = 0; i < ptr->numberLines; ++i)
			{
				if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
				{
					++totalFiles;
					if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
					{
						++coveredFiles;
					}

					totalLines += ptr->lines[i].DebugCount;
					coveredLines += ptr->lines[i].HitCount;
				}
			}
		}

		time_t t = time(0);   // get time now
		ofs << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
		ofs << "<clover generated=\"" << t << "\"  clover=\"3.1.5\">" << std::endl;
		ofs << "<project timestamp=\"" << t << "\">" << std::endl;
		ofs << "<metrics classes=\"0\" files=\"" << totalFiles << "\" packages=\"1\"  loc=\"" << totalLines << "\" ncloc = \"" << coveredLines << "\" ";
		// ofs << "coveredstatements=\"300\" statements=\"500\" coveredmethods=\"50\" methods=\"80\" ";
		// ofs << "coveredconditionals=\"100\" conditionals=\"120\" coveredelements=\"900\" elements=\"1000\" ";
		ofs << "complexity=\"0\" />" << std::endl;
		ofs << "<package name=\"" << packageName << "\">" << std::endl;
		for (auto& it : lineData)
		{
			auto ptr = it.second.get();

			ofs << "<file name=\"" << it.first << "\">" << std::endl;

			for (size_t i = 0; i < ptr->numberLines; ++i)
			{
				if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
				{
					if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
					{
						ofs << "<line num=\"" << i << "\" count=\"1\" type=\"stmt\"/>" << std::endl;
					}
					else
					{
						ofs << "<line num=\"" << i << "\" count=\"0\" type=\"stmt\"/>" << std::endl;
					}
				}
			}

			ofs << "</file>" << std::endl;
		}

		ofs << "</package>" << std::endl;
		ofs << "</project>" << std::endl;
		ofs << "</clover>" << std::endl;
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
		ofs << "<coverage line-rate=\"" << lineRate << "\"" << " " << "version=\"\">" << std::endl;
		ofs << "\t" << "<packages>" << std::endl;
		
		ofs << "\t\t" << "<package name=\"" << packageName << "\" line-rate=\"" << lineRate << "\">" << std::endl;
		ofs << "\t\t\t" << "<classes>" << std::endl;
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

			ofs << "\t\t\t\t" << "<class name=\"" << name << "\" filename=\"" << it.first.substr(2) << "\" line-rate=\"" << lineRate << "\">" << std::endl;
			ofs << "\t\t\t\t\t" << "<lines>" << std::endl;

			for (size_t i = 0; i < ptr->numberLines; ++i)
			{
				if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
				{
					if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
					{
						ofs << "\t\t\t\t\t\t" << "<line number=\"" << i << "\" hits=\"1\"/>" << std::endl;
					}
					else
					{
						ofs << "\t\t\t\t\t\t" << "<line number=\"" << i << "\" hits=\"0\"/>" << std::endl;
					}
				}
			}

			ofs << "\t\t\t\t\t" << "</lines>" << std::endl;
			ofs << "\t\t\t\t" << "</class>" << std::endl;
		}

		ofs << "\t\t\t" << "</classes>" << std::endl;
		ofs << "\t\t" << "</package>" << std::endl;
		ofs << "\t" << "</packages>" << std::endl;
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
