#include "CppUnitTest.h"
#include <SDKDDKVer.h>
#include <vector>

#include "FileInfo.h"
#include "FileSystem.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			template<> static std::wstring ToString<std::vector<bool>>(const class std::vector<bool>& t)
			{
				static constexpr wchar_t VALUE_TRUE[] = L"T";
				static constexpr wchar_t VALUE_FALSE[] = L"F";

				std::wstring result;
				for (auto value : t)
				{
					result.append(value ? VALUE_TRUE : VALUE_FALSE);
				}
				return result;
			}
		}
	}
}

namespace TestFile
{
	TEST_CLASS(TestFileInfo)
	{
		static constexpr char TEST_FILENAME[] = "C:\\proj\\src\\file.cpp";
	public:
		TEST_CLASS_CLEANUP(CleanUp)
		{
			FileSystem::DeleteTestFiles();
		}

		void DoTest(const std::vector<bool> expectRelevant, const std::stringstream& fileContain)
		{
			FileSystem::CreateTestFile(TEST_FILENAME, fileContain.str());

			FileInfo fileInfo(TEST_FILENAME);
			Assert::AreEqual(expectRelevant.size(), fileInfo.numberLines);
			Assert::AreEqual(expectRelevant, fileInfo.relevant);

			// PDB lineNumbers are 1-based; we work 0-based.
			for (size_t lineNumber = 1; lineNumber <= expectRelevant.size(); lineNumber++)
			{
				if (expectRelevant[lineNumber - 1])
				{
					Assert::IsNotNull(fileInfo.LineInfo(lineNumber));
				}
				else
				{
					Assert::IsNull(fileInfo.LineInfo(lineNumber));
				}
			}

			Assert::IsNull(fileInfo.LineInfo(expectRelevant.size() + 1));
			Assert::IsNull(fileInfo.LineInfo(expectRelevant.size() + 100));
		}

		TEST_METHOD(PragmaTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "#pragma DisableCodeCoverage\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "#pragma EnableCodeCoverage\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, false, false, false, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(WhitespacePragmaTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "		#pragma DisableCodeCoverage\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "		#pragma EnableCodeCoverage\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, false, false, false, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(InvalidDisablePragmaTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "#pragma DisableCodeCoverageX\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "#pragma EnableCodeCoverage\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, true, true, true, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(InvalidEnablePragmaTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "#pragma DisableCodeCoverage\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "#pragma EnableCodeCoverageX\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, false, false, false, false, false };
			DoTest(expectRelevant, stream);
		}


		TEST_METHOD(SingleLineCommentTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "// DisableCodeCoverage\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "// EnableCodeCoverage\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, false, false, false, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(WhitespaceSingleLineCommentTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "		// DisableCodeCoverage\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "		// EnableCodeCoverage\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, false, false, false, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(SingleLineCommentWithoutSpaceTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "		//DisableCodeCoverage\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "		//EnableCodeCoverage\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, false, false, false, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(SingleLineCommentWithUnnecessaryCommentBeforeFlagTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "		//comment DisableCodeCoverage\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "		//comment EnableCodeCoverage\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, true, true, true, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(SingleLineCommentWithUnnecessaryCommentAfterFlagTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "		//DisableCodeCoverage comment\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "		//EnableCodeCoverage comment\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, false, false, false, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(SingleLineCommentEndLineTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "		std::cout << \"Not show\";//DisableCodeCoverage\n";
			stream << "		std::cout << \"Not show\";//EnableCodeCoverage\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, false, false, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(SingleLineCommentInvalidDisableTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "//DisableCodeCoverageX\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "//EnableCodeCoverage\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, true, true, true, true, true };
			DoTest(expectRelevant, stream);
		}

		TEST_METHOD(SingleLineCommentInvalidEnableTest)
		{
			std::stringstream stream;
			stream << "void func()\n";
			stream << "{\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "//DisableCodeCoverage\n";
			stream << "		std::cout << \"Not show\";\n";
			stream << "//EnableCodeCoverageX\n";
			stream << "		std::cout << \"Show\";\n";
			stream << "}\n";

			std::vector<bool> expectRelevant{ true, true, true, false, false, false, false, false };
			DoTest(expectRelevant, stream);
		}
	};
}