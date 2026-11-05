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

// A port with a buffer that can be full. Given: when a device keeps sending
// into a full buffer the new bytes are lost, exactly as in a serial driver.
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

// How many bytes a second a serial line carries.
//
// A byte is not eight bits on the wire: it is a start bit, the data bits and
// the stop bits. Ten for the usual eight data bits, no parity and one stop bit.
inline double wire_bytes_per_second(int baud, int bits_per_byte = 10) {
  // TODO: baud divided by the bits a byte takes. Zero for anything that is not
  // positive, since a line with no speed carries nothing.
  (void)baud;
  (void)bits_per_byte;
  return 0.0;
}

// How long the reader can stop reading before the buffer is full and the device
// starts losing bytes.
inline double stall_budget(std::size_t buffer_bytes, double bytes_per_second) {
  // TODO: the buffer divided by how fast it fills. A device that sends nothing
  // fills it never, which is infinity.
  (void)buffer_bytes;
  (void)bytes_per_second;
  return 0.0;
}

// What a sequence number told the tracker about one message.
struct Verdict {
  std::size_t lost = 0;      // messages missing between this and the last one
  bool duplicate = false;    // this is the message just seen again
  bool late = false;         // this belongs before the one just seen
};

// Counts what a stream of numbered messages lost, repeated and reordered.
template <class T>
class SequenceTracker {
  static_assert(std::is_unsigned<T>::value, "a sequence number is unsigned, so that it wraps");

 public:
  Verdict accept(T sequence) {
    // TODO: count the message, then compare it with the one you expected.
    //
    // The first message sets the expectation and loses nothing, since nothing
    // came before it.
    //
    // After that, how far ahead of the expected number this one is has to be
    // computed IN T. Written as sequence - expected_ it is done in int after
    // promotion, 0 minus 255 is -255, and an ordered stream that wraps looks as
    // if it jumped backwards. static_cast the difference back to T.
    //
    // Less than half the range ahead: that many messages are missing, so add
    // them to lost_ and expect the one after this. At least half: it is behind.
    // The message just seen again is a duplicate, anything earlier is late, and
    // NEITHER moves the expectation, or the next one in order counts as a loss.
    (void)sequence;
    (void)started_;
    (void)expected_;
    (void)lost_;
    (void)duplicates_;
    (void)late_;
    ++received_;
    return Verdict{};
  }

  std::size_t received() const { return received_; }
  std::size_t lost() const { return lost_; }
  std::size_t duplicates() const { return duplicates_; }
  std::size_t late() const { return late_; }

  // The widest gap this counter can tell from a message going backwards.
  static constexpr std::size_t widest_gap() {
    // TODO: half the range of T, less one. For eight bits that is 127.
    return 0;
  }

 private:
  bool started_ = false;
  T expected_ = 0;
  std::size_t received_ = 0;
  std::size_t lost_ = 0;
  std::size_t duplicates_ = 0;
  std::size_t late_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
