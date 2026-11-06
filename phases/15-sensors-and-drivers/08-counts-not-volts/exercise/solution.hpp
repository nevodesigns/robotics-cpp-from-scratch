#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// What an analogue to digital converter does to a voltage: it cuts the input range into
// 2^bits equal steps and reports which step the input is in, as a code. A step is a least
// significant bit, an LSB.
//
// The convention: the range is divided into 2^bits steps, NOT 2^bits - 1, so the highest
// code is 2^bits - 1 and the very top of the range is not a code of its own. A code stands
// for the middle of its step.
class Quantizer {
 public:
  Quantizer(int bits, double low, double high)
      : steps_(1L << (bits < 1 ? 1 : (bits > 30 ? 30 : bits))), low_(low), high_(high) {}

  double lsb() const {
    // TODO: the range over the number of steps.
    return 0.0;
  }

  long max_code() const {
    // TODO: the highest code is one less than the number of steps.
    return 0;
  }

  // Which step the input is in.
  long code(double x) const {
    // TODO: floor of (x - low) / lsb, which is the step the input is in. Below the range is
    // the lowest code and above it is the highest: the converter clips, it does not wrap.
    (void)x;
    (void)steps_;
    (void)low_;
    (void)high_;
    return 0;
  }

  // The voltage a code stands for: the middle of its step.
  double value(long code) const {
    // TODO: low + (code + 0.5) * lsb
    (void)code;
    return 0.0;
  }

  // Whether the input is at or beyond either end of the range, where the code has stopped
  // saying how much.
  bool saturated(double x) const {
    // TODO
    (void)x;
    return false;
  }

 private:
  long steps_;
  double low_;
  double high_;
};

#endif  // LESSON_SOLUTION_HPP
