#include "CoverageRunner.h"
#include "RuntimeOptions.h"
#include "MergeRunner.h"

#include <algorithm>
#include <iostream>
#include <format>
#include <sstream>
#include <string>

void ShowHelp()
{
  std::cout << "Usage: coverage.exe [opts] -- [executable] [optional args]" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -quiet:             Suppress output information from coverage tool. Equivalent to -verbose=none" << std::endl;
  std::cout << "  -verbose [level]:   Allow to show a level of log. The accepted level flags are: error / warning / info / trace / none. By default is setup to 'trace'" << std::endl;
  std::cout << "  -format [fmt]:      Specify 'native', 'nativeV2' for native coverage format or 'cobertura' for cobertura XML or 'clover' for Clover" << std::endl;
  std::cout << "  -o [name]:          Write output information to the given filename" << std::endl;
  std::cout << "  -p [name]:          Assume source code can be found in the given path name" << std::endl;
  std::cout << "  -w [name]:          Working directory where we execute the given executable filename" << std::endl;
  std::cout << "  -m [name]:          Merge current output to given path name or copy output if not existing" << std::endl;
  std::cout << "  -pkg [name]:        Name of package under test (executable or dll)" << std::endl;
  std::cout << "  -help:              Show help" << std::endl;
  std::cout << "  -solution [name]:   Convert only file under this path (the path to file will be in relative format)." << std::endl;
  std::cout << "                      Typical usage is to give sln path of project." << std::endl;
  std::cout << "  -codeanalysis:" << std::endl;
  std::cout << "  -- [name]:          Run coverage on the given executable filename" << std::endl;
  std::cout << "Return code:" << std::endl;
  std::cout << "  0:                  Success run" << std::endl;
  std::cout << "  1:                  Executable missing" << std::endl;
  std::cout << "  2:                  Coverage failure" << std::endl;
  std::cout << "  3:                  Merge failure" << std::endl;
  std::cout << "  4:                  Application return error code" << std::endl;
  std::cout << "Example:" << std::endl;
  std::cout << "  coverage.exe -- myProgram.exe -param 1" << std::endl;
  std::cout << "    Run coverage on myProgram.exe with argument -param 1" << std::endl;
  std::cout << "  coverage.exe -o coverageLocal.cov -m fullcoverage.cov -- myProgram.exe" << std::endl;
  std::cout << "    Run coverage on myProgram.exe and create coverageLocal.cov coverage result and merge this result with anothers into fullcoverage.cov" << std::endl;
  std::cout << std::endl;
}

void ParseCommandLine(int argc, const char** argv)
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
      opts._verboseLevel = VerboseLevel::None;
    }
    else if (s == "-verbose" || s == "--verbose")
    {
      ++i;
      if (i == argc)
      {
        throw std::exception("Unexpected end of parameters. Expected level of verbose.");
      }
      std::string lvl(argv[i]);
      if (lvl == "none")
      {
        opts._verboseLevel = VerboseLevel::None;
      }
      else if (lvl == "error")
      {
        opts._verboseLevel = VerboseLevel::Error;
      }
      else if (lvl == "warning")
      {
        opts._verboseLevel = VerboseLevel::Warning;
      }
      else if (lvl == "info")
      {
        opts._verboseLevel = VerboseLevel::Info;
      }
      else if (lvl == "trace")
      {
        opts._verboseLevel = VerboseLevel::Trace;
      }
      else
      {
        throw std::exception(std::format("Unsupported verbose level: {0}.", lvl).c_str());
      }
    }
    else if (s == "-codeanalysis")
    {
      opts.UseStaticCodeAnalysis = true;
    }
    else if (s == "-solution")
    {
      ++i;
      if (i == argc)
      {
        throw std::exception("Unexpected end of parameters. Expected output file name.");
      }

      std::string t(argv[i]);
      opts.SolutionPath = t;
      if (!std::filesystem::exists(opts.SolutionPath))
        throw std::exception("The solution path provide is not existing.");
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
      else if (t == "nativeV2")
      {
        opts.ExportFormat = RuntimeOptions::NativeV2;
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
  if ((opts.ExportFormat != RuntimeOptions::Native && opts.ExportFormat != RuntimeOptions::NativeV2) && !opts.MergedOutput.empty())
  {
    throw std::exception("Merge mode is only for RuntimeOptions::Native or NativeV2 mode.");
  }

  auto idx = cmdLine.find(" -- ");
  if (idx == std::string::npos)
  {
    throw std::exception("Expected executable filename in command line.");
  }

  idx += 2;
  while (idx < cmdLine.size() && cmdLine[idx + 1] == ' ') { ++idx; }

  std::string childCommand = cmdLine.substr(idx + 1);
  if (childCommand.empty())
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
  if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Trace))
  {
    std::cout << "Executable: " << opts.Executable << std::endl;
    std::cout << "Arguments: " << opts.ExecutableArguments << std::endl;
  }
#endif
}

int main(int argc, const char** argv)
{
#ifdef _DEBUG
  int parsing = 0;
  std::cout << "--- Arguments --- " << std::endl;
  while (parsing < argc)
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
  catch (const std::exception& e)
  {
    if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Error))
    {
      std::cerr << "Error: " << e.what() << std::endl;
    }

    // When you miss --, exception is throw SO your need syntax help !
    ShowHelp();
    return 1; // Command error
  }

  try
  {
    if (opts.Executable.empty())
    {
      if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Error))
      {
        std::cerr << "Error: Missing executable file" << std::endl;
      }
      ShowHelp();
      return 1; // Command error
    }
    else
    {
      // Run
      CoverageRunner debug(opts);
      if (!debug.Start())
      {
        return 4;
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 2; // Coverage error
  }

  // Merge
  try
  {
    if (!opts.MergedOutput.empty())
    {
      if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Info))
      {
        std::cout << "Merge into " << opts.MergedOutput << std::endl;
      }
      auto merge = MergeRunner::createMergeRunner(opts);
      merge->execute();
    }
  }
  catch (const std::exception& e)
  {
    if (RuntimeOptions::Instance().isAtLeastLevel(VerboseLevel::Error))
    {
      std::cerr << "Error: " << e.what() << std::endl;
    }
    return 3; // Coverage error
  }
  return 0;
}
