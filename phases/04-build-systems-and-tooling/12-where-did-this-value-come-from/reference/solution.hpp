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
  std::vector<std::vector<int> > table(a.size() + 1, std::vector<int>(b.size() + 1, 0));
  for (std::size_t i = 0; i <= a.size(); ++i) table[i][0] = static_cast<int>(i);
  for (std::size_t j = 0; j <= b.size(); ++j) table[0][j] = static_cast<int>(j);

  for (std::size_t i = 1; i <= a.size(); ++i) {
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const int change = table[i - 1][j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
      const int remove = table[i - 1][j] + 1;
      const int insert = table[i][j - 1] + 1;
      table[i][j] = std::min(change, std::min(remove, insert));
    }
  }
  return table[a.size()][b.size()];
}

// Settings, each remembering which place it came from.
class Settings {
 public:
  // Set a value from a source. A weaker source does not replace a stronger one that is already there;
  // the same or a stronger source does, so a later line in the same file wins over an earlier one.
  void set(int source, const std::string& key, const std::string& value) {
    std::map<std::string, Entry>::iterator it = entries_.find(key);
    if (it != entries_.end() && it->second.source > source) return;
    Entry e;
    e.value = value;
    e.source = source;
    entries_[key] = e;
  }

  bool get(const std::string& key, std::string* value, int* source) const {
    const std::map<std::string, Entry>::const_iterator it = entries_.find(key);
    if (it == entries_.end()) return false;
    *value = it->second.value;
    if (source != nullptr) *source = it->second.source;
    return true;
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
    std::vector<std::string> unknown;
    for (std::map<std::string, Entry>::const_iterator it = entries_.begin(); it != entries_.end(); ++it) {
      if (std::find(known.begin(), known.end(), it->first) == known.end()) unknown.push_back(it->first);
    }
    return unknown;
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
  std::string best;
  int best_distance = 3;
  for (const std::string& name : known) {
    const int d = edit_distance(typed, name);
    if (d < best_distance && d < static_cast<int>(typed.size())) {
      best_distance = d;
      best = name;
    }
  }
  return best;
}

#endif  // LESSON_SOLUTION_HPP
