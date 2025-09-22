#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include <iostream>
#include <format>
#include <vector>

#include <Windows.h>

#pragma warning(disable: 4091)
#include <DbgHelp.h>
#pragma warning(default: 4091)

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

typedef int(__cdecl *InvokeMethodSignature)();

namespace MinimumTest
{
	TEST_CLASS(UnitTest1)
	{
	public:

		TEST_METHOD(TestMethod1)
		{

			HANDLE process = GetCurrentProcess();
			SymSetOptions(SYMOPT_LOAD_LINES);
			SymInitialize(process, NULL, TRUE);

			static const int MaxCallStack = 62; // // ## The sum of the FramesToSkip and FramesToCapture parameters must be less than 63 on Win2003
			void* callers[MaxCallStack];
			auto stackCount = RtlCaptureStackBackTrace(1, MaxCallStack, callers, NULL);

			SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO *>(calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1));
			symbol->MaxNameLen = 255;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

			SymFromAddr(process, reinterpret_cast<DWORD64>(callers[0]), NULL, symbol);


			if (false)
			{
				std::cout << "Hello world." << std::endl;
			}
		}

		TEST_METHOD(TestDisableCoveragePragma)
		{
#pragma DisableCodeCoverage

			std::cout << "This won't show up in code coverage." << std::endl;

#pragma EnableCodeCoverage
		}

		TEST_METHOD(TestDisableCoverageSingleLineComment)
		{
			// DisableCodeCoverage

			std::cout << "This won't show up in code coverage." << std::endl;

			// EnableCodeCoverage
		}

		TEST_METHOD(TestDisableCoverageSingleLineCommentWithoutWhitespace)
		{
			//DisableCodeCoverage

			std::cout << "This won't show up in code coverage." << std::endl;

			//EnableCodeCoverage
		}

		TEST_METHOD(TestDisableCoverageSingleLineCommentWithAdditional)
		{
			// DisableCodeCoverage necessary comment

			std::cout << "This won't show up in code coverage." << std::endl;

			// EnableCodeCoverage necessary comment
		}

		TEST_METHOD(TestDisableCoverageSingleLineCommentEndCodeLine)
		{
			std::cout << "This won't show up in code coverage." << std::endl; // DisableCodeCoverage
			// EnableCodeCoverage
		}

		TEST_METHOD(TestDisableCoverageSingleLineCommentEndCodeLineWithoutWhitespace)
		{
			std::cout << "This won't show up in code coverage." << std::endl;//DisableCodeCoverage
			// EnableCodeCoverage
		}

		static void TestDLL()
		{
			HINSTANCE hinstLib;
			InvokeMethodSignature RunDllTest;
			BOOL fFreeResult = FALSE;

			// Get a handle to the DLL module.

			hinstLib = LoadLibrary(TEXT("Library.dll"));

			// If the handle is valid, try to get the function address.

			bool success = false;
			if (hinstLib != NULL)
			{
				RunDllTest = (InvokeMethodSignature)GetProcAddress(hinstLib, "RunAll");

				// If the function addresses are valid, call the function.
				if (RunDllTest)
				{
					success = (RunDllTest() == 0);
				}
				else
				{
					std::cout << "Cannot find method 'RunAll'." << std::endl;
					Assert::Fail();
				}

				// Free the DLL module.
				fFreeResult = FreeLibrary(hinstLib);
			}
			else
			{
				std::cout << "Error: " << GetLastError() << std::endl;
				Assert::Fail();
			}
		}

		TEST_METHOD(TestLoadDLL)
		{
			// Test DLL's twice because of loading/unloading.
			TestDLL();
			TestDLL();
		}

		static void TestException()
		{
			throw std::exception("Test succeeded!");
		}

		TEST_METHOD(TestTrivial)
		{
			int value = 12;
			int count = 0;
			for (int i = 0; i < 10; ++i)
			{
				if (i >= value)
				{
					++count;
				}
				else
				{
					count -= 1;
				}
			}
			std::cout << count << std::endl;
		}

		TEST_METHOD(TestExceptions)
		{
			try
			{
				TestException();
				Assert::Fail();
			}
			catch (std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}
		}

		TEST_METHOD(TestDebuggerPresence)
		{
			if (IsDebuggerPresent())
			{
				// Shouldn't be called!

				std::cout << "We have a debugger!" << std::endl;
				Assert::Fail();
			}
			else
			{
				std::cout << "We do NOT have a debugger!" << std::endl;
			}
		}
		
		template<typename Type>
		struct MyTemplate
		{
			Type add(Type a)
			{
				return a + 42;
			}

			Type remove(Type a)
			{
				return a - 42;
			}
		};
		// Defined but never use
		using MyTemplateDouble = MyTemplate<double>;

		TEST_METHOD(TestTemplatePartial)
		{
			// Full used
			MyTemplate<int> i;
			Assert::AreEqual(52, i.add(10));
			Assert::AreEqual(-32, i.remove(10));

			// For this template: missing remove()
			MyTemplate<float> f;
			Assert::AreEqual(50.0f, f.add(8.0f));
		}

		TEST_METHOD(TestLoopCount)
		{
			const std::vector<int> a{ 1,2,3,4,5,6,7,8,9 };

			std::cout << "My list: " << std::endl;
			int count = 0;
			for (const int& i : a)
			{
				std::cout << std::format("{0}", i) << std::endl;
				count += i;
			}
			Assert::AreEqual(45, count);
		}

#define MACRO_TEST(condition)							\
		if(condition) {									\
			std::cout << "Is true" << std::endl;		\
		} else {										\
			std::cout << "Is False" << std::endl;		\
		}

#define MACRO_EXPECT_TEST(instruction)					\
		try{											\
			instruction;								\
		} catch(...) {		   							\
			std::cout << "Exception" << std::endl;		\
		}

		TEST_METHOD(TestMacroPartial)
		{
			int a = 5;
			MACRO_EXPECT_TEST(a = a + 5);
			Assert::AreEqual(10, a);

			MACRO_TEST(a == 12);
		}
	};
}