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
	enum class LineType
	{
		CODE,
		ENABLE_COVERAGE,
		DISABLE_COVERAGE
	};

	static constexpr std::string_view DISABLE_COVERAGE = "DisableCodeCoverage";
	static constexpr std::string_view ENABLE_COVERAGE = "EnableCodeCoverage";
	static constexpr std::string_view PRAGMA_LINE = "#pragma";
	static constexpr std::string_view DOUBLE_FORWARD_SLASH_LINE = "//";
	static constexpr std::string_view FORWARD_SLASH_ASTERISK_LINE = "/*";
	static constexpr std::string_view FORWARD_SLASH_ASTERISK_LINE_END = "*/";

	bool IsCoverageFlag(const std::string::const_iterator& iter, const ptrdiff_t iterSize,
											const std::string_view& coverageFlag, const bool multilineComment)
	{
		if (iterSize < coverageFlag.size())
		{
			return false;
		}
		if (iterSize == coverageFlag.size())
		{
			return std::equal(coverageFlag.begin(), coverageFlag.end(), iter);
		}

		if (!multilineComment)
		{
			return false;
		}

		if (!std::equal(coverageFlag.begin(), coverageFlag.end(), iter))
		{
			return false;
		}

		const ptrdiff_t restSize = iterSize - coverageFlag.size();
		if (restSize < FORWARD_SLASH_ASTERISK_LINE_END.size()) {
			return false;
		}

		return std::equal(FORWARD_SLASH_ASTERISK_LINE_END.begin(),
											FORWARD_SLASH_ASTERISK_LINE_END.end(),
											iter + coverageFlag.size());
	}

	LineType GetLineType(const std::string& line)
	{
		static const std::vector<std::string_view> prefixCoverage{ PRAGMA_LINE, DOUBLE_FORWARD_SLASH_LINE, FORWARD_SLASH_ASTERISK_LINE };
		for (const std::string_view& prefix : prefixCoverage)
		{
			const size_t idx = line.find(prefix);
			if (idx != std::string::npos)
			{
				std::string::const_iterator jdx = std::find_if_not(line.begin() + idx + prefix.length(), line.end(), std::isspace);
				if (jdx != line.end())
				{
					std::string::const_iterator kdx = std::find_if(jdx, line.end(), std::isspace);
					const ptrdiff_t size = kdx - jdx;
					const bool multiLineComment = prefix == FORWARD_SLASH_ASTERISK_LINE;

					if (IsCoverageFlag(jdx, size, DISABLE_COVERAGE, multiLineComment))
					{
						return LineType::DISABLE_COVERAGE;
					}
					if (IsCoverageFlag(jdx, size, ENABLE_COVERAGE, multiLineComment))
					{
						return LineType::ENABLE_COVERAGE;
					}
				}
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
			}
			else if (lineType == LineType::ENABLE_COVERAGE)
			{
				relevant.push_back(current);
				current = true;
				continue;
			}

			relevant.push_back(current);
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
