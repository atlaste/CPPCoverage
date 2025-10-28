#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include "FileCallbackInfo.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestFormat
{
	TEST_CLASS(TestWriteReport)
	{
	public:

		TEST_CLASS_INITIALIZE(Init)
		{
			auto& options = RuntimeOptions::Instance();
			options.CodePaths.push_back("C:\\proj\\src\\");
			options.CodePaths.push_back("C:\\proj\\empty\\");
			options.CodePaths.push_back("C:\\proj\\lib\\");

			options.PackageName = "MyPackage.exe";
		}

		TEST_CLASS_CLEANUP(CleanUp)
		{
			auto& options = RuntimeOptions::Instance();
			options.CodePaths.clear();
			options.PackageName.clear();
		}

		void WriteReport(const std::string& expectReport, RuntimeOptions::ExportFormatType exportFormat, const FileCallbackInfo::MergedProfileInfoMap& mergedProfileData)
		{
			auto createFileLineInfoArray = [](const std::vector<std::tuple<uint16_t, uint16_t>>& lineInfo) -> std::vector<FileLineInfo>
			{
				std::vector<FileLineInfo> arrayFileLineInfo;
				arrayFileLineInfo.resize(lineInfo.size());
				for (unsigned i = 0; i < lineInfo.size(); i++)
				{
					arrayFileLineInfo[i].DebugCount = std::get<0>(lineInfo.at(i));
					arrayFileLineInfo[i].HitCount = std::get<1>(lineInfo.at(i));
				}
				return arrayFileLineInfo;
			};

			auto addFile = [](FileCallbackInfo& fileCallbackInfo, const std::string filename, const std::vector<FileLineInfo>& lines)
			{
				std::istringstream iss{};
				auto fileInfo = new FileInfo(iss);
				fileInfo->relevant.assign(lines.size(), true);
				fileInfo->numberLines = lines.size();
				fileInfo->lines = lines;

				fileCallbackInfo.lineData[filename] = std::unique_ptr<FileInfo>(fileInfo);
			};

			FileCallbackInfo fileCallbackInfo("report.txt");

			addFile(fileCallbackInfo, "C:\\proj\\src\\srcFile.cpp", createFileLineInfoArray({ {4, 4}, {4, 2}, {0, 0}, {4, 0}, {4, 2} }));
			addFile(fileCallbackInfo, "C:\\proj\\src\\srcFile.h", createFileLineInfoArray({ {6, 6}, {3, 3} }));
			addFile(fileCallbackInfo, "C:\\proj\\lib\\libFile.cpp", createFileLineInfoArray({ {7, 0}, {4, 0}, {2, 0} }));
			addFile(fileCallbackInfo, "C:\\proj\\lib\\libFile.h", createFileLineInfoArray({ {18, 12}, {14, 7}, {3, 0} }));

			std::stringstream ss;
			fileCallbackInfo.WriteReport(exportFormat, mergedProfileData, ss);

			Assert::AreEqual(expectReport, ss.str());
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
			WriteReport(expectReport, RuntimeOptions::ExportFormatType::Native, mergedProfileData);
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
			WriteReport(expectReport, RuntimeOptions::ExportFormatType::Cobertura, mergedProfileData);
		}
	};
}