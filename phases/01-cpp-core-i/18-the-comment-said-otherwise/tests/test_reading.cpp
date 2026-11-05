#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// What a reader who trusted each comment would have predicted. These are the
// claims, written out, and they are not the code.
double comment_smooth(double estimate, double reading) { return estimate + 0.2 * (reading - estimate); }
double comment_deadband(double value, double width) { return std::fabs(value) < width ? 0.0 : value; }
double comment_last_mean(const MeanCase& c) {
  const std::size_t take = std::min(c.readings.size(), static_cast<std::size_t>(c.n));
  double sum = 0.0;
  for (std::size_t i = c.readings.size() - take; i < c.readings.size(); ++i) sum += c.readings[i];
  return sum / static_cast<double>(take);
}
int comment_percent(int raw) { return static_cast<int>(std::lround(raw * 100.0 / 1023.0)); }
bool comment_stale(double age, double limit) { return age >= limit; }

bool same(double a, double b) { return std::fabs(a - b) < 1e-9; }

}  // namespace

RC_TEST("a comment is a claim and the code is the fact, and they disagree in 12 of 25 cases") {
  int smooth_wrong = 0;
  int deadband_wrong = 0;
  int mean_wrong = 0;
  int percent_wrong = 0;
  int stale_wrong = 0;

  for (int i = 0; i < kSmoothCases; ++i)
    if (!same(comment_smooth(kSmoothInputs[i].first, kSmoothInputs[i].second),
              smooth(kSmoothInputs[i].first, kSmoothInputs[i].second)))
      ++smooth_wrong;
  for (int i = 0; i < kDeadbandCases; ++i)
    if (!same(comment_deadband(kDeadbandInputs[i].first, kDeadbandInputs[i].second),
              deadband(kDeadbandInputs[i].first, kDeadbandInputs[i].second)))
      ++deadband_wrong;
  for (int i = 0; i < kLastMeanCases; ++i) {
    const MeanCase c = last_mean_case(i);
    if (!same(comment_last_mean(c), last_mean(c.readings, c.n))) ++mean_wrong;
  }
  for (int i = 0; i < kPercentCases; ++i)
    if (comment_percent(kPercentInputs[i]) != percent(kPercentInputs[i])) ++percent_wrong;
  for (int i = 0; i < kStaleCases; ++i)
    if (comment_stale(kStaleInputs[i].first, kStaleInputs[i].second) !=
        is_stale(static_cast<int>(kStaleInputs[i].first), static_cast<int>(kStaleInputs[i].second)))
      ++stale_wrong;

  const int total = smooth_wrong + deadband_wrong + mean_wrong + percent_wrong + stale_wrong;
  std::cout << "  five functions, each with a comment, and where the comment was wrong:\n\n";
  std::cout << "    smooth      " << smooth_wrong << " of " << kSmoothCases << "\n";
  std::cout << "    deadband    " << deadband_wrong << " of " << kDeadbandCases << "\n";
  std::cout << "    last_mean   " << mean_wrong << " of " << kLastMeanCases << "\n";
  std::cout << "    percent     " << percent_wrong << " of " << kPercentCases << "\n";
  std::cout << "    is_stale    " << stale_wrong << " of " << kStaleCases << "\n";
  std::cout << "    all         " << total << " of 25\n\n";
  std::cout << "  is_stale is the control: its comment is exactly right. Reading code is\n";
  std::cout << "  not distrusting it, it is finding out which comment is the wrong one.\n\n";

  RC_CHECK_EQ(smooth_wrong, 5);
  RC_CHECK_EQ(deadband_wrong, 3);
  RC_CHECK_EQ(mean_wrong, 2);
  RC_CHECK_EQ(percent_wrong, 2);
  RC_CHECK_EQ(stale_wrong, 0);
  RC_CHECK_EQ(total, 12);
}

