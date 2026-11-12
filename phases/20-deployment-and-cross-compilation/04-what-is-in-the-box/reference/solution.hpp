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
  std::uint64_t h = 14695981039346656037ULL;
  for (char c : bytes) {
    h ^= static_cast<unsigned char>(c);
    h *= 1099511628211ULL;
  }
  return h;
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
  if (p.empty() || p.size() > 255 || p[0] == '/') return false;
  for (char c : p)
    if (c == '\\' || c == ':' || c == ' ' || c == '\t' || c == '\n' || c == '\0' || c == '\r') return false;
  std::size_t start = 0;
  while (start <= p.size()) {
    const std::size_t slash = p.find('/', start);
    const std::string part = p.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
    if (part.empty() || part == "." || part == "..") return false;
    if (slash == std::string::npos) break;
    start = slash + 1;
  }
  return true;
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
  MakeResult r;
  if (version.empty() || version.find_first_of(" \t\n\r") != std::string::npos) {
    r.error = "bad version";
    return r;
  }
  std::set<std::string> seen;
  for (const FileData& f : files) {
    if (!safe_path(f.path)) {
      r.error = "unsafe path: " + f.path;
      return r;
    }
    if (!seen.insert(f.path).second) {
      r.error = "duplicate path: " + f.path;
      return r;
    }
  }
  std::sort(files.begin(), files.end(), [](const FileData& a, const FileData& b) { return a.path < b.path; });
  std::string text = "rcbundle 1\nversion " + version + "\n";
  for (const FileData& f : files) text += "file " + f.path + " " + std::to_string(f.content.size()) + " " + hex16(hash64(f.content)) + "\n";
  text += "end " + hex16(hash64(text)) + "\n";
  r.ok = true;
  r.text = text;
  return r;
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
  ParseResult r;
  std::vector<std::string> lines;
  std::vector<std::size_t> starts;
  std::size_t pos = 0;
  while (pos < text.size()) {
    const std::size_t nl = text.find('\n', pos);
    if (nl == std::string::npos) break;                        // a last line with no newline is not a line
    starts.push_back(pos);
    lines.push_back(text.substr(pos, nl - pos));
    pos = nl + 1;
  }
  if (lines.empty() || lines[0] != "rcbundle 1") {
    r.error = "bad header";
    return r;
  }
  if (lines.size() < 2 || lines[1].rfind("version ", 0) != 0 || lines[1].size() <= 8 || lines[1].find(' ', 8) != std::string::npos) {
    r.error = "bad version line";
    return r;
  }
  r.manifest.version = lines[1].substr(8);
  std::set<std::string> seen;
  std::size_t end_line = lines.size();
  for (std::size_t i = 2; i < lines.size(); ++i) {
    const std::string& l = lines[i];
    if (l.rfind("end ", 0) == 0) {
      end_line = i;
      break;
    }
    bool good = false;
    Entry e;
    if (l.rfind("file ", 0) == 0) {
      const std::size_t a = l.find(' ', 5);
      const std::size_t b = a == std::string::npos ? a : l.find(' ', a + 1);
      if (a != std::string::npos && b != std::string::npos && l.find(' ', b + 1) == std::string::npos) {
        const std::string size = l.substr(a + 1, b - a - 1);
        const std::string hash = l.substr(b + 1);
        bool digits = !size.empty() && size.size() <= 18;
        for (char c : size) digits = digits && c >= '0' && c <= '9';
        bool hex = hash.size() == 16;
        for (char c : hash) hex = hex && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
        if (digits && hex) {
          e.path = l.substr(5, a - 5);
          e.size = std::stoull(size);
          e.hash = std::stoull(hash, nullptr, 16);
          good = true;
        }
      }
    }
    if (!good) {
      r.error = "bad file line " + std::to_string(i + 1);
      return r;
    }
    if (!safe_path(e.path)) {
      r.error = "unsafe path: " + e.path;
      return r;
    }
    if (!seen.insert(e.path).second) {
      r.error = "duplicate path: " + e.path;
      return r;
    }
    r.manifest.entries.push_back(e);
  }
  if (end_line == lines.size() || end_line + 1 != lines.size()) {
    r.error = "no end line";
    return r;
  }
  const std::string& endl_text = lines[end_line];
  const std::string before = text.substr(0, starts[end_line]);
  if (endl_text != "end " + hex16(hash64(before))) {
    r.error = "bad checksum";
    return r;
  }
  r.ok = true;
  return r;
}

enum class Mismatch { missing, extra, size, hash };

struct Difference {
  std::string path;
  Mismatch kind = Mismatch::missing;
};

// How the files on the robot differ from the manifest: for each manifest entry in order, `missing` if no file has that path, otherwise `size` if the size differs, otherwise `hash` if the hash differs; then each file that is not in the manifest as `extra`, in path order.
// If a path is given twice among the files, the first is the one looked at.
inline std::vector<Difference> verify_files(const Manifest& m, const std::vector<FileData>& files) {
  std::map<std::string, const FileData*> by_path;
  for (const FileData& f : files) by_path.insert({f.path, &f});
  std::vector<Difference> out;
  std::set<std::string> listed;
  for (const Entry& e : m.entries) {
    listed.insert(e.path);
    auto it = by_path.find(e.path);
    if (it == by_path.end()) out.push_back({e.path, Mismatch::missing});
    else if (it->second->content.size() != e.size) out.push_back({e.path, Mismatch::size});
    else if (hash64(it->second->content) != e.hash) out.push_back({e.path, Mismatch::hash});
  }
  for (const auto& kv : by_path)
    if (listed.count(kv.first) == 0) out.push_back({kv.first, Mismatch::extra});
  return out;
}

#endif  // LESSON_SOLUTION_HPP
