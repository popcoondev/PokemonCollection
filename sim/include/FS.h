#ifndef SIM_FS_H
#define SIM_FS_H

#include <cstddef>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

class File {
public:
  File() = default;
  explicit File(const std::filesystem::path& path) { open(path); }

  bool open(const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
      valid = false;
      data.clear();
      position = 0;
      return false;
    }
    data.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    position = 0;
    valid = true;
    return true;
  }

  size_t read(uint8_t* buffer, size_t length) {
    if (!valid || buffer == nullptr) return 0;
    const size_t remaining = data.size() - std::min(position, data.size());
    const size_t count = std::min(length, remaining);
    if (count == 0) return 0;
    memcpy(buffer, data.data() + position, count);
    position += count;
    return count;
  }

  void close() {
    valid = false;
    data.clear();
    position = 0;
  }

  explicit operator bool() const { return valid; }
  const std::string& contents() const { return data; }

private:
  std::string data;
  size_t position = 0;
  bool valid = false;
};

#endif
