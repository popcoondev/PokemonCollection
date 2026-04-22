#ifndef SIM_SD_H
#define SIM_SD_H

#include <filesystem>
#include <string>

class SimSd {
public:
  static bool begin(const char* executablePath = nullptr);
  static const std::filesystem::path& root();
  static std::filesystem::path resolve(const std::string& sdPath);

private:
  static std::filesystem::path rootPath;
};

#endif
