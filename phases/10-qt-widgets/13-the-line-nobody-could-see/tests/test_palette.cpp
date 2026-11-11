#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const Rgb kBlack{0, 0, 0};
const Rgb kWhite{255, 255, 255};
const Rgb kGreen{0, 255, 0};
const Rgb kBlue{0, 0, 255};

// The rule that is wrong: dark if the average of the channels is under half.
Rgb naive_text_on(const Rgb& c) { return (c.r + c.g + c.b) / 3 > 127 ? kBlack : kWhite; }

}  // namespace

RC_TEST("relative luminance is 0 for black, 1 for white, and weighs green most") {
  RC_CHECK_NEAR(relative_luminance(kBlack), 0.0, 1e-12);
  RC_CHECK_NEAR(relative_luminance(kWhite), 1.0, 1e-9);
  RC_CHECK_NEAR(relative_luminance(Rgb{255, 0, 0}), 0.2126, 1e-4);
  RC_CHECK_NEAR(relative_luminance(kGreen), 0.7152, 1e-4);
  RC_CHECK_NEAR(relative_luminance(kBlue), 0.0722, 1e-4);
  RC_CHECK_NEAR(relative_luminance(Rgb{128, 128, 128}), 0.2159, 1e-3);     // half the channel value is a fifth of the light
  RC_CHECK_NEAR(relative_luminance(Rgb{5, 5, 5}), 5.0 / 255.0 / 12.92, 1e-9);   // the straight part near black
}

RC_TEST("the contrast of black on white is 21, of a colour on itself is 1, and it is the same either way round") {
  RC_CHECK_NEAR(contrast_ratio(kBlack, kWhite), 21.0, 1e-6);
  RC_CHECK_NEAR(contrast_ratio(kWhite, kBlack), 21.0, 1e-6);
  RC_CHECK_NEAR(contrast_ratio(kGreen, kGreen), 1.0, 1e-12);
  RC_CHECK_NEAR(contrast_ratio(kWhite, kGreen), contrast_ratio(kGreen, kWhite), 1e-12);
  std::cout << std::fixed << std::setprecision(2) << "  white on green " << contrast_ratio(kWhite, kGreen) << ", black on green " << contrast_ratio(kBlack, kGreen) << ", white on blue "
            << contrast_ratio(kWhite, kBlue) << ", black on blue " << contrast_ratio(kBlack, kBlue) << "\n\n";
  RC_CHECK_NEAR(contrast_ratio(kWhite, kGreen), 1.37, 0.01);
  RC_CHECK_NEAR(contrast_ratio(kBlack, kGreen), 15.30, 0.01);
  RC_CHECK_NEAR(contrast_ratio(kWhite, kBlue), 8.59, 0.01);
  RC_CHECK_NEAR(contrast_ratio(kBlack, kBlue), 2.44, 0.01);
}

RC_TEST("the average of the channels puts white text on green, and the contrast says black") {
  RC_CHECK(naive_text_on(kGreen) == kWhite);
  RC_CHECK(text_on(kGreen) == kBlack);
  RC_CHECK(text_on(kBlue) == kWhite);
  RC_CHECK(text_on(kWhite) == kBlack);
  RC_CHECK(text_on(kBlack) == kWhite);
  RC_CHECK(text_on(Rgb{240, 228, 66}) == kBlack);              // the yellow of the palette

  int naive_worse = 0;
  int below_target = 0;
  int total = 0;
  double worst = 100.0;
  for (int r = 0; r <= 255; r += 51) {
    for (int g = 0; g <= 255; g += 51) {
      for (int b = 0; b <= 255; b += 51) {
        const Rgb c{r, g, b};
        ++total;
        const double good = contrast_ratio(c, text_on(c));
        const double bad = contrast_ratio(c, naive_text_on(c));
        if (bad < good - 1e-9) ++naive_worse;
        if (good < 4.5) ++below_target;
        if (good < worst) worst = good;
      }
    }
  }
  std::cout << "  over " << total << " colours: the average rule picked the worse of black and white for " << naive_worse << ", and the contrast rule's worst result is " << worst << "\n\n";
  RC_CHECK_EQ(total, 216);
  RC_CHECK(naive_worse > 0);
  RC_CHECK_EQ(below_target, 0);                 // the better of black and white is always at least 4.5
}

RC_TEST("the palette has eight different colours that repeat, and a negative index counts back") {
  int same = 0;
  for (int i = 0; i < 8; ++i)
    for (int j = i + 1; j < 8; ++j)
      if (series_color(i) == series_color(j)) ++same;
  RC_CHECK_EQ(same, 0);
  RC_CHECK(series_color(0) == (Rgb{0, 114, 178}));
  RC_CHECK(series_color(7) == kBlack);
  RC_CHECK(series_color(8) == series_color(0));
  RC_CHECK(series_color(21) == series_color(5));
  RC_CHECK(series_color(-1) == series_color(7));
  RC_CHECK(series_color(-8) == series_color(0));
  RC_CHECK(series_color(-2147483647 - 1) == series_color(0));    // the smallest int: no overflow, no crash
}

RC_TEST("on a white background only some of the palette can be seen, and the yellow is not one") {
  std::cout << std::fixed << std::setprecision(2) << "  contrast of the eight colours on white:";
  for (int i = 0; i < 8; ++i) std::cout << " " << contrast_ratio(series_color(i), kWhite);
  std::cout << "\n";
  const std::vector<int> lines = usable_series(kWhite, 3.0);
  std::cout << "  usable for a line (at least 3): " << lines.size() << " of 8, indices";
  for (int i : lines) std::cout << " " << i;
  std::cout << "\n\n";

  std::vector<int> expected;
  for (int i = 0; i < 8; ++i)
    if (contrast_ratio(series_color(i), kWhite) >= 3.0) expected.push_back(i);
  RC_CHECK(lines == expected);
  RC_CHECK(lines.size() >= 3 && lines.size() <= 6);
  bool has_blue = false, has_yellow = false;
  for (int i : lines) {
    if (i == 0) has_blue = true;
    if (i == 6) has_yellow = true;
  }
  RC_CHECK(has_blue);
  RC_CHECK(!has_yellow);
  RC_CHECK_NEAR(contrast_ratio(series_color(6), kWhite), 1.32, 0.02);

  RC_CHECK_EQ(static_cast<int>(usable_series(kWhite, 1.0).size()), 8);
  RC_CHECK_EQ(static_cast<int>(usable_series(kWhite, 25.0).size()), 0);
  RC_CHECK_EQ(static_cast<int>(usable_series(kBlack, 3.0).size()) < 8, true);        // black does not show on black
}
