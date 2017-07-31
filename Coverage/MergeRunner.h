#pragma once

#include "RuntimeOptions.h"

#include <assert.h>
#include <experimental/filesystem> // C++ 17  feature
#include <fstream>
#include <map>

class MergeRunner
{
private:
    RuntimeOptions _options;    ///< Copy local of option.

    struct Profile 
    {
        std::string res;
        std::string prof;
    };

    typedef std::map<std::string, Profile> DictCoverage;

    DictCoverage makeDictionary(const std::string& filename)
    {
        DictCoverage dictOutput;

        std::fstream outputFile(filename.c_str(), std::fstream::in);
        if (!outputFile.is_open())
        {
            const std::string msg = "Merge failure: Impossible to open file: " + filename;
            throw std::exception(msg.c_str());
        }

        std::string buffer;
        while (std::getline(outputFile, buffer))
        {
            //Search FILE:
            if (buffer.find_first_of("FILE:") != std::string::npos)
            {
                std::string filename = buffer.substr(6);

                Profile profile;
                
                // Read next line (we suppose file is not corrupt)
                std::getline(outputFile, buffer);
                assert(buffer.find_first_of("RES:") != std::string::npos);
                profile.res = buffer.substr(5);

                std::getline(outputFile, buffer);
                assert(buffer.find_first_of("PROF:") != std::string::npos);
                profile.prof = buffer.substr(6);

                dictOutput[filename] = profile;
            }
        }
        outputFile.close();
        return dictOutput;
    }

    void merge(const DictCoverage& dictOutput, DictCoverage& dictMerge)
    {
        auto& itOutput = dictOutput.cbegin();
        while(itOutput != dictOutput.cend())
        {
            auto itMerge = dictMerge.find(itOutput->first);
            if(itMerge != dictMerge.end())
            {
                if(itMerge->second.res.size() == itOutput->second.res.size())
                {
                    auto src = itOutput->second.res.cbegin();
                    for(auto& dst : itMerge->second.res)
                    {
                        // Rules: c > p > u > _
                        // In ASCII or UTF8  c < p < u < _ !!!
                        if(dst > *src)
                            dst = *src; // So easy !
                        ++src;
                    }
                }
                else
                {
                    // Source is different from both version ?
                    std::cerr << "Merge warning: impossible to merge " << itOutput->first << ": size between src/dst is not same." << std::endl;
                }
            }
            else
            {
                dictMerge[itOutput->first] = itOutput->second;
            }

            ++itOutput;
        }
    }

public:
    /// Constructor
    /// \param[in] opts: application option. Need MergedOutput and OutputFile valid and defined + ExportFormat MUST BE Native.
    MergeRunner(const RuntimeOptions& opts):
    _options(opts)
    {
        assert(!_options.MergedOutput.empty());
        assert(!_options.OutputFile.empty());
        assert(_options.ExportFormat == RuntimeOptions::Native); // Support only this !
    }

    /// Run merge
    void execute()
    {
        std::experimental::filesystem::path outputPath(_options.OutputFile);
        std::experimental::filesystem::path mergedPath(_options.MergedOutput);
        
        // Check we have data
        if(!std::experimental::filesystem::exists(outputPath))
        {
            const std::string msg = "Merge failure: Impossible to find output file: " + _options.OutputFile;
            throw std::exception(msg.c_str());
        }

        // Nothing to merge = Copy and quit
        if (!std::experimental::filesystem::exists(mergedPath))
        {
            std::experimental::filesystem::copy(outputPath, mergedPath);
            return;
        }

        // ---- Make merge ---------------------------------------------------------------
        // Step 1: Parse output files and define a dictionary
        DictCoverage dictOutput = makeDictionary(_options.OutputFile);
        DictCoverage dictMerge  = makeDictionary(_options.MergedOutput);

        // Step 2: Parse merge
        merge(dictOutput, dictMerge);

        // Step 3: Write dictionary (on empty file)
        std::fstream mergeFile(_options.MergedOutput.c_str(), std::fstream::out);
        for(const auto& cover : dictMerge)
        {
            mergeFile << "FILE: " << cover.first << std::endl;
            mergeFile << "RES: " << cover.second.res << std::endl;
            mergeFile << "PROF: " << cover.second.prof << std::endl;
        }
        mergeFile.close();
    }
};