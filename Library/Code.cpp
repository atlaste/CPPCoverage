#include <iostream>

struct MyClass
{
	static void DoSomething()
	{
		std::cout << "Hello from DLL" << std::endl;
	}
};

extern "C"
{
	__declspec(dllexport) int __stdcall RunAll()
	{
		MyClass::DoSomething();
		return 0;
	}
}
