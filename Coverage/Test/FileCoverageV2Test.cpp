#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include "FileCoverageV2.h"
#include "FileSystem.h"

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
		static constexpr auto max = FileCoverageV2::maskCount;
		static constexpr auto c = FileCoverageV2::maskIsCode;
		static constexpr auto p = FileCoverageV2::maskIsPartial;

		std::vector<uint8_t> makeCoverage(const FileCoverageV2::LineArray& data)
		{
			const auto total_size = data.size() * sizeof(FileCoverageV2::LineArray::value_type);
			std::vector<uint8_t> result(total_size);
			std::memcpy(result.data(), data.data(), total_size);
			return result;
		}
	public:
		TEST_METHOD(MakeFileCoverageV2UsingFileInfoTest)
		{
			static constexpr char TEST_FILENAME[] = "C:\\proj\\src\\file.cpp";

			std::stringstream fileContain;
			fileContain << "\n";
			fileContain << "1. partially cov\n";
			fileContain << "2. full cov\n";
			fileContain << "3. full cov\n";
			fileContain << "4. uncovered\n";

			FileSystem::CreateTestFile(TEST_FILENAME, fileContain.str());

			FileInfo fileInfo(TEST_FILENAME);
			fileInfo.lines[1].DebugCount = 2;
			fileInfo.lines[1].HitCount = 1;

			fileInfo.lines[2].DebugCount = 1;
			fileInfo.lines[2].HitCount = 1;

			fileInfo.lines[3].DebugCount = 1;
			fileInfo.lines[3].HitCount = 1;

			fileInfo.lines[4].DebugCount = 1;
			fileInfo.lines[4].HitCount = 0;

			FileSystem::DeleteTestFiles();

			FileCoverageV2 coverage(fileInfo, "0123456789ABCDEFGHIJKLMNOPQRSTUV");

			static constexpr char EXPECT_STREAM[] =
				"		<file path=\"filename\" md5=\"0123456789ABCDEFGHIJKLMNOPQRSTUV\">\n"
				"			<stats nbLinesInFile=\"5\" nbLinesOfCode=\"4\" nbLinesCovered=\"3\"/>\n"
				"			<coverage>AAABwAGAAYAAgA==</coverage>\n"
				"		</file>\n";

			std::stringstream ss;
			coverage.write("filename", ss);
			Assert::AreEqual(EXPECT_STREAM, ss.str().c_str());
		}

		TEST_METHOD(MakeFileCoverageV2UsingStringTest)
		{
			const std::vector<uint8_t> input_str = makeCoverage({ 0, 0, c, c, c | p | 1, c | 10000, c | 3000, 0, c | p | 1 });
			FileCoverageV2 coverage(input_str, "0123456789ABCDEFGHIJKLMNOPQRSTUV");

			static constexpr char EXPECT_STREAM[] =
				"		<file path=\"filename\" md5=\"0123456789ABCDEFGHIJKLMNOPQRSTUV\">\n"
				"			<stats nbLinesInFile=\"9\" nbLinesOfCode=\"6\" nbLinesCovered=\"4\"/>\n"
				"			<coverage>AAAAAACAAIABwBCnuIsAAAHA</coverage>\n"
				"		</file>\n";

			std::stringstream ss;
			coverage.write("filename", ss);
			Assert::AreEqual(EXPECT_STREAM, ss.str().c_str());
		}

		TEST_METHOD(MergeTest)
		{
			const std::vector<uint8_t> input1_str = makeCoverage({ 0, 0, c, c, c | p | 1, c | 10000, c | 3000, 0, c });
			FileCoverageV2 coverage(input1_str, "0123456789ABCDEFGHIJKLMNOPQRSTUV");

			const std::vector<uint8_t> input2_str = makeCoverage({ 10, c | p | 20, c, c, c | p | 4, c | p | 50, c | 3000, c | 8, c | 2 });
			FileCoverageV2 other(input2_str, "AA23456789ABCDEFGHIJKLMNOPQRSTUV");

			auto result = coverage.merge(other);
			Assert::IsTrue(result);

			static constexpr char EXPECT_STREAM[] =
				"		<file path=\"filename\" md5=\"0123456789ABCDEFGHIJKLMNOPQRSTUV\">\n"
				"			<stats nbLinesInFile=\"9\" nbLinesOfCode=\"9\" nbLinesCovered=\"7\"/>\n"
				"			<coverage>CoAUgACAAIAFwEKncJcIgAKA</coverage>\n"
				"		</file>\n";

			std::stringstream ss;
			coverage.write("filename", ss);
			Assert::AreEqual(EXPECT_STREAM, ss.str().c_str());
		}

		TEST_METHOD(MergeDifferentLinesCountTest)
		{
			const std::vector<uint8_t> input1_str = makeCoverage({ 0, 0, c });
			FileCoverageV2 coverage(input1_str, "0123456789ABCDEFGHIJKLMNOPQRSTUV");

			const std::vector<uint8_t> input2_str = makeCoverage({ 10, c | p | 20, c, c, c | p | 4, c | p | 50, c | 3000, c | 8, c | 2 });
			FileCoverageV2 other(input2_str, "AA23456789ABCDEFGHIJKLMNOPQRSTUV");

			auto result = coverage.merge(other);
			Assert::IsFalse(result);

			static constexpr char EXPECT_STREAM[] =
				"		<file path=\"filename\" md5=\"0123456789ABCDEFGHIJKLMNOPQRSTUV\">\n"
				"			<stats nbLinesInFile=\"3\" nbLinesOfCode=\"1\" nbLinesCovered=\"0\"/>\n"
				"			<coverage>AAAAAACA</coverage>\n"
				"		</file>\n";

			std::stringstream ss;
			coverage.write("filename", ss);
			Assert::AreEqual(EXPECT_STREAM, ss.str().c_str());
		}
	};
}