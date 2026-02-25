#include "CoverageRunner.h"

//---------------------------------------------------------------------------------------

SourceManager CoverageRunner::_sources;

//---------------------------------------------------------------------------------------

void SourceManager::setupExcludeFilter(const RuntimeOptions& opts)
{
  for (const auto& filter : opts.excludeFilter)
  {
    _excludeFilter.emplace_back(std::regex(filter, std::regex_constants::ECMAScript /*| std::regex_constants::icase*/));
  }
}

bool SourceManager::isExcluded(const std::filesystem::path& originalPath) const
{
  for (const auto& filter : _excludeFilter)
  {
    if (std::regex_search(originalPath.string(), filter))
    {
      return true;
    }
  }
  return false;
}

bool SourceManager::searchFromCodePath(const PSRCCODEINFO& lineInfo, const FileCallbackInfo& fileInfo, std::filesystem::path& finalPath)
{
  const auto originalPath = std::filesystem::path(lineInfo->FileName);
  const auto itPath = _conversion.find(originalPath);
  if (itPath != _conversion.cend())
  {
    finalPath = itPath->second;
    return !finalPath.empty();
  }
  else
  {
    finalPath = std::filesystem::path();

    // Search file is inside exclude list
    if (!isExcluded(originalPath))
    {
      if (!fileInfo.PathMatches(lineInfo->FileName))
      {
        const auto& search = [&]()
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
                return;
              }
              else
              {
                finalPath = allFolders.filename() / finalPath;
                allFolders = allFolders.parent_path();
              }
            }
            // If found nothing, reset path
            finalPath = std::filesystem::path();
          }
        };
        search();
      }
      else
      {
        if( std::filesystem::exists(originalPath) )
        {
          finalPath = originalPath;
        }
      }
    }

    // Save already meet path
    _conversion.emplace( originalPath, finalPath );

    return !finalPath.empty();
  }
}