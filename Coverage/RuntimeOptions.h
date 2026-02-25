#pragma once

#include <list>
#include <string>
#include <vector>

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
  RuntimeOptions() :
    UseStaticCodeAnalysis(false),
    ExportFormat(Native)
  {}
  virtual ~RuntimeOptions() = default;
  
  VerboseLevel _verboseLevel = VerboseLevel::Trace;

  bool UseStaticCodeAnalysis;

  enum ExportFormatType
  {
    Native,
    NativeV2,
    Cobertura,
    Clover
  } ExportFormat = Native;


  std::string OutputFile;

  std::string MergedOutput;
  std::string WorkingDirectory;
  std::list<std::string> CodePaths;
  std::string Executable;
  std::string ExecutableArguments;
  std::string PackageName = "Program.exe";
  std::string SolutionPath;
  std::vector<std::string> excludeFilter;

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