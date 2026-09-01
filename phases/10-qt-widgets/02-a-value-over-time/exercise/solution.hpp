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
    // TODO: keep the sample, then drop what no longer belongs.
    //
    // Age first. A sample older than the window is not wanted whatever the
    // capacity is, and dropping by count alone makes the chart cover a
    // different amount of time at every sample rate, which is what makes two
    // runs impossible to compare.
    //
    // Then capacity, which is the guard rather than the policy: it is what
    // stops a signal arriving faster than expected from growing this without
    // limit.
    (void)time;
    (void)value;
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

  // TODO: the lowest and highest value the series holds.
  return Range{0.0, 1.0};
}

// Room above and below, so the extremes are not drawn on the frame.
//
// A fraction of the span, which is zero when the span is zero. That is not a
// mistake in this function: widening a flat signal is a separate decision and
// belongs in a separate function, because doing both here would hide which one
// a caller wanted.
inline Range padded(const Range& range, double fraction) {
  // TODO: room above and below, as a fraction of the span.
  (void)fraction;
  return range;
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
  // TODO: widen the axis around its middle when it is narrower than this, and
  // leave it alone when it is not.
  //
  // This is the one that stops a chart lying. A signal that is genuinely
  // steady still moves in its last digit, and an axis fitted to that shows a
  // flat line as violent oscillation, because the scale expands until the
  // noise fills the height.
  (void)minimum_span;
  return range;
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
  // TODO: where this sample lands.
  //
  // Across is decided by how old the sample is as a fraction of the whole
  // window, so the newest sits at the right edge and a gap in the data is drawn
  // as a gap. Note that this function is given a time and not an index, which
  // is what makes the wrong version hard to write.
  //
  // Down is decided by where the value sits in the range, flipped, because a
  // value growing upward becomes a row growing downward. Guard a range with no
  // span, or a steady signal divides by zero.
  (void)sample;
  (void)newest_time;
  (void)window;
  (void)range;
  (void)across;
  (void)down;
  (void)margin;
  return Point{};
}

#endif  // LESSON_SOLUTION_HPP
