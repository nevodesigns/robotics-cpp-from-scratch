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
//
// Every zero below is a placeholder. Do not run the suite to find the answers:
// trace each case on paper from the code above, write your number, and only then
// run it. The suite lists every guess that was wrong, with what you said and what
// the code returned, and a wrong guess is where the learning is.
//
// Start with the comment's claim, write down what it predicts, and then read the
// code to see whether the claim is true. Several are not.

const double kSmoothGuesses[kSmoothCases] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
const double kDeadbandGuesses[kDeadbandCases] = {0.0, 0.0, 0.0, 0.0, 0.0};
const double kLastMeanGuesses[kLastMeanCases] = {0.0, 0.0, 0.0, 0.0, 0.0};
const int kPercentGuesses[kPercentCases] = {0, 0, 0, 0, 0};
const bool kStaleGuesses[kStaleCases] = {false, false, false, false};

// ---- Your change. ----

// TODO: percent, fixed. 0 to 1023 maps to 0 to 100, with 1023 giving exactly 100.
//
// The old percent divides by 1024. That is the number of values an ADC can give,
// and the full scale is one less, 1023. The least change is that one number.
// Do not also round: that is a different change, it moves more than half of all
// the answers, and the suite will name it as too big.
//
//   - divide by 1023 and nothing else;
//   - a raw below 0 gives 0 and a raw above 1023 gives 100, not an extrapolation.
//
// The suite checks it against the old function across the whole range, so a
// change that fixes 1023 and moves the middle will be named.
inline int percent_exact(int raw) {
  (void)raw;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
