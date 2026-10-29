#include "FileCoverageV2.h"
#include "base64.h"

#include <cassert>
#include <format>

FileCoverageV2::FileCoverageV2(const FileInfo& info, const std::string_view& md5)
{
  assert(info.relevant.size() == info.lines.size());
  auto itRelevant = info.relevant.cbegin();
  _code.resize(info.lines.size());
  md5Code = md5;

  auto itCoverage = _code.begin();

  for (const auto& line : info.lines)
  {
    *itCoverage = encodeLine(*itRelevant, line);
    ++itCoverage;
    ++itRelevant;
  }
}

FileCoverageV2::FileCoverageV2(const std::vector<uint8_t>& coverage, const std::string_view& md5)
{
  _code.resize(coverage.size() / sizeof(LineArray::value_type));

  md5Code = md5;
  std::memcpy(_code.data(), coverage.data(), coverage.size());
}

FileCoverageV2::LineArray::value_type FileCoverageV2::encodeLine(bool isCode, const FileLineInfo& line)
{
  LineArray::value_type code = 0;
  if (isCode)
  {
    // |    15   |     14     | 13 ---- 0 | 
    // | is code | is partial |   count   |
    if (line.DebugCount > 0)
    {
      code |= FileCoverageV2::maskIsCode;
    }
    if ((line.HitCount > 0) && (line.DebugCount != line.HitCount))
    {
      code |= FileCoverageV2::maskIsPartial;
    }
    code |= std::min<LineArray::value_type>(line.HitCount, FileCoverageV2::maskCount);
  }
  return code;
}

std::pair<size_t, size_t> FileCoverageV2::stats() const
{
  size_t nbLinesCovered = 0;
  size_t nbLinesCode = 0;
  for (const auto& line : _code)
  {
    if ((line & maskIsCode) == maskIsCode)
    {
      nbLinesCode++;
      if ((line & maskCount) > 0)
      {
        nbLinesCovered++;
      }
    }
  }
  return { nbLinesCode, nbLinesCovered };
}

bool FileCoverageV2::merge(const FileCoverageV2& other)
{
  if (_code.size() != other._code.size())
    return false;

  auto src = other._code.cbegin();

  for (auto& line : _code)
  {
    const size_t count = (size_t) (line & maskCount) + (size_t) (*src & maskCount);

    const bool isCode = ((line & maskIsCode) | (*src & maskIsCode)) == maskIsCode || count > 0;
    const bool isPartial = (line & maskIsPartial) == maskIsPartial && (*src & maskIsPartial) == maskIsPartial;

    line = (uint16_t) std::min<size_t>(count, maskCount);
    line |= isCode ? maskIsCode : 0;
    line |= isPartial ? maskIsPartial : 0;

    ++src;
  }
  return true;
}

void FileCoverageV2::writeHeader(std::ostream& ofs)
{
  const std::string version("2.0");

  ofs << R"(<?xml version="1.0" encoding="utf-8"?>)" << std::endl;
  ofs << std::format(R"(<CppCoverage version="{0}">)", version) << std::endl;
}

void FileCoverageV2::openDirectory(std::ostream& ofs, const std::string& aDir)
{
  ofs << std::format(R"(	<directory path="{0}">)", aDir) << std::endl;
}

void FileCoverageV2::closeDirectory(std::ostream& ofs)
{
  ofs << "	</directory>" << std::endl;
}

void FileCoverageV2::writeFooter(std::ostream& ofs)
{
  ofs << "</CppCoverage>" << std::endl;
}

void FileCoverageV2::write(const std::string& filepath, std::ostream& ofs) const
{
  const auto& [codeLines, coveredLines] = stats();
  ofs << std::format(R"(		<file path="{0}" md5="{1}">)", filepath, md5Code) << std::endl;
  ofs << std::format(R"(			<stats nbLinesInFile="{0}" nbLinesOfCode="{1}" nbLinesCovered="{2}"/>)", _code.size(), codeLines, coveredLines) << std::endl;
  ofs << R"(			<coverage>)" << Base64::Encode(reinterpret_cast<const uint8_t*>(_code.data()), _code.size() * sizeof(LineArray::value_type)) << "</coverage>" << std::endl;
  ofs << R"(		</file>)" << std::endl;
}