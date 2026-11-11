#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// A chart is a window onto time, and the three things a person does to it are zoom with the wheel, drag to pan, and run into the end of the data. Each has
// a way of being nearly right. Zoom about the middle and the point under the cursor flies off the screen. A wheel step that is 1.12 in and 0.88 out does not come
// back to where it started. A pan that is not clamped shows the empty space before the first sample.

// The visible interval of time.
struct Window {
  double t0 = 0.0;
  double t1 = 1.0;
};

// The window after zooming by `factor` (above 1 shows less time, below 1 more), with the time `anchor` staying at the same fraction of the window, so that
// the point under the cursor stays under it. A factor that is not positive leaves the window as it is.
inline Window zoom_about(const Window& w, double anchor, double factor) {
  // TODO: if factor is not positive, or the window has no width, return w. fraction = (anchor - t0) / span. new_span = span / factor.
  // t0 = anchor - fraction * new_span, and t1 = t0 + new_span.
  (void)w;
  (void)anchor;
  (void)factor;
  return Window{0.0, 0.0};
}

// The window after dragging by `dx` pixels to the right on a chart `width` pixels wide: the data follows the hand, so the window moves the other way.
// A width that is not positive leaves it as it is.
inline Window pan_by_pixels(const Window& w, double dx, double width) {
  // TODO: shift = -dx / width * (t1 - t0), added to both ends. Return w if width is not positive.
  (void)w;
  (void)dx;
  (void)width;
  return Window{0.0, 0.0};
}

// The zoom factor for a wheel turn of `angle_delta` (120 for one notch, as Qt reports it, positive away from the user): 1.2 to the power of the notches, so
// that a turn in and the same turn out multiply to exactly 1.
inline double wheel_factor(int angle_delta) {
  // TODO: std::pow(1.2, angle_delta / 120.0).
  (void)angle_delta;
  return 0.0;
}

// The window kept inside the data from `lo` to `hi`, and no narrower than `min_span`. Too wide becomes the whole of it; past an end it is pushed back with
// its width kept. If the data is itself narrower than min_span the window is the data.
inline Window clamp_to(const Window& w, double lo, double hi, double min_span) {
  // TODO: whole = hi - lo; if whole is not positive or is at most min_span, the window is {lo, hi}. span = the window's width, raised to min_span and cut to
  // whole. Centre it where it was: t0 = middle - span / 2. If t0 < lo, t0 = lo; if t1 > hi, t1 = hi and t0 = hi - span.
  (void)w;
  (void)lo;
  (void)hi;
  (void)min_span;
  return Window{0.0, 0.0};
}

#endif  // LESSON_SOLUTION_HPP
