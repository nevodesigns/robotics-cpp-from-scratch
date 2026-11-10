#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <string>
#include <vector>

// Replace the parts of a log line that change from run to run, so what is left can be compared.
//
// Two things are volatile in this log: the time, written t=12.345, and an address, written 0x7ffd5a3c. Both become a
// fixed word. Everything else is left exactly as it was, since changing it would hide a real difference.
inline std::string normalize(const std::string& line) {
  std::string out;
  std::size_t i = 0;
  while (i < line.size()) {
    // t= followed by a number
    if (line.compare(i, 2, "t=") == 0 && (i == 0 || line[i - 1] == ' ')) {
      std::size_t j = i + 2;
      const std::size_t digits_start = j;
      while (j < line.size() && ((line[j] >= '0' && line[j] <= '9') || line[j] == '.')) ++j;
      if (j > digits_start) {
        out += "t=<time>";
        i = j;
        continue;
      }
    }
    // 0x followed by hexadecimal digits
    if (line.compare(i, 2, "0x") == 0) {
      std::size_t j = i + 2;
      const std::size_t digits_start = j;
      while (j < line.size() && ((line[j] >= '0' && line[j] <= '9') || (line[j] >= 'a' && line[j] <= 'f') ||
                                 (line[j] >= 'A' && line[j] <= 'F'))) {
        ++j;
      }
      if (j > digits_start) {
        out += "0x<addr>";
        i = j;
        continue;
      }
    }
    out += line[i];
    ++i;
  }
  return out;
}

// One line of a difference: kept (' '), removed from the expected ('-') or added in the actual ('+').
struct DiffLine {
  char op = ' ';
  std::string text;
};

// The smallest set of removals and additions that turns `expected` into `actual`.
//
// Longest common subsequence: table[i][j] is how many lines the first i of expected and the first j of actual have
// in common, in order. Walking back from the corner, equal lines are kept, and otherwise the walk goes in the
// direction that keeps more.
inline std::vector<DiffLine> diff(const std::vector<std::string>& expected, const std::vector<std::string>& actual) {
  const std::size_t n = expected.size();
  const std::size_t m = actual.size();
  std::vector<std::vector<int> > table(n + 1, std::vector<int>(m + 1, 0));
  for (std::size_t i = 1; i <= n; ++i) {
    for (std::size_t j = 1; j <= m; ++j) {
      if (expected[i - 1] == actual[j - 1]) table[i][j] = table[i - 1][j - 1] + 1;
      else table[i][j] = table[i - 1][j] >= table[i][j - 1] ? table[i - 1][j] : table[i][j - 1];
    }
  }

  std::vector<DiffLine> backwards;
  std::size_t i = n;
  std::size_t j = m;
  while (i > 0 || j > 0) {
    DiffLine d;
    if (i > 0 && j > 0 && expected[i - 1] == actual[j - 1]) {
      d.op = ' ';
      d.text = expected[i - 1];
      --i;
      --j;
    } else if (j > 0 && (i == 0 || table[i][j - 1] >= table[i - 1][j])) {
      d.op = '+';
      d.text = actual[j - 1];
      --j;
    } else {
      d.op = '-';
      d.text = expected[i - 1];
      --i;
    }
    backwards.push_back(d);
  }
  return std::vector<DiffLine>(backwards.rbegin(), backwards.rend());
}

// How many lines were added or removed.
inline int changes(const std::vector<DiffLine>& d) {
  int count = 0;
  for (const DiffLine& line : d) {
    if (line.op != ' ') ++count;
  }
  return count;
}

#endif  // LESSON_SOLUTION_HPP
