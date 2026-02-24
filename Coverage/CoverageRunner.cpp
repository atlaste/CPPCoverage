#include "CoverageRunner.h"

//---------------------------------------------------------------------------------------

SourceManager CoverageRunner::_sources;

//---------------------------------------------------------------------------------------

bool SourceManager::searchFromCodePath(const PSRCCODEINFO& lineInfo, const FileCallbackInfo& fileInfo, std::filesystem::path& finalPath) const
{
  const auto itPath = _conversion.find(lineInfo->FileName);
  if (itPath != _conversion.cend())
  {
    finalPath = itPath->second;
    return true;
  }
  else
  {
    bool found = fileInfo.PathMatches(lineInfo->FileName);
    if (!found)
    {
      for (const auto& codepath : RuntimeOptionsSingleton::Instance().CodePaths)
      {
        // Try to reinterpret path (file from another server ?)
        finalPath = std::filesystem::path(lineInfo->FileName);
        auto allFolders = finalPath.parent_path();
        // Start with filename
        finalPath = finalPath.filename();
        const auto source = std::filesystem::path(codepath);

        while (!allFolders.filename().string().empty())
        {
          auto testPath = source / finalPath;
          if (std::filesystem::exists(testPath))
          {
            finalPath = testPath;
            return true;
          }
          else
          {
            finalPath = allFolders.filename() / finalPath;
            allFolders = allFolders.parent_path();
          }
        }
      }
    }
    else
    {
      finalPath = lineInfo->FileName;
    }
    return found;
  }
}