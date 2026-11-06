#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// What an analogue to digital converter does to a voltage.
//
// It cuts the input range into 2^bits equal steps and reports which step the input is
// in, as a whole number: a code. Everything inside one step is the same code, so a
// reading is never the voltage, it is the voltage with the detail below one step thrown
// away. A step is a least significant bit, an LSB, and it is the range over 2^bits.
//
// The convention here is the usual one, and it is stated because the other one is the
// commonest bug: the range is divided into 2^bits steps, not 2^bits - 1, so the highest
// code is 2^bits - 1 and the very top of the range is not reachable as a code of its own.
// A code stands for the middle of its step.
class Quantizer {
 public:
  Quantizer(int bits, double low, double high)
      : steps_(1L << (bits < 1 ? 1 : (bits > 30 ? 30 : bits))), low_(low), high_(high) {}

  double lsb() const { return (high_ - low_) / static_cast<double>(steps_); }
  long max_code() const { return steps_ - 1; }

  // Which step the input is in. Below the range is the lowest code and above it is the
  // highest: the converter clips, and does not wrap or report an error, so a reading at
  // either end is ambiguous about how far out the input really was.
  long code(double x) const {
    const long c = static_cast<long>(std::floor((x - low_) / lsb()));
    if (c < 0) return 0;
    if (c > max_code()) return max_code();
    return c;
  }

  // The voltage a code stands for: the middle of its step, which is the choice that makes
  // the error at most half a step either way.
  double value(long code) const { return low_ + (static_cast<double>(code) + 0.5) * lsb(); }

  // Whether the input is at or beyond either end, where the code has stopped saying how
  // much.
  bool saturated(double x) const { return x <= low_ || x >= high_; }

 private:
  long steps_;
  double low_;
  double high_;
};

#endif  // LESSON_SOLUTION_HPP
