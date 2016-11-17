#pragma once

#include <string>

struct RuntimeOptions
{
private:
	RuntimeOptions() :
		Quiet(false),
		ExportFormat(Native)
	{}

public:
	static RuntimeOptions& Instance()
	{
		static RuntimeOptions instance;
		return instance;
	}

	bool Quiet;

	enum ExportFormatType
	{
		Native,
		Cobertura

	} ExportFormat;


	std::string OutputFile;

	std::string CodePath;
	std::string Executable;
	std::string ExecutableArguments;
};