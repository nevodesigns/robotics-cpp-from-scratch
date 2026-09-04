// rc/test/generate.hpp
//
// A seeded source of inputs, a property to hold them to, and a shrinker that
// makes the failure readable, from lesson 05-04.
//
// Nine hand written cases against a parser with no overflow check found nothing,
// because the person who thinks of the cases is the person who wrote the code.
// Generated inputs found it in 1.6 values on average, and the difference between
// finding it at once and never finding it was entirely the generator:
//
//   generator            found it in   mean values
//   any printable byte      0 of 20              -
//   digits, up to 40       20 of 20            1.6
//   digits, up to 10        0 of 20              -
//
// The first almost never emits twenty digits in a row. The third cannot emit
// twenty digits at all, so it would report that parser correct for ever. A
// passing property test is a statement about the generator's reach as much as
// about the code.
//
// Shrinking took the failures from a mean of 29.17 characters to exactly 20,
// which is the shortest number the parser gets wrong, without being told.

#ifndef RC_TEST_GENERATE
#define RC_TEST_GENERATE

#include <cstdint>
#include <string>
#include <vector>

namespace rc {
namespace test {

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

  // A value in [0, limit), or 0 when the limit is not positive.
  int below(int limit) {
    state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
    if (limit <= 0) return 0;
    return static_cast<int>((state_ >> 11) % static_cast<std::uint64_t>(limit));
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

// Make a failing value as small as it can be while it still fails.
//
// `smaller` proposes candidates and knows nothing about why the value failed;
// `holds` decides. Take the first candidate that still breaks the property and
// start again from there, until nothing proposed breaks it any more.
//
// This is what makes generated testing usable. A generator that finds a fault
// hands you thirty six characters of noise; the same fault shrunk is twenty
// digits, all but the first of them a 1, and the shape of the bug is legible in
// the value itself.
template <class T, class Smaller, class Holds>
T shrink(T failing, Smaller smaller, Holds holds, int* steps = nullptr) {
  bool improved = true;
  while (improved) {
    improved = false;
    for (const T& candidate : smaller(failing)) {
      if (!holds(candidate)) {
        failing = candidate;
        improved = true;
        if (steps != nullptr) ++*steps;
        break;
      }
    }
  }
  return failing;
}

// Generate values until one breaks the property, then shrink it.
//
// The count of trials is reported because it is the only measure of how hard
// the generator was working. One trial in two means the generator is aimed
// straight at the fault; a million trials and nothing means either the code is
// right or the generator cannot reach the fault, and those are very different
// and look identical.
template <class T, class Generate, class Holds, class Smaller>
Counterexample<T> hunt(Source& source, long trials, Generate generate, Holds holds,
                       Smaller smaller) {
  Counterexample<T> result;
  for (long i = 1; i <= trials; ++i) {
    const T candidate = generate(source);
    if (holds(candidate)) continue;

    result.found = true;
    result.trials = i;
    result.value = shrink(candidate, smaller, holds, &result.shrink_steps);
    return result;
  }
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

}  // namespace test
}  // namespace rc

#endif  // RC_TEST_GENERATE
