#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include "CoverageRunner.h"
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

namespace TestCoverageRunner
{
  TEST_CLASS(TestSourceManager)
  {
  public:

    TEST_METHOD(checkIsExcluded)
    {
      SourceManager manager;

      RuntimeOptions options;

      options.excludeFilter.emplace_back("/moc_");
      manager.setupExcludeFilter(options);

      Assert::IsFalse(manager.isExcluded(std::filesystem::path("c:\\My/Path/Without/Issue.cpp")));

      Assert::IsTrue(manager.isExcluded(std::filesystem::path("c:\\My/Qt/moc_issue.cpp")));

      Assert::IsFalse(manager.isExcluded(std::filesystem::path("/My/template/predefined C++ types (compiler internal)")));

      options.excludeFilter.emplace_back("(compiler internal)");
      manager.setupExcludeFilter(options);

      Assert::IsTrue(manager.isExcluded(std::filesystem::path("c:\\My/template/predefined C++ types (compiler internal)")));
    }
  };
}