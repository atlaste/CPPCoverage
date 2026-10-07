#include "MergeRunner.h"

#include "MergeRunnerV1.h"
#include "MergeRunnerV2.h"

MergeRunner::MergeRunner(const RuntimeOptions& opts) :
  options(opts)
{
  assert(!options.MergedOutput.empty());
  assert(!options.OutputFile.empty());

  switch (opts.ExportFormat)
  {
    case RuntimeOptions::ExportFormatType::Native:
      runner = std::make_unique<MergeRunnerV1>();
      break;
    case RuntimeOptions::ExportFormatType::NativeV2:
      runner = std::make_unique<MergeRunnerV2>();
      break;
    default:
      throw std::runtime_error("This format does not support merge feature !");
  }
}

void MergeRunner::execute()
{
  const std::filesystem::path outputPath(options.OutputFile);
  const std::filesystem::path mergedPath(options.MergedOutput);

  // Check we have data
  if (!std::filesystem::exists(outputPath))
  {
    const std::string msg = "Merge failure: Impossible to find output file: " + options.OutputFile;
    throw std::exception(msg.c_str());
  }

  // Nothing to merge = Copy and quit
  if (!std::filesystem::exists(mergedPath))
  {
    std::filesystem::copy(outputPath, mergedPath);
    return;
  }

  // ---- Make merge ---------------------------------------------------------------
  // Step 1: Merge two output
  runner->merge(options.MergedOutput, options.OutputFile);

  // Step 2: Write result to file
  std::ofstream mergeFile(options.MergedOutput.c_str());
  runner->saveResultToStream(mergeFile);
}