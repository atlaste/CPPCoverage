#include "FileSystem.h"
#include <fstream>
#include <optional>
#include <unordered_map>

namespace FileSystem
{
  class RealFile : public IFile
  {
  public:
    explicit RealFile(const std::string& filename) : ifs(filename) {}

    bool IsOpen() const override
    {
      return ifs.is_open();
    }

    std::streamsize Read(std::string& buffer) override
    {
      if (!ifs.good())
        return 0;

      ifs.read(buffer.data(), buffer.size());
      return ifs.gcount();
    }
  private:
    std::ifstream ifs;
  };

  class RealFileSystemImpl
  {
  public:
    static RealFileSystemImpl& getInstance()
    {
      static RealFileSystemImpl instance;
      return instance;
    }
    RealFileSystemImpl(RealFileSystemImpl const&) = delete;
    void operator=(RealFileSystemImpl const&) = delete;

    void CreateTestFile(const std::string& filename, const std::string& contain)
    {
      throw std::runtime_error("Not implemented yet");
    }

    void DeleteTestFiles()
    {
      throw std::runtime_error("Not implemented yet");
    }

    IFilePtr OpenFile(const std::string& filename)
    {
      return std::make_shared<RealFile>(filename);
    }
  private:
    explicit RealFileSystemImpl() {}
  };

  // ================================= UNIT TEST =================================

  class MemoryFile : public IFile
  {
  public:
    explicit MemoryFile(const std::optional<std::string>& contain)
      : memory(contain)
    {}

    bool IsOpen() const override
    {
      return memory.has_value();
    }

    std::streamsize Read(std::string& buffer) override
    {
      if (!IsOpen())
        return 0;

      const auto& contain = memory.value();
      const auto remain = contain.size() - offset;
      if (remain == 0)
        return 0;

      const auto toRead = std::min(remain, buffer.size());
      memcpy(buffer.data(), contain.c_str() + offset, toRead);
      offset += toRead;
      return toRead;
    }
  private:
    std::optional<std::string> memory;
    size_t offset = 0;
  };

  class MemoryFileSystemImpl
  {
  public:
    static MemoryFileSystemImpl& getInstance()
    {
      static MemoryFileSystemImpl instance;
      return instance;
    }
    MemoryFileSystemImpl(MemoryFileSystemImpl const&) = delete;
    void operator=(MemoryFileSystemImpl const&) = delete;

    void CreateTestFile(const std::string& filename, const std::string& contain)
    {
      if (files.contains(filename))
      {
        files.erase(filename);
      }
      files.insert({ filename, contain });
    }

    void DeleteTestFiles()
    {
      files.clear();
    }

    IFilePtr OpenFile(const std::string& filename)
    {
      if (!files.contains(filename))
      {
        return std::make_shared<MemoryFile>(std::nullopt);
      }

      return std::make_shared<MemoryFile>(files[filename]);
    }
  private:
    explicit MemoryFileSystemImpl() {}
    std::unordered_map<std::string, std::string> files;
  };

#ifndef UNITTEST
  using FileSystemImpl = RealFileSystemImpl;
#else
  using FileSystemImpl = MemoryFileSystemImpl;
#endif

  IFilePtr OpenFile(const std::string& filename)
  {
    return FileSystemImpl::getInstance().OpenFile(filename);
  }

  void CreateTestFile(const std::string& filename, const std::string& contain)
  {
    FileSystemImpl::getInstance().CreateTestFile(filename, contain);
  }

  void DeleteTestFiles()
  {
    FileSystemImpl::getInstance().DeleteTestFiles();
  }
}