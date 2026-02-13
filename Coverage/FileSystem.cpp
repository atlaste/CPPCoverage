#include "FileSystem.h"
#include <filesystem>
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

    bool ReadLine(std::string& line) override
    {
      if (!ifs.good())
        return false;

      std::getline(ifs, line);
      return true;
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

    bool PathExists(const std::string& path)
    {
      return std::filesystem::exists(path);
    }

    bool IsDirectory(const std::string& path)
    {
      return std::filesystem::is_directory(path);
    }

    bool IsFile(const std::string& path)
    {
      return std::filesystem::is_regular_file(path);
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

    bool ReadLine(std::string& line) override
    {
      if (!IsOpen())
        return false;

      const auto& contain = memory.value();
      if (offset >= contain.size())
        return false;

      auto endLine = contain.find('\n', offset);
      if (endLine == std::string::npos)
      {
        line.assign(contain.c_str() + offset);
        offset = contain.size();
        return true;
      }

      line.assign(contain.c_str() + offset, endLine - offset);
      offset = endLine + 1;
      return true;
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

    bool PathExists(const std::string& path)
    {
      throw std::runtime_error("Not implemented yet");
    }

    bool IsDirectory(const std::string& path)
    {
      throw std::runtime_error("Not implemented yet");
    }

    bool IsFile(const std::string& path)
    {
      throw std::runtime_error("Not implemented yet");
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

  bool PathExists(const std::string& path)
  {
    return FileSystemImpl::getInstance().PathExists(path);
  }

  bool IsDirectory(const std::string& path)
  {
    return FileSystemImpl::getInstance().IsDirectory(path);
  }

  bool IsFile(const std::string& path)
  {
    return FileSystemImpl::getInstance().IsFile(path);
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