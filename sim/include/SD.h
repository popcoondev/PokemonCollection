#ifndef SIM_SD_H_WRAPPER
#define SIM_SD_H_WRAPPER

#include "FS.h"
#include "SimSd.h"

constexpr int FILE_READ = 0;

class SDClass {
public:
  std::filesystem::path resolvePath(const char* path) const {
    return SimSd::resolve(path != nullptr ? path : "");
  }

  bool exists(const char* path) const {
    const auto resolved = resolvePath(path);
    return !resolved.empty() && std::filesystem::exists(resolved);
  }

  File open(const char* path, int mode = FILE_READ) const {
    (void)mode;
    const auto resolved = resolvePath(path);
    return File(resolved);
  }
};

inline SDClass SD;

#endif
