#pragma once

#include <string>
#include <unordered_set>

#include <Windows.h>

enum class VerboseLevel
{
  Error = 0x01,
  Warning = 0x03,
  Info = 0x07,
  Trace = 0x0F,
  None = 0
};

struct RuntimeOptions
{
  explicit RuntimeOptions() = default;
  virtual ~RuntimeOptions() = default;

  VerboseLevel _verboseLevel = VerboseLevel::Trace;

  bool UseStaticCodeAnalysis = false;

  enum class ExportFormatType
  {
    Native,
    NativeV2,
    Cobertura,
    Clover
  } ExportFormat = ExportFormatType::Native;


  std::string OutputFile;

  std::string MergedOutput;
  std::string WorkingDirectory;
  std::unordered_set<std::string> CodePaths;
  std::string Executable;
  std::string ExecutableArguments;
  std::string PackageName = "Program.exe";
  std::string SolutionPath;

  // When Attach is true, the coverage runner will attach (DebugActiveProcess)
  // to an already-running process identified by AttachPid instead of launching
  // a new process with CreateProcess. This is primarily used internally when
  // a mismatched-bitness child process is spawned by the main target (e.g.
  // vstest.console.exe (x64) spawning testhost.x86.exe (x86)); the parent
  // Coverage-x64.exe then launches Coverage-x86.exe with -attach <pid> to
  // instrument the x86 child.
  bool Attach = false;
  DWORD AttachPid = 0;

  // When true, after the coverage run finishes and any sibling-bitness
  // CPPCoverage helper processes have produced their own .cov files, the
  // master process merges those files into its own -o output file and
  // deletes them. Only supported for Native / NativeV2 export formats
  // (the same restriction that applies to -m / MergedOutput).
  bool ConsolidateAuxiliary = false;

  bool isAtLeastLevel(const VerboseLevel& level) const { return (static_cast<int>(_verboseLevel) & static_cast<int>(level)) == static_cast<int>(level); }
};

struct RuntimeOptionsSingleton : public RuntimeOptions
{
private:
  RuntimeOptionsSingleton() = default;

public:
  ~RuntimeOptionsSingleton() override = default;

  static RuntimeOptions& Instance()
  {
    static RuntimeOptionsSingleton instance;
    return instance;
  }
};