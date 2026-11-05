#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <type_traits>

#include <rc/core/compat.hpp>
#include <rc/io/link.hpp>

using rc::io::BytePort;

// A port with a buffer that can be full.
//
// This is the receive buffer of a real serial port in the one respect that
// matters here: it holds a fixed number of bytes, and when a device keeps
// sending into a full one the new bytes are lost. The device end puts bytes in
// with deliver(), and the code under test takes them out with read().
class BoundedPort : public BytePort {
 public:
  explicit BoundedPort(std::size_t capacity) : capacity_(capacity) {}

  void deliver(rc::span<const std::uint8_t> bytes) {
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      if (held_.size() >= capacity_) {
        ++overflowed_;
        continue;
      }
      held_.push_back(bytes[i]);
    }
  }

  std::size_t held() const { return held_.size(); }
  std::size_t overflowed() const { return overflowed_; }

  int read(rc::span<std::uint8_t> into) override {
    std::size_t count = 0;
    while (count < into.size() && !held_.empty()) {
      into[count++] = held_.front();
      held_.pop_front();
    }
    return static_cast<int>(count);
  }

  int write(rc::span<const std::uint8_t> from) override { return static_cast<int>(from.size()); }

 private:
  std::size_t capacity_;
  std::deque<std::uint8_t> held_;
  std::size_t overflowed_ = 0;
};

// How many bytes a second a serial line carries. A byte is not eight bits on
// the wire: it is a start bit, the data bits and the stop bits, ten for the
// usual eight data bits, no parity and one stop bit.
inline double wire_bytes_per_second(int baud, int bits_per_byte = 10) {
  if (baud <= 0 || bits_per_byte <= 0) return 0.0;
  return static_cast<double>(baud) / static_cast<double>(bits_per_byte);
}

// How long the reader can stop reading before the buffer is full and the device
// starts losing bytes, given how fast it is sending.
inline double stall_budget(std::size_t buffer_bytes, double bytes_per_second) {
  if (bytes_per_second <= 0.0) return std::numeric_limits<double>::infinity();
  return static_cast<double>(buffer_bytes) / bytes_per_second;
}

// What a sequence number told the tracker about one message.
struct Verdict {
  std::size_t lost = 0;      // messages missing between this and the last one
  bool duplicate = false;    // this is the message just seen again
  bool late = false;         // this belongs before the one just seen
};

// Counts what a stream of numbered messages lost, repeated and reordered.
//
// A message that carries a counter lets the receiver say something a byte count
// never can: how many did not arrive. The counter wraps, and the arithmetic has
// to be done in the counter's own width, because that is where the wrap
// happens. Done in int, 0 minus 255 is -255 and a perfectly ordered stream looks
// like it jumped backwards.
//
// A counter of N bits can see a gap of at most 2^(N-1) - 1. Past that, a gap is
// indistinguishable from a message going backwards, and a gap of exactly 2^N
// is indistinguishable from nothing having happened at all.
template <class T>
class SequenceTracker {
  static_assert(std::is_unsigned<T>::value, "a sequence number is unsigned, so that it wraps");

 public:
  Verdict accept(T sequence) {
    Verdict verdict;
    ++received_;

    if (!started_) {
      started_ = true;
      expected_ = static_cast<T>(sequence + 1u);
      return verdict;
    }

    // The subtraction is in T. Written as sequence - expected_ it would be done
    // in int after promotion, and the wrap would be lost.
    const T ahead = static_cast<T>(sequence - expected_);

    if (ahead < kHalf) {
      verdict.lost = static_cast<std::size_t>(ahead);
      lost_ += verdict.lost;
      expected_ = static_cast<T>(sequence + 1u);
      return verdict;
    }

    // Behind where it should be. The message just seen again is a duplicate,
    // anything earlier is late, and neither moves the expectation, or the next
    // message in order would be counted as a loss.
    if (sequence == static_cast<T>(expected_ - 1u)) {
      verdict.duplicate = true;
      ++duplicates_;
    } else {
      verdict.late = true;
      ++late_;
    }
    return verdict;
  }

  std::size_t received() const { return received_; }
  std::size_t lost() const { return lost_; }
  std::size_t duplicates() const { return duplicates_; }
  std::size_t late() const { return late_; }

  // The widest gap this counter can tell from a message going backwards.
  static constexpr std::size_t widest_gap() { return static_cast<std::size_t>(kHalf) - 1u; }

 private:
  static constexpr T kHalf = static_cast<T>(static_cast<T>(~T(0)) / 2u + 1u);

  bool started_ = false;
  T expected_ = 0;
  std::size_t received_ = 0;
  std::size_t lost_ = 0;
  std::size_t duplicates_ = 0;
  std::size_t late_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
