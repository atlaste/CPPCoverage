#pragma once

#include <memory>
#include <string>

class IFile
{
public:
  virtual bool IsOpen() const = 0;
  virtual std::streamsize Read(std::string& buffer) = 0;
  virtual bool ReadLine(std::string& line) = 0;
};

using IFilePtr = std::shared_ptr<IFile>;

namespace FileSystem
{
  IFilePtr OpenFile(const std::string& filename);
  bool PathExists(const std::string& path);
  bool IsDirectory(const std::string& path);
  bool IsFile(const std::string& path);

  // functions only for unittest
  void CreateTestFile(const std::string& filename, const std::string& contain);
  void DeleteTestFiles();
}