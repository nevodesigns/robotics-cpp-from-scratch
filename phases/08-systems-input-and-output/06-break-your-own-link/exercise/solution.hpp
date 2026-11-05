#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <deque>

#include <rc/core/compat.hpp>
#include <rc/io/link.hpp>

using rc::io::BytePort;

// Which faults to inject, and how often.
//
// Rates are parts per million of the bytes that pass, not a fraction. An
// integer comparison gives the same faults for the same seed on every
// compiler, which a floating point one does not promise.
struct FaultOptions {
  std::uint32_t seed = 1;
  std::uint32_t drop_per_million = 0;   // a byte vanishes
  std::uint32_t flip_per_million = 0;   // one bit of a byte is inverted
  std::size_t read_chunk = 0;           // at most this many bytes per read, 0 for no limit
  std::size_t write_chunk = 0;          // at most this many bytes per write, 0 for no limit
};

// A port that holds bytes in memory. This one is given: feed() puts bytes in
// from the device end, read() takes them out, and whatever is written is kept.
class MemoryPort : public BytePort {
 public:
  void feed(rc::span<const std::uint8_t> bytes) {
    for (std::size_t i = 0; i < bytes.size(); ++i) incoming_.push_back(bytes[i]);
  }

  std::size_t pending() const { return incoming_.size(); }
  const std::deque<std::uint8_t>& written() const { return written_; }

  int read(rc::span<std::uint8_t> into) override {
    std::size_t count = 0;
    while (count < into.size() && !incoming_.empty()) {
      into[count++] = incoming_.front();
      incoming_.pop_front();
    }
    return static_cast<int>(count);
  }

  int write(rc::span<const std::uint8_t> from) override {
    for (std::size_t i = 0; i < from.size(); ++i) written_.push_back(from[i]);
    return static_cast<int>(from.size());
  }

 private:
  std::deque<std::uint8_t> incoming_;
  std::deque<std::uint8_t> written_;
};

// A BytePort that damages what passes through it, the same way every time for a
// given seed. Your job.
class FaultPort : public BytePort {
 public:
  FaultPort(BytePort& inner, FaultOptions options) : inner_(inner), options_(options) {}

  int read(rc::span<std::uint8_t> into) override {
    // TODO: read from inner_ one byte at a time, so a dropped byte can be
    // replaced by the next and the count you return is honest.
    //
    // For each byte taken: count it in seen_. Then decide, from your random
    // number generator and the options, whether to drop it (count it, and do not
    // hand it on) and, if it survives, whether to flip one bit of it (count it).
    //
    // Never return more than read_chunk bytes when that is set. A negative from
    // the inner port is passed up if nothing was delivered yet.
    //
    // Use an integer generator and an integer comparison against the per million
    // rate. xorshift32 is five lines: state ^= state << 13; state ^= state >> 17;
    // state ^= state << 5. A state of zero never changes, so a seed of zero must
    // become something else.
    (void)into;
    return 0;
  }

  int write(rc::span<const std::uint8_t> from) override {
    // TODO: offer the inner port at most write_chunk bytes when that is set.
    // The return value is how many it took, and the caller has to notice.
    (void)from;
    return 0;
  }

  std::size_t seen() const { return seen_; }
  std::size_t dropped() const { return dropped_; }
  std::size_t flipped() const { return flipped_; }

 private:
  BytePort& inner_;
  FaultOptions options_;
  std::size_t seen_ = 0;
  std::size_t dropped_ = 0;
  std::size_t flipped_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
