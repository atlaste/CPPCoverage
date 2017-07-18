#include <iostream>

struct MyClass
{
	static void DoSomething()
	{
		std::cout << "Hello from DLL" << std::endl;
	}

	static void TestIfThenElsePerk(bool input)
	{
		if (input)
		{
			std::cout << "This is the 'then'" << std::endl;
		}
		else
		{
			std::cout << "This is the 'else'" << std::endl;
		}
	}
};

extern "C"
{
	__declspec(dllexport) int __stdcall RunAll()
	{
		MyClass::DoSomething();

		// Static code analysis should give us a 100% coverage in this. Let's see:
		MyClass::TestIfThenElsePerk(true);
		MyClass::TestIfThenElsePerk(false);
		return 0;
	}
}
