#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <vector>

// Every lesson so far has handed you code to write. Most of a working life is the
// other thing: code somebody else wrote, with a comment on top of it, that you
// have to understand before you can change it. Nothing in this curriculum has
// taught that, and it is a skill with a method.
//
// The method is to treat a comment as a CLAIM and the code as the FACT, and to find
// out where they differ by predicting before running. Below are five functions
// from a robot's sensor path, written by someone who is no longer here. Each has
// a comment saying what it does.
//
// Measured on this file's own cases, 25 in all. A reader who trusted every
// comment predicted wrongly in 12 of them:
//
//   smooth      5 of 6   the comment says 20 percent, the code moves 80
//   deadband    3 of 5   the comment says unchanged, the code shifts it
//   last_mean   2 of 5   the comment says all of them, the code divides by n
//   percent     2 of 5   the comment says 0 to 100, the code stops at 99
//   is_stale    0 of 4   the comment is exactly right
//
// The last row matters as much as the others. Reading code is not distrusting
// it. It is finding out which of the comments is the one that is wrong, and the
// only way to find out is to trace an input by hand and see.
//
// Your job has two halves. First, fill in the GUESSES below: for each case, what
// the code returns, worked out by reading it and tracing the arithmetic, BEFORE
// you run anything. The suite compares your guesses with what the code does and
// tells you which ones were wrong, so run it afterwards and learn from the miss.
// Second, change one of the functions safely, which is a different skill from
// reading it and depends on it.
//
// The method, in the order that works:
//
//   1. Find where it is called and what it is given, before reading the body.
//   2. Read the signature and the comment as a claim, and write the claim down.
//   3. Pick the smallest input that exercises the line you are unsure of, and
//      trace it with a pencil. Do not run it yet.
//   4. Predict. Then run. A wrong prediction is the most useful thing that
//      can happen, because it marks the exact line you misread.
//   5. Pin what it does with a test that records the current answers, before
//      you change anything. If a change moves an answer, you will know which.
//   6. Change the least you can, and run the pin again.

// ---- The code you did not write. Do not edit it. ----

// Smooths a noisy reading. Exponential moving average with a smoothing factor of
// 0.2, so each new reading moves the estimate 20 percent of the way towards it.
inline double smooth(double estimate, double reading) {
  const double alpha = 0.8;
  return estimate * (1.0 - alpha) + reading * alpha;
}

// Anything within width of zero reads as zero. Anything further out passes
// through unchanged.
inline double deadband(double value, double width) {
  if (value > -width && value < width) return 0.0;
  return value > 0.0 ? value - width : value + width;
}

// Mean of the last n readings, or of all of them if there are fewer than n.
inline double last_mean(const std::vector<double>& readings, int n) {
  const std::size_t count = readings.size();
  const std::size_t wanted = static_cast<std::size_t>(n);
  const std::size_t start = count > wanted ? count - wanted : 0;
  double sum = 0.0;
  for (std::size_t i = start; i < count; ++i) sum += readings[i];
  return sum / n;
}

// Converts a 10 bit ADC reading, 0 to 1023, to a percentage, 0 to 100.
inline int percent(int raw) { return raw * 100 / 1024; }

// True when the reading is as old as the limit or older.
inline bool is_stale(int age_ms, int limit_ms) { return age_ms >= limit_ms; }

// ---- The cases. Do not edit them either. ----

struct Pair {
  double first;
  double second;
};

const int kSmoothCases = 6;
const Pair kSmoothInputs[kSmoothCases] = {{10, 20}, {10, 10}, {0, 5}, {-4, 4}, {100, 0}, {3, 3.5}};

const int kDeadbandCases = 5;
const Pair kDeadbandInputs[kDeadbandCases] = {{0.05, 0.1}, {0.5, 0.1}, {-0.5, 0.1}, {0.0, 0.1}, {2.0, 0.5}};

struct MeanCase {
  std::vector<double> readings;
  int n;
};

const int kLastMeanCases = 5;
inline MeanCase last_mean_case(int index) {
  switch (index) {
    case 0: return {{1, 2, 3, 4}, 2};
    case 1: return {{1, 2, 3, 4}, 4};
    case 2: return {{2, 4}, 4};
    case 3: return {{5}, 3};
    default: return {{1, 2, 3, 4, 5}, 3};
  }
}

const int kPercentCases = 5;
const int kPercentInputs[kPercentCases] = {0, 256, 512, 767, 1023};

const int kStaleCases = 4;
const Pair kStaleInputs[kStaleCases] = {{99, 100}, {100, 100}, {101, 100}, {0, 100}};

// ---- Your guesses: what each call returns. Work them out by hand first. ----

const double kSmoothGuesses[kSmoothCases] = {18.0, 10.0, 4.0, 2.4, 20.0, 3.4};
const double kDeadbandGuesses[kDeadbandCases] = {0.0, 0.4, -0.4, 0.0, 1.5};
const double kLastMeanGuesses[kLastMeanCases] = {3.5, 2.5, 1.5, 5.0 / 3.0, 4.0};
const int kPercentGuesses[kPercentCases] = {0, 25, 50, 74, 99};
const bool kStaleGuesses[kStaleCases] = {false, true, true, false};

// ---- Your change. ----

// percent, fixed: 0 to 1023 maps to 0 to 100, with 1023 giving exactly 100.
//
// The old one divided by 1024, which is the number of values and not the size of
// the range. The full scale is 1023, and changing that one number is the whole
// fix. Measured against the old function over 0 to 1023: it differs on 49 of 1024
// values, never by more than 1, never decreases, and gives 100 at the top.
//
// Rounding to the nearest percent is a different change and a bigger one. It
// moves 560 of the 1024 answers, because the old function truncated and every
// caller has been living with that. If rounding is wanted it is a second
// decision, made on its own, with the first one already pinned.
//
// Anything outside the range is clamped rather than extrapolated.
inline int percent_exact(int raw) {
  if (raw <= 0) return 0;
  if (raw >= 1023) return 100;
  return raw * 100 / 1023;
}

#endif  // LESSON_SOLUTION_HPP
