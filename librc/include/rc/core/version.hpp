// rc/core/version.hpp
//
// Reading and ordering versions, from lesson 04-09, graduated.
//
// Versions compare as numbers, not as text: "1.9.0" < "1.10.0" is false as text and compare gives -1 as versions. A pre-release comes before its
// release, and its identifiers compare one at a time with numbers as numbers: "1.0.0-beta.11" < "1.0.0-beta.2" is true as text and compare gives 1. Eight
// versions shuffled and sorted with compare came out in the order the specification publishes. A number is compared as digits so a very long one cannot overflow.
// compatible follows the rule of CMake's SameMajorVersion: the same major, at least as new, with a stricter rule below 1.0.0, and no pre-release offered to a request
// for a release: asked for 1.4.0 among 1.2.0, 1.9.4, 2.0.1, 1.10.0, 1.11.0-rc.1 and 0.9.0, the newest that satisfied it was 1.10.0.

#ifndef RC_CORE_VERSION_HPP
#define RC_CORE_VERSION_HPP

#include <string>

namespace rc {
namespace core {

// A version of the form major.minor.patch, with an optional -prerelease and an optional +build.
struct Version {
  int major = 0;
  int minor = 0;
  int patch = 0;
  std::string pre;       // "rc.1", or empty for a release
};

namespace version_detail {

inline bool all_digits(const std::string& s) {
  if (s.empty()) return false;
  for (char c : s) {
    if (c < '0' || c > '9') return false;
  }
  return true;
}

// One number of a version: digits only, no leading zero unless it is exactly "0", and not absurdly long.
inline bool read_number(const std::string& s, int* out) {
  if (!all_digits(s) || s.size() > 9) return false;
  if (s.size() > 1 && s[0] == '0') return false;
  *out = std::stoi(s);
  return true;
}

// Compare two numbers written as digit strings without turning them into integers, so a very long
// one cannot overflow: a longer number is bigger, and equal lengths compare as text.
inline int compare_digits(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
  const int c = a.compare(b);
  return c < 0 ? -1 : (c > 0 ? 1 : 0);
}

}  // namespace version_detail

// Read "1.10.2", "1.0.0-rc.1" or "2.3.4+build7". The build part after + is allowed and ignored. Anything
// else is refused and leaves *out as it was: "1.2", "1.2.3.4", "1.x.0", "01.2.3", "-1.0.0", "1.0.0-".
inline bool parse_version(const char* text, Version* out) {
  std::string s(text);

  const std::string::size_type plus = s.find('+');
  if (plus != std::string::npos) s = s.substr(0, plus);

  Version v;
  const std::string::size_type dash = s.find('-');
  if (dash != std::string::npos) {
    v.pre = s.substr(dash + 1);
    s = s.substr(0, dash);
    if (v.pre.empty()) return false;
  }

  // Exactly three numbers separated by dots.
  const std::string::size_type d1 = s.find('.');
  if (d1 == std::string::npos) return false;
  const std::string::size_type d2 = s.find('.', d1 + 1);
  if (d2 == std::string::npos) return false;
  if (s.find('.', d2 + 1) != std::string::npos) return false;

  if (!version_detail::read_number(s.substr(0, d1), &v.major)) return false;
  if (!version_detail::read_number(s.substr(d1 + 1, d2 - d1 - 1), &v.minor)) return false;
  if (!version_detail::read_number(s.substr(d2 + 1), &v.patch)) return false;

  *out = v;
  return true;
}

// -1 if a is older than b, 0 if they are the same version, 1 if a is newer.
//
// The numbers compare as numbers. A pre-release comes BEFORE the release it leads up to, so 1.0.0-rc.1 is older
// than 1.0.0. Between two pre-releases the dot separated identifiers compare one at a time: a number is
// lower than a word, two numbers compare as numbers, two words compare as text, and if every identifier so
// far is equal the one with fewer identifiers is older.
inline int compare(const Version& a, const Version& b) {
  if (a.major != b.major) return a.major < b.major ? -1 : 1;
  if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
  if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;

  if (a.pre.empty() && b.pre.empty()) return 0;
  if (a.pre.empty()) return 1;       // a is the release, b is only a candidate for it
  if (b.pre.empty()) return -1;

  std::string::size_type ia = 0;
  std::string::size_type ib = 0;
  while (true) {
    const bool a_done = ia > a.pre.size();
    const bool b_done = ib > b.pre.size();
    if (a_done && b_done) return 0;
    if (a_done) return -1;
    if (b_done) return 1;

    std::string::size_type ea = a.pre.find('.', ia);
    if (ea == std::string::npos) ea = a.pre.size();
    std::string::size_type eb = b.pre.find('.', ib);
    if (eb == std::string::npos) eb = b.pre.size();
    const std::string xa = a.pre.substr(ia, ea - ia);
    const std::string xb = b.pre.substr(ib, eb - ib);

    const bool na = version_detail::all_digits(xa);
    const bool nb = version_detail::all_digits(xb);
    if (na && nb) {
      const int c = version_detail::compare_digits(xa, xb);
      if (c != 0) return c;
    } else if (na != nb) {
      return na ? -1 : 1;            // a number is lower than a word
    } else {
      const int c = xa.compare(xb);
      if (c != 0) return c < 0 ? -1 : 1;
    }

    ia = ea + 1;
    ib = eb + 1;
  }
}

// Does the version we have satisfy the version somebody asked for?
//
// The same rule as CMake's SameMajorVersion: the major number must match, and we must be at least as new. A
// change of major number is allowed to break things, so 2.0.0 does not satisfy a request for 1.4.0 even though
// it is newer. Below 1.0.0 nothing is promised, so for a request of 0.x the minor number must match too.
// A pre-release is only offered to somebody who asked for one: a candidate for 1.11.0 is newer than 1.4.0 and
// is still not what a request for a release should be given.
inline bool compatible(const Version& wanted, const Version& have) {
  if (have.major != wanted.major) return false;
  if (!have.pre.empty() && wanted.pre.empty()) return false;
  if (wanted.major == 0 && have.minor != wanted.minor) return false;
  return compare(have, wanted) >= 0;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_VERSION_HPP
