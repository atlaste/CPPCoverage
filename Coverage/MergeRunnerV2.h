#pragma once

#include "base64.h"
#include "FileCallbackInfo.h"
#include "MergeRunner.h"

#include <filesystem>
#include <sstream>
#include <map>
#include <regex>

namespace TestFormat
{
  class TestNativeV2;
}

class MergeRunnerV2 : public MergeRunner
{
public:
  friend class TestFormat::TestNativeV2;
private:
  using DictCoverage = std::unordered_map<std::string, FileCoverageV2>;

  std::string clean(const std::string& content) const
  {
    // Need to remove return line (not supported by regex)
    const std::regex PatternClean("\r|\n|\t", std::regex_constants::ECMAScript);
    return  std::regex_replace(content, PatternClean, "");
  }

  void parseFile(const std::string& filename, std::istream& stream, DictCoverage& codeCoverage, const std::string& fileline) const
  {
    try
    {
      // read whole data about file
      std::string filedata = fileline;
      std::string line;
      while (std::getline(stream, line))
      {
        static constexpr std::string_view END_FILE = "</file>";
        const auto pos = line.find(END_FILE);
        if (pos == std::string::npos)
        {
          filedata += clean(line);
        }
        else
        {
          filedata += clean(line.substr(0, pos + END_FILE.size()));
          break;
        }
      }

      std::regex pattern(R"(<file path=\"([^\"]*)\" md5=\"(\w{32})\"><stats nbLinesInFile=\"(\d*)\" nbLinesOfCode=\"(\d*)\" nbLinesCovered=\"(\d*)\"\/><coverage>([^<]*)<)",
                         std::regex_constants::ECMAScript | std::regex_constants::icase);
      std::smatch regex_result;
      std::regex_search(filedata, regex_result, pattern);
      if (!regex_result.ready())
      {
        return;
      }

      const auto filename = regex_result.str(1);
      const auto md5Code = regex_result.str(2);

      std::string values;
      Base64::Decode(regex_result.str(6), values);
      assert(values.size() % 2 == 0);

      FileCoverageV2 profile(values.size() / sizeof(FileCoverageV2::LineArray::value_type));
      profile._nbLinesFile = std::stoi(regex_result.str(3));
      profile._nbLinesCode = std::stoi(regex_result.str(4));
      profile._nbLinesCovered = std::stoi(regex_result.str(5));

      profile.md5Code = regex_result.str(2);
      std::memcpy(profile._code.data(), values.data(), values.size());

      codeCoverage[filename] = profile;
    }
    catch (const std::regex_error& e)
    {
      std::cerr << "Bad regexp: " << e.what() << std::endl;
    }
    catch (const std::runtime_error& e)
    {
      std::cerr << "Bad data into " << filename << " with error: " << e.what() << std::endl;
    }
  }

  DictCoverage makeDictionary(const std::string& filename) const
  {
    std::ifstream outputFile(filename.c_str(), std::fstream::in);
    if (!outputFile.is_open())
    {
      const std::string msg = "Merge failure: Impossible to open file: " + filename;
      throw std::exception(msg.c_str());
    }

    return createDictionary(filename, outputFile);
  }

  DictCoverage createDictionary(const std::string& filename, std::istream& stream) const
  {
    DictCoverage dictOutput;

    std::string line;
    while (std::getline(stream, line))
    {
      std::string lineClean = clean(line);
      line.clear();

      if (lineClean.starts_with("<file"))
      {
        parseFile(filename, stream, dictOutput, lineClean);
      }
    }

    return dictOutput;
  }

  void merge(const DictCoverage& dictOutput, DictCoverage& dictMerge)
  {
    auto itOutput = dictOutput.cbegin();
    while (itOutput != dictOutput.cend())
    {
      auto itMerge = dictMerge.find(itOutput->first);
      if (itMerge != dictMerge.end())
      {
        if (!itMerge->second.merge(itOutput->second))
        {
          // Source is different from both version ?
          std::cerr << "Merge warning: impossible to merge " << itMerge->first << ": size between src/dst is not same." << std::endl;
        }
      }
      else
      {
        dictMerge[itOutput->first] = itOutput->second;
      }

      ++itOutput;
    }
  }

public:
  /// Constructor
  /// \param[in] opts: application option. Need MergedOutput and OutputFile valid and defined + ExportFormat MUST BE Native.
  MergeRunnerV2(const RuntimeOptions& opts) :
    MergeRunner(opts)
  {
    assert(_options.ExportFormat == RuntimeOptions::NativeV2); // Support only this !
  }

  /// Run merge
  void execute() override
  {
    std::filesystem::path outputPath(_options.OutputFile);
    std::filesystem::path mergedPath(_options.MergedOutput);

    // Check we have data
    if (!std::filesystem::exists(outputPath))
    {
      const std::string msg = "Merge failure: Impossible to find output file: " + _options.OutputFile;
      throw std::exception(msg.c_str());
    }

    // Nothing to merge = Copy and quit
    if (!std::filesystem::exists(mergedPath))
    {
      std::filesystem::copy(outputPath, mergedPath);
      return;
    }

    // ---- Make merge ---------------------------------------------------------------
    // Step 1: Parse output files and define a dictionary
    DictCoverage dictOutput = makeDictionary(_options.OutputFile);
    DictCoverage dictMerge = makeDictionary(_options.MergedOutput);

    // Step 2: Parse merge
    merge(dictOutput, dictMerge);

    // Step 3: Write dictionary (on empty file)
    std::ofstream ofs(_options.MergedOutput);

    FileCoverageV2::writeHeader(ofs);

    for (const auto& cover : dictMerge)
    {
      cover.second.write(cover.first, ofs);
    }

    FileCoverageV2::writeFooter(ofs);

    ofs.close();
  }
};