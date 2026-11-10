#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <map>
#include <string>
#include <vector>

// Where a setting can come from, weakest first. A later one wins over an earlier one.
const int kDefault = 0;
const int kFile = 1;
const int kEnvironment = 2;
const int kCommandLine = 3;

inline const char* source_name(int source) {
  switch (source) {
    case kDefault: return "default";
    case kFile: return "file";
    case kEnvironment: return "environment";
    case kCommandLine: return "command line";
    default: return "unknown";
  }
}

// How many single-character edits (insert, delete, change) turn one word into another.
//
// The usual table: cell [i][j] is the cost of turning the first i letters of a into the first j of b. A matching
// letter costs nothing extra, and otherwise the cheapest of a change, a deletion and an insertion.
inline int edit_distance(const std::string& a, const std::string& b) {
  // TODO: a table of (a.size() + 1) by (b.size() + 1) numbers. table[i][0] = i and table[0][j] = j.
  // For each i and j from 1: change = table[i-1][j-1] + (a[i-1] == b[j-1] ? 0 : 1),
  // remove = table[i-1][j] + 1, insert = table[i][j-1] + 1, and table[i][j] is the smallest of them.
  // The answer is table[a.size()][b.size()].
  (void)a;
  (void)b;
  return 0;
}

// Settings, each remembering which place it came from.
class Settings {
 public:
  // Set a value from a source. A weaker source does not replace a stronger one that is already there;
  // the same or a stronger source does, so a later line in the same file wins over an earlier one.
  void set(int source, const std::string& key, const std::string& value) {
    // TODO: if the key is already there with a STRONGER source (a larger number), do nothing. Otherwise
    // store the value and the source: the same or a stronger source replaces what was there.
    (void)source;
    (void)key;
    (void)value;
    (void)entries_;
  }

  bool get(const std::string& key, std::string* value, int* source) const {
    // TODO: find the key. Not found: return false. Found: copy its value into *value, its source into
    // *source if source is not null, and return true.
    (void)key;
    (void)value;
    (void)source;
    return false;
  }

  // "speed_limit = 2 (command line)": the value and where it came from, for a log a person will read.
  std::string explain(const std::string& key) const {
    std::string value;
    int source = 0;
    if (!get(key, &value, &source)) return key + " is not set";
    return key + " = " + value + " (" + source_name(source) + ")";
  }

  // Every key that has been set and is not one the program knows about. A key nobody reads is almost
  // always a typo, and a typo is a setting that silently did nothing.
  std::vector<std::string> unknown_keys(const std::vector<std::string>& known) const {
    // TODO: every key in entries_ that is not in `known`, in the order the map holds them.
    (void)known;
    return std::vector<std::string>();
  }

 private:
  struct Entry {
    std::string value;
    int source = 0;
  };
  std::map<std::string, Entry> entries_;
};

// The known name closest to a mistyped one, if any is close enough to be a likely slip: within two edits, and
// fewer edits than the word has letters, so a short word is not matched to everything.
inline std::string suggest(const std::string& typed, const std::vector<std::string>& known) {
  // TODO: the known name with the smallest edit_distance to `typed`, but only if that distance is
  // below 3 AND below typed.size() (a two letter word is not "close" to everything). Otherwise "".
  (void)typed;
  (void)known;
  return "";
}

#endif  // LESSON_SOLUTION_HPP
