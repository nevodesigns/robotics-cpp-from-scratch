#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

// Speed from the counts of an encoder, by looking at how many arrived over a window of
// samples.
//
// An encoder reports a position in whole counts, and a speed has to be made from it. The
// obvious way is to take the difference between this sample and the last and divide by the
// time. At a high speed that is fine. At a low one a sample holds none of a count or one,
// so the speed it reports is zero or one count per sample, which is a hundred percent out
// at half a count per sample.
//
// A longer window averages that away. It counts the counts over `window` samples, so the
// resolution is a count over the whole window and not over one sample, and the price is
// delay: the number it gives is the average speed over the window, which is the speed half
// a window ago.
class WindowVelocity {
 public:
  WindowVelocity(std::size_t window, double sample_period)
      : window_(window == 0 ? 1 : window), dt_(sample_period), history_(window_ + 1, 0) {}

  // The latest count, in; the speed in counts a second, out. Until the window has filled
  // it uses the oldest count it has, so it never divides by a window it has not seen.
  double update(long count) {
    history_[head_] = count;
    head_ = (head_ + 1) % history_.size();
    if (seen_ < history_.size()) ++seen_;

    // The oldest count still in the window is `seen_ - 1` samples back, up to window_.
    const std::size_t span = seen_ - 1;
    if (span == 0) return 0.0;
    const std::size_t oldest = (head_ + history_.size() - seen_) % history_.size();
    const std::size_t newest = (head_ + history_.size() - 1) % history_.size();
    return static_cast<double>(history_[newest] - history_[oldest]) / (static_cast<double>(span) * dt_);
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
//
// At a low speed counts come far apart, and the time between two of them says the speed
// to within the resolution of a clock, however slow it is. At a high speed they come so
// close together that the clock's resolution is a large part of the gap, so it fails
// the other way.
//
// Two cases make it more than a division. A stopped encoder produces no edges, so a speed
// computed only from the last two would hold the last speed for ever: the answer is
// bounded by the time since the last edge, which can only be going slower. And a reversal
// is not a speed: the edge after a change of direction is the same position crossed the
// other way, and the time since the last edge says nothing about how fast it is moving.
class PeriodVelocity {
 public:
  // An edge: the time, from a timer, at which the count changed, and which way.
  void on_edge(double time, int direction) {
    if (direction != direction_) {
      direction_ = direction;
      have_previous_ = false;     // the first edge after a reversal has nothing to pair with
    } else if (have_last_) {
      previous_ = last_;
      have_previous_ = true;
    }
    last_ = time;
    have_last_ = true;
  }

  // Counts a second, signed, at time `now`. Zero when there is nothing to say.
  double estimate(double now) const {
    if (!have_last_ || !have_previous_) return 0.0;

    const double gap = last_ - previous_;
    if (gap <= 0.0) return 0.0;
    double speed = 1.0 / gap;

    // Nothing has arrived for longer than a gap, so it is going slower than the gap says.
    const double since = now - last_;
    if (since > gap && since > 0.0) speed = 1.0 / since;

    return speed * static_cast<double>(direction_);
  }

 private:
  bool have_last_ = false;
  bool have_previous_ = false;
  double last_ = 0.0;
  double previous_ = 0.0;
  int direction_ = 1;
};

// Which to believe. The edge timing is the better at a low speed, where counts are far
// apart, and the window is the better at a high one. Below the threshold, and with an
// estimate to use, it is the period; otherwise the window.
inline double pick_velocity(double period_estimate, double window_estimate, double threshold) {
  const double magnitude = std::fabs(period_estimate);
  if (magnitude > 0.0 && magnitude < threshold) return period_estimate;
  return window_estimate;
}

#endif  // LESSON_SOLUTION_HPP
