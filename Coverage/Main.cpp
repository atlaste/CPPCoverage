#include "CoverageRunner.h"
#include "RuntimeOptions.h"
#include "MergeRunner.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
  std::cout << "                      Convert only file under this path (the path to file will be in relative format)." << std::endl;
  std::cout << "  -w [name]:          Working directory where we execute the given executable filename" << std::endl;
  std::cout << "  -m [name]:          Merge current output to given path name or copy output if not existing" << std::endl;
  std::cout << "  -pkg [name]:        Name of package under test (executable or dll)" << std::endl;
  std::cout << "  -help:              Show help" << std::endl;
  std::cout << "  -solution [name]:   Convert only file under this path (the path to file will be in relative format)." << std::endl;
  std::cout << "                      Typical usage is to give sln path of project." << std::endl;
  std::cout << "                      The flag used to ignore code coverage for directories or files (by the PassToCPPCoverage method)." << std::endl;
  std::cout << "  -codeanalysis:" << std::endl;
  std::cout << "  -attach [pid]:      Attach to an already-running process with the given PID instead" << std::endl;
  std::cout << "                      of launching a new one. The -- <executable> argument becomes" << std::endl;
  std::cout << "                      optional (it will be derived from the PID when omitted)." << std::endl;
  std::cout << "                      This is used internally to hand cross-bitness child processes" << std::endl;
  std::cout << "                      off to the sibling Coverage-x86.exe / Coverage-x64.exe." << std::endl;
  std::cout << "  -consolidate:       After the run, merge any .cov files produced by sibling-bitness" << std::endl;
  std::cout << "                      helper processes into the main -o output file and delete them," << std::endl;
  std::cout << "                      so that you end up with a single coverage file containing both" << std::endl;
  std::cout << "                      bitnesses. Only supported for -format native / nativeV2." << std::endl;
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

