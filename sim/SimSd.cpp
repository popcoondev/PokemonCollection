#include "SimSd.h"

#include <filesystem>

#include "SD.h"

namespace fs = std::filesystem;

namespace {
std::string gRootPath;

std::string findSdRootFrom(fs::path current) {
  while (!current.empty()) {
    const fs::path candidate = current / "data" / "sd";
    if (fs::exists(candidate) && fs::is_directory(candidate)) {
      return candidate.string();
    }
    const fs::path parent = current.parent_path();
    if (parent == current) {
      break;
    }
    current = parent;
  }
  return {};
}
}

namespace SimSd {

bool begin() {
  return begin(nullptr);
}

bool begin(const char* anchorPath) {
  if (!gRootPath.empty()) {
    return true;
  }
  gRootPath = findSdRootFrom(fs::current_path());
  if (gRootPath.empty() && anchorPath != nullptr && anchorPath[0] != '\0') {
    fs::path anchor(anchorPath);
    if (!anchor.is_absolute()) {
      anchor = fs::absolute(anchor);
    }
    gRootPath = findSdRootFrom(anchor.parent_path());
  }
  return !gRootPath.empty();
}

std::string rootPath() {
  return gRootPath;
}

std::string resolvePath(const char* path) {
  if (gRootPath.empty() || path == nullptr) {
    return {};
  }
  std::string relative(path);
  if (!relative.empty() && relative.front() == '/') {
    relative.erase(relative.begin());
  }
  return (fs::path(gRootPath) / relative).string();
}

}

File::File(const std::string& actualPath, bool writeMode, const std::string& virtualPath) {
  actualPath_ = actualPath;
  virtualPath_ = virtualPath.empty() ? actualPath : virtualPath;
  isDirectory_ = fs::exists(actualPath_) && fs::is_directory(actualPath_);
  if (isDirectory_) {
    dirIter_ = std::make_shared<fs::directory_iterator>(actualPath_);
    dirEnd_ = std::make_shared<fs::directory_iterator>();
    return;
  }
  std::ios::openmode mode = std::ios::binary;
  mode |= writeMode ? (std::ios::out | std::ios::trunc) : std::ios::in;
  stream_ = std::make_shared<std::fstream>();
  stream_->open(actualPath, mode);
}

File::operator bool() const {
  return isDirectory_ || (stream_ && stream_->is_open());
}

bool File::isDirectory() const {
  return isDirectory_;
}

int File::read() {
  if (!stream_ || !stream_->is_open()) {
    return -1;
  }
  const int value = stream_->get();
  return stream_->good() || value != EOF ? value : -1;
}

size_t File::read(uint8_t* buffer, size_t length) {
  if (!stream_ || !stream_->is_open() || buffer == nullptr || length == 0) {
    return 0;
  }
  stream_->read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(length));
  return static_cast<size_t>(stream_->gcount());
}

size_t File::write(uint8_t value) {
  return write(&value, 1);
}

size_t File::write(const uint8_t* buffer, size_t length) {
  if (!stream_ || !stream_->is_open() || buffer == nullptr || length == 0) {
    return 0;
  }
  stream_->write(reinterpret_cast<const char*>(buffer), static_cast<std::streamsize>(length));
  return stream_->good() ? length : 0;
}

size_t File::available() {
  if (!stream_ || !stream_->is_open()) {
    return 0;
  }
  const std::streampos current = stream_->tellg();
  stream_->seekg(0, std::ios::end);
  const std::streampos end = stream_->tellg();
  stream_->seekg(current);
  return current < end ? static_cast<size_t>(end - current) : 0;
}

size_t File::size() {
  if (!stream_ || !stream_->is_open()) {
    return 0;
  }
  const std::streampos current = stream_->tellg();
  stream_->seekg(0, std::ios::end);
  const std::streampos end = stream_->tellg();
  stream_->seekg(current);
  return static_cast<size_t>(end);
}

bool File::seek(size_t position) {
  if (!stream_ || !stream_->is_open()) {
    return false;
  }
  stream_->seekg(static_cast<std::streamoff>(position), std::ios::beg);
  stream_->seekp(static_cast<std::streamoff>(position), std::ios::beg);
  return stream_->good();
}

size_t File::position() {
  if (!stream_ || !stream_->is_open()) {
    return 0;
  }
  return static_cast<size_t>(stream_->tellg());
}

String File::path() const {
  return String(virtualPath_);
}

File File::openNextFile() {
  if (!isDirectory_ || !dirIter_ || !dirEnd_ || *dirIter_ == *dirEnd_) {
    return File();
  }
  const fs::directory_entry entry = dirIter_->operator*();
  const fs::path nextPath = entry.path();
  ++(*dirIter_);
  std::string childVirtualPath;
  if (!virtualPath_.empty() && virtualPath_[0] == '/') {
    childVirtualPath = virtualPath_;
    if (!childVirtualPath.empty() && childVirtualPath.back() != '/') {
      childVirtualPath += "/";
    }
    childVirtualPath += nextPath.filename().string();
  }
  return File(nextPath.string(), false, childVirtualPath);
}

void File::close() {
  if (stream_ && stream_->is_open()) {
    stream_->close();
  }
}

std::string File::contents() {
  if (!stream_ || !stream_->is_open()) {
    return {};
  }
  const std::streampos current = stream_->tellg();
  stream_->seekg(0, std::ios::end);
  const std::streampos end = stream_->tellg();
  stream_->seekg(0, std::ios::beg);
  std::string data;
  data.resize(static_cast<size_t>(end));
  stream_->read(data.data(), end);
  stream_->clear();
  stream_->seekg(current);
  return data;
}

SimSDClass SD;

bool SimSDClass::begin() {
  return SimSd::begin();
}

bool SimSDClass::exists(const char* path) const {
  const std::string resolved = SimSd::resolvePath(path);
  return !resolved.empty() && fs::exists(resolved);
}

File SimSDClass::open(const char* path, int mode) const {
  const std::string resolved = SimSd::resolvePath(path);
  return File(resolved, mode == FILE_WRITE, path != nullptr ? path : "");
}

bool SimSDClass::mkdir(const char* path) const {
  const std::string resolved = SimSd::resolvePath(path);
  return !resolved.empty() && fs::create_directories(resolved);
}

bool SimSDClass::remove(const char* path) const {
  const std::string resolved = SimSd::resolvePath(path);
  return !resolved.empty() && fs::remove(resolved);
}

std::filesystem::path SimSDClass::resolvePath(const char* path) const {
  const std::string resolved = SimSd::resolvePath(path);
  return resolved.empty() ? std::filesystem::path() : std::filesystem::path(resolved);
}
