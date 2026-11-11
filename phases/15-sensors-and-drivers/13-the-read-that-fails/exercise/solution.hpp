#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>

// CRC-8 with polynomial 0x07, starting from zero, over `size` bytes. The CRC of "123456789" is 0xF4.
inline std::uint8_t crc8(const std::uint8_t* data, std::size_t size) {
  // TODO: crc = 0. For each byte: crc ^= byte, then eight times: if the top bit of crc is set,
  // crc = (crc << 1) ^ 0x07, otherwise crc = crc << 1 (keep it to eight bits).
  (void)data;
  (void)size;
  return 0;
}

// A frame is its payload followed by the CRC-8 of the payload. At least one payload byte.
inline bool frame_ok(const std::uint8_t* frame, std::size_t size) {
  // TODO: false for a null pointer or fewer than 2 bytes. Otherwise the CRC of the first size - 1 bytes
  // must equal the last byte.
  (void)frame;
  (void)size;
  return false;
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
  // TODO: up to max_attempts times: count the attempt; if read() returns false count a timeout and try again;
  // if the frame passes frame_ok(), set ok and return; otherwise count a corrupt frame and try again.
  (void)read;
  (void)max_attempts;
  (void)frame;
  (void)size;
  return ReadOutcome{};
}

// The chance that at least one of `attempts` tries gets through when each fails with probability `p_fail`.
inline double success_after(double p_fail, int attempts) {
  // TODO: 1 - p_fail ^ attempts. Zero for no attempts, one if p_fail is zero or less, zero if p_fail is one or more.
  (void)p_fail;
  (void)attempts;
  return -1.0;
}

// The fewest attempts that make success at least `target`, or zero if no number of attempts can.
inline int attempts_needed(double p_fail, double target) {
  // TODO: 1 for a target of zero or less; 0 if p_fail >= 1 or target >= 1; otherwise the smallest n,
  // up to 1000, with success_after(p_fail, n) >= target.
  (void)p_fail;
  (void)target;
  return -1;
}

#endif  // LESSON_SOLUTION_HPP
