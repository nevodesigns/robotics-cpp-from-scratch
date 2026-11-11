// rc/plot/zoom.hpp
//
// Zoom, pan and clamp, from lesson 10-09, graduated.
//
// Zooming by 2 about the middle moved the point at time 80 on a 100 second, 1000 pixel chart from x = 800 to x = 1100, off the screen; zooming about it left it at 800. A wheel step of 1.12 in and 0.88 out was 0.9856 and
// five of each left the window at a width of 107.5 where it began at 100; 1.2 to the power of the notches came back to the window it started with.

#ifndef RC_PLOT_ZOOM_HPP
#define RC_PLOT_ZOOM_HPP

#include <cmath>

namespace rc {
namespace plot {

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
  if (!(factor > 0.0)) return w;
  const double span = w.t1 - w.t0;
  if (!(span > 0.0)) return w;
  const double fraction = (anchor - w.t0) / span;
  const double new_span = span / factor;
  Window out;
  out.t0 = anchor - fraction * new_span;
  out.t1 = out.t0 + new_span;
  return out;
}

// The window after dragging by `dx` pixels to the right on a chart `width` pixels wide: the data follows the hand, so the window moves the other way.
// A width that is not positive leaves it as it is.
inline Window pan_by_pixels(const Window& w, double dx, double width) {
  if (!(width > 0.0)) return w;
  const double shift = -dx / width * (w.t1 - w.t0);
  Window out;
  out.t0 = w.t0 + shift;
  out.t1 = w.t1 + shift;
  return out;
}

// The zoom factor for a wheel turn of `angle_delta` (120 for one notch, as Qt reports it, positive away from the user): 1.2 to the power of the notches, so
// that a turn in and the same turn out multiply to exactly 1.
inline double wheel_factor(int angle_delta) { return std::pow(1.2, static_cast<double>(angle_delta) / 120.0); }

// The window kept inside the data from `lo` to `hi`, and no narrower than `min_span`. Too wide becomes the whole of it; past an end it is pushed back with
// its width kept. If the data is itself narrower than min_span the window is the data.
inline Window clamp_to(const Window& w, double lo, double hi, double min_span) {
  const double whole = hi - lo;
  Window out = w;
  if (!(whole > 0.0)) return Window{lo, hi};
  if (whole <= min_span) return Window{lo, hi};
  double span = w.t1 - w.t0;
  if (span < min_span) span = min_span;
  if (span > whole) span = whole;
  const double middle = (w.t0 + w.t1) / 2.0;
  out.t0 = middle - span / 2.0;
  out.t1 = out.t0 + span;
  if (out.t0 < lo) {
    out.t0 = lo;
    out.t1 = lo + span;
  }
  if (out.t1 > hi) {
    out.t1 = hi;
    out.t0 = hi - span;
  }
  return out;
}
}  // namespace plot
}  // namespace rc

#endif  // RC_PLOT_ZOOM_HPP
