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
};