#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

// Speed from the counts of an encoder, by counting how many arrived over a window of
// samples: at a low speed one sample holds zero or one count, so the speed from a single
// difference is zero or one count per sample. A longer window has the resolution of a
// count over the whole window, and costs delay.
class WindowVelocity {
 public:
  WindowVelocity(std::size_t window, double sample_period)
      : window_(window == 0 ? 1 : window), dt_(sample_period), history_(window_ + 1, 0) {}

  // The latest count, in; the speed in counts a second, out.
  double update(long count) {
    // TODO: keep the last window + 1 counts in a ring (head_ is where the next goes, seen_
    // counts how many are valid, up to window + 1).
    //
    // The speed is the newest count minus the oldest count still in the window, divided by
    // the time between them: the number of samples between them, which is seen_ - 1, times
    // dt_. Until the window has filled, use the oldest it has: never divide by a window it
    // has not seen. With a single count there is no speed yet: zero.
    (void)count;
    (void)head_;
    (void)seen_;
    (void)dt_;
    (void)history_;
    return 0.0;
  }

  // How late it is: the average over a window is the value half a window ago.
  double delay_seconds() const { return 0.5 * static_cast<double>(window_) * dt_; }

 private:
  std::size_t window_;
  double dt_;
  std::vector<long> history_;
  std::size_t head_ = 0;
  std::size_t seen_ = 0;
};

// Speed from the time between counts.
class PeriodVelocity {
 public:
  // An edge: the time, from a timer, at which the count changed, and which way (+1 or -1).
  void on_edge(double time, int direction) {
    // TODO: a change of direction is not a speed. The edge after a reversal is the same
    // position crossed the other way, so the time since the last edge says nothing about how
    // fast it is going. On a reversal, take the new direction and forget the previous edge:
    // there is nothing to pair this one with.
    //
    // On an edge in the same direction as before, the last edge becomes the previous one.
    // Either way this edge is now the last.
    (void)time;
    (void)direction;
    (void)have_last_;
    (void)have_previous_;
    (void)last_;
    (void)previous_;
    (void)direction_;
  }

  // Counts a second, signed, at time `now`. Zero when there is nothing to say.
  double estimate(double now) const {
    // TODO: with two edges in the same direction, the speed is one count over the gap
    // between them, signed by the direction. With fewer, zero.
    //
    // A stopped encoder produces no edges, so a speed computed only from the last two would
    // hold the last speed for ever. If more than a gap has passed since the last edge, the
    // speed can only be lower than the gap says: it is at most one count over the time since
    // the last edge. Use that instead.
    (void)now;
    return 0.0;
  }

 private:
  bool have_last_ = false;
  bool have_previous_ = false;
  double last_ = 0.0;
  double previous_ = 0.0;
  int direction_ = 1;
};

// Which to believe: the edge timing at a low speed, where counts are far apart, and the
// window at a high one.
inline double pick_velocity(double period_estimate, double window_estimate, double threshold) {
  // TODO: the period estimate if there is one (not zero) and its size is below the
  // threshold, otherwise the window estimate.
  (void)period_estimate;
  (void)window_estimate;
  (void)threshold;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
