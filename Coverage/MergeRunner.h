#pragma once

#include "RuntimeOptions.h"

#include <cassert>
#include <memory>

class MergeRunner
{
protected:
    RuntimeOptions _options;    ///< Copy local of option.

    // Avoid copy constructor
    MergeRunner(const MergeRunner& ) = delete;

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

	// Allow to build good merge runner
    static std::unique_ptr<MergeRunner> createMergeRunner(const RuntimeOptions& opts);
};