#pragma once

#include "FileInfo.h"

class FileCoverageV2
{
public:
  static const uint16_t maskCount = 0x3FFF;		///< Count access of this line
  static const uint16_t maskIsCode = 0x8000;
  static const uint16_t maskIsPartial = 0x4000;

  using LineArray = std::vector<uint16_t>;

  explicit FileCoverageV2(const FileInfo& info, const std::string_view& md5);
  explicit FileCoverageV2(const std::vector<uint8_t>& coverage, const std::string_view& md5);

  bool merge(const FileCoverageV2& other);

  static void writeHeader(std::ostream& ofs);
  static void openDirectory(std::ostream& ofs, const std::string& aDir);
  static void closeDirectory(std::ostream& ofs);
  static void writeFooter(std::ostream& ofs);
  void write(const std::string& filepath, std::ostream& ofs) const;
private:
  LineArray _code;
  std::string md5Code;

  LineArray::value_type encodeLine(bool isCode, const FileLineInfo& line);
  std::pair<size_t, size_t> stats() const;
};