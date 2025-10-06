#pragma once

#include <string>

enum class VerboseLevel
{
    Error   = 0x01,
	Warning = 0x03,
	Info    = 0x07,
	Trace   = 0x0F,
	None    = 0
}; 

struct RuntimeOptions
{
private:
	RuntimeOptions() :
		UseStaticCodeAnalysis(false),
		ExportFormat(Native)
	{}

	std::string sourcePath;

public:
	static RuntimeOptions& Instance()
	{
		static RuntimeOptions instance;
		return instance;
	}

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
	std::string CodePath;
	std::string Executable;
	std::string ExecutableArguments;
	std::string PackageName = "Program.exe";
	std::string SolutionPath;

	bool isAtLeastLevel(const VerboseLevel& level) const { return (static_cast<int>(_verboseLevel) & static_cast<int>(level)) == static_cast<int>(level);}
	std::string SourcePath()
	{
		if (sourcePath.empty() && !CodePath.empty())
		{
			auto idx = CodePath.find("x64");
			if (idx == std::string::npos)
			{
				idx = CodePath.find("Debug");
			}
			if (idx == std::string::npos)
			{
				idx = CodePath.find("Release");
			}
			if (idx == std::string::npos)
			{
				idx = CodePath.find('\\');
			}
			if (idx == std::string::npos)
			{
				throw "Cannot locate source file base for this executable";
			}
			sourcePath = CodePath.substr(0, idx);
		}
		return sourcePath;
	}
};