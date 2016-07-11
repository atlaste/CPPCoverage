#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include <iostream>

#include "../Library/Code.h"

#include <Windows.h>

#pragma warning(disable: 4091)
#include <DbgHelp.h>
#pragma warning(default: 4091)


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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

			SymFromAddr(process, reinterpret_cast<DWORD64>(callers[0]), 0, symbol);


			if (false)
			{

				std::cout << "Hello world." << std::endl;
				Code c;
				c.Main();
			}
		}
	};
}