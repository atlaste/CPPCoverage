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
		Cobertura,
		Clover

	} ExportFormat;


	std::string OutputFile;

	std::string CodePath;
	std::string Executable;
	std::string ExecutableArguments;
};