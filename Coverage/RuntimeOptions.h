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

	std::string SourcePath()
	{
		if (sourcePath.empty())
		{
			if (CodePath.size() != 0)
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
			else
			{
				sourcePath = CodePath;
			}
		}
		return sourcePath;
	}
};