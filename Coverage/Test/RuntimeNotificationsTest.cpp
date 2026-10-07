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
	private:
		static constexpr char DEFAULT_SLN_PATH_WITHOUT_LAST_BACKSLASH[] = "C:\\proj\\sln";
		static constexpr char DEFAULT_SLN_PATH[] = "C:\\proj\\sln\\";
	public:
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

		TEST_CLASS_CLEANUP(CleanUp)
		{
			FileSystem::DeleteTestFiles();
		}

		// ============================== IGNORE FILE ==============================

		TEST_METHOD(IgnoreFileNotExist)
		{
			static constexpr std::string_view LINE = "IGNORE FILE: C:\\proj\\lib\\notExist.cpp";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\notExist.cpp"));
		}

		TEST_METHOD(IgnoreFileDirectoryPath)
		{
			static constexpr std::string_view LINE = "IGNORE FILE: C:\\proj\\lib\\";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
		}

		TEST_METHOD(IgnoreFileSuccess)
		{
			// check also trim path
			static constexpr std::string_view LINE = "IGNORE FILE:   C:\\proj\\lib\\ignoreFile.c  ";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(IgnoreFileRelativeSolutionPath)
		{
			// check also trim path
			static constexpr std::string_view LINE = "IGNORE FILE:..\\lib\\ignoreFile.c  ";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(IgnoreFileRelativeSolutionPathWithoutLastBackslash)
		{
			static constexpr std::string_view LINE = "IGNORE FILE:..\\lib\\ignoreFile.c  ";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH_WITHOUT_LAST_BACKSLASH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(IgnoreFileWithoutParentDirRelativeSolutionPath)
		{
			static constexpr std::string_view LINE = "IGNORE FILE: \\dir\\file.c";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.c"));
		}

		TEST_METHOD(IgnoreFileWithoutParentDirRelativeSolutionPathWithoutLastBackslash)
		{
			static constexpr std::string_view LINE = "IGNORE FILE: \\dir\\file.c";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH_WITHOUT_LAST_BACKSLASH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.c"));
		}

		TEST_METHOD(IgnoreFileWithoutParentDirStartsWithDirName)
		{
			static constexpr std::string_view LINE = "IGNORE FILE: dir\\file.c";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH_WITHOUT_LAST_BACKSLASH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.c"));
		}

		TEST_METHOD(IgnoreFileRelativeWithoutSolutionPath)
		{
			static constexpr std::string_view LINE = "IGNORE FILE:..\\lib\\ignoreFile.c  ";

			RuntimeOptions opts;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
		}

		TEST_METHOD(IgnoreFileTooManyParentDir)
		{
			static constexpr std::string_view LINE = "IGNORE FILE:..\\..\\..\\lib\\ignoreFile.c  ";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.c"));
		}

		// ============================== IGNORE FOLDER ==============================

		TEST_METHOD(IgnoreFolderNotExist)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER: C:\\proj\\notExist\\";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\notExist\\"));
		}

		TEST_METHOD(IgnoreFolderFilePath)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER: C:\\proj\\lib\\ignoreFile.cpp";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.cpp"));
		}

		TEST_METHOD(IgnoreFolderSuccess)
		{
			// check also trim path
			static constexpr std::string_view LINE = "IGNORE FOLDER:   C:\\proj\\src\\ignoreFolder1  ";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
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

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder2\\srcFile.cpp"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder2\\srcFile.h"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\lib\\ignoreFile.h"));
		}

		TEST_METHOD(IgnoreFolderRelativeSolutionPathWithoutLastBackslash)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER:   ..\\src\\ignoreFolder1";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH_WITHOUT_LAST_BACKSLASH;
			RuntimeNotifications notifications(opts);
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

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.h"));
		}

		TEST_METHOD(IgnoreFolderWithoutParentDirRelativeSolutionPathWithoutLastBackslash)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER: \\dir";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH_WITHOUT_LAST_BACKSLASH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.h"));
		}

		TEST_METHOD(IgnoreFolderWithoutParentDirStartsWithDirName)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER: dir";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH_WITHOUT_LAST_BACKSLASH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.cpp"));
			Assert::IsTrue(notifications.IgnoreFile("C:\\proj\\sln\\dir\\file.h"));
		}

		TEST_METHOD(IgnoreFolderRelativeWithoutSolutionPath)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER:   ..\\src\\ignoreFolder1";

			RuntimeOptions opts;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h"));
		}

		TEST_METHOD(IgnoreFolderTooManyParentDir)
		{
			static constexpr std::string_view LINE = "IGNORE FOLDER:   ..\\..\\..\\src\\ignoreFolder1";

			RuntimeOptions opts;
			opts.SolutionPath = DEFAULT_SLN_PATH;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.cpp"));
			Assert::IsFalse(notifications.IgnoreFile("C:\\proj\\src\\ignoreFolder1\\srcFile.h"));
		}

		// ============================== CODE ANALYSIS ==============================

		TEST_METHOD(EnableCodeAnalysis)
		{
			static constexpr std::string_view LINE = "ENABLE CODE ANALYSIS";

			RuntimeOptions opts;
			opts.UseStaticCodeAnalysis = false;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsTrue(opts.UseStaticCodeAnalysis);
		}

		TEST_METHOD(DisableCodeAnalysis)
		{
			static constexpr std::string_view LINE = "DISABLE CODE ANALYSIS";

			RuntimeOptions opts;
			opts.UseStaticCodeAnalysis = true;
			RuntimeNotifications notifications(opts);
			notifications.Handle(LINE.data(), LINE.size());

			Assert::IsFalse(opts.UseStaticCodeAnalysis);
		}
	};
}