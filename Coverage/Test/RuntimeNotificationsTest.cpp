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
			// create test files
			FileSystem::CreateTestFile("C:\\proj\\sln\\dir\\file.cpp", "file.cpp contents");
			FileSystem::CreateTestFile("C:\\proj\\sln\\dir\\file.h", "file.h contents");
			FileSystem::CreateTestFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp", "srcFile.cpp contents 1");
			FileSystem::CreateTestFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h", "srcFile.h contents 1");
			FileSystem::CreateTestFile("C:\\proj\\src\\ignoreFolder2\\srcFile.cpp", "srcFile.cpp contents 2");
			FileSystem::CreateTestFile("C:\\proj\\src\\ignoreFolder2\\srcFile.h", "srcFile.h contents 2");
			FileSystem::CreateTestFile("C:\\proj\\lib\\ignoreFile.cpp", "ignoreFile.cpp contents");
			FileSystem::CreateTestFile("C:\\proj\\lib\\ignoreFile.c", "ignoreFile.c contents");
			FileSystem::CreateTestFile("C:\\proj\\lib\\ignoreFile.h", "ignoreFile.h contents");
		}

		TEST_METHOD_INITIALIZE(MethodInit)
		{
			// set default SolutionPath
			auto& options = RuntimeOptions::Instance();
			options.SolutionPath = "C:\\proj\\sln\\";
		}

		TEST_CLASS_CLEANUP(CleanUp)
		{
			auto& options = RuntimeOptions::Instance();
			options.SolutionPath.clear();

			FileSystem::DeleteTestFiles();
		}

		// ============================== IGNORE FILE ==============================

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

		TEST_METHOD(IgnoreFileRelativeSolutionPath)
		{
			// check also trim path
			static constexpr std::string_view LINE = "IGNORE FILE:..\\lib\\ignoreFile.c  ";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(IgnoreFileRelativeSolutionPathWithoutLastBackslash)
		{
			RuntimeOptions::Instance().SolutionPath = "C:\\proj\\sln";

			static constexpr std::string_view LINE = "IGNORE FILE:..\\lib\\ignoreFile.c  ";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(IgnoreFileWithoutParentDirRelativeSolutionPath)
		{
			static constexpr std::string_view LINE = "IGNORE FILE: \\dir\\file.c";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.c"));
		}

		TEST_METHOD(IgnoreFileWithoutParentDirRelativeSolutionPathWithoutLastBackslash)
		{
			RuntimeOptions::Instance().SolutionPath = "C:\\proj\\sln";

			static constexpr std::string_view LINE = "IGNORE FILE: \\dir\\file.c";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.c"));
		}

		TEST_METHOD(IgnoreFileWithoutParentDirStartsWithDirName)
		{
			RuntimeOptions::Instance().SolutionPath = "C:\\proj\\sln";

			static constexpr std::string_view LINE = "IGNORE FILE: dir\\file.c";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.c"));
		}

		TEST_METHOD(IgnoreFileRelativeWithoutSolutionPath)
		{
			RuntimeOptions::Instance().SolutionPath.clear();

			static constexpr std::string_view LINE = "IGNORE FILE:..\\lib\\ignoreFile.c  ";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
		}

		TEST_METHOD(IgnoreFileTooManyParentDir)
		{
			static constexpr std::string_view LINE = "IGNORE FILE:..\\..\\..\\lib\\ignoreFile.c  ";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
		}

		// ============================== IGNORE FOLDER ==============================

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

		TEST_METHOD(IgnoreFolderRelativeSolutionPath)
		{
			// check also trim path
			static constexpr std::string_view LINE = "IGNORE FOLDER:   ..\\src\\ignoreFolder1\\";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder2\\srcFile.cpp"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder2\\srcFile.h"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(IgnoreFolderRelativeSolutionPathWithoutLastBackslash)
		{
			RuntimeOptions::Instance().SolutionPath = "C:\\proj\\sln";

			static constexpr std::string_view LINE = "IGNORE FOLDER:   ..\\src\\ignoreFolder1";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder2\\srcFile.cpp"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder2\\srcFile.h"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(IgnoreFolderWithoutParentDirRelativeSolutionPath)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER: \\dir\\";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.h"));
		}

		TEST_METHOD(IgnoreFolderWithoutParentDirRelativeSolutionPathWithoutLastBackslash)
		{
			RuntimeOptions::Instance().SolutionPath = "C:\\proj\\sln";

			static constexpr std::string_view LINE = "IGNORE FOLDER: \\dir";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.h"));
		}

		TEST_METHOD(IgnoreFolderWithoutParentDirStartsWithDirName)
		{
			RuntimeOptions::Instance().SolutionPath = "C:\\proj\\sln";

			static constexpr std::string_view LINE = "IGNORE FOLDER: dir";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.h"));
		}

		TEST_METHOD(IgnoreFolderRelativeWithoutSolutionPath)
		{
			RuntimeOptions::Instance().SolutionPath.clear();

			static constexpr std::string_view LINE = "IGNORE FOLDER:   ..\\src\\ignoreFolder1";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h"));
		}

		TEST_METHOD(IgnoreFolderTooManyParentDir)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER:   ..\\..\\..\\src\\ignoreFolder1";

			RuntimeNotifications notifications;
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h"));
		}

		// ============================== CODE ANALYSIS ==============================

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