#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstdlib>
#include <iostream>

// assert is the first checking tool anybody meets, and the thing nobody
// mentions is that it is compiled out of the build you ship.
//
// NDEBUG is defined in a release build by convention and by every build system
// including this one, and <cassert> then defines assert to nothing at all. Not
// to a cheaper check. To nothing: the expression inside it is not evaluated,
// so anything it was doing stops happening.
//
// For a bounds check that is the right trade, made deliberately. For a
// precondition on a motor command it means the only build that checks your
// speed limit is the one that never leaves your desk.
//
// So there are two kinds of check and they need two different tools. assert is
// for things you believe cannot happen, which you want verified while
// developing and are content to pay nothing for afterwards. This header is for
// things you believe should not happen, which must still be noticed on the
// robot.

// What failed, and where.
//
// The expression as written and the file and line, because a check that
// reports only that something was wrong leaves the reader no better off than
// the symptom did.
struct CheckFailure {
  const char* file = nullptr;
  int line = 0;
  const char* expression = nullptr;
  const char* message = nullptr;
};

using CheckHandler = void (*)(const CheckFailure&);

// Prints and carries on.
//
// The default, and it is deliberately not abort. A robot that is moving does
// not become safer by stopping its own control loop, and the caller is the
// only code that knows whether this violation means clamp, retry, or stop.
inline void log_failure(const CheckFailure& failure) {
  std::cerr << "check failed: " << (failure.expression ? failure.expression : "")
            << "\n  at " << (failure.file ? failure.file : "") << ":" << failure.line
            << "\n  " << (failure.message ? failure.message : "") << "\n";
}

// Prints and stops, for the cases where carrying on is worse.
inline void abort_on_failure(const CheckFailure& failure) {
  log_failure(failure);
  std::abort();
}

// Says nothing, for a test that is deliberately provoking failures.
inline void ignore_failure(const CheckFailure&) {}

// Where the installed handler lives.
//
// A function local static inside an inline function, which lesson 04-04
// established is one object across every translation unit that includes this
// header, rather than one per unit.
inline CheckHandler& check_handler_storage() {
  static CheckHandler handler = &log_failure;
  return handler;
}

// TODO 1: install a handler.
//
// Assign through check_handler_storage() above, which is the one object every
// translation unit shares.
//
// Refuse a null one. A null handler is a crash on the first failure, which
// turns a reporting mechanism into a worse version of the thing it replaced,
// so substitute ignore_failure instead. The test installs nullptr and expects
// a check to fail quietly rather than to take the process with it.
inline void set_check_handler(CheckHandler handler) {
  (void)handler;
}

inline CheckHandler current_check_handler() { return check_handler_storage(); }

// Reports a failure and answers false, so the macro below can be used in an
// expression and the caller can branch on it.
// TODO 2: report a failure, and answer false.
//
// Call whatever handler is installed, passing the failure through, then return
// false so the macro below can be used in an expression and the caller can
// branch on it.
//
// Returning false rather than aborting is the design. The caller is the only
// code that knows whether this violation means clamp, retry or stop.
inline bool report_check_failure(const CheckFailure& failure) {
  (void)failure;
  return false;
}

// The check itself.
//
// Not conditional on NDEBUG, which is the entire point. It evaluates its
// condition exactly once, answers true when it holds, and otherwise reports
// through the handler and answers false.
//
// Written as an expression rather than a statement so the caller has to say
// what happens next:
//
//   if (!RC_EXPECT(speed <= limit, "commanded speed above the limit"))
//     speed = limit;
//
// which is longer than an assert and says who decided.
#define RC_EXPECT(condition, message)                                      \
  ((condition) ? true                                                      \
               : report_check_failure(                                     \
                     CheckFailure{__FILE__, __LINE__, #condition, (message)}))

// A commanded speed, checked and clamped.
//
// What the whole header is for, in one function. The check reports that
// something upstream asked for too much, and the clamp decides what to do
// about it, and both still happen in the build that ships.
// TODO 3: a commanded speed, checked and clamped.
//
// Two checks, one for each end, each written as
//
//   if (!RC_EXPECT(condition, "what went wrong")) return the_clamped_value;
//
// and the requested speed returned unchanged when both hold.
//
// Note what that shape says. The check reports that something upstream asked
// for too much; the return decides what to do about it. Both still happen in
// the build that ships, which an assert would not.
//
// A request exactly at the limit is not a violation, so get the comparison
// right: the test asks for the limit itself and expects no report.
inline double safe_speed(double requested, double limit) {
  (void)limit;
  return requested;
}

#endif  // LESSON_SOLUTION_HPP
