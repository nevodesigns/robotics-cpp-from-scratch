#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"

namespace {

constexpr double kWidth = 1000.0;

// Where a time is on a chart of this window.
double x_of(const Window& w, double t) { return (t - w.t0) / (w.t1 - w.t0) * kWidth; }

}  // namespace

RC_TEST("zooming about the middle throws the point under the cursor off the screen, and zooming about it leaves it put") {
  const Window w{0.0, 100.0};
  const double anchor = 80.0;
  const double factor = 2.0;

  // About the middle, which is what a first zoom does.
  const double mid = (w.t0 + w.t1) / 2.0;
  const double span = (w.t1 - w.t0) / factor;
  const Window centred{mid - span / 2.0, mid + span / 2.0};
  const Window about = zoom_about(w, anchor, factor);

  std::cout << "  the point at time 80 is at x = " << x_of(w, anchor) << "; after zooming by 2 about the middle it is at x = " << x_of(centred, anchor)
            << ", about the cursor at x = " << x_of(about, anchor) << "\n";
  RC_CHECK_NEAR(x_of(w, anchor), 800.0, 1e-9);
  RC_CHECK_NEAR(x_of(centred, anchor), 1100.0, 1e-9);        // beyond the right edge
  RC_CHECK_NEAR(x_of(about, anchor), 800.0, 1e-9);
  RC_CHECK_NEAR(about.t1 - about.t0, 50.0, 1e-9);
}

RC_TEST("the anchor keeps its place for every window, anchor and factor, in and out") {
  const double starts[] = {0.0, -5.0, 1234.5};
  const double spans[] = {0.5, 10.0, 300.0};
  const double fractions[] = {0.0, 0.25, 0.9, 1.0};
  const double factors[] = {0.1, 0.5, 1.0, 1.2, 2.0, 7.5};
  int checked = 0;
  int moved = 0;
  int wrong_width = 0;
  for (double start : starts) {
    for (double span : spans) {
      for (double fraction : fractions) {
        for (double factor : factors) {
          const Window w{start, start + span};
          const double anchor = start + fraction * span;
          const Window z = zoom_about(w, anchor, factor);
          ++checked;
          if (std::fabs(x_of(z, anchor) - x_of(w, anchor)) > 1e-6) ++moved;
          if (std::fabs((z.t1 - z.t0) - span / factor) > 1e-9 * span) ++wrong_width;
        }
      }
    }
  }
  std::cout << "  " << checked << " zooms: " << moved << " moved the anchor, " << wrong_width << " had the wrong width\n\n";
  RC_CHECK_EQ(checked, 216);
  RC_CHECK_EQ(moved, 0);
  RC_CHECK_EQ(wrong_width, 0);

  const Window w{3.0, 9.0};
  RC_CHECK_NEAR(zoom_about(w, 5.0, 0.0).t0, 3.0, 1e-15);       // a factor that is not positive changes nothing
  RC_CHECK_NEAR(zoom_about(w, 5.0, -2.0).t1, 9.0, 1e-15);
}

RC_TEST("a wheel step of 1.12 in and 0.88 out does not come back, and 1.2 to the power of the notches does") {
  Window naive{0.0, 100.0};
  Window good{0.0, 100.0};
  const double anchor = 40.0;
  for (int i = 0; i < 5; ++i) {
    naive = zoom_about(naive, anchor, 1.0 + 120.0 / 1000.0);      // one notch in: 1.12
    good = zoom_about(good, anchor, wheel_factor(120));
  }
  for (int i = 0; i < 5; ++i) {
    naive = zoom_about(naive, anchor, 1.0 - 120.0 / 1000.0);      // one notch out: 0.88
    good = zoom_about(good, anchor, wheel_factor(-120));
  }
  std::cout << "  five notches in and five out, starting at a width of 100: the 1.12 and 0.88 steps leave " << naive.t1 - naive.t0 << ", the powers of 1.2 leave "
            << good.t1 - good.t0 << "\n\n";
  RC_CHECK(std::fabs((naive.t1 - naive.t0) - 100.0) > 5.0);
  RC_CHECK_NEAR(good.t0, 0.0, 1e-9);
  RC_CHECK_NEAR(good.t1, 100.0, 1e-9);
  RC_CHECK_NEAR(wheel_factor(120), 1.2, 1e-12);
  RC_CHECK_NEAR(wheel_factor(120) * wheel_factor(-120), 1.0, 1e-12);
  RC_CHECK_NEAR(wheel_factor(0), 1.0, 1e-12);
  RC_CHECK_NEAR(wheel_factor(60), std::sqrt(1.2), 1e-12);     // a half notch, as a trackpad reports
}

