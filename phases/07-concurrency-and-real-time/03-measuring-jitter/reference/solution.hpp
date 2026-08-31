#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <string>
#include <vector>

// Fixed memory decided up front. Recording is an index calculation and an
// increment: no allocation, and no growth however long the loop runs, which
// matters because this code lives inside the loop it is measuring.
class Histogram {
 public:
  Histogram(double bucket_width, int bucket_count)
      : width_(bucket_width <= 0.0 ? 1.0 : bucket_width),
        buckets_(bucket_count <= 0 ? 1 : static_cast<std::size_t>(bucket_count), 0) {}

  void record(double value) {
    ++count_;
    if (value > worst_ || count_ == 1) worst_ = value;

    if (value < 0.0) { ++buckets_[0]; return; }

    const std::size_t index = static_cast<std::size_t>(value / width_);

    // The overflow check comes before the array access, not after. Counting
    // beyond the range separately, and keeping the true worst value, is what
    // stops the histogram lying about its own tail: folding a four millisecond
    // stall into the last bucket would report the worst case as the top of the
    // range.
    if (index >= buckets_.size()) {
      ++overflows_;
      return;
    }
    ++buckets_[index];
  }

  long count() const { return count_; }
  long overflows() const { return overflows_; }
  double worst() const { return count_ == 0 ? 0.0 : worst_; }

  // The upper edge of the bucket the pth percentile falls in. Overflows count
  // towards the total, so a distribution with a long tail reports the top of
  // the range rather than pretending the tail is not there.
  double percentile(double p) const {
    if (count_ == 0) return 0.0;

    const double clamped = p < 0.0 ? 0.0 : (p > 100.0 ? 100.0 : p);

    // Computed in floating point on purpose. Integer arithmetic here truncates
    // and the answer comes out systematically low.
    //
    // At least one sample, so that the zeroth percentile answers the smallest
    // bucket that actually holds something rather than matching an empty bucket
    // at the bottom of the range and reporting a value nothing ever reached.
    double wanted = (clamped / 100.0) * static_cast<double>(count_);
    if (wanted < 1.0) wanted = 1.0;

    long running = 0;
    for (std::size_t i = 0; i < buckets_.size(); ++i) {
      running += buckets_[i];
      if (static_cast<double>(running) >= wanted) {
        return static_cast<double>(i + 1) * width_;
      }
    }
    return worst_;   // the percentile lies out in the overflow
  }

  long over(double budget) const {
    long exceeded = overflows_;
    for (std::size_t i = 0; i < buckets_.size(); ++i) {
      // A sample in bucket i is somewhere in [i*width, (i+1)*width). Counting
      // it as over budget only when the whole bucket is over avoids claiming a
      // sample exceeded a budget it may not have.
      if (static_cast<double>(i) * width_ >= budget) exceeded += buckets_[i];
    }
    return exceeded;
  }

  std::string render(int width_in_characters) const {
    if (count_ == 0) return "  (no samples)\n";

    long tallest = 1;
    for (const long n : buckets_) if (n > tallest) tallest = n;

    std::string out;
    for (std::size_t i = 0; i < buckets_.size(); ++i) {
      if (buckets_[i] == 0) continue;
      const int bar = static_cast<int>((buckets_[i] * width_in_characters) / tallest);

      out += "  ";
      out += std::to_string(static_cast<long>(static_cast<double>(i) * width_));
      out += " to ";
      out += std::to_string(static_cast<long>(static_cast<double>(i + 1) * width_));
      out += "  ";
      for (int c = 0; c < bar; ++c) out += '#';
      out += "  ";
      out += std::to_string(buckets_[i]);
      out += "\n";
    }
    if (overflows_ > 0) {
      out += "  beyond the range  " + std::to_string(overflows_) + "\n";
    }
    return out;
  }

 private:
  double width_ = 1.0;
  std::vector<long> buckets_;
  long count_ = 0;
  long overflows_ = 0;
  double worst_ = 0.0;
};

struct Tick {
  double interval = 0.0;   // since the previous tick
  double lateness = 0.0;   // interval minus the period it should have been
  bool first = true;       // nothing to compare against yet
};

// Turns a stream of timestamps into intervals and lateness. Taking the time as
// an argument rather than reading a clock is what makes this testable, the same
// seam as lesson 03-05.
class LoopMonitor {
 public:
  explicit LoopMonitor(double period) : period_(period) {}

  Tick tick(double now) {
    if (!started_) {
      started_ = true;
      last_ = now;
      return Tick{0.0, 0.0, true};
    }

    Tick result;
    result.interval = now - last_;

    // Positive means late. A controller that assumed the period has integrated
    // over this much more time than it accounted for.
    result.lateness = result.interval - period_;
    result.first = false;
    last_ = now;
    return result;
  }

  void reset() { started_ = false; }

 private:
  double period_ = 0.0;
  double last_ = 0.0;
  bool started_ = false;
};

#endif  // LESSON_SOLUTION_HPP
