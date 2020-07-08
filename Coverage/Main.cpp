#include "CoverageRunner.h"
#include "RuntimeOptions.h"
#include "MergeRunner.h"

#include <algorithm>
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
    std::cout << "  -w [name]:     Working directory where we execute the given executable filename" << std::endl;
    std::cout << "  -m [name]:     Merge current output to given path name or copy output if not existing" << std::endl;
    std::cout << "  -pkg [name]:   Name of package under test (executable or dll)" << std::endl;
    std::cout << "  -- [name]:     run coverage on the given executable filename" << std::endl;
    std::cout << "Return code:" << std::endl;
    std::cout << "  0:             Success run" << std::endl;
    std::cout << "  1:             Executable missing" << std::endl;
    std::cout << "  2:             Coverage failure" << std::endl;
    std::cout << "  3:             Merge failure" << std::endl;
    std::cout << std::endl;
}

void ParseCommandLine(int argc, const char **argv)
{
    RuntimeOptions& opts = RuntimeOptions::Instance();

    LPTSTR cmd = GetCommandLine();
    std::string cmdLine = cmd;
    
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
        else if (s == "-w")
        {
            ++i;
            if (i == argc)
            {
                throw std::exception("Unexpected end of parameters. Expected environment path name.");
            }

            std::string t(argv[i]);
            opts.WorkingDirectory = t;
        }
        else if (s == "-m")
        {
            ++i;
            if (i == argc)
            {
                throw std::exception("Unexpected end of parameters. Expected merge path name.");
            }

            std::string t(argv[i]);
            opts.MergedOutput = t;
        }
        else if (s == "-pkg")
        {
            ++i;
            if (i == argc)
            {
                throw std::exception("Unexpected end of parameters. Expected package name.");
            }

            std::string t(argv[i]);
            opts.PackageName = t;
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
            std::string message("Incorrect parameter: ");
            message += s;
            throw std::exception(message.c_str());
        }
    }

    // Check we can merge
    if (opts.ExportFormat != RuntimeOptions::Native && !opts.MergedOutput.empty())
    {
        throw std::exception("Merge mode is only for RuntimeOptions::Native mode.");
    }

    auto idx = cmdLine.find(" -- ");
    if (idx == std::string::npos)
    {
        throw std::exception("Expected executable filename in command line.");
    }

    idx += 2;
    while (idx < cmdLine.size() && cmdLine[idx + 1] == ' ') { ++idx; }

    std::string childCommand = cmdLine.substr(idx+1);
    if (childCommand.size() == 0)
    {
        throw std::exception("Expected executable filename in command line.");
    }

    opts.ExecutableArguments = childCommand;
    /*
    size_t pos = opts.ExecutableArguments.find(opts.Executable);
    opts.ExecutableArguments = opts.ExecutableArguments.substr( pos + opts.Executable.size());
    if(opts.ExecutableArguments[0] == '\"')
        opts.ExecutableArguments = opts.ExecutableArguments.substr( std::min<size_t>(2, opts.ExecutableArguments.size()) );
    else if(!opts.ExecutableArguments.empty())
        opts.ExecutableArguments = opts.ExecutableArguments.substr(1);
    */
#ifdef _DEBUG
    std::cout << "Executable: " << opts.Executable << std::endl;
    std::cout << "Arguments: "  << opts.ExecutableArguments << std::endl;
#endif
}

int main(int argc, const char** argv)
{
#ifdef _DEBUG
    int parsing = 0;
    std::cout << "--- Arguments --- " << std::endl;
    while(parsing < argc)
    {
        std::cout << parsing << ": " << argv[parsing] << std::endl;
        ++parsing;
    }
#endif

    RuntimeOptions& opts = RuntimeOptions::Instance();

    try
    {
        ParseCommandLine(argc, argv);
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        
        // When you miss --, exception is throw SO your need syntax help !
        ShowHelp();
        return 1; // Command error
    }
    
    try
    {
        if(opts.Executable.empty())
        {
            std::cerr << "Error: Missing executable file" << std::endl;
            ShowHelp();
            return 1; // Command error
        }
        else
        {
            // Run
            CoverageRunner debug(opts);
            debug.Start();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2; // Coverage error
    }

    // Merge
    try
    {
        if(!opts.MergedOutput.empty())
        {
            std::cout << "Merge into " << opts.MergedOutput << std::endl;
            MergeRunner merge(opts);
            merge.execute();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 3; // Coverage error
    }
    return 0;
}
