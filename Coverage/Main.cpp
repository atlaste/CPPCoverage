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

int main(int argc, const char** argv)
{
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

	return 0;
}
