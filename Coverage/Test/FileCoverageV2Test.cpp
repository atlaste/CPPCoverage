#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include "FileCoverageV2.h"

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

namespace TestNativeV2
{
	TEST_CLASS(FileCoverage)
	{
	public:

		TEST_METHOD(WriteTest)
		{
			const auto max = FileCoverageV2::maskCount;
			const auto c = FileCoverageV2::maskIsCode;
			const auto p = FileCoverageV2::maskIsPartial;
			FileCoverageV2 coverage(9);
			coverage.md5Code = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
			coverage._code = { 0, 0, c, c, c | p | 1, c | 10000, c | 3000, 0, c | p | 1 };
			coverage.updateStats();

			Assert::AreEqual(9u, coverage._nbLinesFile);
			Assert::AreEqual(6u, coverage._nbLinesCode);
			Assert::AreEqual(4u, coverage._nbLinesCovered);

			static constexpr char EXPECT_STREAM[] =
				"		<file path=\"filename\" md5=\"0123456789ABCDEFGHIJKLMNOPQRSTUV\">\n"
				"			<stats nbLinesInFile=\"9\" nbLinesOfCode=\"6\" nbLinesCovered=\"4\"/>\n"
				"			<coverage>AAAAAACAAIABwBCnuIsAAAHA</coverage>\n"
				"		</file>\n";

			std::stringstream ss;
			coverage.write("filename", ss);
			Assert::AreEqual(EXPECT_STREAM, ss.str().c_str());
		}
	};
}