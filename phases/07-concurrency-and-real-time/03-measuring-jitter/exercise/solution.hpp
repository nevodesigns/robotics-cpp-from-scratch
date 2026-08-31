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
    // TODO
    //
    //   count it, and keep the true worst value seen
    //   work out which bucket it belongs in
    //   check for overflow BEFORE indexing the array, and count overflows
    //     separately rather than folding them into the last bucket
    //
    // Folding the tail into the last bucket makes a four millisecond stall read
    // as the top of the range, which is a histogram lying about its own tail.
    (void)value;
  }

  long count() const { return count_; }
  long overflows() const { return overflows_; }
  double worst() const { return count_ == 0 ? 0.0 : worst_; }

  // The upper edge of the bucket the pth percentile falls in. Overflows count
  // towards the total, so a distribution with a long tail reports the top of
  // the range rather than pretending the tail is not there.
  double percentile(double p) const {
    // TODO: walk the buckets accumulating counts until you pass p percent of
    // the total, and return that bucket's upper edge.
    //
    // Do the index arithmetic in floating point. Integers truncate here and the
    // answer comes out systematically low.
    //
    // Require at least one sample, or the zeroth percentile matches an empty
    // bucket at the bottom of the range and reports a value nothing reached.
    (void)p;
    return 0.0;
  }

  long over(double budget) const {
    // TODO: how many samples exceeded the budget, overflows included. This is
    // the number a timing specification is actually written against.
    (void)budget;
    return 0;
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
    // TODO: the interval since the previous tick, and the lateness, which is
    // that interval minus the period. The first tick has nothing to compare
    // against, so it reports zero and says so.
    (void)now;
    return Tick{};
  }

  void reset() { started_ = false; }

 private:
  double period_ = 0.0;
  double last_ = 0.0;
  bool started_ = false;
};

#endif  // LESSON_SOLUTION_HPP
