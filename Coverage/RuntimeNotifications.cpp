#include <iostream>

#include "RuntimeNotifications.h"
#include "RuntimeOptions.h"
#include "FileSystem.h"
#include "Util.h"

struct RuntimeFileFilter : RuntimeCoverageFilter
{
  RuntimeFileFilter(const std::string& s) :
    file(s)
  {}

  std::string file;

  bool IgnoreFile(const std::string& found) const override
  {
    return Util::InvariantEquals(found, file);
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


std::string RuntimeNotifications::Trim(const std::string& t)
{
  std::string s = t;
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {
    return !Util::IsSpace(c); }
  ));
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int(c)) {
    return !Util::IsSpace(c);
  }).base(), s.end());
  return s;
}

std::string RuntimeNotifications::GetFQN(std::string s)
{
  if (s.empty()) return s;

  if (s.size() > 1 && s[1] == ':')
  {
    return s;
  }

  static constexpr char BACKSLASH = '\\';
  static constexpr std::string_view parentDir = "..\\";

  auto cp = RuntimeOptionsSingleton::Instance().SolutionPath;
  if (cp.empty()) { return s; }

  while (s.size() > parentDir.size() && s.starts_with(parentDir))
  {
    if (cp[cp.size() - 1] == BACKSLASH)
    {
      cp.pop_back();
    }
    auto idx = cp.find_last_of(BACKSLASH);
    if (idx == std::string::npos)
    {
      return s;
    }

    cp = cp.substr(0, idx);
    s = s.substr(parentDir.size(), s.size());
  }

  if (!cp.empty() && cp[cp.size() - 1] != BACKSLASH)
  {
    cp.push_back(BACKSLASH);
  }

  if (s[s.size() - 1] == BACKSLASH)
  {
    s.pop_back();
  }

  if (!s.empty() && (s[0] == BACKSLASH))
  {
    return cp + s.substr(1);
  }

  return cp + s;
}

void RuntimeNotifications::PrintInvalidNotification(const std::string& notification, const std::string_view information)
{
  if (RuntimeOptionsSingleton::Instance().isAtLeastLevel(VerboseLevel::Warning))
  {
    std::cout << "WARNING: Invalid path " << notification << ". " << information << std::endl;
  }
}

void RuntimeNotifications::Handle(const char* data, const size_t size)
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

    if (!FileSystem::PathExists(fullname))
    {
      PrintInvalidNotification(fullname, FOLDER_NOT_EXIST);
    }
    else if (!FileSystem::IsDirectory(fullname))
    {
      PrintInvalidNotification(fullname, PATH_IS_NOT_FOLDER);
    }
    else
    {
      if (RuntimeOptionsSingleton::Instance().isAtLeastLevel(VerboseLevel::Info))
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

    if (!FileSystem::PathExists(fullname))
    {
      PrintInvalidNotification(fullname, FILE_NOT_EXIST);
    }
    else if (!FileSystem::IsFile(fullname))
    {
      PrintInvalidNotification(fullname, PATH_IS_NOT_FILE);
    }
    else
    {
      if (RuntimeOptionsSingleton::Instance().isAtLeastLevel(VerboseLevel::Info))
      {
        std::cout << "Ignoring file: " << file << std::endl;
      }
      postProcessing.emplace_back(std::make_unique<RuntimeFileFilter>(fullname));
    }
  }
  else if (s == "ENABLE CODE ANALYSIS")
  {
    RuntimeOptionsSingleton::Instance().UseStaticCodeAnalysis = true;
  }
  else if (s == "DISABLE CODE ANALYSIS")
  {
    RuntimeOptionsSingleton::Instance().UseStaticCodeAnalysis = false;
  }
  else if (RuntimeOptionsSingleton::Instance().isAtLeastLevel(VerboseLevel::Error))
  {
    std::cout << "Unknown option passed to coverage: " << s << std::endl;
  }
}

bool RuntimeNotifications::IgnoreFile(const std::string& filename) const
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