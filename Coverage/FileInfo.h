#pragma once

#include "FileLineInfo.h"
#include "RuntimeOptions.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

struct FileInfo
{
	FileInfo(const std::string& filename)
	{
		std::ifstream ifs(filename);

		bool current = true;

		std::string line;
		while (std::getline(ifs, line))
		{
			// Process str
			size_t idx = line.find("#pragma");
			if (idx != std::string::npos)
			{
				size_t jdx = line.find_first_not_of(' ', idx + 7);
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
						current = false;
					}
					else if (prag == "EnableCodeCoverage")
					{
						relevant.push_back(current);
						current = true;
						continue;
					}
				}
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
