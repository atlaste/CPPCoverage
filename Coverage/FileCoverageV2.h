#pragma once

#include "base64.h"
#include "FileInfo.h"

#include <format>
#include <fstream>

struct FileCoverageV2
{
	static const uint16_t maskCount		= 0x3FFF;		///< Count access of this line
	static const uint16_t maskIsCode	= 0x8000;
	static const uint16_t maskIsPartial = 0x4000;

	using LineArray = std::vector<uint16_t>;
	LineArray _code;
	size_t _nbLinesFile		= 0;
	size_t _nbLinesCode		= 0;
	size_t _nbLinesCovered	= 0;
	std::string md5Code;

	FileCoverageV2(size_t nbLines = 0) :
		_nbLinesFile(nbLines)
	{
		_code.resize(nbLines);
	}

	LineArray::value_type encodeLine(bool isCode, const FileLineInfo& line)
	{
		LineArray::value_type code = 0;
		if (isCode)
		{
			// |    15   |     14     | 13 ---- 0 | 
			// | is code | is partial |   count   |
			if (line.DebugCount > 0)
			{
				code |= FileCoverageV2::maskIsCode;
				_nbLinesCode += 1;

			}
			if (line.DebugCount != line.HitCount)
			{
				code |= FileCoverageV2::maskIsPartial;
			}
			if (line.HitCount > 0)
			{
				_nbLinesCovered += 1;
			}
			code |= std::min<FileCoverageV2::LineArray::value_type>(line.HitCount, FileCoverageV2::maskCount);
		}
		return code;
	}

	void updateStats()
	{
		_nbLinesCovered = 0;
		for (const auto& line : _code)
		{
			if ((line & maskIsCode) == maskIsCode)
			{
				if ((line & maskCount) > 0)
				{
					_nbLinesCovered += 1;
				}
			}
		}
	}

	bool merge(const FileCoverageV2& other)
	{
		if (_code.size() != other._code.size())
			return false;

		auto src = other._code.cbegin();

		for (auto& line : _code)
		{
			const size_t count = (size_t)(line & maskCount) + (size_t)(*src & maskCount);

			const bool isCode = (line & maskIsCode) == maskIsCode;
			const bool isPartial = (line & maskIsPartial) == maskIsPartial && (*src & maskIsPartial) == maskIsPartial;

			line = (uint16_t)std::min<size_t>(count, maskCount);
			line |= isCode ? maskIsCode : 0;
			line |= isPartial ? maskIsPartial : 0;

			++src;
		}
		updateStats();
		return true;
	}

	static void writeHeader(std::ofstream& ofs)
	{
		const std::string version("2.0");

		ofs << R"(<?xml version="1.0" encoding="utf-8"?>)" << std::endl;
		ofs << std::format(R"(<CppCoverage version="{0}">)", version) << std::endl;
	}

	static void writeFooter(std::ofstream& ofs)
	{
		ofs << "</CppCoverage>" << std::endl;
	}

	void write(const std::string& filepath, std::ostream& ofs) const
	{
		ofs << std::format(R"(	<file path="{0}" md5="{1}">)", filepath, md5Code) << std::endl;
		ofs << std::format(R"(		<stats nbLinesInFile="{0}" nbLinesOfCode="{1}" nbLinesCovered="{2}"/>)", _nbLinesFile, _nbLinesCode, _nbLinesCovered) << std::endl;
		ofs << R"(		<coverage>)" << Base64::Encode(std::string(reinterpret_cast<const char*>(_code.data()), _code.size() * sizeof(LineArray::value_type))) << "</coverage>" << std::endl;
		ofs << R"(	</file>)" << std::endl;
	}
};