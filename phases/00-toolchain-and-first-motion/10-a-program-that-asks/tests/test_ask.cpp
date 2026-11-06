#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "solution.hpp"

namespace {

// What a hand written reader does when it forgets to clean up after a failure.
int successes_without_clearing(const char* typed, int attempts) {
  std::istringstream in(typed);
  int good = 0;
  for (int i = 0; i < attempts; ++i) {
    double value = 0.0;
    if (in >> value) ++good;
  }
  return good;
}

}  // namespace

RC_TEST("a number on a line is read, and the problem text is empty") {
  std::istringstream in("42.5\n");
  const Entry e = read_number(in, 0.0, 100.0);
  RC_CHECK(e.ok);
  RC_CHECK_NEAR(e.value, 42.5, 1e-12);
  RC_CHECK_EQ(std::string(e.problem), std::string(""));
}

RC_TEST("words are not a number, and the stream is usable afterwards") {
  std::istringstream in("abc\n7\n");
  const Entry first = read_number(in, 0.0, 100.0);
  RC_CHECK(!first.ok);
  RC_CHECK_EQ(std::string(first.problem), std::string("not a number"));
  const Entry second = read_number(in, 0.0, 100.0);
  RC_CHECK(second.ok);
  RC_CHECK_NEAR(second.value, 7.0, 1e-12);
}

RC_TEST("a number followed by something else is refused, not read as the number") {
  // A refusal of everything would pass the table below, so the plain 12 has to be accepted too.
  std::istringstream plain("12\n");
  RC_CHECK(read_number(plain, 0.0, 100.0).ok);

  const char* typed[] = {"12abc\n", "3,5\n", "7 8\n", "9.5.1\n"};
  std::cout << "  typed        what >> alone would take   what read_number says\n";
  for (const char* text : typed) {
    std::istringstream raw(text);
    double alone = 0.0;
    raw >> alone;

    std::istringstream in(text);
    const Entry e = read_number(in, 0.0, 100.0);
    std::string shown(text);
    shown.pop_back();
    std::cout << "  " << std::left << std::setw(10) << shown << "   " << std::setw(24) << alone << "   "
              << (e.ok ? "accepted" : e.problem) << "\n";
    RC_CHECK(!e.ok);
  }
  std::cout << "\n";
}

RC_TEST("spaces around a number are fine") {
  std::istringstream in("  15   \n");
  const Entry e = read_number(in, 0.0, 100.0);
  RC_CHECK(e.ok);
  RC_CHECK_NEAR(e.value, 15.0, 1e-12);
}

RC_TEST("a good number outside the allowed range is refused, with its own reason") {
  std::istringstream in("400\n-1\n100\n0\n");
  const Entry high = read_number(in, 0.0, 100.0);
  RC_CHECK(!high.ok);
  RC_CHECK_EQ(std::string(high.problem), std::string("out of range"));
  const Entry low = read_number(in, 0.0, 100.0);
  RC_CHECK(!low.ok);
  RC_CHECK_EQ(std::string(low.problem), std::string("out of range"));
  // The limits themselves are allowed.
  RC_CHECK(read_number(in, 0.0, 100.0).ok);
  RC_CHECK(read_number(in, 0.0, 100.0).ok);
}

RC_TEST("nothing to read is its own answer, and asking does not go on for ever") {
  std::istringstream empty("");
  const Entry e = read_number(empty, 0.0, 100.0);
  RC_CHECK(!e.ok);
  RC_CHECK_EQ(std::string(e.problem), std::string("no input"));

  std::istringstream nothing("");
  const Asked asked = ask(nothing, 0.0, 100.0, 1000);
  std::cout << "  asking an empty input up to 1000 times: " << asked.attempts_used << " attempt used\n\n";
  RC_CHECK(!asked.entry.ok);
  RC_CHECK(asked.attempts_used < 5);
}

RC_TEST("asking again gets past bad answers, and without clearing the stream it never would") {
  std::istringstream in("abc\n-5\n12abc\n7\n");
  const Asked asked = ask(in, 0.0, 100.0, 10);
  RC_CHECK(asked.entry.ok);
  RC_CHECK_NEAR(asked.entry.value, 7.0, 1e-12);
  RC_CHECK_EQ(asked.attempts_used, 4);

  const int stuck = successes_without_clearing("abc\n7\n7\n7\n", 10);
  std::cout << "  typed abc then three sevens, ten reads with >> and no clear: " << stuck << " succeeded\n"
            << "  ask() on abc, -5, 12abc, 7: value " << asked.entry.value << " after " << asked.attempts_used
            << " attempts\n\n";
  RC_CHECK_EQ(stuck, 0);

  std::istringstream limited("a\nb\nc\nd\ne\n");
  const Asked gave_up = ask(limited, 0.0, 100.0, 3);
  RC_CHECK(!gave_up.entry.ok);
  RC_CHECK_EQ(gave_up.attempts_used, 3);
}
