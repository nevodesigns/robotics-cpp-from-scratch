#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// The thing under test, behind an interface so one set of checks can be pointed
// at more than one implementation. This is the rate limiter from lesson 01-02:
// move `current` toward `target`, by no more than `max_step`.
class Limiter {
 public:
  virtual ~Limiter() = default;
  virtual double apply(double current, double target, double max_step) const = 0;
};

// Everything a rate limiter must do, whatever is inside it.
//
// Written as properties rather than as remembered answers, because a property
// is something you can write before any implementation exists, and because a
// list of remembered answers only ever catches the mistakes you already thought
// of.
//
// Each check below is here because an implementation exists that passes every
// other one and fails this. That is the standard worth holding a test suite to:
// not "does it pass" but "what would have to be wrong for this to fail".
inline bool checks_pass(const Limiter& limiter) {
  // TODO: return true when this limiter is right, and false when it is not.
  //
  // The suite points this at seven implementations: one that is correct and six
  // that are each wrong in one specific way. You must accept the first and
  // reject all six, and a set of checks that manages both had to have been
  // written from what a rate limiter *must do* rather than from what any
  // particular one *does*.
  //
  // Both directions matter equally. A suite that rejects correct code is worse
  // than no suite, because the next real failure is not believed.
  //
  // Four kinds of question are worth asking, and between them they cover all
  // six:
  //
  //   Does it arrive?      Within one step of the target it should land on it,
  //                        not step past and come back.
  //   Does it limit?       A distant target should be approached by one step,
  //                        not reached.
  //   Both directions?     Downward is a direction too, and a limiter that only
  //                        ever adds passes every check written with the target
  //                        above the current value.
  //   The boundaries?      Already there. A step of zero. Exactly one step
  //                        away. These are where an implementation stops being
  //                        described by the cases either side of them, and one
  //                        of the six is wrong at exactly one of them.
  //
  // Compare with a tolerance rather than for equality: these are fractional
  // numbers, and a correct implementation rejected for its last few bits is
  // exactly the failure described above.
  (void)limiter;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
