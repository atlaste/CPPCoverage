#include "TestCHighFile.h"

#ifdef __cplusplus
extern "C" {
	
#endif

int testCFunctionHigh1(int x)
{
	int result = 0;
	if (x == 0)
	{
		result = x + 1;
	}
	else
	{
		result = 0xff;
	}

	return result;
}


int testCFunctionHigh2(char *str)
{
	int result = 0;

	if (*str == '\0')
	{
		result = 0;
	}

	return result;
}

void doWhileHighTest(int x)
{
	do
	{
		x++;

	} while (x < 10);
}

void whileHighTest(int x)
{
	while (x < 10)
	{
		x++;
	}
}

#ifdef __cplusplus
}
#endif