void ParseCommandLine(int argc, const char** argv, RuntimeOptions& opts)
{
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
    else if (s == "-consolidate")
    {
      opts.ConsolidateAuxiliary = true;
    }
    else if (s == "-attach")
    {
      ++i;
      if (i == argc)
      {
        throw std::exception("Unexpected end of parameters. Expected PID after -attach.");
      }

      try
      {
        opts.AttachPid = static_cast<DWORD>(std::stoul(argv[i]));
      }
      catch (const std::exception&)
      {
        throw std::exception("Invalid PID passed to -attach.");
      }

      if (opts.AttachPid == 0)
      {
        throw std::exception("Invalid PID passed to -attach.");
      }

      opts.Attach = true;
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
        opts.ExportFormat = RuntimeOptions::ExportFormatType::Native;
      }
      else if (t == "nativeV2")
      {
        opts.ExportFormat = RuntimeOptions::ExportFormatType::NativeV2;
      }
      else if (t == "cobertura")
      {
        opts.ExportFormat = RuntimeOptions::ExportFormatType::Cobertura;
      }
      else if (t == "clover")
      {
        opts.ExportFormat = RuntimeOptions::ExportFormatType::Clover;
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
      opts.CodePaths.push_back(t);
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
  if (opts.ExportFormat != RuntimeOptions::ExportFormatType::Native &&
      opts.ExportFormat != RuntimeOptions::ExportFormatType::NativeV2 &&
      !opts.MergedOutput.empty())
  {
    throw std::exception("Merge mode is only for RuntimeOptions::Native or NativeV2 mode.");
  }

  // -consolidate piggy-backs on the Native / NativeV2 merge machinery, so
  // reject it for formats that can't round-trip through MergeRunner.
  if (opts.ExportFormat != RuntimeOptions::ExportFormatType::Native &&
      opts.ExportFormat != RuntimeOptions::ExportFormatType::NativeV2 &&
      opts.ConsolidateAuxiliary)
  {
    throw std::exception("-consolidate is only supported for -format native or nativeV2.");
  }

  auto idx = cmdLine.find(" -- ");
  if (idx == std::string::npos)
  {
    // In -attach mode the executable part is optional. We'll derive the
    // executable path below via QueryFullProcessImageName on AttachPid.
    if (!opts.Attach)
    {
      throw std::exception("Expected executable filename in command line.");
    }
  }
  else
  {
    idx += 2;
    while (idx < cmdLine.size() && cmdLine[idx + 1] == ' ') { ++idx; }

    std::string childCommand = cmdLine.substr(idx + 1);
    if (childCommand.empty() && !opts.Attach)
    {
      throw std::exception("Expected executable filename in command line.");
    }

    opts.ExecutableArguments = childCommand;
  }

  if (opts.Attach && opts.Executable.empty())
  {
    // Derive the target executable path from the attach PID so the rest of
    // the coverage pipeline (package name, output filename, etc.) keeps
    // working unchanged.
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, opts.AttachPid);
    if (!hProc)
    {
      throw std::exception("Could not open attach-target process to derive executable path.");
    }

    char pathBuffer[MAX_PATH] = { 0 };
    DWORD bufSize = MAX_PATH;
    if (!QueryFullProcessImageNameA(hProc, 0, pathBuffer, &bufSize))
    {
      CloseHandle(hProc);
      throw std::exception("Could not query attach-target executable path.");
    }
    CloseHandle(hProc);

    opts.Executable = pathBuffer;
  }
  /*
  size_t pos = opts.ExecutableArguments.find(opts.Executable);
  opts.ExecutableArguments = opts.ExecutableArguments.substr( pos + opts.Executable.size());
  if(opts.ExecutableArguments[0] == '\"')
      opts.ExecutableArguments = opts.ExecutableArguments.substr( std::min<size_t>(2, opts.ExecutableArguments.size()) );
  else if(!opts.ExecutableArguments.empty())
      opts.ExecutableArguments = opts.ExecutableArguments.substr(1);
  */
#ifdef _DEBUG
  if (opts.isAtLeastLevel(VerboseLevel::Trace))
  {
    std::cout << "Executable: " << opts.Executable << std::endl;
    std::cout << "Arguments: " << opts.ExecutableArguments << std::endl;
  }
#endif
}

class UTF8CodePage {
public:
  UTF8CodePage() : oldCodePage(::GetConsoleOutputCP())
  {
    ::SetConsoleOutputCP(GetACP());
  }
  ~UTF8CodePage()
  {
    ::SetConsoleOutputCP(oldCodePage);
  }

private:
  UINT oldCodePage;
};

