#include "SimSd.h"

std::filesystem::path SimSd::rootPath;

namespace {
std::filesystem::path findSdRootFrom(std::filesystem::path current) {
  for (;;) {
    const std::filesystem::path candidate = current / "data" / "sd";
    if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) {
      return candidate;
    }
    if (current == current.root_path()) {
      break;
    }
    current = current.parent_path();
  }
  return {};
}
}  // namespace

bool SimSd::begin(const char* executablePath) {
  rootPath = findSdRootFrom(std::filesystem::current_path());
  if (!rootPath.empty()) {
    return true;
  }

  if (executablePath != nullptr && executablePath[0] != '\0') {
    std::filesystem::path exePath(executablePath);
    if (exePath.has_filename()) {
      exePath = exePath.parent_path();
    }
    rootPath = findSdRootFrom(std::filesystem::absolute(exePath));
  }
  return !rootPath.empty();
}

const std::filesystem::path& SimSd::root() {
  return rootPath;
}

std::filesystem::path SimSd::resolve(const std::string& sdPath) {
  if (rootPath.empty() || sdPath.empty() || sdPath[0] != '/') {
    return {};
  }
  return rootPath / sdPath.substr(1);
}
