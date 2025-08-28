#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include "FileCoverageV2.h"
#include "MergeRunnerV2.h"
#include "RuntimeOptions.h"

#ifndef NOMINMAX
#	define NOMINMAX
#	include <Windows.h>
#endif

#pragma warning(disable: 4091)
#include <DbgHelp.h>
#pragma warning(default: 4091)

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			template<> static std::wstring ToString<FileCoverageV2::LineArray>(const class FileCoverageV2::LineArray& t) { return L"FileCoverageV2::LineArray"; }
		}
	}
}

namespace TestFormat
{
	TEST_CLASS(TestNativeV2)
	{
	public:

		TEST_METHOD(Merge)
		{
			const auto max = FileCoverageV2::maskCount;
			const auto c   = FileCoverageV2::maskIsCode;
			const auto p   = FileCoverageV2::maskIsPartial;
			FileCoverageV2 coverage(9);
			coverage._code = { 0, 0, c, c, c | p | 1, c | 10000, c | 3000, 0, c | p | 1 };
			coverage.updateStats();

			{
				FileCoverageV2 merge(9);
				merge._code = { 0, 0, c, c | 5, c | p | 1, c | 10000, c | 1000, 0, c | 5 };
				merge.merge(coverage);
				FileCoverageV2::LineArray reference = { 0, 0, c, c | 5, c | p | 2, c | max, c | 4000, 0, c | 6 };

				Assert::AreEqual(reference.size(), merge._code.size());
				Assert::AreEqual(reference, merge._code);
				Assert::AreEqual(coverage._nbLinesFile, merge._nbLinesFile);
				Assert::AreEqual(coverage._nbLinesCode, merge._nbLinesCode);
				Assert::AreEqual(5ull, merge._nbLinesCovered);

				// Write data
				const std::string filename("demo");
				std::stringstream ss;
				merge.md5Code = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
				merge.write(filename, ss);

				// Create reader
				auto options = RuntimeOptions::Instance();
				options.MergedOutput = filename; /// Dummy valid value
				options.OutputFile   = filename; /// Dummy valid value
				options.ExportFormat = RuntimeOptions::ExportFormatType::NativeV2;
				MergeRunnerV2 runner(options);
				auto contentClean = runner.clean(ss.str());
				auto dict = runner.createDictionnary(filename, contentClean);
				Assert::AreEqual(1ull, dict.size());

				// Check dictionary
				const auto& saved = dict[filename];
				Assert::AreEqual(reference.size(), saved._code.size());
				Assert::AreEqual(reference, saved._code);
				Assert::AreEqual(merge._nbLinesFile,    saved._nbLinesFile);
				Assert::AreEqual(merge._nbLinesCode,    saved._nbLinesCode);
				Assert::AreEqual(merge._nbLinesCovered, saved._nbLinesCovered);
			}
		}
	};
}