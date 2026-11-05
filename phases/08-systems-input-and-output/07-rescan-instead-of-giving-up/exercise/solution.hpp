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
// The parser from lesson 08-02 reads a length, takes that many bytes, and when
// the checksum fails has already thrown them away. If the length was a lie
// because a byte went missing, those bytes included the start of the next real
// frame, and it is lost with the damaged one.
//
// Yours keeps every byte it has taken until the frame is proven good. When the
// checksum fails it gives up only the start byte and looks again at the rest.
class RescanParser {
 public:
  static constexpr std::uint8_t kStartByte = 0xAA;

  RescanParser(Check check, rc::span<std::uint8_t> window)
      : window_(window),
        check_bytes_(check == Check::Crc16 ? 2u : 1u),
        check_(check),
        longest_(window.size() > 2 ? (window.size() - 2 > 255 ? 255 : window.size() - 2) : 0) {}

  // Takes one byte. True when it completed a valid frame, which stays readable
  // from payload() until the next push() or next().
  bool push(std::uint8_t byte) {
    release();
    if (size_ == window_.size()) discard_front(1);
    window_[size_++] = byte;
    return step();
  }

  // Looks again at the bytes already held, without taking a new one.
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

  void release() {
    if (held_ != 0) {
      discard_front(held_);
      held_ = 0;
    }
  }

  bool step() {
    // TODO: look at the bytes held in window_[0, size_) and either find a valid
    // frame or decide to wait for more.
    //
    // Repeat:
    //   1. Throw away anything in front of a start byte, counting it in
    //      discarded_.
    //   2. With fewer than two bytes held, wait: return false.
    //   3. The length byte window_[1] is a claim, so check it before using it. It
    //      must be at least check_bytes_ and at most longest_. Otherwise count it
    //      in malformed_, give up the start byte, and go round again.
    //   4. With fewer than 2 + length bytes held, the rest has not arrived: wait.
    //   5. Check the frame. CRC-8 is the last byte over everything between the
    //      length and itself. CRC-16 is the last two bytes, high byte first. If
    //      it passes: set payload_size_ to length - check_bytes_, set held_ to
    //      the whole frame's size so release() frees it at the next call, count
    //      it in frames_, and return true.
    //   6. If it fails, this is the lesson: do not throw away the frame. Count
    //      the failure in checksum_failures_ and rescans_, give up the start
    //      byte ALONE, and go round again, because the next real frame may be
    //      sitting inside the bytes you just read.
    (void)check_;
    (void)held_;
    (void)payload_size_;
    (void)frames_;
    (void)checksum_failures_;
    (void)rescans_;
    (void)malformed_;
    (void)discarded_;
    (void)longest_;
    (void)check_bytes_;
    return false;
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
