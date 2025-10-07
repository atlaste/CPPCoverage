#pragma once

#include <string>
#include <iostream>
#include <algorithm>
#include <utility>
#include <functional>
#include <cctype>
#include <memory>
#include <vector>

#include <Windows.h>

#include "RuntimeOptions.h"

struct RuntimeCoverageFilter
{
  virtual bool IgnoreFile(const std::string& file) const = 0;
};

struct RuntimeFileFilter : RuntimeCoverageFilter
{
  RuntimeFileFilter(const std::string& s) :
    file(s)
  {}

  std::string file;

  bool IgnoreFile(const std::string& found) const override
  {
    if (found.size() == file.size())
    {
      for (size_t i = 0; i < found.size(); ++i)
      {
        if (std::tolower(found[i]) != std::tolower(file[i]))
        {
          return false;
        }
      }
      return true;
    }
    return false;
  }
};

struct RuntimeFolderFilter : RuntimeCoverageFilter
{
  RuntimeFolderFilter(const std::string& s) :
    folder(s)
  {}

  std::string folder;

  bool IgnoreFile(const std::string& found) const override
  {
    if (found.size() > folder.size())
    {
      size_t i;
      for (i = 0; i < folder.size(); ++i)
      {
        if (std::tolower(found[i]) != std::tolower(folder[i]))
        {
          return false;
        }
      }

      if (found[i] != '\\') { return false; }

      return true;
    }
    return false;
  }
};


struct RuntimeNotifications
{
  std::vector<std::unique_ptr<RuntimeCoverageFilter>> postProcessing;

  std::string Trim(const std::string& t)
  {
    std::string s = t;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {
      return !std::isspace(c); }
    ));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int(c)) {
      return !std::isspace(c);
    }).base(), s.end());
    return s;
  }

  std::string GetFQN(std::string s)
  {
    if (s.empty()) return s;

    if (s.size() > 1 && s[1] == ':')
    {
      return s;
    }

    auto cp = RuntimeOptions::Instance().SolutionPath;
    if (cp.empty()) { return s; }

    while (s.size() > 3 && s.substr(s.size() - 3, 3) == "..\\")
    {
      if (cp[cp.size() - 1] == '\\')
      {
        cp = cp.substr(0, cp.size() - 1);
      }
      auto idx = cp.find_last_of('\\');
      if (idx == std::string::npos)
      {
        return s;
      }
      else
      {
        cp = cp.substr(0, idx);
        s = s.substr(0, s.size() - 3);
      }
    }

    if (!cp.empty() && cp[cp.size() - 1] != '\\')
    {
      cp += "\\";
    }

    if (!s.empty() && (s[0] == '\\'))
    {
      return cp + s.substr(1);
    }

    return cp + s;
  }

  void PrintInvalidNotification(const std::string& notification, const std::string_view information)
  {
    if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Warning))
    {
      std::cout << "WARNING: Invalid path " << notification << ". " << information << std::endl;
    }
  }

  void Handle(const char* data, const size_t size)
  {
    static constexpr std::string_view IGNORE_FOLDER = "IGNORE FOLDER:";
    static constexpr std::string_view IGNORE_FILE = "IGNORE FILE:";

    static constexpr std::string_view FOLDER_NOT_EXIST = "The folder is not exist!";
    static constexpr std::string_view PATH_IS_NOT_FOLDER = "The path is not the folder!";
    static constexpr std::string_view FILE_NOT_EXIST = "The file is not exist!";
    static constexpr std::string_view PATH_IS_NOT_FILE = "The path is not a regular file!";

    std::string s(data, data + size);
    if (s.substr(0, IGNORE_FOLDER.size()) == IGNORE_FOLDER)
    {
      auto folder = Trim(s.substr(IGNORE_FOLDER.size()));
      auto fullname = GetFQN(folder);

      if (!std::filesystem::exists(fullname))
      {
        PrintInvalidNotification(fullname, FOLDER_NOT_EXIST);
      }
      else if (!std::filesystem::is_directory(fullname))
      {
        PrintInvalidNotification(fullname, PATH_IS_NOT_FOLDER);
      }
      else
      {
        if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Info))
        {
          std::cout << "Ignoring folder: " << fullname << std::endl;
        }
        postProcessing.emplace_back(std::make_unique<RuntimeFolderFilter>(fullname));
      }
    }
    else if (s.substr(0, IGNORE_FILE.size()) == IGNORE_FILE)
    {
      auto file = Trim(s.substr(IGNORE_FILE.size()));
      auto fullname = GetFQN(file);

      if (!std::filesystem::exists(fullname))
      {
        PrintInvalidNotification(fullname, FILE_NOT_EXIST);
      }
      else if (!std::filesystem::is_regular_file(fullname))
      {
        PrintInvalidNotification(fullname, PATH_IS_NOT_FILE);
      }
      else
      {
        if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Info))
        {
          std::cout << "Ignoring file: " << file << std::endl;
        }
        postProcessing.emplace_back(std::make_unique<RuntimeFileFilter>(fullname));
      }
    }
    else if (s == "ENABLE CODE ANALYSIS")
    {
      RuntimeOptions::Instance().UseStaticCodeAnalysis = true;
    }
    else if (s == "DISABLE CODE ANALYSIS")
    {
      RuntimeOptions::Instance().UseStaticCodeAnalysis = false;
    }
    else if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Error))
    {
      std::cout << "Unknown option passed to coverage: " << s << std::endl;
    }
  }

  bool IgnoreFile(const std::string& filename) const
  {
    for (const auto& it : postProcessing)
    {
      if (it->IgnoreFile(filename))
      {
        return true;
      }
    }
    return false;
  }
};