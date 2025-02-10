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
	static constexpr std::string_view PRAGMA_LINE = "#pragma";

	enum class LineType
	{
		CODE,
		ENABLE_COVERAGE,
		DISABLE_COVERAGE
	};

	bool StringStartsWith(const std::string& line, const std::string_view& prefix)
	{
		if (line.length() < prefix.length())
			return false;
		return std::mismatch(prefix.begin(), prefix.end(), line.begin()).first == prefix.end();
	}

	LineType GetLineType(std::string& line)
	{
		line.erase(line.begin(), std::find_if_not(line.begin(), line.end(), std::isspace));
		if (StringStartsWith(line, PRAGMA_LINE))
		{
			size_t jdx = line.find_first_not_of(' ', PRAGMA_LINE.length());
			if (jdx != std::string::npos)
			{
				std::string prag = line.substr(jdx);
				size_t kdx = prag.find(' ');
				if (kdx != std::string::npos)
				{
					prag = prag.substr(0, kdx);
				}

				if (prag == "DisableCodeCoverage")
				{
					return LineType::DISABLE_COVERAGE;
				}
				else if (prag == "EnableCodeCoverage")
				{
					return LineType::ENABLE_COVERAGE;
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
