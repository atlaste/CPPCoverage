#pragma once

#include "FileLineInfo.h"
#include "FileCoverageV2.h"
#include "BreakpointData.h"
#include "RuntimeOptions.h"
#include "CallbackInfo.h"
#include "md5.h"
#include "ProfileNode.h"
#include "RuntimeNotifications.h"

#include <iostream>
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <Windows.h>
#include <ctime>

struct FileCallbackInfo
{
  FileCallbackInfo(const std::string& filename) :
    filename(filename)
  {
    if (RuntimeOptions::Instance().CodePaths.empty())
    {
      auto idx = filename.find("x64");
      if (idx == std::string::npos)
      {
        idx = filename.find("Debug");
      }
      if (idx == std::string::npos)
      {
        idx = filename.find("Release");
      }
      if (idx == std::string::npos)
      {
        idx = filename.find('\\');
      }
      if (idx == std::string::npos)
      {
        throw "Cannot locate source file base for this executable";
      }
      sourcePath = filename.substr(0, idx);
    }
  }

  std::string filename;
  std::string sourcePath;

  using MergedProfileInfoMap = std::unordered_map<std::string, std::unique_ptr<std::vector<ProfileInfo>>>;
  using FileInfoMap = std::unordered_map<std::string, std::unique_ptr<FileInfo>>;

  FileInfoMap lineData;

  void Filter(RuntimeNotifications& notifications)
  {
    FileInfoMap newLineData;
    for (auto& it : lineData)
    {
      if (!notifications.IgnoreFile(it.first))
      {
        std::unique_ptr<FileInfo> tmp(nullptr);
        std::swap(tmp, it.second);
        newLineData[it.first] = std::move(tmp);
      }
      else if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Trace))
      {
        std::cout << "Removing file " << it.first << std::endl;
      }
    }
    std::swap(lineData, newLineData);
  }

  bool PathMatches(const char* first, const std::string& second)
  {
    const char* ptr = first;
    const char* gt = second.data();
    const char* gte = gt + second.size();

    for (; *ptr && gt != gte; ++ptr, ++gt)
    {
      char lhs = tolower(*gt);
      char rhs = tolower(*ptr);
      if (lhs != rhs) { return false; }
    }

    return true;
  }

  bool PathMatches(const char* filename)
  {
    if (!sourcePath.empty())
    {
      return PathMatches(filename, sourcePath);
    }

    const auto& codePaths = RuntimeOptions::Instance().CodePaths;
    for (const auto& codePath : codePaths)
    {
      if (PathMatches(filename, codePath))
      {
        return true;
      }
    }

    return false;
  }

  FileLineInfo* LineInfo(const std::string& filename, DWORD64 lineNumber)
  {
    for (const auto& [name, fileInfo] : lineData)
    {
      if (Util::InvariantEquals(name, filename))
      {
        return fileInfo->LineInfo(size_t(lineNumber));
      }
    }

    auto newLineData = new FileInfo(filename);
    lineData[filename] = std::unique_ptr<FileInfo>(newLineData);
    return newLineData->LineInfo(size_t(lineNumber));
  }

  void WriteReport(RuntimeOptions::ExportFormatType exportFormat, const MergedProfileInfoMap& mergedProfileInfo, std::ostream& stream)
  {
    switch (exportFormat)
    {
      case RuntimeOptions::Clover:    WriteClover(stream); break;
      case RuntimeOptions::Cobertura: WriteCobertura(stream); break;
      case RuntimeOptions::NativeV2:  WriteNativeV2(stream); break;
      default: WriteNative(stream, mergedProfileInfo); break;
    }
  }

