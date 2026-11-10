// rc/core/build_info.hpp
//
// What a binary was built with, from lesson 04-10, graduated.
//
// A speed, a crash or a number from a log means nothing without the build that produced it. One line carries it: "gcc-11 c++17 debug none 64bit little". Built
// here, the default build read gcc-11 c++17 debug none, the address sanitizer build read gcc-11 c++17 debug address (found through the __SANITIZE_ADDRESS__ macro), and clang
// read clang-14. A sanitizer slows a program by an amount that depends on what it does, so speeds are comparable only between builds with the same compiler, version, optimisation and
// pointer width, and neither under a sanitizer; a build that is not optimised, or has a sanitizer, says why its speed cannot be trusted.

#ifndef RC_CORE_BUILD_INFO_HPP
#define RC_CORE_BUILD_INFO_HPP

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace rc {
namespace core {

// What a binary was built with. Every field is something that changes what the program does or
// how fast it does it.
struct BuildInfo {
  std::string compiler;        // "gcc", "clang" or "msvc"
  int version = 0;             // the major version, or _MSC_VER for msvc
  int standard = 17;           // the C++ standard, 17 for C++17
  bool optimized = false;      // true when NDEBUG is set, which is what a release build does
  std::string sanitizer;       // "none", "address" or "thread"
  int pointer_bits = 64;
  bool little_endian = true;
};

// The build this code was compiled into. Given.
inline BuildInfo current_build() {
  BuildInfo b;

#if defined(__clang__)
  b.compiler = "clang";
  b.version = __clang_major__;
#elif defined(_MSC_VER)
  b.compiler = "msvc";
  b.version = _MSC_VER;
#elif defined(__GNUC__)
  b.compiler = "gcc";
  b.version = __GNUC__;
#else
  b.compiler = "unknown";
#endif

#if defined(_MSVC_LANG)
  b.standard = static_cast<int>((_MSVC_LANG - 200000L) / 100L);     // 201703 is C++17
#else
  b.standard = static_cast<int>((__cplusplus - 200000L) / 100L);
#endif

#if defined(NDEBUG)
  b.optimized = true;
#endif

  b.sanitizer = "none";
#if defined(__SANITIZE_ADDRESS__)
  b.sanitizer = "address";
#elif defined(__SANITIZE_THREAD__)
  b.sanitizer = "thread";
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer)
  b.sanitizer = "address";
#  elif __has_feature(thread_sanitizer)
  b.sanitizer = "thread";
#  endif
#endif

  b.pointer_bits = static_cast<int>(sizeof(void*) * 8);

  // Found by looking at the bytes of a number, not by asking a macro.
  const unsigned int one = 1;
  b.little_endian = *reinterpret_cast<const unsigned char*>(&one) == 1;
  return b;
}

// One line a person can read and a program can read back, such as
//   gcc-11 c++17 release none 64bit little
inline std::string describe(const BuildInfo& b) {
  std::string line = b.compiler + "-" + std::to_string(b.version);
  line += " c++" + std::to_string(b.standard);
  line += b.optimized ? " release" : " debug";
  line += " " + b.sanitizer;
  line += " " + std::to_string(b.pointer_bits) + "bit";
  line += b.little_endian ? " little" : " big";
  return line;
}

namespace build_detail {

inline bool read_int(const std::string& s, int* out) {
  if (s.empty() || s.size() > 9) return false;
  for (char c : s) {
    if (c < '0' || c > '9') return false;
  }
  *out = std::stoi(s);
  return true;
}

// Split a line at single spaces. Two spaces in a row, or a space at either end, make an empty piece,
// which is how a malformed line is noticed.
inline std::vector<std::string> split(const std::string& line) {
  std::vector<std::string> parts;
  std::string::size_type start = 0;
  while (true) {
    const std::string::size_type space = line.find(' ', start);
    if (space == std::string::npos) {
      parts.push_back(line.substr(start));
      break;
    }
    parts.push_back(line.substr(start, space - start));
    start = space + 1;
  }
  return parts;
}

}  // namespace build_detail

// Read back what describe wrote. False, leaving *out alone, for anything that is not exactly that format.
inline bool parse_build(const char* text, BuildInfo* out) {
  const std::vector<std::string> p = build_detail::split(text);
  if (p.size() != 6) return false;

  BuildInfo b;

  const std::string::size_type dash = p[0].rfind('-');
  if (dash == std::string::npos || dash == 0) return false;
  b.compiler = p[0].substr(0, dash);
  if (b.compiler != "gcc" && b.compiler != "clang" && b.compiler != "msvc" && b.compiler != "unknown") return false;
  if (!build_detail::read_int(p[0].substr(dash + 1), &b.version)) return false;

  if (p[1].size() < 4 || p[1].compare(0, 3, "c++") != 0) return false;
  if (!build_detail::read_int(p[1].substr(3), &b.standard)) return false;

  if (p[2] == "release") b.optimized = true;
  else if (p[2] == "debug") b.optimized = false;
  else return false;

  if (p[3] != "none" && p[3] != "address" && p[3] != "thread") return false;
  b.sanitizer = p[3];

  if (p[4].size() < 4 || p[4].compare(p[4].size() - 3, 3, "bit") != 0) return false;
  if (!build_detail::read_int(p[4].substr(0, p[4].size() - 3), &b.pointer_bits)) return false;

  if (p[5] == "little") b.little_endian = true;
  else if (p[5] == "big") b.little_endian = false;
  else return false;

  *out = b;
  return true;
}

// May a speed measured on one of these be compared with a speed measured on the other?
//
// Only if it is the same compiler and version, both optimised or both not, and neither carries a
// sanitizer: a sanitizer slows a program by an amount that depends on what it does, so a time
// measured under one says nothing about the same code without it.
inline bool comparable_for_speed(const BuildInfo& a, const BuildInfo& b) {
  if (a.sanitizer != "none" || b.sanitizer != "none") return false;
  if (a.compiler != b.compiler || a.version != b.version) return false;
  if (a.optimized != b.optimized) return false;
  return a.pointer_bits == b.pointer_bits;
}

// Reasons not to trust a speed measured on this build, one string each. Empty for a build that is fit to time.
inline std::vector<std::string> speed_warnings(const BuildInfo& b) {
  std::vector<std::string> warnings;
  if (!b.optimized) warnings.push_back("not optimised: assertions are on and the compiler has not tried to be fast");
  if (b.sanitizer != "none") warnings.push_back("built with the " + b.sanitizer + " sanitizer, which slows it down");
  return warnings;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_BUILD_INFO_HPP
