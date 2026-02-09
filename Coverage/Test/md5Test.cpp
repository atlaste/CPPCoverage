#include "CppUnitTest.h"
#include <SDKDDKVer.h>
#include <memory>

#include "md5.h"

#ifndef NOMINMAX
#	define NOMINMAX
#	include <Windows.h>
#endif

#pragma warning(disable: 4091)
#include <DbgHelp.h>
#pragma warning(default: 4091)

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestHash
{
	TEST_CLASS(TestMD5)
	{
	public:

		TEST_CLASS_CLEANUP(CleanUp)
		{
			FileSystem::DeleteTestFiles();
		}

		TEST_METHOD(FileNotExist)
		{
			MD5 md5;
			const auto result = md5.encode("C:\\proj\\src\\fileNotExist.cpp");
			constexpr size_t EXPECT_LEN = 0;
			Assert::AreEqual(EXPECT_LEN, result.length());
		}

		TEST_METHOD(FileExist)
		{
			FileSystem::CreateTestFile("C:\\proj\\src\\srcFile.cpp", "Lorem\nIpsum");

			MD5 md5;
			const auto result = md5.encode("C:\\proj\\src\\srcFile.cpp");
			constexpr const char EXPECT[] = "d5f43904ac1340720a2b2f7920d7c9c9";
			Assert::AreEqual(EXPECT, result.c_str());
		}

		TEST_METHOD(HugeFileExist)
		{
			MD5 md5;

			std::string buffer;
			buffer.resize(1024 * 1024 + 10);
			std::fill(buffer.begin(), buffer.end(), 'A');
			FileSystem::CreateTestFile("C:\\proj\\src\\srcFile.cpp", buffer);

			const auto result = md5.encode("C:\\proj\\src\\srcFile.cpp");
			constexpr const char EXPECT[] = "16d6031b311eafc134e16ca5a744250a";
			Assert::AreEqual(EXPECT, result.c_str());
		}
	};
}