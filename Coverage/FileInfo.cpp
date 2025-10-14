#include "FileInfo.h"
#include "RuntimeOptions.h"
#include "Util.h"

#include <algorithm>
#include <fstream>

FileInfo::FileInfo(std::istream& ifs)
{
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

bool FileInfo::StringStartsWith(const std::string::const_iterator& start, const std::string::const_iterator& end, const std::string_view& prefix)
{
  const auto distance = std::distance(start, end);
  if ((distance < 0) || ((size_t) distance < prefix.length()))
    return false;
  return std::mismatch(prefix.begin(), prefix.end(), start).first == prefix.end();
}

std::string::const_iterator FileInfo::GetBeginCoverageFlag(const std::string& line)
{
  // try to find #pragma (before pragma may be only whitespace)
  std::string::const_iterator idx = std::find_if_not(line.begin(), line.end(), Util::IsSpace);
  if (StringStartsWith(idx, line.end(), PRAGMA_LINE))
  {
    std::string::const_iterator jdx = std::find_if_not(idx + PRAGMA_LINE.length(), line.end(), Util::IsSpace);
    return jdx;
  }

  // try to find single-line comment (before single-line comment may be everything)
  size_t commentIdx = line.find(DOUBLE_FORWARD_SLASH_LINE);
  if (commentIdx != std::string::npos)
  {
    std::string::const_iterator jdx = std::find_if_not(line.begin() + commentIdx + DOUBLE_FORWARD_SLASH_LINE.length(), line.end(), Util::IsSpace);
    return jdx;
  }

  // prefix not found
  return line.end();
}

FileInfo::LineType FileInfo::GetLineType(const std::string& line)
{
  std::string::const_iterator coverageBeginValue = GetBeginCoverageFlag(line);
  if (coverageBeginValue != line.end())
  {
    std::string::const_iterator coverageEndValue = std::find_if(coverageBeginValue, line.end(), Util::IsSpace);
    const ptrdiff_t size = coverageEndValue - coverageBeginValue;
    if (IsCoverageFlag(coverageBeginValue, size, DISABLE_COVERAGE))
    {
      return LineType::DISABLE_COVERAGE;
    }
    if (IsCoverageFlag(coverageBeginValue, size, ENABLE_COVERAGE))
    {
      return LineType::ENABLE_COVERAGE;
    }
  }

  return LineType::CODE;
}

FileLineInfo* FileInfo::LineInfo(size_t lineNumber)
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
      if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Warning))
      {
        std::cout << "Warning: line number out of bounds: " << lineNumber << " >= " << numberLines << std::endl;
      }
    }

    return nullptr;
  }
}