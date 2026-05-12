#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include "MergeRunnerV2.h"
#include "RuntimeOptions.h"

#ifndef NOMINMAX
#	define NOMINMAX
#	include <Windows.h>
#endif

// #pragma warning(disable: 4091)
// #include <DbgHelp.h>
// #pragma warning(default: 4091)

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
// 
// namespace Microsoft
// {
//   namespace VisualStudio
//   {
//     namespace CppUnitTestFramework
//     {
//       template<> static std::wstring ToString<FileCoverageV2::LineArray>(const class FileCoverageV2::LineArray& t) { return L"FileCoverageV2::LineArray"; }
//     }
//   }
// }

namespace TestMerge
{
  TEST_CLASS(TestMergeRunner)
  {
  public:

    TEST_METHOD(MergeNativeV2TwoFile)
    {
      const std::filesystem::path workingDir = std::filesystem::current_path().parent_path().parent_path();

      const std::filesystem::path output1 = workingDir / "DataTest" / "Test1_UT.tmp.cov";
      const std::filesystem::path output2 = workingDir / "DataTest" / "Test2_UT.tmp.cov";
      const std::filesystem::path merged("./Outputs/Merged.cov");
      std::filesystem::remove_all(merged.parent_path());
      std::filesystem::create_directories(merged.parent_path());

      Assert::IsFalse( std::filesystem::exists(merged) );

      RuntimeOptions options;
      options.ExportFormat = RuntimeOptions::ExportFormatType::NativeV2;
      options.MergedOutput = std::filesystem::absolute(merged).string();
      options.OutputFile   = std::filesystem::absolute(output1).string();

      // Merge on empty file
      {
        auto merge = MergeRunner::createMergeRunner(options);
        try
        {
          merge->execute();
        }
        catch (...)
        {
          Assert::Fail();
        }
        const auto result = merge->read(options.MergedOutput);
        Assert::AreEqual(7ull, result->nbCoveredFile());
      }

      Assert::IsTrue(std::filesystem::exists(merged));
      options.OutputFile = output2.string();

      // Merge with something
      {
        auto merge = MergeRunner::createMergeRunner(options);
        try
        {
          merge->execute();
        }
        catch (...)
        {
          Assert::Fail();
        }
        const auto result = merge->read(options.MergedOutput);
        Assert::AreEqual(10ull,  result->nbCoveredFile());
        Assert::AreEqual(131ull, result->nbLineCovered(std::filesystem::path("lib/TestM/B.h")));
      }
    }
  };
}