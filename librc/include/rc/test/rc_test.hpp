// rc/test/rc_test.hpp
//
// The test framework this curriculum runs on. It is deliberately small enough
// to read in one sitting, because you are asked to trust it from lesson one.
// Phase 05 rebuilds it from nothing, so nothing here stays a mystery.
//
// Usage:
//
//   #include <rc/test/rc_test.hpp>
//
//   RC_TEST("a stopped robot does not move") {
//     RC_CHECK_NEAR(step(0.0, 0.0).x, 0.0, 1e-9);
//   }
//
// The header supplies main(). One test file builds into one test binary, and
// the binary exits non zero if any check failed.

#ifndef RC_TEST_RC_TEST_HPP
#define RC_TEST_RC_TEST_HPP

#include <csetjmp>
#include <csignal>
#include <cstddef>
#if !defined(_WIN32)
#  include <setjmp.h>   // sigsetjmp and siglongjmp, which are POSIX rather than standard
#endif
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace rc::test {

struct Failure {
  std::string test;
  std::string file;
  int line = 0;
  std::string message;
};

struct Case {
  std::string name;
  void (*fn)();
};

// A single translation unit owns the registry. Tests add themselves to it
// before main() runs, using the constructor of a global object.
inline std::vector<Case>& registry() {
  static std::vector<Case> cases;
  return cases;
}

inline std::vector<Failure>& failures() {
  static std::vector<Failure> found;
  return found;
}

inline const std::string*& current_test() {
  static const std::string* name = nullptr;
  return name;
}

struct Registrar {
  Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

// Thrown by the RC_REQUIRE family to stop the current test. A check that has
// already failed must not be followed by code that depends on it, or a wrong
// answer turns into a crash and the learner sees a stack trace instead of a
// sentence telling them what went wrong.
struct Abort {};

inline void record(const char* file, int line, const std::string& message) {
  failures().push_back(
      {current_test() ? *current_test() : "unknown", file, line, message});
}

inline bool nearly_equal(double a, double b, double tolerance) {
  return std::fabs(a - b) <= tolerance;
}

// Surviving a crash in the learner's code.
//
// Beginners write code that divides by zero and reads past the end of arrays.
// If that took the whole test binary down, one mistake would destroy every
// other result and the learner would be left with a signal number and nothing
// else. So the runner catches the two signals that ordinary mistakes produce,
// reports which test caused it and what it usually means, and carries on.
//
// This is best effort by design. Continuing after a fault means the process
// state may not be entirely trustworthy, so treat later results as indicative
// and fix the crash first. That is still far better than no output at all.

// Entering a signal handler blocks that signal until the handler returns. Jumping
// out with plain longjmp never returns, so the signal stays blocked for the rest
// of the process and the second fault of the same kind kills it outright. That
// is the entire reason sigsetjmp exists: it saves and restores the signal mask.
// Windows has no such distinction, so plain setjmp is correct there.
#if defined(_WIN32)
using JumpBuffer = std::jmp_buf;
#  define RC_SETJMP(buffer) setjmp(buffer)
#  define RC_LONGJMP(buffer, value) std::longjmp(buffer, value)
#else
using JumpBuffer = sigjmp_buf;
#  define RC_SETJMP(buffer) sigsetjmp(buffer, 1)
#  define RC_LONGJMP(buffer, value) siglongjmp(buffer, value)
#endif

inline JumpBuffer& crash_jump() {
  static JumpBuffer buffer;
  return buffer;
}

inline volatile std::sig_atomic_t& crash_signal() {
  static volatile std::sig_atomic_t number = 0;
  return number;
}

extern "C" inline void rc_test_crash_handler(int number) {
  crash_signal() = number;
  RC_LONGJMP(crash_jump(), 1);
}

inline std::string describe_crash(int number) {
  if (number == SIGFPE) {
    return "this test crashed with an arithmetic fault.\n  On every supported "
           "platform that almost always means an integer division by zero,\n  "
           "which is undefined behaviour rather than an error you can catch.\n  "
           "Guard the divisor before dividing.";
  }
  if (number == SIGSEGV) {
    return "this test crashed by touching memory it does not own.\n  Usually an "
           "index past the end of an array, or a pointer that was never set.\n  "
           "Build with the asan preset to be told exactly where.";
  }
  return "this test crashed with signal " + std::to_string(number) + ".";
}

// The try blocks live here rather than in run_all. A jump out of a signal
// handler cannot unwind past an active try region, so the function holding
// setjmp must not contain one. Keeping the two in separate frames is what makes
// both recovery paths work at once.
inline void run_one_case(const Case& c) {
  try {
    c.fn();
  } catch (const Abort&) {
    // The failure was already recorded. Stopping here is the point.
  } catch (const std::exception& e) {
    record("unknown", 0, std::string("the test threw an exception: ") + e.what());
  } catch (...) {
    record("unknown", 0, "the test threw something that is not an exception");
  }
}

inline int run_all() {
  int passed = 0;
  for (const Case& c : registry()) {
    const std::size_t before = failures().size();
    const std::string name = c.name;
    current_test() = &name;

    crash_signal() = 0;
    auto previous_fpe = std::signal(SIGFPE, rc_test_crash_handler);
    auto previous_segv = std::signal(SIGSEGV, rc_test_crash_handler);

    if (RC_SETJMP(crash_jump()) == 0) {
      run_one_case(c);
    } else {
      record("unknown", 0, describe_crash(static_cast<int>(crash_signal())));
    }

    std::signal(SIGFPE, previous_fpe);
    std::signal(SIGSEGV, previous_segv);
    current_test() = nullptr;
    const bool ok = failures().size() == before;
    std::cout << (ok ? "  pass  " : "  FAIL  ") << c.name << "\n";
    if (ok) ++passed;
  }

  std::cout << "\n" << passed << "/" << registry().size() << " tests passed\n";

  for (const Failure& f : failures()) {
    std::cout << "\n" << f.file << ":" << f.line << "\n  in test: " << f.test
              << "\n  " << f.message << "\n";
  }
  return failures().empty() ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace rc::test

#define RC_TEST_CONCAT_INNER(a, b) a##b
#define RC_TEST_CONCAT(a, b) RC_TEST_CONCAT_INNER(a, b)

#define RC_TEST(name)                                                       \
  static void RC_TEST_CONCAT(rc_test_fn_, __LINE__)();                      \
  static const ::rc::test::Registrar RC_TEST_CONCAT(rc_test_reg_, __LINE__)( \
      name, &RC_TEST_CONCAT(rc_test_fn_, __LINE__));                        \
  static void RC_TEST_CONCAT(rc_test_fn_, __LINE__)()

#define RC_CHECK(expr)                                                     \
  do {                                                                     \
    if (!(expr)) {                                                         \
      ::rc::test::record(__FILE__, __LINE__,                               \
                         std::string("expected this to be true: " #expr)); \
    }                                                                      \
  } while (false)

#define RC_CHECK_EQ(actual, expected)                                    \
  do {                                                                   \
    const auto rc_a = (actual);                                          \
    const auto rc_b = (expected);                                        \
    if (!(rc_a == rc_b)) {                                               \
      std::ostringstream rc_msg;                                         \
      rc_msg << "expected " #actual " to equal " #expected << "\n    got:      " \
             << rc_a << "\n    expected: " << rc_b;                      \
      ::rc::test::record(__FILE__, __LINE__, rc_msg.str());              \
    }                                                                    \
  } while (false)

#define RC_CHECK_NEAR(actual, expected, tolerance)                        \
  do {                                                                    \
    const double rc_a = static_cast<double>(actual);                       \
    const double rc_b = static_cast<double>(expected);                     \
    if (!::rc::test::nearly_equal(rc_a, rc_b, (tolerance))) {              \
      std::ostringstream rc_msg;                                          \
      rc_msg << "expected " #actual " to be within " << (tolerance)        \
             << " of " #expected << "\n    got:      " << rc_a             \
             << "\n    expected: " << rc_b;                                \
      ::rc::test::record(__FILE__, __LINE__, rc_msg.str());                \
    }                                                                     \
  } while (false)

#define RC_CHECK_THROWS(expr)                                              \
  do {                                                                     \
    bool rc_threw = false;                                                 \
    try {                                                                  \
      (void)(expr);                                                        \
    } catch (...) {                                                        \
      rc_threw = true;                                                     \
    }                                                                      \
    if (!rc_threw) {                                                       \
      ::rc::test::record(__FILE__, __LINE__,                               \
                         std::string("expected this to throw: " #expr));   \
    }                                                                      \
  } while (false)

// The REQUIRE family records the same failure as CHECK and then stops the test.
// Use it whenever the lines that follow would be meaningless, or dangerous, if
// the check did not hold: sizes before indexing, pointers before dereferencing.

#define RC_REQUIRE(expr)                                                       \
  do {                                                                         \
    if (!(expr)) {                                                             \
      ::rc::test::record(__FILE__, __LINE__,                                   \
                         std::string("required, and it was not true: " #expr));\
      throw ::rc::test::Abort{};                                               \
    }                                                                          \
  } while (false)

#define RC_REQUIRE_EQ(actual, expected)                                    \
  do {                                                                     \
    const auto rc_a = (actual);                                            \
    const auto rc_b = (expected);                                          \
    if (!(rc_a == rc_b)) {                                                 \
      std::ostringstream rc_msg;                                           \
      rc_msg << "required " #actual " to equal " #expected                  \
             << "\n    got:      " << rc_a << "\n    expected: " << rc_b;   \
      ::rc::test::record(__FILE__, __LINE__, rc_msg.str());                \
      throw ::rc::test::Abort{};                                           \
    }                                                                      \
  } while (false)

#ifndef RC_TEST_NO_MAIN
int main() { return ::rc::test::run_all(); }
#endif

#endif  // RC_TEST_RC_TEST_HPP
