#include <iostream>
#include <string>

#include <Windows.h>

extern "C" { __declspec(noinline) static void __stdcall PassToCPPCoverage(size_t count, const char* data) { __noop(); } }

typedef int(__cdecl *InvokeMethodSignature)();

namespace TestNamespace
{
	struct Foo
	{
		__declspec(noinline) static void Test() { }
	};
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
		}

		// Free the DLL module.
		fFreeResult = FreeLibrary(hinstLib);
	}
	else
	{
		std::cout << "Error: " << GetLastError() << std::endl;
	}
}

void TestFoo(int d)
{
	if (d < 10)
	{
		for (int i = 0; i < 10; ++i)
		{
			std::cout << d << " ";
		}
		std::cout << std::endl;
		TestFoo(d + 1);
	}
}

int main()
{
	std::string opts2 = "IGNORE FOLDER: MinimumTest";
	PassToCPPCoverage(opts2.size(), opts2.data());

	std::string opts = "IGNORE FOLDER: MinimumTestApp";
	PassToCPPCoverage(opts.size(), opts.data());

	TestNamespace::Foo::Test();
	
	TestDLL();
	for (int i = 0; i < 10; ++i)
	{
		TestFoo(0);
	}

	return 0;
}