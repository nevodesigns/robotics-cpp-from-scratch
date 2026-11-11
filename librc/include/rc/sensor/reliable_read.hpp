// rc/sensor/reliable_read.hpp
//
// A bounded, checked read, from lesson 15-13, graduated.
//
// CRC-8 (polynomial 0x07, start zero; the CRC of "123456789" is 0xF4), a frame check, and a read that tries up to N times and says
// whether it timed out or got a bad frame. The CRC missed 0 of 40 single bit flips and 0 of 6 swaps of two bytes; a plain sum missed all 6 swaps.
//
// With attempts failing at 0.28 (20 percent timeouts, 10 percent flipped bits), the fraction that never got through was 0.2757, 0.0771, 0.0223,
// 0.0062 and 0.0015 for 1 to 5 attempts, against 0.2800, 0.0784, 0.0220, 0.0061 and 0.0017 from p to the power of the attempts.
// For 99 percent success: 2 attempts at a failure rate of 0.10, 4 at 0.28, 7 at 0.50 and 44 at 0.90.

#ifndef RC_SENSOR_RELIABLE_READ_HPP
#define RC_SENSOR_RELIABLE_READ_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace rc {
namespace sensor {

// CRC-8 with polynomial 0x07, starting from zero, over `size` bytes. The CRC of "123456789" is 0xF4.
inline std::uint8_t crc8(const std::uint8_t* data, std::size_t size) {
  std::uint8_t crc = 0;
  for (std::size_t i = 0; i < size; ++i) {
    crc = static_cast<std::uint8_t>(crc ^ data[i]);
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc & 0x80) ? static_cast<std::uint8_t>((crc << 1) ^ 0x07) : static_cast<std::uint8_t>(crc << 1);
  }
  return crc;
}

// A frame is its payload followed by the CRC-8 of the payload. At least one payload byte.
inline bool frame_ok(const std::uint8_t* frame, std::size_t size) {
  if (frame == nullptr || size < 2) return false;
  return crc8(frame, size - 1) == frame[size - 1];
}

// What a bounded read did.
struct ReadOutcome {
  bool ok = false;
  int attempts = 0;       // how many times the bus was asked
  int timeouts = 0;       // attempts that returned no bytes
  int corrupt = 0;        // attempts that returned bytes with a bad CRC
};

// Ask the bus for a frame of `size` bytes, up to `max_attempts` times, until one has a good CRC.
// `read` is called as read(frame, size) and returns true if it filled the bytes and false for a timeout.
template <class Read>
ReadOutcome read_reliably(Read&& read, int max_attempts, std::uint8_t* frame, std::size_t size) {
  ReadOutcome outcome;
  for (int i = 0; i < max_attempts; ++i) {
    ++outcome.attempts;
    if (!read(frame, size)) {
      ++outcome.timeouts;
      continue;
    }
    if (frame_ok(frame, size)) {
      outcome.ok = true;
      return outcome;
    }
    ++outcome.corrupt;
  }
  return outcome;
}

// The chance that at least one of `attempts` tries gets through when each fails with probability `p_fail`.
inline double success_after(double p_fail, int attempts) {
  if (attempts <= 0) return 0.0;
  if (p_fail <= 0.0) return 1.0;
  if (p_fail >= 1.0) return 0.0;
  return 1.0 - std::pow(p_fail, attempts);
}

// The fewest attempts that make success at least `target`, or zero if no number of attempts can.
inline int attempts_needed(double p_fail, double target) {
  if (target <= 0.0) return 1;
  if (p_fail >= 1.0 || target >= 1.0) return 0;
  for (int n = 1; n <= 1000; ++n)
    if (success_after(p_fail, n) >= target) return n;
  return 0;
}

}  // namespace sensor
}  // namespace rc

#endif  // RC_SENSOR_RELIABLE_READ_HPP
