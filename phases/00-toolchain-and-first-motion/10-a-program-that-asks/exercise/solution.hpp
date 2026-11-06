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
  // TODO: four things, in this order.
  //
  // 1. Try `in >> value`. If it fails, either nothing was left (in.eof() is true) or what was
  //    there is not a number. Remember which, then in.clear() and throw the rest of the line away
  //    with in.ignore(std::numeric_limits<std::streamsize>::max(), '\n'). Set the problem to
  //    "no input" or "not a number" and return.
  //
  // 2. The number was read, but is anything else on the line? Skip spaces and tabs with
  //    in.peek() and in.get(). If the next character is not '\n' and not end of input, throw the
  //    line away and report "not a number": 12abc is not 12.
  //
  // 3. Consume the newline if there is one.
  //
  // 4. If value is below low or above high, report "out of range". Otherwise ok is true and
  //    value is the number.
  (void)in;
  (void)low;
  (void)high;
  return Entry{};
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
  // TODO: call read_number up to `attempts` times, counting each in attempts_used, and stop at
  // the first good answer. Also stop when in.eof() is true: nobody is going to answer.
  (void)in;
  (void)low;
  (void)high;
  (void)attempts;
  return Asked{};
}

#endif  // LESSON_SOLUTION_HPP
