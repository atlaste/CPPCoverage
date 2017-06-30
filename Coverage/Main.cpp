#include "CoverageRunner.h"
#include "RuntimeOptions.h"

#include <string>
#include <sstream>
#include <iostream>

void ShowHelp()
{
	std::cout << "Usage: coverage.exe [opts] -- [executable] [optional args]" << std::endl;
	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "  -quiet:        suppress output information from coverage tool" << std::endl;
	std::cout << "  -format [fmt]: specify 'native' for native coverage format or 'cobertura' for cobertura XML" << std::endl;
	std::cout << "  -o [name]:     write output information to the given filename" << std::endl;
	std::cout << "  -p [name]:     assume source code can be found in the given path name" << std::endl;
	std::cout << "  -- [name]:     run coverage on the given executable filename" << std::endl;
	std::cout << std::endl;
}

void ParseCommandLine(int argc, const char **argv)
{
	RuntimeOptions& opts = RuntimeOptions::Instance();

	LPTSTR cmd = GetCommandLine();
	std::string cmdLine = cmd;

	auto idx = cmdLine.find(" -- ");
	if (idx == std::string::npos)
	{
		throw std::exception("Expected executable filename in command line.");
	}

	idx += 2;
	while (idx < cmdLine.size() && cmdLine[idx + 1] == ' ') { ++idx; }

	std::string childCommand = cmdLine.substr(idx);
	if (childCommand.size() == 0)
	{
		throw std::exception("Expected executable filename in command line.");
	}

	opts.ExecutableArguments = childCommand;
	cmdLine = cmdLine.substr(0, idx);

	// Parse arguments
	for (int i = 1; i < argc; ++i)
	{
		std::string s(argv[i]);

		if (s == "-quiet" || s == "--quiet")
		{
			opts.Quiet = true;
		}
		else if (s == "-codeanalysis")
		{
			opts.UseStaticCodeAnalysis = true;
		}
		else if (s == "-format")
		{
			++i;
			if (i == argc)
			{
				throw std::exception("Unexpected end of parameters. Export type should be cobertura or native.");
			}

			std::string t(argv[i]);
			if (t == "native")
			{
				opts.ExportFormat = RuntimeOptions::Native;
			}
			else if (t == "cobertura")
			{
				opts.ExportFormat = RuntimeOptions::Cobertura;
			}
			else if (t == "clover")
			{
				opts.ExportFormat = RuntimeOptions::Clover;
			}
			else
			{
				throw std::exception("Unsupported export type. Export type should be cobertura or native.");
			}
		}
		else if (s == "-o")
		{
			++i;
			if (i == argc)
			{
				throw std::exception("Unexpected end of parameters. Expected output file name.");
			}

			std::string t(argv[i]);
			opts.OutputFile = t;
		}
		else if (s == "-p")
		{
			++i;
			if (i == argc)
			{
				throw std::exception("Unexpected end of parameters. Expected code path name.");
			}

			std::string t(argv[i]);
			opts.CodePath = t;
		}
		else if (s == "--")
		{
			++i;
			if (i == argc)
			{
				throw std::exception("Unexpected end of parameters. Expected executable file name.");
			}

			std::string t(argv[i]);
			opts.Executable = t;
			break;
		}
		else if (s == "-help")
		{
			ShowHelp();
		}
		else
		{
			throw std::exception("Incorrect parameter.");
		}
	}

}

static BOOL CALLBACK EnumerateSymbols(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
	std::cout << "Symbol: " << pSymInfo->Name << std::endl;
	return TRUE;
}

void Test()
{
	SymSetOptions(SYMOPT_LOAD_ANYTHING);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	auto str = "C:\\Users\\atlas\\Desktop\\Eigen projectjes\\OpenSource\\VSOpenCPPCoverage\\x64\\Debug\\MinimumTestApp.exe";
	auto result = CreateProcess(str, NULL, NULL, NULL, FALSE,
								DEBUG_PROCESS, NULL, NULL, &si, &pi);
	if (result == 0)
	{
		auto err = GetLastError();
		std::cout << "Error running process: " << err << std::endl;
		return;
	}

	if (!SymInitialize(pi.hProcess, NULL, FALSE))
	{
		auto err = GetLastError();
		std::cout << "Symbol initialization failed: " << err << std::endl;
		return;
	}

	bool first = false;
	DEBUG_EVENT debugEvent = { 0 };

	DWORD64 imageBase = 0;
	HANDLE imageFile = 0;
	DWORD continueStatus = DBG_CONTINUE;

	while (!first)
	{
		if (!WaitForDebugEvent(&debugEvent, 10))
		{
			auto err = GetLastError();
			std::cout << "Wait for debug event failed: " << err << std::endl;
			return;
		}

		if (debugEvent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
		{
			imageBase = reinterpret_cast<DWORD64>(debugEvent.u.CreateProcessInfo.lpBaseOfImage);
			imageFile = debugEvent.u.CreateProcessInfo.hFile;

			auto dllBase = SymLoadModuleEx(
				pi.hProcess,
				imageFile,
				str,
				NULL,
				imageBase,
				0,
				NULL,
				0);

			IMAGEHLP_MODULE64 ModuleInfo;
			memset(&ModuleInfo, 0, sizeof(ModuleInfo));
			ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
			BOOL info = SymGetModuleInfo64(pi.hProcess, dllBase, &ModuleInfo);

			if (ModuleInfo.SymType != SymPdb)
			{
				std::cout << "Failed to load PDB" << std::endl;
				return;
			}

			if (!dllBase)
			{
				auto err = GetLastError();
				std::cout << "Loading the module failed: " << err << std::endl;
				return;
			}

			if (!SymEnumSymbols(pi.hProcess, dllBase, NULL, EnumerateSymbols, nullptr))
			{
				auto err = GetLastError();
				std::cout << "Error: " << err << std::endl;
			}

			first = true;

			continueStatus = DBG_CONTINUE;
		}

		ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus);
		continueStatus = DBG_CONTINUE;
	}
	// cleanup code is omitted

	std::cout << "Done. " << std::endl;
}

int main(int argc, const char** argv)
{
	// {
	// 	Test();
	// 
	// 	std::string s;
	// 	std::getline(std::cin, s);
	// }

	RuntimeOptions& opts = RuntimeOptions::Instance();

	try
	{
		ParseCommandLine(argc, argv);

		if (opts.Executable.size() == 0)
		{
			ShowHelp();
		}
		else
		{
			// Run
			CoverageRunner debug(opts);
			debug.Start();
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	}

	std::string s;
	std::getline(std::cin, s);

	return 0;
}
