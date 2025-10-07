#include "MergeRunner.h"

#include "MergeRunnerV1.h"
#include "MergeRunnerV2.h"

std::unique_ptr<MergeRunner> MergeRunner::createMergeRunner(const RuntimeOptions& opts)
{
  switch (opts.ExportFormat)
  {
    case RuntimeOptions::Native:
      return std::make_unique<MergeRunnerV1>(opts);
    case RuntimeOptions::NativeV2:
      return std::make_unique<MergeRunnerV2>(opts);
  }
  throw std::runtime_error("This format does not support merge feature !");
}