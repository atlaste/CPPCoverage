#pragma once

#include "FileLineInfo.h"

#include <iostream>
#include <string>
#include <vector>

struct FileInfo
{
private:
  static constexpr std::string_view DISABLE_COVERAGE = "DisableCodeCoverage";
  static constexpr std::string_view ENABLE_COVERAGE = "EnableCodeCoverage";
  static constexpr std::string_view PRAGMA_LINE = "#pragma";
  static constexpr std::string_view DOUBLE_FORWARD_SLASH_LINE = "//";

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

  bool StringStartsWith(const std::string::const_iterator& start, const std::string::const_iterator& end, const std::string_view& prefix);

  std::string::const_iterator GetBeginCoverageFlag(const std::string& line);

  LineType GetLineType(const std::string& line);
public:
  FileInfo(const std::string& filename);

  std::vector<bool> relevant;
  std::vector<FileLineInfo> lines;
  size_t numberLines;

  FileLineInfo* LineInfo(size_t lineNumber);
};