RC_TEST("dragging right moves the window left by the same distance on the screen") {
  const Window w{0.0, 100.0};
  const Window dragged = pan_by_pixels(w, 100.0, kWidth);       // 100 pixels is a tenth of the width
  std::cout << "  a drag of 100 pixels to the right on 1000 pixels of 0 to 100 gives " << dragged.t0 << " to " << dragged.t1 << "\n\n";
  RC_CHECK_NEAR(dragged.t0, -10.0, 1e-9);
  RC_CHECK_NEAR(dragged.t1, 90.0, 1e-9);
  // The point that was under the cursor, at x = 500 (time 50), is now at x = 600 (the hand moved 100 pixels).
  RC_CHECK_NEAR(x_of(dragged, 50.0), 600.0, 1e-9);
  const Window back = pan_by_pixels(dragged, -100.0, kWidth);
  RC_CHECK_NEAR(back.t0, 0.0, 1e-9);
  RC_CHECK_NEAR(back.t1, 100.0, 1e-9);
  RC_CHECK_NEAR(pan_by_pixels(w, 50.0, 0.0).t1, 100.0, 1e-15);      // no width: unchanged
}

RC_TEST("the window stops at the ends of the data, keeps its width when pushed, and cannot be narrower than the minimum") {
  const double lo = 0.0, hi = 60.0;
  const Window before = clamp_to(Window{-10.0, 15.0}, lo, hi, 1.0);
  const Window after = clamp_to(Window{50.0, 75.0}, lo, hi, 1.0);
  const Window inside = clamp_to(Window{10.0, 20.0}, lo, hi, 1.0);
  const Window too_wide = clamp_to(Window{-100.0, 400.0}, lo, hi, 1.0);
  const Window too_narrow = clamp_to(Window{29.9999, 30.0001}, lo, hi, 1.0);
  std::cout << "  past the start " << before.t0 << " to " << before.t1 << "; past the end " << after.t0 << " to " << after.t1 << "; too wide " << too_wide.t0 << " to "
            << too_wide.t1 << "; too narrow, width " << too_narrow.t1 - too_narrow.t0 << "\n\n";
  RC_CHECK_NEAR(before.t0, 0.0, 1e-12);
  RC_CHECK_NEAR(before.t1, 25.0, 1e-12);
  RC_CHECK_NEAR(after.t0, 35.0, 1e-12);
  RC_CHECK_NEAR(after.t1, 60.0, 1e-12);
  RC_CHECK_NEAR(inside.t0, 10.0, 1e-12);
  RC_CHECK_NEAR(inside.t1, 20.0, 1e-12);
  RC_CHECK_NEAR(too_wide.t0, 0.0, 1e-12);
  RC_CHECK_NEAR(too_wide.t1, 60.0, 1e-12);
  RC_CHECK_NEAR(too_narrow.t1 - too_narrow.t0, 1.0, 1e-9);
  RC_CHECK_NEAR((too_narrow.t0 + too_narrow.t1) / 2.0, 30.0, 1e-9);

  // Data narrower than the minimum is shown whole.
  const Window tiny = clamp_to(Window{0.0, 1.0}, 0.0, 0.4, 1.0);
  RC_CHECK_NEAR(tiny.t0, 0.0, 1e-12);
  RC_CHECK_NEAR(tiny.t1, 0.4, 1e-12);
  const Window no_data = clamp_to(Window{2.0, 3.0}, 5.0, 5.0, 1.0);
  RC_CHECK_NEAR(no_data.t0, 5.0, 1e-12);
  RC_CHECK_NEAR(no_data.t1, 5.0, 1e-12);
}
