#pragma once

#include <string>
#include <memory>
#include <vector>

struct RuntimeCoverageFilter
{
  virtual bool IgnoreFile(const std::string& file) const = 0;
};

struct RuntimeNotifications
{
  std::vector<std::unique_ptr<RuntimeCoverageFilter>> postProcessing;

  std::string Trim(const std::string& t);
  std::string GetFQN(std::string s);
  void PrintInvalidNotification(const std::string& notification, const std::string_view information);
  void Handle(const char* data, const size_t size);
  bool IgnoreFile(const std::string& filename) const;
};