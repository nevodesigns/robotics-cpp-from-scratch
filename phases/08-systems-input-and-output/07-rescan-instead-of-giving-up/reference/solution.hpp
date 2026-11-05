#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <rc/core/compat.hpp>
#include <rc/io/checksum.hpp>

// Which check closes a frame. The length byte counts the payload and the check
// together, which is what the Link from lesson 08-04 does with one byte of
// CRC-8, so a frame sent by one is read by the other.
enum class Check { Crc8, Crc16 };

// A frame parser that holds on to what it is looking at.
//
// The parser from lesson 08-02 reads a length and then takes that many bytes,
// and when the checksum fails it has already thrown them away. If the length
// was a lie because a byte went missing, the bytes it took included the start
// of the next real frame, and that frame is lost with the damaged one. Lesson
// 08-06 measured it: a dropped byte cost 1.72 to 1.89 frames, not one.
//
// This one keeps every byte it has taken until the frame is proven good. When
// the checksum fails it gives up only the start byte and looks again at the
// rest, because the next real frame may be sitting in the bytes it just read.
class RescanParser {
 public:
  static constexpr std::uint8_t kStartByte = 0xAA;

  // The window must hold the largest frame it will accept: a start byte, a
  // length byte, and up to 255 bytes after them. A smaller window accepts a
  // smaller frame and counts a bigger claim as malformed.
  RescanParser(Check check, rc::span<std::uint8_t> window)
      : window_(window),
        check_bytes_(check == Check::Crc16 ? 2u : 1u),
        check_(check),
        longest_(window.size() > 2 ? (window.size() - 2 > 255 ? 255 : window.size() - 2) : 0) {}

  // Takes one byte. True when it completed a valid frame, which stays readable
  // from payload() until the next push() or next().
  bool push(std::uint8_t byte) {
    release();
    if (size_ == window_.size()) discard_front(1);   // unreachable while step() keeps its promise
    window_[size_++] = byte;
    return step();
  }

  // Looks again at the bytes already held, without taking a new one. A rescan
  // can uncover a second whole frame in the wreckage of the first, and this is
  // how it is collected. Call it after push() returns true until it returns
  // false.
  bool next() {
    release();
    return step();
  }

  rc::span<const std::uint8_t> payload() const {
    return rc::span<const std::uint8_t>(window_.data() + 2, payload_size_);
  }

  std::size_t frames() const { return frames_; }
  std::size_t checksum_failures() const { return checksum_failures_; }
  std::size_t rescans() const { return rescans_; }
  std::size_t malformed() const { return malformed_; }
  std::size_t discarded() const { return discarded_; }

 private:
  void discard_front(std::size_t count) {
    if (count > size_) count = size_;
    std::memmove(window_.data(), window_.data() + count, size_ - count);
    size_ -= count;
  }

  // A delivered frame stays in the window so the caller can read it in place.
  // It is released at the next call.
  void release() {
    if (held_ != 0) {
      discard_front(held_);
      held_ = 0;
    }
  }

  bool check_passes(std::size_t length) const {
    const std::size_t body = length - check_bytes_;
    const rc::span<const std::uint8_t> bytes(window_.data() + 2, body);
    if (check_ == Check::Crc8) return rc::io::crc8(bytes) == window_[2 + body];

    const std::uint16_t sent = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(window_[2 + body]) << 8) | window_[3 + body]);
    return rc::io::crc16_ccitt(bytes) == sent;
  }

  bool step() {
    for (;;) {
      // Anything in front of a start byte is not the start of a frame.
      std::size_t skip = 0;
      while (skip < size_ && window_[skip] != kStartByte) ++skip;
      if (skip != 0) {
        discarded_ += skip;
        discard_front(skip);
      }
      if (size_ < 2) return false;

      // The length is a claim, and it indexes memory, so it is checked first.
      // It must leave room for the check, and it must fit the window.
      const std::size_t length = window_[1];
      if (length < check_bytes_ || length > longest_) {
        ++malformed_;
        discard_front(1);
        continue;
      }

      const std::size_t total = 2 + length;
      if (size_ < total) return false;   // the rest of it has not arrived

      if (check_passes(length)) {
        payload_size_ = length - check_bytes_;
        held_ = total;
        ++frames_;
        return true;
      }

      // The point of this lesson. A parser that discards the whole candidate
      // here loses every real frame that began inside it. Giving up the start
      // byte alone and looking again at the rest costs nothing when the next
      // byte is not a start, and recovers the next frame when it is.
      ++checksum_failures_;
      ++rescans_;
      discard_front(1);
    }
  }

  rc::span<std::uint8_t> window_;
  std::size_t check_bytes_ = 1;
  Check check_ = Check::Crc8;
  std::size_t longest_ = 0;
  std::size_t size_ = 0;
  std::size_t held_ = 0;
  std::size_t payload_size_ = 0;
  std::size_t frames_ = 0;
  std::size_t checksum_failures_ = 0;
  std::size_t rescans_ = 0;
  std::size_t malformed_ = 0;
  std::size_t discarded_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
