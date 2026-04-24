#ifndef SIM_ARDUINO_H
#define SIM_ARDUINO_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include <SDL.h>

class String {
public:
  String() = default;
  String(const char* value) : value_(value != nullptr ? value : "") {}
  String(const std::string& value) : value_(value) {}
  String(int value) : value_(std::to_string(value)) {}
  String(unsigned value) : value_(std::to_string(value)) {}
  String(long value) : value_(std::to_string(value)) {}
  String(unsigned long value) : value_(std::to_string(value)) {}

  size_t length() const { return value_.size(); }
  const char* c_str() const { return value_.c_str(); }

  void toLowerCase() {
    std::transform(value_.begin(), value_.end(), value_.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
  }

  int compareTo(const String& other) const {
    if (value_ == other.value_) return 0;
    return value_ < other.value_ ? -1 : 1;
  }

  bool startsWith(const char* prefix) const {
    const std::string p = prefix != nullptr ? prefix : "";
    return value_.rfind(p, 0) == 0;
  }

  bool endsWith(const char* suffix) const {
    const std::string s = suffix != nullptr ? suffix : "";
    return value_.size() >= s.size() && value_.compare(value_.size() - s.size(), s.size(), s) == 0;
  }

  int indexOf(const String& needle) const {
    const std::size_t pos = value_.find(needle.value_);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }

  int lastIndexOf(char ch) const {
    const std::size_t pos = value_.find_last_of(ch);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }

  String substring(size_t from) const {
    return from >= value_.size() ? String("") : String(value_.substr(from));
  }

  String substring(size_t from, size_t to) const {
    if (from >= value_.size() || to <= from) return String("");
    return String(value_.substr(from, to - from));
  }

  void remove(size_t index) {
    if (index < value_.size()) {
      value_.erase(index);
    }
  }

  void remove(size_t index, size_t count) {
    if (index < value_.size()) {
      value_.erase(index, count);
    }
  }

  String& operator=(const char* value) {
    value_ = value != nullptr ? value : "";
    return *this;
  }

  String& operator+=(const String& rhs) {
    value_ += rhs.value_;
    return *this;
  }

  String& operator+=(const char* rhs) {
    value_ += (rhs != nullptr ? rhs : "");
    return *this;
  }

  bool operator==(const char* rhs) const {
    return value_ == (rhs != nullptr ? rhs : "");
  }

  bool operator!=(const char* rhs) const {
    return !(*this == rhs);
  }

  bool operator==(const String& rhs) const {
    return value_ == rhs.value_;
  }

  bool operator!=(const String& rhs) const {
    return value_ != rhs.value_;
  }

  bool operator<(const String& rhs) const {
    return value_ < rhs.value_;
  }

  friend String operator+(const String& lhs, const String& rhs) {
    return String(static_cast<std::string>(lhs) + static_cast<std::string>(rhs));
  }

  friend String operator+(const String& lhs, const char* rhs) {
    return String(static_cast<std::string>(lhs) + (rhs != nullptr ? rhs : ""));
  }

  friend String operator+(const char* lhs, const String& rhs) {
    return String((lhs != nullptr ? lhs : "") + static_cast<std::string>(rhs));
  }

  char operator[](size_t index) const {
    return value_[index];
  }

  operator std::string() const { return value_; }

private:
  std::string value_;
};

inline long random(long minValue, long maxValue) {
  if (maxValue <= minValue) return minValue;
  return minValue + (std::rand() % (maxValue - minValue));
}

inline unsigned long millis() {
  return static_cast<unsigned long>(SDL_GetTicks());
}

template <typename T, typename U, typename V>
inline auto constrain(T value, U low, V high) -> std::common_type_t<T, U, V> {
  using R = std::common_type_t<T, U, V>;
  const R rv = static_cast<R>(value);
  const R rl = static_cast<R>(low);
  const R rh = static_cast<R>(high);
  return std::min(std::max(rv, rl), rh);
}

template <typename T>
inline T max(T lhs, T rhs) {
  return std::max(lhs, rhs);
}

template <typename T>
inline T min(T lhs, T rhs) {
  return std::min(lhs, rhs);
}

#endif
