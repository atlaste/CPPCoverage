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

int main()
{
	TestDLL();
	std::cout << "Hello world!" << std::endl;

	return 0;
}