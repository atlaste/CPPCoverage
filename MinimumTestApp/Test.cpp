#include <iostream>

#include <Windows.h>

typedef int(__cdecl *InvokeMethodSignature)();

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
			std::cout << "Hello world!" << d << std::endl;
		}
		TestFoo(d + 1);
	}
}

int main()
{
	
	TestDLL();
	for (int i = 0; i < 10; ++i)
	{
		TestFoo(0);
	}

	return 0;
}