RC_TEST("your guesses, traced by hand, against what the code does") {
  std::vector<std::string> misses;
  int right = 0;

  auto miss = [&](const std::string& which, int index, double said, double actual) {
    std::ostringstream line;
    line << which << " case " << index << ": you said " << said << ", the code gave " << actual;
    misses.push_back(line.str());
  };

  for (int i = 0; i < kSmoothCases; ++i) {
    const double actual = smooth(kSmoothInputs[i].first, kSmoothInputs[i].second);
    if (same(kSmoothGuesses[i], actual)) ++right; else miss("smooth", i, kSmoothGuesses[i], actual);
  }
  for (int i = 0; i < kDeadbandCases; ++i) {
    const double actual = deadband(kDeadbandInputs[i].first, kDeadbandInputs[i].second);
    if (same(kDeadbandGuesses[i], actual)) ++right; else miss("deadband", i, kDeadbandGuesses[i], actual);
  }
  for (int i = 0; i < kLastMeanCases; ++i) {
    const MeanCase c = last_mean_case(i);
    const double actual = last_mean(c.readings, c.n);
    if (same(kLastMeanGuesses[i], actual)) ++right; else miss("last_mean", i, kLastMeanGuesses[i], actual);
  }
  for (int i = 0; i < kPercentCases; ++i) {
    const int actual = percent(kPercentInputs[i]);
    if (kPercentGuesses[i] == actual) ++right; else miss("percent", i, kPercentGuesses[i], actual);
  }
  for (int i = 0; i < kStaleCases; ++i) {
    const bool actual = is_stale(static_cast<int>(kStaleInputs[i].first), static_cast<int>(kStaleInputs[i].second));
    if (kStaleGuesses[i] == actual) ++right; else miss("is_stale", i, kStaleGuesses[i] ? 1 : 0, actual ? 1 : 0);
  }

  std::cout << "  " << right << " of 25 guesses were right.\n";
  for (const std::string& line : misses) std::cout << "    " << line << "\n";
  std::cout << "\n";

  RC_CHECK_EQ(right, 25);
}

RC_TEST("a fix that changes as little as it can, and says how little") {
  // Pin what the old one did across the whole range, before judging the new one.
  int differs = 0;
  int largest_gap = 0;
  int previous = -1;
  bool never_decreases = true;
  bool in_range = true;

  for (int raw = 0; raw <= 1023; ++raw) {
    const int fixed = percent_exact(raw);
    const int old_answer = percent(raw);
    if (fixed != old_answer) ++differs;
    if (std::abs(fixed - old_answer) > largest_gap) largest_gap = std::abs(fixed - old_answer);
    if (fixed < previous) never_decreases = false;
    if (fixed < 0 || fixed > 100) in_range = false;
    previous = fixed;
  }

  std::cout << "  percent_exact against the old percent, over raw 0 to 1023:\n";
  std::cout << "    values where they differ   " << differs << " of 1024\n";
  std::cout << "    largest gap                " << largest_gap << "\n";
  std::cout << "    old percent(1023)          " << percent(1023) << "\n";
  std::cout << "    percent_exact(1023)        " << percent_exact(1023) << "\n\n";

  RC_CHECK_EQ(percent_exact(0), 0);
  RC_CHECK_EQ(percent_exact(1023), 100);
  RC_CHECK_EQ(percent_exact(512), 50);
  RC_CHECK_EQ(percent_exact(256), 25);
  RC_CHECK(never_decreases);
  RC_CHECK(in_range);
  RC_CHECK(largest_gap <= 1);

  // The least change: it differs from the old one somewhere, and in few places.
  // Dividing by 1023 moves 49 answers. Rounding as well moves 560, which is a
  // second decision and is refused here for being the larger change.
  RC_CHECK(differs > 0);
  RC_CHECK(differs <= 100);

  // Outside the range it clamps, and does not extrapolate.
  RC_CHECK_EQ(percent_exact(-5), 0);
  RC_CHECK_EQ(percent_exact(5000), 100);
}
