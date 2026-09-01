#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <vector>

// One reading, and when it was taken.
struct Sample {
  double time = 0.0;
  double value = 0.0;
};

// The last few seconds of a signal, bounded twice over.
//
// Bounded in **time**, because a chart is about what is happening now and a
// window of seconds is what a reader means by that. Bounded in **memory**,
// because a signal arriving faster than expected must not be able to grow this
// without limit, and something running for a week has to stay the same size as
// something running for a minute.
//
// Dropping from the front of a vector moves everything after it, which lesson
// 03-03 measured. At a few hundred samples that cost is invisible, and the
// moment it is not, the ring buffer from lesson 07-02 is the answer. Choosing
// the simple one first and knowing where the limit is beats reaching for the
// clever one before there is a reason.
class Series {
 public:
  Series(std::size_t capacity, double window)
      : capacity_(capacity == 0 ? 1 : capacity), window_(window) {
    samples_.reserve(capacity_);
  }

  void add(double time, double value) {
    samples_.push_back(Sample{time, value});

    // Age first. A sample older than the window is not wanted whatever the
    // capacity is, and dropping by count alone makes the chart cover a
    // different amount of time at every sample rate, which is the bug that
    // makes two runs impossible to compare.
    const double oldest_wanted = time - window_;
    std::size_t drop = 0;
    while (drop < samples_.size() && samples_[drop].time < oldest_wanted) ++drop;
    if (drop > 0) samples_.erase(samples_.begin(), samples_.begin() + static_cast<std::ptrdiff_t>(drop));

    // Then capacity, which is the guard rather than the policy.
    if (samples_.size() > capacity_) {
      const std::size_t excess = samples_.size() - capacity_;
      samples_.erase(samples_.begin(), samples_.begin() + static_cast<std::ptrdiff_t>(excess));
    }
  }

  std::size_t size() const { return samples_.size(); }
  bool empty() const { return samples_.empty(); }
  std::size_t capacity() const { return capacity_; }
  double window() const { return window_; }

  // Index zero is the oldest sample still held.
  const Sample& at(std::size_t index) const { return samples_[index]; }

  double oldest_time() const { return samples_.empty() ? 0.0 : samples_.front().time; }
  double newest_time() const { return samples_.empty() ? 0.0 : samples_.back().time; }

 private:
  std::size_t capacity_ = 1;
  double window_ = 1.0;
  std::vector<Sample> samples_;
};

// The part of the value axis a chart will draw.
struct Range {
  double low = 0.0;
  double high = 1.0;
};

inline double span(const Range& range) { return range.high - range.low; }

inline Range range_of(const Series& series) {
  if (series.empty()) return Range{0.0, 1.0};

  Range range{series.at(0).value, series.at(0).value};
  for (std::size_t i = 1; i < series.size(); ++i) {
    const double value = series.at(i).value;
    if (value < range.low) range.low = value;
    if (value > range.high) range.high = value;
  }
  return range;
}

// Room above and below, so the extremes are not drawn on the frame.
//
// A fraction of the span, which is zero when the span is zero. That is not a
// mistake in this function: widening a flat signal is a separate decision and
// belongs in a separate function, because doing both here would hide which one
// a caller wanted.
inline Range padded(const Range& range, double fraction) {
  const double room = span(range) * fraction;
  return Range{range.low - room, range.high + room};
}

// The one that stops a chart lying.
//
// A signal that is genuinely steady still moves in its last digit, and an axis
// fitted to that shows a flat line as violent oscillation, because the scale
// expands until the noise fills the height. Measured: a battery reading steady
// to within two microvolts, autoscaled, uses the entire chart.
//
// A minimum span is the fix, and choosing it is a judgement about the signal:
// for a battery, a tenth of a volt is a change worth seeing and a microvolt is
// not.
inline Range at_least(const Range& range, double minimum_span) {
  const double current = span(range);
  if (current >= minimum_span) return range;

  const double middle = (range.low + range.high) / 2.0;
  const double half = minimum_span / 2.0;
  return Range{middle - half, middle + half};
}

// Where a sample lands on a surface, in that surface's units.
struct Point {
  double across = 0.0;
  double down = 0.0;
};

// Time runs left to right across the whole window, not across the samples that
// happen to have arrived. A chart whose x axis is the sample count jumps
// backwards whenever the rate changes, and stretches a gap in the data into
// something that looks like data.
inline Point place_sample(const Sample& sample, double newest_time, double window,
                          const Range& range, double across, double down, double margin) {
  const double usable_across = across - 2.0 * margin;
  const double usable_down = down - 2.0 * margin;

  const double age = newest_time - sample.time;
  const double along = window > 1e-12 ? 1.0 - (age / window) : 1.0;

  const double height = span(range);
  const double up = height > 1e-12 ? (sample.value - range.low) / height : 0.5;

  Point point;
  point.across = margin + along * usable_across;

  // The same flip as every other surface in this curriculum: a value growing
  // upward becomes a row growing downward.
  point.down = margin + (1.0 - up) * usable_down;
  return point;
}

#endif  // LESSON_SOLUTION_HPP
