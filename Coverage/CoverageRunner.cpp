#include "CoverageRunner.h"

#include <string>
//---------------------------------------------------------------------------------------

SourceManager CoverageRunner::_sources;

//---------------------------------------------------------------------------------------

void SourceManager::setupExcludeFilter(const RuntimeOptions& opts)
{
  _excludeFilter.clear();
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

void SourceManager::searchRealPath(std::filesystem::path& finalPath, const std::filesystem::path& original, bool& exclude) const
{
  // Search if original path exist into valid filter
  bool isInGoodPath = false;
	for (const auto& codepath : RuntimeOptionsSingleton::Instance().CodePaths)
	{
		if (original.string().starts_with(codepath.string()))
		{
			isInGoodPath = true;
			break;
		}
	}

  // If file not exists on disk but path look like good, return missing file
  if ( isInGoodPath && !std::filesystem::exists(original) )
  {
		finalPath = std::filesystem::path();
		exclude = false;
    return;
  }

  // Try to remap path based on CodePath (happens when CI run coverage on different disk or path )
	for (const auto& codepath : RuntimeOptionsSingleton::Instance().CodePaths)
	{
		// Try to reinterpret path (file from another server ?)
		finalPath = original;
		auto allFolders = finalPath.parent_path();
		// Start with filename
		finalPath = finalPath.filename();
		const auto source = codepath;

		while (!allFolders.filename().string().empty())
		{
			auto testPath = source / finalPath;
			if (std::filesystem::exists(testPath))
			{
				finalPath = testPath;
        exclude = false;
        return;
			}
			else
			{
				finalPath = allFolders.filename() / finalPath;
				allFolders = allFolders.parent_path();
			}
		}
		// If found nothing, reset path and consider exclude file
		finalPath = std::filesystem::path();
    exclude = true;
	}
}

SourceManager::SearchResult SourceManager::searchFromCodePath(const PSRCCODEINFO& lineInfo, const FileCallbackInfo& fileInfo, std::filesystem::path& finalPath)
{
  SearchResult result;
  const auto originalPath = std::filesystem::path(lineInfo->FileName);
  const auto itPath = _conversion.find(originalPath);
  if (itPath != _conversion.cend())
  {
    finalPath = itPath->second;
  }
  else
  {
    result.isNew = true;
    finalPath = std::filesystem::path();

    // Search file is not inside exclude list and into CodePaths range
    if ( !isExcluded(originalPath) )
    {
      searchRealPath(finalPath, originalPath, result.isExcluded);
    }
    else
    {
      result.isExcluded = true;
    }

    // Save already meet path
    _conversion.emplace( originalPath, finalPath );
  }
  result.isFound = !finalPath.empty();

  return result;
}