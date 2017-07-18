#pragma once

#include <string>
#include <iostream>
#include <algorithm>
#include <utility>
#include <functional>
#include <cctype>
#include <memory>
#include <vector>

#include <Windows.h>

#include "RuntimeOptions.h"

struct RuntimeCoverageFilter
{
	virtual bool IgnoreFile(const std::string& file) = 0;
};

struct RuntimeFileFilter : RuntimeCoverageFilter
{
	RuntimeFileFilter(const std::string& s) :
		file(s)
	{}

	std::string file;

	virtual bool IgnoreFile(const std::string& found) override
	{
		if (found.size() == file.size())
		{
			for (size_t i = 0; i < found.size(); ++i)
			{
				if (std::tolower(found[i]) != std::tolower(file[i]))
				{
					return false;
				}
			}
			return true;
		}
		return false;
	}
};

struct RuntimeFolderFilter : RuntimeCoverageFilter
{
	RuntimeFolderFilter(const std::string& s) :
		folder(s)
	{}

	std::string folder;

	virtual bool IgnoreFile(const std::string& found) override
	{
		if (found.size() > folder.size())
		{
			size_t i;
			for (i = 0; i < folder.size(); ++i)
			{
				if (std::tolower(found[i]) != std::tolower(folder[i]))
				{
					return false;
				}
			}

			if (found[i] != '\\') { return false; }

			return true;
		}
		return false;
	}
};


struct RuntimeNotifications
{
	std::vector<std::unique_ptr<RuntimeCoverageFilter>> postProcessing;

	std::string Trim(const std::string& t)
	{
		std::string s = t;
		s.erase(s.begin(), std::find_if(s.begin(), s.end(),
										std::not1(std::ptr_fun<int, int>(std::isspace))));
		s.erase(std::find_if(s.rbegin(), s.rend(),
							 std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
		return s;
	}

	std::string GetFQN(std::string s)
	{
		if (s.size() == 0) return s;

		if (s.size() > 1 && s[1] == ':')
		{
			return s;
		}
		else if (s[0] == '\\')
		{
			auto cp = RuntimeOptions::Instance().CodePath;
			if (cp.size() > 1 && cp[1] == ':')
			{
				return cp.substr(0, 2) + s;
			}
			else
			{
				return s;
			}
		}
		else
		{
			auto cp = RuntimeOptions::Instance().SourcePath();
			if (cp.size() == 0) { return s; }

			while (s.size() > 3 && s.substr(s.size() - 3, 3) == "..\\")
			{
				if (cp[cp.size() - 1] == '\\')
				{
					cp = cp.substr(0, cp.size() - 1);
				}
				auto idx = cp.find_last_of('\\');
				if (idx == std::string::npos)
				{
					return s;
				}
				else
				{
					cp = cp.substr(0, idx);
					s = s.substr(0, s.size() - 3);
				}
			}

			if (cp.size() > 0 && cp[cp.size() - 1] != '\\')
			{
				cp += "\\";
			}

			return cp + s;
		}
	}

	void Handle(const char* data, const size_t size)
	{
		std::string s(data, data + size);
		if (s.substr(0, 14) == "IGNORE FOLDER:")
		{
			auto folder = Trim(s.substr(14));
			auto fullname = GetFQN(folder);

			std::cout << "Ignoring folder: " << fullname << std::endl;
			postProcessing.emplace_back(std::make_unique<RuntimeFolderFilter>(fullname));
		}
		else if (s.substr(0, 12) == "IGNORE FILE:")
		{
			auto file = Trim(s.substr(12));
			auto fullname = GetFQN(file);

			std::cout << "Ignoring file: " << file << std::endl;
			postProcessing.emplace_back(std::make_unique<RuntimeFileFilter>(fullname));
		}
		else if (s == "ENABLE CODE ANALYSIS")
		{
			RuntimeOptions::Instance().UseStaticCodeAnalysis = true;
		}
		else if (s == "DISABLE CODE ANALYSIS")
		{
			RuntimeOptions::Instance().UseStaticCodeAnalysis = false;
		}
		else
		{
			std::cout << "Unknown option passed to coverage: " << s << std::endl;
		}
	}

	bool IgnoreFile(const std::string& filename)
	{
		for (auto& it : postProcessing)
		{
			if (it->IgnoreFile(filename))
			{
				return true;
			}
		}
		return false;
	}
};