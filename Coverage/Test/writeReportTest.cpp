#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include "FileCallbackInfo.h"
#include "FileSystem.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestFormat
{
	TEST_CLASS(TestWriteReport)
	{
		static inline FileCallbackInfo* fileCallbackInfo = nullptr;

		static std::vector<FileLineInfo> createFileLineInfoArray(const std::vector<std::tuple<uint16_t, uint16_t>>& lineInfo)
		{
			std::vector<FileLineInfo> arrayFileLineInfo;
			arrayFileLineInfo.resize(lineInfo.size());
			for (unsigned i = 0; i < lineInfo.size(); i++)
			{
				arrayFileLineInfo[i].DebugCount = std::get<0>(lineInfo.at(i));
				arrayFileLineInfo[i].HitCount = std::get<1>(lineInfo.at(i));
			}
			return arrayFileLineInfo;
		}

		static void addFile(FileCallbackInfo& fileCallbackInfo, const std::string& filename, const std::vector<FileLineInfo>& lines)
		{
			fileCallbackInfo.lineData[filename] = std::make_unique<FileInfo>(filename);
			fileCallbackInfo.lineData[filename]->lines = lines;
		}

		TEST_CLASS_INITIALIZE(Init)
		{
			auto& options = RuntimeOptions::Instance();
			options.CodePaths.push_back("C:\\proj\\src\\");
			options.CodePaths.push_back("C:\\proj\\empty\\");
			options.CodePaths.push_back("C:\\proj\\lib\\");

			options.PackageName = "MyPackage.exe";

			// create test files
			FileSystem::CreateTestFile("C:\\proj\\src\\srcFile.cpp", "first line\nsecond line\nthird line\nfourth line\nfifth line");
			FileSystem::CreateTestFile("C:\\proj\\src\\srcFile.h", "first line\nsecond line");
			FileSystem::CreateTestFile("C:\\proj\\lib\\libFile.cpp", "first line\nsecond line\nthird line");
			FileSystem::CreateTestFile("C:\\proj\\lib\\libFile.h", "1st line\n2nd line\n3rd line");

			// create instance of FileCallbackInfo
			fileCallbackInfo = new FileCallbackInfo("report.txt");
			Assert::IsNotNull(fileCallbackInfo);

			// add informations about test files
			addFile(*fileCallbackInfo, "C:\\proj\\src\\srcFile.cpp", createFileLineInfoArray({ {4, 4}, {4, 2}, {0, 0}, {4, 0}, {4, 2} }));
			addFile(*fileCallbackInfo, "C:\\proj\\src\\srcFile.h", createFileLineInfoArray({ {6, 6}, {3, 3} }));
			addFile(*fileCallbackInfo, "C:\\proj\\lib\\libFile.cpp", createFileLineInfoArray({ {7, 0}, {4, 0}, {2, 0} }));
			addFile(*fileCallbackInfo, "C:\\proj\\lib\\libFile.h", createFileLineInfoArray({ {18, 12}, {14, 7}, {3, 0} }));
		}

		TEST_CLASS_CLEANUP(CleanUp)
		{
			auto& options = RuntimeOptions::Instance();
			options.CodePaths.clear();
			options.PackageName.clear();

			FileSystem::DeleteTestFiles();

			delete fileCallbackInfo;
		}

		TEST_METHOD(WriteReportNativeWithoutProfileData)
		{
			const std::string expectReport =
				"FILE: C:\\proj\\src\\srcFile.cpp\n" \
				"RES: cp_up\n" \
				"PROF: \n" \
				"FILE: C:\\proj\\src\\srcFile.h\n" \
				"RES: cc\n" \
				"PROF: \n" \
				"FILE: C:\\proj\\lib\\libFile.cpp\n" \
				"RES: uuu\n" \
				"PROF: \n" \
				"FILE: C:\\proj\\lib\\libFile.h\n" \
				"RES: ppu\n" \
				"PROF: \n";

			FileCallbackInfo::MergedProfileInfoMap mergedProfileData;

			std::stringstream ss;
			fileCallbackInfo->WriteReport(RuntimeOptions::ExportFormatType::Native, mergedProfileData, ss);
			Assert::AreEqual(expectReport, ss.str());
		}

		TEST_METHOD(WriteReportNativeV2)
		{
			const std::string expectReport =
				R"(<?xml version="1.0" encoding="utf-8"?>)""\n" \
				R"(<CppCoverage version="2.0">)""\n" \
				R"(	<directory path="C:\proj\src\">)""\n" \
				R"(		<file path="srcFile.cpp" md5="a2bab6536c40355d0adbca65a76d94aa">)""\n" \
				R"(			<stats nbLinesInFile="5" nbLinesOfCode="4" nbLinesCovered="3"/>)""\n" \
				R"(			<coverage>BIACwAAAAIACwA==</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="srcFile.h" md5="311f4a819681297456bb96840218f676">)""\n" \
				R"(			<stats nbLinesInFile="2" nbLinesOfCode="2" nbLinesCovered="2"/>)""\n" \
				R"(			<coverage>BoADgA==</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(	</directory>)""\n" \
				R"(	<directory path="C:\proj\lib\">)""\n" \
				R"(		<file path="libFile.cpp" md5="a4eabc0c3e65e7df3a3bb1ccc1adcd9f">)""\n" \
				R"(			<stats nbLinesInFile="3" nbLinesOfCode="3" nbLinesCovered="0"/>)""\n" \
				R"(			<coverage>AIAAgACA</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(		<file path="libFile.h" md5="67dec9250469e3c947656e54256e6a20">)""\n" \
				R"(			<stats nbLinesInFile="3" nbLinesOfCode="3" nbLinesCovered="2"/>)""\n" \
				R"(			<coverage>DMAHwACA</coverage>)""\n" \
				R"(		</file>)""\n" \
				R"(	</directory>)""\n" \
				R"(</CppCoverage>)""\n";

			FileCallbackInfo::MergedProfileInfoMap mergedProfileData;  // it doesn't matter, can be empty

			std::stringstream ss;
			fileCallbackInfo->WriteReport(RuntimeOptions::ExportFormatType::NativeV2, mergedProfileData, ss);
			Assert::AreEqual(expectReport, ss.str());
		}

		TEST_METHOD(WriteReportCobertura)
		{
			const std::string expectReport =
				R"(<?xml version="1.0" encoding="utf-8"?>)""\n" \
				R"(<coverage line-rate="0.25" version="">)""\n" \
				R"(	<packages>)""\n" \
				R"(		<package name="MyPackage.exe" line-rate="0.25">)""\n" \
				R"(			<classes>)""\n" \
				R"(				<class name="srcFile.cpp" filename="\proj\src\srcFile.cpp" line-rate="0.25">)""\n" \
				R"(					<lines>)""\n" \
				R"(						<line number="1" hits="1"/>)""\n" \
				R"(						<line number="2" hits="0"/>)""\n" \
				R"(						<line number="4" hits="0"/>)""\n" \
				R"(						<line number="5" hits="0"/>)""\n" \
				R"(					</lines>)""\n" \
				R"(				</class>)""\n" \
				R"(				<class name="srcFile.h" filename="\proj\src\srcFile.h" line-rate="1">)""\n" \
				R"(					<lines>)""\n" \
				R"(						<line number="1" hits="1"/>)""\n" \
				R"(						<line number="2" hits="1"/>)""\n" \
				R"(					</lines>)""\n" \
				R"(				</class>)""\n" \
				R"(				<class name="libFile.cpp" filename="\proj\lib\libFile.cpp" line-rate="0">)""\n" \
				R"(					<lines>)""\n" \
				R"(						<line number="1" hits="0"/>)""\n" \
				R"(						<line number="2" hits="0"/>)""\n" \
				R"(						<line number="3" hits="0"/>)""\n" \
				R"(					</lines>)""\n" \
				R"(				</class>)""\n" \
				R"(				<class name="libFile.h" filename="\proj\lib\libFile.h" line-rate="0">)""\n" \
				R"(					<lines>)""\n" \
				R"(						<line number="1" hits="0"/>)""\n" \
				R"(						<line number="2" hits="0"/>)""\n" \
				R"(						<line number="3" hits="0"/>)""\n" \
				R"(					</lines>)""\n" \
				R"(				</class>)""\n" \
				R"(			</classes>)""\n" \
				R"(		</package>)""\n" \
				R"(	</packages>)""\n" \
				R"(	<sources>)""\n" \
				R"(		<source>C:</source>)""\n" \
				R"(	</sources>)""\n" \
				R"(</coverage>)""\n";

			FileCallbackInfo::MergedProfileInfoMap mergedProfileData;  // it doesn't matter, can be empty

			std::stringstream ss;
			fileCallbackInfo->WriteReport(RuntimeOptions::ExportFormatType::Cobertura, mergedProfileData, ss);
			Assert::AreEqual(expectReport, ss.str());
		}
	};
}