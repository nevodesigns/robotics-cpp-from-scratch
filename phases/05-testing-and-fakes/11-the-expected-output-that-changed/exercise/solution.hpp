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
  // TODO: go through the line. A "t=" at the start of the line or after a space, followed by at least one digit
  // or '.', is replaced by "t=<time>" (and the number is skipped). "0x" followed by at least one hexadecimal digit
  // is replaced by "0x<addr>" (and the digits are skipped). Every other character is copied unchanged.
  (void)line;
  return "";
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
  // TODO: the longest common subsequence. Build table[i][j], the number of lines the first i of expected and the
  // first j of actual have in common in order: if expected[i-1] == actual[j-1] it is table[i-1][j-1] + 1, otherwise
  // the larger of table[i-1][j] and table[i][j-1]. Then walk back from [n][m]: equal lines are kept (' '), else if
  // moving along actual keeps at least as much go that way and add the line ('+'), else remove the line from
  // expected ('-'). Collect the lines backwards and return them in the right order.
  (void)expected;
  (void)actual;
  return std::vector<DiffLine>();
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
