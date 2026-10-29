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

namespace TestMergeRunner
{
	TEST_CLASS(TestMergeRunnerV2)
	{
	public:

		TEST_CLASS_CLEANUP(CleanUp)
		{
			FileSystem::DeleteTestFiles();
		}

		TEST_METHOD(SimpleMerge)
		{
			const std::string report1 =
				R"(<?xml version="1.0" encoding="utf-8"?>)""\n" \
				R"(<CppCoverage version="2.0">)""\n" \
				R"(	<directory path="directoryName">)""\n" \
				R"(		<file path="fileInsideDir" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="4" nbLinesCovered="4"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(	</directory>)""\n" \
				R"(	<file path="demo" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(		<stats nbLinesInFile="9" nbLinesOfCode="6" nbLinesCovered="5"/>)""\n" \
				R"(		<coverage>AAAAAACABYACwP+/oI8AAAaA</coverage>)""\n" \
				R"(	</file>)""\n" \
				R"(	<file path="diffLines" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(		<stats nbLinesInFile="3" nbLinesOfCode="3" nbLinesCovered="0"/>)""\n" \
				R"(		<coverage>AIAAgACA</coverage>)""\n" \
				R"(	</file>)""\n" \
				R"(</CppCoverage>)""\n";

			const std::string report2 =
				R"(<?xml version="1.0" encoding="utf-8"?>)""\n" \
				R"(<CppCoverage version="2.0">)""\n" \
				R"(	<directory path="additionalDir">)""\n" \
				R"(		<file path="fileInsideDir" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="5" nbLinesCovered="3"/>)""\n" \
				R"(			<coverage>AcBggAAAAIAQAATAAIA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(	</directory>)""\n" \
				R"(	<directory path="directoryName">)""\n" \
				R"(		<file path="fileInsideDir" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="5" nbLinesCovered="3"/>)""\n" \
				R"(			<coverage>AcBggAAAAIAQAATAAIA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="additionalFileInsideDir" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="5" nbLinesCovered="3"/>)""\n" \
				R"(			<coverage>AcBggAAAAIAQAATAAIA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(	</directory>)""\n" \
				R"(	<file path="demo" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(		<stats nbLinesInFile="9" nbLinesOfCode="7" nbLinesCovered="5"/>)""\n" \
				R"(		<coverage>AAAQADCAAcAEwACAEcoAgGCA</coverage>)""\n" \
				R"(	</file>)""\n" \
				R"(	<file path="diffLines" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(		<stats nbLinesInFile="4" nbLinesOfCode="4" nbLinesCovered="0"/>)""\n" \
				R"(		<coverage>AIAAgACAAIA=</coverage>)""\n" \
				R"(	</file>)""\n" \
				R"(</CppCoverage>)""\n";

			const std::string expectReport =
				R"(<?xml version="1.0" encoding="utf-8"?>)""\n" \
				R"(<CppCoverage version="2.0">)""\n" \
				R"(	<directory path="additionalDir">)""\n" \
				R"(		<file path="fileInsideDir" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="5" nbLinesCovered="3"/>)""\n" \
				R"(			<coverage>AcBggAAAAIAQAATAAIA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(	</directory>)""\n" \
				R"(	<directory path="directoryName">)""\n" \
				R"(		<file path="fileInsideDir" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="6" nbLinesCovered="5"/>)""\n" \
				R"(			<coverage>BoBmgAAA/78QgAbAAIA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="additionalFileInsideDir" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="5" nbLinesCovered="3"/>)""\n" \
				R"(			<coverage>AcBggAAAAIAQAATAAIA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(	</directory>)""\n" \
				R"(		<file path="demo" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="9" nbLinesOfCode="8" nbLinesCovered="7"/>)""\n" \
				R"(			<coverage>AAAQgDCABoAGwP+/sZkAgGaA</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="diffLines" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="3" nbLinesOfCode="3" nbLinesCovered="0"/>)""\n" \
				R"(			<coverage>AIAAgACA</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(</CppCoverage>)""\n";

			FileSystem::CreateTestFile("report1", report1);
			FileSystem::CreateTestFile("report2", report2);

			MergeRunnerV2 merger;
			merger.merge("report1", "report2");

			std::stringstream ss;
			merger.saveResultToStream(ss);

			Assert::AreEqual(expectReport, ss.str());
		}

		TEST_METHOD(MergedFileNotExist)
		{
			MergeRunnerV2 merger;
			auto func = [&] { merger.merge("report1_notExist", "report2_notExist"); };
			Assert::ExpectException<std::exception>(func);
		}

		TEST_METHOD(InvalidReportsMerge)
		{
			const std::string report1 =
				R"(<?xml version="1.0" encoding="utf-8"?>)""\n" \
				R"(<CppCoverage version="2.0">)""\n" \
				R"(	<directory path=123>)""\n" \
				R"(		<file path="file1" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="4" nbLinesCovered="4"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="file2" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="AD" nbLinesOfCode="4" nbLinesCovered="4"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(	</directory>)""\n" \
				R"(</CppCoverage>)""\n";

			const std::string report2 =
				R"(<?xml version="1.0" encoding="utf-8"?>)""\n" \
				R"(<CppCoverage version="2.0">)""\n" \
				R"(	<directory path="pathDir">)""\n" \
				R"(		<file path="file1" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="XY" nbLinesCovered="4"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="file2" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="4" nbLinesCovered="0x01"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file ppath="file3" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="4" nbLinesCovered="4"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="file4" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nnbLinesInFile="7" nbLinesOfCode="4" nbLinesCovered="4"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="file5" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" x="4" nbLinesCovered="4"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="file6" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nnbLinesInFile="7" nbLinesOfCode="4"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(	</directory>)""\n" \
				R"(</CppCoverage>)""\n";

			const std::string expectReport =
				R"(<?xml version="1.0" encoding="utf-8"?>)""\n" \
				R"(<CppCoverage version="2.0">)""\n" \
				R"(		<file path="file1" md5="0123456789ABCDEFGHIJKLMNOPQRSTUV">)""\n" \
				R"(			<stats nbLinesInFile="7" nbLinesOfCode="4" nbLinesCovered="4"/>)""\n" \
				R"(			<coverage>BYAGgAAA/78AAALAAAA=</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(</CppCoverage>)""\n";

			FileSystem::CreateTestFile("report1", report1);
			FileSystem::CreateTestFile("report2", report2);

			MergeRunnerV2 merger;
			merger.merge("report1", "report2");

			std::stringstream ss;
			merger.saveResultToStream(ss);

			Assert::AreEqual(expectReport, ss.str());
		}
	};
}