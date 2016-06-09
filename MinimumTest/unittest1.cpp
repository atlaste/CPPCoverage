#include "CppUnitTest.h"
#include <SDKDDKVer.h>

#include <iostream>

#include "../Library/Code.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MinimumTest
{
	TEST_CLASS(UnitTest1)
	{
	public:

		TEST_METHOD(TestMethod1)
		{
			if (false)
			{

				std::cout << "Hello world." << std::endl;
				Code c;
				c.Main();
			}
		}
	};
}