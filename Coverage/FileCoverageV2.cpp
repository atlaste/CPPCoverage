#include "FileCoverageV2.h"
#include "base64.h"

#include <cassert>
#include <format>

FileCoverageV2::FileCoverageV2(size_t nbLines) :
  _nbLinesFile(nbLines)
{
  _code.resize(nbLines);
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
      _nbLinesCode += 1;
    }
    if (line.HitCount > 0)
    {
      if (line.DebugCount != line.HitCount)
      {
        code |= FileCoverageV2::maskIsPartial;
      }
      _nbLinesCovered += 1;
    }
    code |= std::min<FileCoverageV2::LineArray::value_type>(line.HitCount, FileCoverageV2::maskCount);
  }
  return code;
}

void FileCoverageV2::updateStats()
{
  _nbLinesCovered = 0;
  _nbLinesCode = 0;
  for (const auto& line : _code)
  {
    if ((line & maskIsCode) == maskIsCode)
    {
      _nbLinesCode++;
      if ((line & maskCount) > 0)
      {
        _nbLinesCovered++;
      }
    }
  }
  assert(_nbLinesCovered <= _nbLinesCode);
}

bool FileCoverageV2::merge(const FileCoverageV2& other)
{
  if (_code.size() != other._code.size())
    return false;

  auto src = other._code.cbegin();

  for (auto& line : _code)
  {
    const size_t count = (size_t) (line & maskCount) + (size_t) (*src & maskCount);

    const bool isCode = (line & maskIsCode) == maskIsCode;
    const bool isPartial = (line & maskIsPartial) == maskIsPartial && (*src & maskIsPartial) == maskIsPartial;

    line = (uint16_t) std::min<size_t>(count, maskCount);
    line |= isCode ? maskIsCode : 0;
    line |= isPartial ? maskIsPartial : 0;

    ++src;
  }
  updateStats();
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
  ofs << std::format(R"(		<file path="{0}" md5="{1}">)", filepath, md5Code) << std::endl;
  ofs << std::format(R"(			<stats nbLinesInFile="{0}" nbLinesOfCode="{1}" nbLinesCovered="{2}"/>)", _nbLinesFile, _nbLinesCode, _nbLinesCovered) << std::endl;
  ofs << R"(			<coverage>)" << Base64::Encode(std::string(reinterpret_cast<const char*>(_code.data()), _code.size() * sizeof(LineArray::value_type))) << "</coverage>" << std::endl;
  ofs << R"(		</file>)" << std::endl;
}