int main(int argc, const char** argv)
{
  UTF8CodePage codePage;

#ifdef _DEBUG
  int parsing = 0;
  std::cout << "--- Arguments --- " << std::endl;
  while (parsing < argc)
  {
    std::cout << parsing << ": " << argv[parsing] << std::endl;
    ++parsing;
  }
#endif

  auto& opts = RuntimeOptionsSingleton::Instance();

  try
  {
    ParseCommandLine(argc, argv, opts);
  }
  catch (const std::exception& e)
  {
    if (opts.isAtLeastLevel(VerboseLevel::Error))
    {
      std::cerr << "Error: " << e.what() << std::endl;
    }

    // When you miss --, exception is throw SO your need syntax help !
    ShowHelp();
    return 1; // Command error
  }

  // Auxiliary coverage outputs produced by sibling-bitness CPPCoverage
  // helpers (see CoverageRunner). Collected from the runner after it exits
  // so we can fold them into the main output below.
  std::vector<std::string> auxiliaryOutputs;

  try
  {
    if (opts.Executable.empty())
    {
      if (opts.isAtLeastLevel(VerboseLevel::Error))
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
      bool success = debug.Start();

      auxiliaryOutputs = debug.auxiliaryCoverageOutputs;

      if (!success)
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

  // Determine the effective local output file (same rule CoverageRunner
  // uses when -o is omitted). We need it below for consolidation and for
  // the -m merge step.
  std::string localOutputFile = opts.OutputFile;
  if (localOutputFile.empty())
  {
    localOutputFile = opts.Executable + ".cov";
  }

  // Consolidate auxiliary (sibling-bitness) .cov files into our own
  // output so the user ends up with a single coverage file. We do this
  // BEFORE the -m merge so that a subsequent merge step only has to fold
  // one consolidated file into MergedOutput.
  //
  // Implementation detail: we reuse MergeRunner by treating each aux file
  // as OutputFile and our local .cov as MergedOutput. After each call the
  // combined result lives in our local .cov, and the aux file is deleted.
  try
  {
    if (opts.ConsolidateAuxiliary && !auxiliaryOutputs.empty())
    {
      if (!std::filesystem::exists(localOutputFile))
      {
        if (opts.isAtLeastLevel(VerboseLevel::Warning))
        {
          std::cerr << "Warning: -consolidate requested but the master coverage file is missing: "
            << localOutputFile << std::endl;
        }
      }
      else
      {
        for (const auto& auxFile : auxiliaryOutputs)
        {
          if (!std::filesystem::exists(auxFile))
          {
            if (opts.isAtLeastLevel(VerboseLevel::Warning))
            {
              std::cerr << "Warning: expected auxiliary coverage file missing: " << auxFile << std::endl;
            }
            continue;
          }

          if (opts.isAtLeastLevel(VerboseLevel::Info))
          {
            std::cout << "Consolidating auxiliary coverage into "
              << localOutputFile << ": " << auxFile << std::endl;
          }

          {
            RuntimeOptions auxOpts = opts;
            auxOpts.OutputFile = auxFile;
            auxOpts.MergedOutput = localOutputFile;
            MergeRunner auxMerge(auxOpts);
            auxMerge.execute();
          }

          std::error_code ec;
          std::filesystem::remove(auxFile, ec);
          if (ec && opts.isAtLeastLevel(VerboseLevel::Warning))
          {
            std::cerr << "Warning: failed to remove consolidated aux file "
              << auxFile << ": " << ec.message() << std::endl;
          }
        }

        // Everything has been folded in; don't let the -m step below try
        // to re-merge the (now-deleted) aux files.
        auxiliaryOutputs.clear();
      }
    }
  }
  catch (const std::exception& e)
  {
    if (opts.isAtLeastLevel(VerboseLevel::Error))
    {
      std::cerr << "Error while consolidating auxiliary coverage: " << e.what() << std::endl;
    }
    return 3; // Coverage error
  }

  // Merge
  try
  {
    if (!opts.MergedOutput.empty())
    {
      if (opts.isAtLeastLevel(VerboseLevel::Info))
      {
        std::cout << "Merge into " << opts.MergedOutput << std::endl;
      }

      {
        MergeRunner merge(opts);
        merge.execute();
      }

      // Also merge coverage collected by any sibling-bitness helper
      // processes. When -consolidate was used, auxiliaryOutputs is empty
      // here because everything has already been folded into our local
      // output file (which we just merged above).
      for (const auto& auxFile : auxiliaryOutputs)
      {
        if (!std::filesystem::exists(auxFile))
        {
          if (opts.isAtLeastLevel(VerboseLevel::Warning))
          {
            std::cerr << "Warning: expected auxiliary coverage file missing: " << auxFile << std::endl;
          }
          continue;
        }

        RuntimeOptions auxOpts = opts;
        auxOpts.OutputFile = auxFile;
        if (opts.isAtLeastLevel(VerboseLevel::Info))
        {
          std::cout << "Merging auxiliary coverage: " << auxFile << std::endl;
        }
        MergeRunner auxMerge(auxOpts);
        auxMerge.execute();
      }
    }
  }
  catch (const std::exception& e)
  {
    if (opts.isAtLeastLevel(VerboseLevel::Error))
    {
      std::cerr << "Error: " << e.what() << std::endl;
    }
    return 3; // Coverage error
  }
  return 0;
}
