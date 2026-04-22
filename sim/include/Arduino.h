#ifndef SIM_ARDUINO_H
#define SIM_ARDUINO_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>

template <typename T, typename U>
constexpr auto max(T a, U b) {
  return a < b ? b : a;
}

template <typename T, typename U>
constexpr auto min(T a, U b) {
  return a < b ? a : b;
}

class String : public std::string {
public:
  using std::string::string;

  String() = default;
  String(const std::string& other) : std::string(other) {}
  String(const char* other) : std::string(other != nullptr ? other : "") {}
  String(unsigned value) : std::string(std::to_string(value)) {}
  String(int value) : std::string(std::to_string(value)) {}
  String(size_t value) : std::string(std::to_string(value)) {}

  size_t length() const { return size(); }

  int compareTo(const String& other) const {
    if (*this < other) return -1;
    if (*this > other) return 1;
    return 0;
  }

  void toLowerCase() {
    std::transform(begin(), end(), begin(), [](unsigned char ch) {
      return static_cast<char>(std::tolower(ch));
    });
  }

  int indexOf(const String& needle) const {
    const size_t pos = find(needle);
    return pos == npos ? -1 : static_cast<int>(pos);
  }

  void remove(size_t index) {
    if (index < size()) {
      erase(index);
    }
  }

  void remove(size_t index, size_t count) {
    if (index < size()) {
      erase(index, count);
    }
  }
};

inline String operator+(const String& lhs, const String& rhs) {
  return String(static_cast<const std::string&>(lhs) + static_cast<const std::string&>(rhs));
}

inline String operator+(const String& lhs, const char* rhs) {
  return String(static_cast<const std::string&>(lhs) + std::string(rhs != nullptr ? rhs : ""));
}

inline String operator+(const char* lhs, const String& rhs) {
  return String(std::string(lhs != nullptr ? lhs : "") + static_cast<const std::string&>(rhs));
}

#endif
