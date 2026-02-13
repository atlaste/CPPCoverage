#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include "RuntimeNotifications.h"
#include "RuntimeOptions.h"
#include "FileSystem.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestRuntimeNotifications
{
	TEST_CLASS(TestNotifications)
	{
		TEST_CLASS_INITIALIZE(Init)
		{
			auto& options = RuntimeOptions::Instance();
			options.SolutionPath = "C:\\proj\\sln\\";

			// create test files
			FileSystem::CreateTestFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp", "srcFile.cpp contents 1");
			FileSystem::CreateTestFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h", "srcFile.h contents 1");
			FileSystem::CreateTestFile("C:\\proj\\src\\ignoreFolder2\\srcFile.cpp", "srcFile.cpp contents 2");
			FileSystem::CreateTestFile("C:\\proj\\src\\ignoreFolder2\\srcFile.h", "srcFile.h contents 2");
			FileSystem::CreateTestFile("C:\\proj\\lib\\ignoreFile.cpp", "ignoreFile.cpp contents");
			FileSystem::CreateTestFile("C:\\proj\\lib\\ignoreFile.c", "ignoreFile.c contents");
			FileSystem::CreateTestFile("C:\\proj\\lib\\ignoreFile.h", "ignoreFile.h contents");
		}

		TEST_CLASS_CLEANUP(CleanUp)
		{
			auto& options = RuntimeOptions::Instance();
			options.SolutionPath.clear();

			FileSystem::DeleteTestFiles();
		}

		TEST_METHOD(IgnoreFileNotExist)
		{
			static constexpr std::string_view LINE = "IGNORE FILE: C:\\proj\\lib\\notExist.cpp";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\notExist.cpp"));
		}

		TEST_METHOD(IgnoreFileDirectoryPath)
		{
			static constexpr std::string_view LINE = "IGNORE FILE: C:\\proj\\lib\\";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
		}

		TEST_METHOD(IgnoreFileSuccess)
		{
			// check also trim path
			static constexpr std::string_view LINE = "IGNORE FILE:   C:\\proj\\lib\\ignoreFile.c  ";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(IgnoreFolderNotExist)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER: C:\\proj\\notExist\\";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\notExist\\"));
		}

		TEST_METHOD(IgnoreFolderFilePath)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER: C:\\proj\\lib\\ignoreFile.cpp";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
		}

		TEST_METHOD(IgnoreFolderSuccess)
		{
			// check also trim path
			static constexpr std::string_view LINE = "IGNORE FOLDER:   C:\\proj\\src\\ignoreFolder1  ";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder2\\srcFile.cpp"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder2\\srcFile.h"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(EnableCodeAnalysis)
		{
			RuntimeOptions::Instance().UseStaticCodeAnalysis = false;

			static constexpr std::string_view LINE = "ENABLE CODE ANALYSIS";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(RuntimeOptions::Instance().UseStaticCodeAnalysis);
		}

		TEST_METHOD(DisableCodeAnalysis)
		{
			RuntimeOptions::Instance().UseStaticCodeAnalysis = true;

			static constexpr std::string_view LINE = "DISABLE CODE ANALYSIS";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(RuntimeOptions::Instance().UseStaticCodeAnalysis);
		}
	};
}