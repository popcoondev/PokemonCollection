#ifndef SIM_FS_H
#define SIM_FS_H

#include "Arduino.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

constexpr int FILE_READ = 0;
constexpr int FILE_WRITE = 1;

class File {
public:
  File() = default;
  explicit File(const std::string& actualPath, bool writeMode = false, const std::string& virtualPath = {});
  File(File&&) = default;
  File& operator=(File&&) = default;
  File(const File&) = default;
  File& operator=(const File&) = default;

  explicit operator bool() const;
  bool isDirectory() const;
  int read();
  size_t read(uint8_t* buffer, size_t length);
  size_t write(uint8_t value);
  size_t write(const uint8_t* buffer, size_t length);
  size_t available();
  size_t size();
  bool seek(size_t position);
  size_t position();
  String path() const;
  File openNextFile();
  void close();
  std::string contents();

private:
  std::filesystem::path actualPath_;
  std::string virtualPath_;
  bool isDirectory_ = false;
  std::shared_ptr<std::fstream> stream_;
  std::shared_ptr<std::filesystem::directory_iterator> dirIter_;
  std::shared_ptr<std::filesystem::directory_iterator> dirEnd_;
};

#endif
