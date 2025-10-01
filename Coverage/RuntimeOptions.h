#pragma once

#include <string>

struct RuntimeOptions
{
private:
	RuntimeOptions() :
		Quiet(false),
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

	bool Quiet;
	bool UseStaticCodeAnalysis;

	enum ExportFormatType
	{
		Native,
		NativeV2,
		Cobertura,
		Clover
	} ExportFormat;


	std::string OutputFile;

	std::string MergedOutput;
	std::string WorkingDirectory;
	std::string CodePath;
	std::string Executable;
	std::string ExecutableArguments;
	std::string PackageName = "Program.exe";
	std::string SolutionPath;
};