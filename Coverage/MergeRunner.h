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
};

class MergeRunner
{
protected:
  const RuntimeOptions _options;    ///< Copy local of option.

  // Avoid copy constructor
  MergeRunner(const MergeRunner&) = delete;

  /// Constructor
  /// \param[in] opts: application option. Need MergedOutput and OutputFile valid and defined + ExportFormat MUST BE Native.
  MergeRunner(const RuntimeOptions& opts) :
    _options(opts)
  {
    assert(!_options.MergedOutput.empty());
    assert(!_options.OutputFile.empty());
  }

public:
  virtual ~MergeRunner() = default;

  /// Run merge
  virtual void execute() = 0;

  /// Read dict on disk
  virtual std::unique_ptr<CoverageResult> read( const std::filesystem::path& ) const = 0;

  // Allow to build good merge runner
  static std::unique_ptr<MergeRunner> createMergeRunner(const RuntimeOptions& opts);
};