#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include <Windows.h>

#pragma warning(disable: 4091)
#include <DbgHelp.h>
#pragma warning(default: 4091)

struct StackTrace
{
	// NOTE: The sum of the FramesToSkip and FramesToCapture parameters must be less than 63 on Win2003
	static const int MaxCallStack = 32;

	size_t stackCount;
	void* callers[MaxCallStack];

	static void CreateStackTrace(StackTrace* ptr)
	{
		ptr->stackCount = RtlCaptureStackBackTrace(1, MaxCallStack, ptr->callers, NULL);
	}

	static std::vector<std::string> HumanReadableStackTrace(HANDLE process, StackTrace* ptr)
	{
		if (ptr->stackCount > MaxCallStack)
		{
			throw std::exception("Stack count mismatch. This means you have heap corruption.");
		}

		std::vector<std::string> result;
		if (ptr->stackCount == 0)
		{
			result.push_back("[stack trace unavailable]");
		}
		else
		{
			// Let's initialize this once for our process.
			SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO *>(calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1));
			symbol->MaxNameLen = 255;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

			for (int i = 0; i < ptr->stackCount; ++i)
			{
				std::ostringstream oss;

				if (!SymFromAddr(process, reinterpret_cast<DWORD64>(ptr->callers[i]), 0, symbol))
				{
					oss << "??? (address: " << reinterpret_cast<uint64_t>(ptr->callers[i]) << ")";
				}
				else
				{
					std::string name(symbol->Name, symbol->Name + symbol->NameLen);

					oss << name;

					DWORD  dwDisplacement;
					IMAGEHLP_LINE64 line;

					line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

					if (SymGetLineFromAddr64(process, symbol->Address, &dwDisplacement, &line))
					{
						// SymGetLineFromAddr64 returned success
						oss << " (" << line.FileName << ":" << line.LineNumber << ")";
					}
					else
					{
						oss << " (address: " << symbol->Address << ")";
					}

					if (name == "main" || name == "WinMain")
					{
						// That's far enough thank you
						break;
					}
				}

				result.push_back(oss.str());
			}
			free(symbol);
		}

		if (ptr->stackCount == MaxCallStack)
		{
			result.push_back("\t...");
		}

		return result;
	}
};