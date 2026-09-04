#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstdint>
#include <string>
#include <vector>

// A source of values that can be asked for the same ones again.
//
// The seed is the whole point. A test that fails on random input and cannot be
// run again on that input has told you there is a bug and taken the evidence
// with it, which is worse than useless: it is a test people learn to rerun.
//
// Deliberately not std::mt19937. That would be fine here, but a hand written
// generator is a few lines, is identical on every standard library, and the
// numbers in this lesson's tables are then the numbers you see.
class Source {
 public:
  explicit Source(std::uint64_t seed)
      : seed_(seed), state_(seed * 6364136223846793005ULL + 1ULL) {}

  std::uint64_t seed() const { return seed_; }

  // TODO 1: a value in [0, limit), or 0 when the limit is not positive.
  //
  // Advance the state, then take the remainder. Use the high bits rather than
  // the low ones: the low bits of a linear congruential generator have short
  // cycles, and the length of a generated string taken from bit zero alternates
  // between two values for ever.
  //
  //   state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
  //   return (state_ >> 11) % limit;
  //
  // A limit of zero or less returns 0 rather than dividing by it.
  int below(int limit) {
    (void)limit;
    return 0;
  }

 private:
  std::uint64_t seed_;
  std::uint64_t state_;
};

// What a hunt found, and how hard it had to look.
template <class T>
struct Counterexample {
  bool found = false;
  long trials = 0;         // how many values were tried before this one failed
  int shrink_steps = 0;    // how many times it was made smaller
  T value{};
};

// TODO 2: make a failing value as small as it can be while it still fails.
//
// `smaller` proposes candidates and knows nothing about why the value failed;
// `holds` decides. Take the first candidate that still breaks the property,
// start again from there, and stop when nothing proposed breaks it any more.
//
// Count the steps into `*steps` when the pointer is not null.
//
// The one thing a shrinker must never get wrong is returning something that
// passes. Everything it hands back has to have failed on the way.
//
// This is what makes generated testing usable. A generator that finds a fault
// hands you thirty six characters of noise; the same fault shrunk is twenty
// digits, all but the first of them a 1, and the shape of the bug is legible in
// the value itself.
template <class T, class Smaller, class Holds>
T shrink(T failing, Smaller smaller, Holds holds, int* steps = nullptr) {
  (void)smaller;
  (void)holds;
  (void)steps;
  return failing;
}

// TODO 3: generate values until one breaks the property, then shrink it.
//
// Ask the source for a value up to `trials` times. The moment one fails,
// record how many it took, shrink it, and return.
//
// If none fail, report the number tried and that nothing was found.
//
// The count of trials matters and is worth returning. It is the only measure of
// how hard the generator was working: one trial in two means it is aimed
// straight at the fault, and a hundred thousand with nothing means either the
// code is right or the generator cannot reach the fault. Those are very
// different and they look identical.
template <class T, class Generate, class Holds, class Smaller>
Counterexample<T> hunt(Source& source, long trials, Generate generate, Holds holds,
                       Smaller smaller) {
  (void)source;
  (void)generate;
  (void)holds;
  (void)smaller;
  Counterexample<T> result;
  result.trials = trials;
  return result;
}

// The candidates for a string: each one with a character removed, then each one
// with a character replaced by something simpler.
//
// Shorter first, because a shorter failing input is always the better one to
// look at, and simplifying only once nothing can be removed.
inline std::vector<std::string> simpler_strings(const std::string& text) {
  std::vector<std::string> candidates;
  candidates.reserve(text.size() * 2);

  for (std::size_t i = 0; i < text.size(); ++i) {
    std::string shorter = text;
    shorter.erase(i, 1);
    candidates.push_back(shorter);
  }
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '1') continue;
    std::string plainer = text;
    plainer[i] = '1';
    candidates.push_back(plainer);
  }
  return candidates;
}

#endif  // LESSON_SOLUTION_HPP
