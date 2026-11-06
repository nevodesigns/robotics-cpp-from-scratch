// rc/sensor/quantizer.hpp
//
// What a converter does to a voltage, from lesson 15-08, graduated.
//
// It cuts the input range into 2^bits equal steps and reports which one the input is in.
// A reading is never the voltage: it is the voltage with the detail below one step
// thrown away, and a code stands for the middle of its step. The convention is stated
// because the other one is the commonest bug: the range is divided into 2^bits steps and
// not 2^bits - 1.
//
// Numbers from that lesson, ten bits over 0 to 3.3 V, a step of 3.2227 mV. A single
// reading is wrong by 0.2817 of a step rms, close to a step over the square root of
// twelve. Averaging a thousand readings of a steady voltage gives exactly the same, because
// they are the same code. With noise of a step added before the converter the average of
// 100 gives 0.1052 and of 1000 gives 0.0348. And a change of a third of a step was noticed
// 31.17 percent of the time with no noise and every time with it.

#ifndef RC_SENSOR_QUANTIZER_HPP
#define RC_SENSOR_QUANTIZER_HPP

#include <cmath>

namespace rc {
namespace sensor {

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

}  // namespace sensor
}  // namespace rc

#endif  // RC_SENSOR_QUANTIZER_HPP