private:

  void WriteClover(std::ostream& stream)
  {
    size_t totalFiles = 0;
    size_t coveredFiles = 0;
    size_t totalLines = 0;
    size_t coveredLines = 0;
    for (auto& it : lineData)
    {
      auto ptr = it.second.get();
      for (size_t i = 0; i < ptr->numberLines; ++i)
      {
        if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
        {
          ++totalFiles;
          if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
          {
            ++coveredFiles;
          }

          totalLines += ptr->lines[i].DebugCount;
          coveredLines += ptr->lines[i].HitCount;
        }
      }
    }

    time_t t = time(0);   // get time now
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    stream << "<clover generated=\"" << t << "\"  clover=\"3.1.5\">" << std::endl;
    stream << "<project timestamp=\"" << t << "\">" << std::endl;
    stream << "<metrics classes=\"0\" files=\"" << totalFiles << "\" packages=\"1\"  loc=\"" << totalLines << "\" ncloc = \"" << coveredLines << "\" ";
    // stream << "coveredstatements=\"300\" statements=\"500\" coveredmethods=\"50\" methods=\"80\" ";
    // stream << "coveredconditionals=\"100\" conditionals=\"120\" coveredelements=\"900\" elements=\"1000\" ";
    stream << "complexity=\"0\" />" << std::endl;
    stream << "<package name=\"" << RuntimeOptions::Instance().PackageName << "\">" << std::endl;
    for (auto& it : lineData)
    {
      auto ptr = it.second.get();

      stream << "<file name=\"" << it.first << "\">" << std::endl;

      for (size_t i = 0; i < ptr->numberLines; ++i)
      {
        if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
        {
          if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
          {
            stream << "<line num=\"" << i << "\" count=\"1\" type=\"stmt\"/>" << std::endl;
          }
          else
          {
            stream << "<line num=\"" << i << "\" count=\"0\" type=\"stmt\"/>" << std::endl;
          }
        }
      }

      stream << "</file>" << std::endl;
    }

    stream << "</package>" << std::endl;
    stream << "</project>" << std::endl;
    stream << "</clover>" << std::endl;
  }

  void WriteCobertura(std::ostream& stream)
  {
    std::unordered_set<char> sourceList;

    double total = 0;
    double covered = 0;
    for (auto& it : lineData)
    {
      sourceList.insert(it.first.front());
      auto ptr = it.second.get();
      for (size_t i = 0; i < ptr->numberLines; ++i)
      {
        if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
        {
          ++total;
          if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
          {
            ++covered;
          }
        }
      }
    }

    double lineRate = covered / total;

    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    stream << "<coverage line-rate=\"" << lineRate << "\"" << " " << "version=\"\">" << std::endl;
    stream << "\t" << "<packages>" << std::endl;

    stream << "\t\t" << "<package name=\"" << RuntimeOptions::Instance().PackageName << "\" line-rate=\"" << lineRate << "\">" << std::endl;
    stream << "\t\t\t" << "<classes>" << std::endl;
    for (auto& it : lineData)
    {
      auto ptr = it.second.get();

      std::string name = it.first;
      auto idx = name.find_last_of('\\');
      if (idx != std::string::npos)
      {
        name = name.substr(idx + 1);
      }

      double total = 0;
      double covered = 0;
      for (size_t i = 0; i < ptr->numberLines; ++i)
      {
        if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
        {
          ++total;
          if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
          {
            ++covered;
          }
        }
      }

      double lineRate = covered / total;

      stream << "\t\t\t\t" << "<class name=\"" << name << "\" filename=\"" << it.first.substr(2) << "\" line-rate=\"" << lineRate << "\">" << std::endl;
      stream << "\t\t\t\t\t" << "<lines>" << std::endl;

      for (size_t i = 0; i < ptr->numberLines; ++i)
      {
        if (ptr->relevant[i] && ptr->lines[i].DebugCount != 0)
        {
          if (ptr->lines[i].HitCount == ptr->lines[i].DebugCount)
          {
            stream << "\t\t\t\t\t\t" << "<line number=\"" << i + 1 << "\" hits=\"1\"/>" << std::endl;
          }
          else
          {
            stream << "\t\t\t\t\t\t" << "<line number=\"" << i + 1 << "\" hits=\"0\"/>" << std::endl;
          }
        }
      }

      stream << "\t\t\t\t\t" << "</lines>" << std::endl;
      stream << "\t\t\t\t" << "</class>" << std::endl;
    }

    stream << "\t\t\t" << "</classes>" << std::endl;
    stream << "\t\t" << "</package>" << std::endl;
    stream << "\t" << "</packages>" << std::endl;
    stream << "\t<sources>" << std::endl;
    for (const auto& source : sourceList)
    {
      stream << "\t\t<source>" << source << ":</source>" << std::endl;
    }
    stream << "\t</sources>" << std::endl;
    stream << "</coverage>" << std::endl;
  }

  void WriteNative(std::ostream& stream, const MergedProfileInfoMap& mergedProfileInfo)
  {
    for (auto& it : lineData)
    {
      stream << "FILE: " << it.first << std::endl;
      auto ptr = it.second.get();

      std::string result;
      result.reserve(ptr->numberLines + 1);

      for (size_t i = 0; i < ptr->numberLines; ++i)
      {
        char state = 'i';
        if (ptr->relevant[i])
        {
          auto& line = ptr->lines[i];
          if (line.DebugCount == 0)
          {
            state = '_';
          }
          else if (line.DebugCount == line.HitCount)
          {
            state = 'c';
          }
          else if (line.HitCount == 0)
          {
            state = 'u';
          }
          else
          {
            state = 'p';
          }
        }
        result.push_back(state);
      }

      stream << "RES: " << result << std::endl;

      auto profInfo = mergedProfileInfo.find(it.first);

      if (profInfo == mergedProfileInfo.end())
      {
        stream << "PROF: " << std::endl;
      }
      else
      {
        stream << "PROF: ";
        for (auto& it : *(profInfo->second.get()))
        {
          stream << int(it.Deep) << ',' << int(it.Shallow) << ',';
        }
        stream << std::endl;
      }
    }
  }

  void WriteNativeV2(std::ostream& stream)
  {
    const auto encodeCoverage = [](const FileInfo& info) -> FileCoverageV2
    {
      assert(info.relevant.size() == info.lines.size());
      auto itRelevant = info.relevant.cbegin();
      FileCoverageV2 coverage(info.lines.size());

      auto itCoverage = coverage._code.begin();

      for (const auto& line : info.lines)
      {
        *itCoverage = coverage.encodeLine(*itRelevant, line);
        ++itCoverage;
        ++itRelevant;
      }
      return coverage;
    };

    MD5 md5;

    FileCoverageV2::writeHeader(stream);

    std::vector<std::string> filepaths;
    filepaths.reserve(lineData.size());

    for (const auto& item : lineData)
    {
      filepaths.push_back(item.first);
    }

    for (const auto& dirPath : RuntimeOptions::Instance().CodePaths)
    {
      bool dirPartAdded = false;

      for (auto& it : lineData)
      {
        auto filepath = it.first;

        // Check if it's a subpath
        if (!dirPath.empty())
        {
          auto relativeFile = std::filesystem::relative(filepath, dirPath);
          if (relativeFile.empty() || relativeFile.native().front() == '.')
          {
            continue; // Skip this item
          }
          filepath = relativeFile.generic_string();
        }

        if (!dirPartAdded && !dirPath.empty())
        {
          dirPartAdded = true;
          FileCoverageV2::openDirectory(stream, dirPath);
        }

        auto coverage = encodeCoverage(*it.second.get());
        coverage.md5Code = md5.encode(it.first);
        coverage.write(filepath, stream);

        filepaths.erase(std::remove(filepaths.begin(), filepaths.end(), it.first), filepaths.end());
      }

      if (dirPartAdded && !dirPath.empty())
      {
        FileCoverageV2::closeDirectory(stream);
      }
    }

    if (!filepaths.empty() && RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Warning))
    {
      std::cerr << "List of refuse coverage files (because not relative to any code path):" << std::endl;

      for (const auto& path : filepaths)
      {
        std::cerr << std::format("- {0}", path) << std::endl;
      }

      std::cerr << std::endl << "List of code paths:" << std::endl;

      for (const auto& dirPath : RuntimeOptions::Instance().CodePaths)
      {
        std::cerr << std::format("- {0}", dirPath) << std::endl;
      }
    }

    FileCoverageV2::writeFooter(stream);
  }
};
