#pragma once

#include "base64.h"
#include "FileCallbackInfo.h"
#include "MergeRunner.h"

#include <filesystem>
#include <unordered_map>
#include <regex>

namespace TestFormat
{
  class TestNativeV2;
}



class MergeRunnerV2 : public MergeRunner
{
public:
  friend class TestFormat::TestNativeV2;

  using CodeCoverage = std::unordered_map<std::string, FileCoverageV2>;
  using DictCoverage = std::unordered_map<std::string, CodeCoverage>;

  struct Result : public CoverageResult
  {
    DictCoverage _dict;

    size_t nbLineCovered(const std::filesystem::path& path) const override
    {
      for(const auto& item : _dict)
      {
        const auto search = item.second.find(path.string());
        if (search != item.second.cend())
        {
          return search->second._nbLinesCovered;
        }
      }
      return 0ull;
    }
    
    size_t nbCoveredFile() const override
    {
      size_t nb = 0;
      for(const auto& item : _dict)
      {
        nb += item.second.size();
      }
      return nb;
    }
  };

private:
  std::string clean(const std::string& content) const
  {
    // Need to remove return line (not supported by regex)
    const std::regex PatternClean("\r|\n|\t", std::regex_constants::ECMAScript);
    return  std::regex_replace(content, PatternClean, "");
  }

  std::string getDir(const std::string& line) const
  {
    std::regex pattern(R"(<directory path=\"([^\"]*)\")", std::regex_constants::ECMAScript | std::regex_constants::icase);
    std::smatch regex_result;
    std::regex_search(line, regex_result, pattern);
    if (!regex_result.ready())
    {
      return "";
    }

    return regex_result.str(1);
  }

  void parseFile(const std::string& filename, std::istream& stream, CodeCoverage& codeCoverage, const std::string& fileline) const
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
    CodeCoverage codeCoverage;
    std::string currentDir;

    std::string line;
    while (std::getline(stream, line))
    {
      std::string lineClean = clean(line);
      line.clear();

      if (lineClean.starts_with("<directory"))
      {
        currentDir = getDir(lineClean);
      }
      else if (lineClean.starts_with("<file"))
      {
        parseFile(filename, stream, codeCoverage, lineClean);
      }
      else if (lineClean.starts_with("</directory>"))
      {
        if (!codeCoverage.empty())
        {
          dictOutput[currentDir] = codeCoverage;
        }

        codeCoverage.clear();
        currentDir.clear();
      }
    }

    if (!codeCoverage.empty())
    {
      dictOutput[""] = codeCoverage;
    }

    return dictOutput;
  }

  void merge(const DictCoverage& dictOutput, DictCoverage& dictMerge)
  {
    // Parsing Output
    auto itDirOutput = dictOutput.cbegin();
    while (itDirOutput != dictOutput.cend())
    {
      // Search this file into the merge dict
      auto itDirMerge = dictMerge.find(itDirOutput->first);
      if (itDirMerge != dictMerge.cend())
      {
        auto itFileOutput = itDirOutput->second.cbegin();
        while (itFileOutput != itDirOutput->second.cend())
        {
          auto fileMerge = itDirMerge->second.find(itFileOutput->first);
          if ( fileMerge != itDirMerge->second.cend() )
          {
            if (!fileMerge->second.merge(itFileOutput->second))
            {
              // Source is different from both version ?
              std::cerr << "Merge warning: impossible to merge " << fileMerge->first << ": size between src/dst is not same." << std::endl;
            }
          }
          else // if is existing into itDirMerge only -> copy
          {
            itDirMerge->second[itFileOutput->first] = itFileOutput->second;
          }
          ++itFileOutput;
        }
      }
      else // if is existing into itDirOutput only -> copy
      {
        dictMerge[itDirOutput->first] = itDirOutput->second;
      }

      ++itDirOutput;
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

  std::unique_ptr<CoverageResult> read( const std::filesystem::path& path ) const override
  {
    auto result = std::make_unique<Result>();
    result->_dict = makeDictionary( path.string() );
    return result;
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
    DictCoverage dictMerge  = makeDictionary(_options.MergedOutput);

    // Step 2: Parse merge
    merge(dictOutput, dictMerge);

    // Step 3: Write dictionary (on empty file)
    std::ofstream ofs(_options.MergedOutput);

    FileCoverageV2::writeHeader(ofs);

    for (const auto& directories : dictMerge)
    {
      const auto& dirName = directories.first;
      if (!dirName.empty())
      {
        FileCoverageV2::openDirectory(ofs, dirName);
      }
      for (const auto& cover : directories.second)
      {
        cover.second.write(cover.first, ofs);
      }
      if (!dirName.empty())
      {
        FileCoverageV2::closeDirectory(ofs);
      }
    }

    FileCoverageV2::writeFooter(ofs);

    ofs.close();
  }
};