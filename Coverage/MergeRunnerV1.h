#pragma once

#include "MergeRunner.h"
#include "FileSystem.h"

#include <filesystem>
#include <map>
#include <iostream>

class MergeRunnerV1 : public IMergeRunner
{
private:
  struct Profile
  {
    std::string res;
    std::string prof;
  };

  using DictCoverage = std::map<std::string, Profile>;

  struct Result : public CoverageResult
  {
    DictCoverage _dict;

    size_t nbCoveredFile() const override
    {
      return _dict.size();
    }

    size_t nbLineCovered(const std::filesystem::path& path) const override
    {
      const auto search = _dict.find(path.string());
      if (search != _dict.cend())
      {
        return count(search->second.res.begin(), search->second.res.end(), 'c');
      }
      return 0ull;
    }

    size_t nbFolders() const override
    {
      return _dict.size();
    }
  };

  DictCoverage makeDictionary(const std::string& filename) const
  {
    DictCoverage dictOutput;

    auto file = FileSystem::OpenFile(filename);
    if (!file->IsOpen())
    {
      const std::string msg = "Merge failure: Impossible to open file: " + filename;
      throw std::exception(msg.c_str());
    }

    std::string buffer;
    while (file->ReadLine(buffer))
    {
      //Search FILE:
      if (buffer.find_first_of("FILE:") != std::string::npos)
      {
        std::string filename = buffer.substr(6);

        Profile profile;

        // Read next line (we suppose file is not corrupt)
        file->ReadLine(buffer);
        assert(buffer.find_first_of("RES:") != std::string::npos);
        profile.res = buffer.substr(5);

        file->ReadLine(buffer);
        assert(buffer.find_first_of("PROF:") != std::string::npos);
        profile.prof = buffer.substr(6);

        dictOutput[filename] = profile;
      }
    }

    return dictOutput;
  }

  void merge(const DictCoverage& dictOutput)
  {
    auto itOutput = dictOutput.cbegin();
    while (itOutput != dictOutput.cend())
    {
      auto itMerge = dictMerge.find(itOutput->first);
      if (itMerge != dictMerge.end())
      {
        if (itMerge->second.res.size() == itOutput->second.res.size())
        {
          auto src = itOutput->second.res.cbegin();
          for (auto& dst : itMerge->second.res)
          {
            // Rules: c > p > u > _
            // In ASCII or UTF8  c < p < u < _ !!!
            if (dst > *src)
              dst = *src; // So easy !
            ++src;
          }
        }
        else
        {
          // Source is different from both version ?
          std::cerr << "Merge warning: impossible to merge " << itOutput->first << ": size between src/dst is not same." << std::endl;
        }
      }
      else
      {
        dictMerge[itOutput->first] = itOutput->second;
      }

      ++itOutput;
    }
  }

  DictCoverage dictMerge;
public:
  explicit MergeRunnerV1()
  {}

  void merge(const std::string& mergedFile, const std::string& outputFile) override
  {
    dictMerge = makeDictionary(mergedFile);
    DictCoverage dictOutput = makeDictionary(outputFile);

    merge(dictOutput);
  }

  std::unique_ptr<CoverageResult> read(const std::filesystem::path& path) const override
  {
    auto r = std::make_unique<Result>();
    r->_dict = makeDictionary(path.string());
    return r;
  }

  void saveResultToStream(std::ostream& outputStream) override
  {
    for (const auto& cover : dictMerge)
    {
      outputStream << "FILE: " << cover.first << std::endl;
      outputStream << "RES: " << cover.second.res << std::endl;
      outputStream << "PROF: " << cover.second.prof << std::endl;
    }
  }
};