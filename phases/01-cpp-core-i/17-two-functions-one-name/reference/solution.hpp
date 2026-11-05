#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Every lesson so far has written std:: in front of the standard library's names
// and rc:: in front of ours, and nobody has said what the two colons are or why
// the prefix is there. Rule L018 forbids using namespace std; in every lesson
// without saying what it would cost. This is that page.
//
// A namespace is a named place for names to live. std is where the standard
// library keeps its names, rc::core is where this curriculum keeps its own, and
// the two colons are the way of saying "the one in there". The point of them is
// that two people can both write a function called limit.
//
// Measured on gcc 11 and clang 14.
//
// A using-directive, using namespace x;, makes everything in x visible without
// its prefix. The danger is not the names you meant. It is that a call can fit a
// function you did not know was there better than the one you wrote.
//
// Lesson 01-02 wrote clamp(double, double, double). C++17 has std::clamp too. A
// call clamp(5, 0, 3) with plain ints, run twice:
//
//   without the directive   the global clamp ran, and the result was a double
//   with using namespace std   std::clamp ran, the global one was never called,
//                              and the result was an int
//
// No warning on either compiler. The ints fit std::clamp exactly and the
// global one only after converting, so the better match won, and the better
// match was the function the author did not write. The suite reproduces the same
// shape with a namespace of its own, so it does not depend on which compiler's
// standard library has what.
//
// Two directives that both offer the same name, with equal fit, are refused:
//
//   call of overloaded limit(double) is ambiguous        (gcc)
//   call to limit is ambiguous                            (clang)
//
// and so is a global variable called count under using namespace std, because
// std has an algorithm of that name: reference to count is ambiguous. Those are
// the good outcomes. They are loud, at the line that is wrong.
//
// An unqualified name can also be found through its arguments. distance(v.begin(),
// v.end()) for a std::vector compiled with no std:: and returned 3, because the
// iterators belong to std and the compiler looks in the namespaces of a call's
// arguments. The same line for an int array was refused: distance was not
// declared in this scope, did you mean std::distance. Code that leans on this is
// correct for one container and broken for the next.
//
// The rule that falls out: write the prefix. std::clamp and ::clamp cannot be
// confused, and neither can be found by accident. A using-declaration, using
// drive::limit;, names one function and is fine in a small scope. A namespace
// alias, namespace fs = std::filesystem;, shortens a long prefix without
// importing anything. A using-directive for a namespace you do not own is the one
// to avoid, and in a header it reaches every file that includes it.

// The global clamp from lesson 01-02, counting its calls so that a test can tell
// whether this function or another one ran.
inline int& clamp_calls() {
  static int calls = 0;
  return calls;
}

inline double clamp(double value, double low, double high) {
  ++clamp_calls();
  return value < low ? low : (value > high ? high : value);
}

// Another clamp, for ints, in a namespace of its own. Nobody calls it by
// accident until somebody writes using namespace general.
namespace general {

inline int clamp(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}

}  // namespace general

namespace drive {

const double kMaxSpeed = 1.0;

// Keeps a speed in range. The leading :: names the global clamp and nothing
// else, so no using-directive elsewhere can change which function this calls.
inline double limit(double speed) { return ::clamp(speed, -kMaxSpeed, kMaxSpeed); }

}  // namespace drive

namespace arm {

// A quarter turn in radians, the most a joint of this arm may be asked for.
const double kQuarterTurn = 1.5707963267948966;

// Keeps an angle in range. The same name as drive::limit, a different job.
inline double limit(double angle) { return ::clamp(angle, -kQuarterTurn, kQuarterTurn); }

}  // namespace arm

#endif  // LESSON_SOLUTION_HPP
