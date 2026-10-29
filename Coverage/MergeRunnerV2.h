#pragma once

#include "base64.h"
#include "MergeRunner.h"
#include "FileCoverageV2.h"
#include "FileSystem.h"

#include <map>
#include <iostream>
#include <regex>
#include <unordered_map>

class MergeRunnerV2 : public IMergeRunner
{
private:
  using CodeCoverage = std::unordered_map<std::string, FileCoverageV2>;
  using DictCoverage = std::unordered_map<std::string, CodeCoverage>;

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

  void parseFile(const std::string& filename, IFilePtr& file, CodeCoverage& codeCoverage, const std::string& fileline) const
  {
    try
    {
      // read whole data about file
      std::string filedata = fileline;
      std::string line;
      while (file->ReadLine(line))
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
      auto result = std::regex_search(filedata, regex_result, pattern);
      if (!result || !regex_result.ready())
      {
        return;
      }

      const auto filename = regex_result.str(1);
      const auto md5Code = regex_result.str(2);

      std::vector<uint8_t> values;
      Base64::Decode(regex_result.str(6), values);
      assert(values.size() % 2 == 0);

      codeCoverage.insert(std::make_pair(filename, FileCoverageV2(values, md5Code)));
    }
    catch (const std::invalid_argument& e)
    {
      std::cerr << "Invalid argument: " << e.what() << std::endl;
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
    auto file = FileSystem::OpenFile(filename);
    if (!file->IsOpen())
    {
      const std::string msg = "Merge failure: Impossible to open file: " + filename;
      throw std::exception(msg.c_str());
    }

    DictCoverage dictOutput;
    CodeCoverage codeCoverage;
    std::string currentDir;

    std::string line;
    while (file->ReadLine(line))
    {
      std::string lineClean = clean(line);
      line.clear();

      if (lineClean.starts_with("<directory"))
      {
        currentDir = getDir(lineClean);
      }
      else if (lineClean.starts_with("<file"))
      {
        parseFile(filename, file, codeCoverage, lineClean);
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

  void merge(const DictCoverage& dictOutput)
  {
    auto itDirOutput = dictOutput.cbegin();
    while (itDirOutput != dictOutput.cend())
    {
      auto itDirMerge = dictMerge.find(itDirOutput->first);
      if (itDirMerge != dictMerge.end())
      {
        auto itFileOutput = itDirOutput->second.cbegin();
        while (itFileOutput != itDirOutput->second.cend())
        {
          auto fileMerge = itDirMerge->second.find(itFileOutput->first);
          if (fileMerge != itDirMerge->second.cend())
          {
            if (!fileMerge->second.merge(itFileOutput->second))
            {
              // Source is different from both version ?
              std::cerr << "Merge warning: impossible to merge " << fileMerge->first << ": size between src/dst is not same." << std::endl;
            }
          }
          else
          {
            dictMerge[itDirOutput->first].insert(*itFileOutput);
          }
          ++itFileOutput;
        }
      }
      else
      {
        dictMerge[itDirOutput->first] = itDirOutput->second;
      }

      ++itDirOutput;
    }
  }

  DictCoverage dictMerge;
public:
  explicit MergeRunnerV2()
  {}

  void merge(const std::string& mergedFile, const std::string& outputFile) override
  {
    dictMerge = makeDictionary(mergedFile);
    DictCoverage dictOutput = makeDictionary(outputFile);

    merge(dictOutput);
  }

  void saveResultToStream(std::ostream& outputStream) override
  {
    FileCoverageV2::writeHeader(outputStream);

    for (const auto& directories : dictMerge)
    {
      const auto& dirName = directories.first;
      if (!dirName.empty())
      {
        FileCoverageV2::openDirectory(outputStream, dirName);
      }
      for (const auto& cover : directories.second)
      {
        cover.second.write(cover.first, outputStream);
      }
      if (!dirName.empty())
      {
        FileCoverageV2::closeDirectory(outputStream);
      }
    }

    FileCoverageV2::writeFooter(outputStream);
  }
};