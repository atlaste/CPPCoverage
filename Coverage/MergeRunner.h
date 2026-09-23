#pragma once

#include "RuntimeOptions.h"

#include <cassert>
#include <filesystem>
#include <memory>

struct CoverageResult
{
  virtual ~CoverageResult() = default;

  virtual size_t nbCoveredFile() const = 0;

  virtual size_t nbLineCovered(const std::filesystem::path& path) const = 0;

  virtual size_t nbFolders() const = 0;
};

class IMergeRunner
{
public:
  virtual void merge(const std::string& mergedFile, const std::string& outputFile) = 0;
  virtual void saveResultToStream(std::ostream& outputStream) = 0;
  
  /// Read dict on disk
  virtual std::unique_ptr<CoverageResult> read( const std::filesystem::path& ) const = 0;
};

class MergeRunner
{
public:
  /// Constructor
  /// \param[in] opts: application option. Need MergedOutput and OutputFile valid and defined + ExportFormat MUST BE Native.
  explicit MergeRunner(const RuntimeOptions& opts);

  // Avoid copy constructor
  MergeRunner(const MergeRunner&) = delete;

  void execute();
  
private:
  const RuntimeOptions& options;
  std::unique_ptr<IMergeRunner> runner;
};