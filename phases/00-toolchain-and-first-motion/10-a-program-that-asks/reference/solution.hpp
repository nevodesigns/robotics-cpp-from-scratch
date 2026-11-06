#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <istream>
#include <limits>

// What came of asking for a number.
struct Entry {
  bool ok = false;
  double value = 0.0;
  const char* problem = "";   // empty when ok, otherwise a reason a person can read
};

// Read one number from one line and judge it.
//
// A number typed by a person is a different thing from a number in the program: it can be
// missing, it can be words, it can be a number followed by words, and it can be a perfectly
// good number that is no use here. All four are handled and each is told apart.
inline Entry read_number(std::istream& in, double low, double high) {
  Entry entry;

  double value = 0.0;
  if (!(in >> value)) {
    // Either nothing was left to read, or what was there is not a number. The two get different
    // messages, and the stream has to be cleaned up before it can be used again.
    const bool at_end = in.eof();
    in.clear();
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    entry.problem = at_end ? "no input" : "not a number";
    return entry;
  }

  // The number was read. Is anything else on the line? Spaces are fine, anything else is
  // not: 12abc is not 12, and 3,5 is not 3.5.
  while (in.peek() == ' ' || in.peek() == '\t') in.get();
  const int next = in.peek();
  if (next != '\n' && next != std::istream::traits_type::eof()) {
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    entry.problem = "not a number";
    return entry;
  }
  if (next == '\n') in.get();

  if (value < low || value > high) {
    entry.problem = "out of range";
    return entry;
  }

  entry.ok = true;
  entry.value = value;
  return entry;
}

// Ask until there is a good answer, but never for ever.
//
// `attempts` bounds the loop. A program that asks until it is satisfied, reading from something that
// never will be, is a program that has hung.
struct Asked {
  Entry entry;
  int attempts_used = 0;
};

inline Asked ask(std::istream& in, double low, double high, int attempts) {
  Asked asked;
  for (int i = 0; i < attempts; ++i) {
    ++asked.attempts_used;
    asked.entry = read_number(in, low, high);
    if (asked.entry.ok) return asked;
    if (in.eof()) return asked;   // nobody is going to answer
  }
  return asked;
}

#endif  // LESSON_SOLUTION_HPP
