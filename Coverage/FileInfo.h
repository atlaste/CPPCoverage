#pragma once

#include "FileLineInfo.h"
#include "RuntimeOptions.h"

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

struct FileInfo
{
private:
	static constexpr std::string_view DISABLE_COVERAGE = "DisableCodeCoverage";
	static constexpr std::string_view ENABLE_COVERAGE = "EnableCodeCoverage";
	static constexpr std::string_view PRAGMA_LINE = "#pragma";

	enum class LineType
	{
		CODE,
		ENABLE_COVERAGE,
		DISABLE_COVERAGE
	};

	bool IsCoverageFlag(const std::string::const_iterator& iter, const ptrdiff_t iterSize,
											const std::string_view& coverageFlag)
	{
		if (iterSize != coverageFlag.size())
		{
			return false;
		}

		return std::equal(coverageFlag.begin(), coverageFlag.end(), iter);
	}

	bool StringStartsWith(const std::string::const_iterator& start, const std::string::const_iterator& end, const std::string_view& prefix)
	{
		if (end - start < prefix.length())
			return false;
		return std::mismatch(prefix.begin(), prefix.end(), start).first == prefix.end();
	}

	std::string::const_iterator GetBeginCoverageFlag(const std::string& line)
	{
		// try to find #pragma (before pragma may be only whitespace)
		std::string::const_iterator idx = std::find_if_not(line.begin(), line.end(), std::isspace);
		if (StringStartsWith(idx, line.end(), PRAGMA_LINE)) {
			std::string::const_iterator jdx = std::find_if_not(idx + PRAGMA_LINE.length(), line.end(), std::isspace);
			return jdx;
		}

		// prefix not found
		return line.end();
	}

	LineType GetLineType(const std::string& line)
	{
		std::string::const_iterator coverageBeginValue = GetBeginCoverageFlag(line);
		if (coverageBeginValue != line.end())
		{
			std::string::const_iterator coverageEndValue = std::find_if(coverageBeginValue, line.end(), std::isspace);
			const ptrdiff_t size = coverageEndValue - coverageBeginValue;
			if (IsCoverageFlag(coverageBeginValue, size, DISABLE_COVERAGE)) {
				return LineType::DISABLE_COVERAGE;
			}
			if (IsCoverageFlag(coverageBeginValue, size, ENABLE_COVERAGE)) {
				return LineType::ENABLE_COVERAGE;
			}
		}

		return LineType::CODE;
	}
public:
	FileInfo(const std::string& filename)
	{
		std::ifstream ifs(filename);

		bool current = true;

		std::string line;
		while (std::getline(ifs, line))
		{
			// Process str
			LineType lineType = GetLineType(line);
			if (lineType == LineType::DISABLE_COVERAGE)
			{
				current = false;
				relevant.push_back(current);
			}
			else if (lineType == LineType::ENABLE_COVERAGE)
			{
				relevant.push_back(current);
				current = true;
			}
			else
			{
				relevant.push_back(current);
			}
		}

		numberLines = relevant.size();

		lines.resize(numberLines);
	}

	std::vector<bool> relevant;
	std::vector<FileLineInfo> lines;
	size_t numberLines;

	FileLineInfo *LineInfo(size_t lineNumber)
	{
		// PDB lineNumbers are 1-based; we work 0-based.
		--lineNumber;

		if (lineNumber < numberLines)
		{
			if (relevant[lineNumber])
			{
				return lines.data() + lineNumber;
			}
			else
			{
				return nullptr;
			}
		}
		else
		{
			// Check for the secret 0xFeeFee line number that hides stuff. Also, the lineNumber == numberLines happens 
			// a lot, but is actually out-of-bounds. We ignore both.
			//
			// Update: Apparently Microsoft has added new reserved line numbers to their debugging symbols. Joy...
			if (lineNumber < 0xf00000 - 1 &&
				lineNumber != numberLines)
			{
				if (!RuntimeOptions::Instance().Quiet)
				{
					std::cout << "Warning: line number out of bounds: " << lineNumber << " >= " << numberLines << std::endl;
				}
			}

			return nullptr;
		}
	}
};
