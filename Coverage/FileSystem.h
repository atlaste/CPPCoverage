#pragma once

#include <memory>
#include <string>

class IFile
{
public:
  virtual bool IsOpen() const = 0;
  virtual std::streamsize Read(std::string& buffer) = 0;
};

using IFilePtr = std::shared_ptr<IFile>;

namespace FileSystem
{
  IFilePtr OpenFile(const std::string& filename);
  void CreateTestFile(const std::string& filename, const std::string& contain);
  void DeleteTestFiles();
}