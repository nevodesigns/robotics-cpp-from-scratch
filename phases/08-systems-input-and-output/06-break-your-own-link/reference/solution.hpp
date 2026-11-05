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
// Rates are parts per million of the bytes that pass, not a fraction. A
// fraction would need a floating point comparison against a random number, and
// the same seed would then not give the same faults on every compiler. An
// integer comparison does, which is what lets a test assert a number measured
// on one machine and have it hold on the others.
struct FaultOptions {
  std::uint32_t seed = 1;
  std::uint32_t drop_per_million = 0;   // a byte vanishes
  std::uint32_t flip_per_million = 0;   // one bit of a byte is inverted
  std::size_t read_chunk = 0;           // at most this many bytes per read, 0 for no limit
  std::size_t write_chunk = 0;          // at most this many bytes per write, 0 for no limit
};

// A port that holds bytes in memory. The device end puts bytes in with
// feed(), the code under test takes them out with read(), and whatever it
// writes is kept for the test to inspect.
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
// given seed.
//
// A link tested only on a perfect port has not been tested. The bench has no
// loose connector and a short cable, and the field has both, so the faults are
// invented on purpose and counted, which is the only way to say what a fault
// cost.
class FaultPort : public BytePort {
 public:
  FaultPort(BytePort& inner, FaultOptions options)
      : inner_(inner), options_(options), state_(options.seed == 0 ? 1u : options.seed) {}

  int read(rc::span<std::uint8_t> into) override {
    std::size_t want = into.size();
    if (options_.read_chunk != 0 && options_.read_chunk < want) want = options_.read_chunk;
    if (want == 0) return 0;

    // Bytes are taken one at a time from the inner port so that a dropped one
    // can be replaced by the next, which keeps the count returned honest: a
    // read that returns three bytes holds three bytes.
    std::size_t count = 0;
    while (count < want) {
      std::uint8_t byte = 0;
      const int got = inner_.read(rc::span<std::uint8_t>(&byte, 1));
      if (got < 0) return count > 0 ? static_cast<int>(count) : got;
      if (got == 0) break;

      ++seen_;
      if (chance(options_.drop_per_million)) {
        ++dropped_;
        continue;
      }
      if (chance(options_.flip_per_million)) {
        byte = static_cast<std::uint8_t>(byte ^ (1u << (next() % 8u)));
        ++flipped_;
      }
      into[count++] = byte;
    }
    return static_cast<int>(count);
  }

  // A short write is not a fault the port invents out of nothing: it is what a
  // full buffer does. Offering the inner port fewer bytes than were handed over
  // reproduces it, and the caller must notice the return value.
  int write(rc::span<const std::uint8_t> from) override {
    std::size_t give = from.size();
    if (options_.write_chunk != 0 && options_.write_chunk < give) give = options_.write_chunk;
    return inner_.write(rc::span<const std::uint8_t>(from.data(), give));
  }

  std::size_t seen() const { return seen_; }
  std::size_t dropped() const { return dropped_; }
  std::size_t flipped() const { return flipped_; }

 private:
  // xorshift32, written out rather than taken from <random> on purpose: the
  // standard fixes the engines but not the distributions, and a test that
  // asserts a measured count needs the same sequence on every toolchain.
  std::uint32_t next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return state_;
  }

  bool chance(std::uint32_t per_million) {
    if (per_million == 0) return false;
    return next() % 1000000u < per_million;
  }

  BytePort& inner_;
  FaultOptions options_;
  std::uint32_t state_;
  std::size_t seen_ = 0;
  std::size_t dropped_ = 0;
  std::size_t flipped_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
