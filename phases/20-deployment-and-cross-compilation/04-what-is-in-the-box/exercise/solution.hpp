#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

struct FileData {
  std::string path;
  std::string content;
};

struct Entry {
  std::string path;
  std::uint64_t size = 0;
  std::uint64_t hash = 0;
};

struct Manifest {
  std::string version;
  std::vector<Entry> entries;
};

// FNV-1a over 64 bits: start at 14695981039346656037, and for each byte xor it in and multiply by 1099511628211. It catches accident and carelessness, and it is NOT a defence against someone who means it: a real release is signed (see "What this does not do").
inline std::uint64_t hash64(const std::string& bytes) {
  // TODO: FNV-1a 64: h = 14695981039346656037; for each byte: h ^= byte (as unsigned char); h *= 1099511628211
  (void)bytes;
  return 0;
}

inline std::string hex16(std::uint64_t v) {
  static const char* digits = "0123456789abcdef";
  std::string s(16, '0');
  for (int i = 15; i >= 0; --i) {
    s[static_cast<std::size_t>(i)] = digits[v & 0xF];
    v >>= 4;
  }
  return s;
}

// May this path be unpacked on the robot? It is relative (no leading '/'), has no backslash, colon, space, tab, newline or NUL, is at most 255 characters, and none of its components, split on '/', is empty, "." or "..".
inline bool safe_path(const std::string& p) {
  // TODO: the rules in the comment: not empty, at most 255, not starting with '/', no backslash colon space tab newline CR or NUL, and no empty, . or .. component when split on '/'
  (void)p;
  return false;
}

struct MakeResult {
  bool ok = false;
  std::string error;
  std::string text;
};

// The manifest of a release, as text, in this form (every line ending in a newline):
//   rcbundle 1
//   version <version>
//   file <path> <size in bytes, decimal> <hash, 16 lower case hex digits>      one per file, SORTED BY PATH, whatever order the files were given in
//   end <hash64 of all the text above, 16 hex digits>
// Refused (ok false, nothing in text) with the error "bad version" for a version that is empty or has a space, tab or newline; "unsafe path: <p>" for a path that safe_path refuses; "duplicate path: <p>" for a path given twice. Checked in that order, files in the order given.
inline MakeResult make_manifest(const std::string& version, std::vector<FileData> files) {
  // TODO: the form and the three refusals in the comment above, checked in that order; files sorted by path; the end line is hash64 of everything above it
  (void)version;
  (void)files;
  return MakeResult();
}

struct ParseResult {
  bool ok = false;
  std::string error;
  Manifest manifest;
};

// Read a manifest. The errors, by their exact text, found in this order:
//   "bad header"            the first line is not "rcbundle 1" (or there is none)
//   "bad version line"      the second line is not "version <v>" with a v that is not empty
//   "bad file line N"       a line from the third on that starts with "file " and is not  file <path> <decimal size> <16 hex digits>, N counting lines from 1
//   "unsafe path: <p>"      a path that safe_path refuses
//   "duplicate path: <p>"   a path twice
//   "no end line"           the text ends without an  end <16 hex>  line as its last line (a truncated file)
//   "bad checksum"          the end hash is not hash64 of all the text before the end line (the file was edited)
// Lines are checked as they come, so a bad file line is reported before a missing end.
inline ParseResult parse_manifest(const std::string& text) {
  // TODO: the seven errors in the comment above, found in that order; a last line with no newline is not a line. Fill manifest.version and the entries as they are read (size as a decimal number, hash from 16 hex digits)
  (void)text;
  return ParseResult();
}

enum class Mismatch { missing, extra, size, hash };

struct Difference {
  std::string path;
  Mismatch kind = Mismatch::missing;
};

// How the files on the robot differ from the manifest: for each manifest entry in order, `missing` if no file has that path, otherwise `size` if the size differs, otherwise `hash` if the hash differs; then each file that is not in the manifest as `extra`, in path order.
// If a path is given twice among the files, the first is the one looked at.
inline std::vector<Difference> verify_files(const Manifest& m, const std::vector<FileData>& files) {
  // TODO: as the comment above: missing, size, hash for each entry in order, then the extras in path order; the first of a repeated path is the one used
  (void)m;
  (void)files;
  return std::vector<Difference>();
}

#endif  // LESSON_SOLUTION_HPP
