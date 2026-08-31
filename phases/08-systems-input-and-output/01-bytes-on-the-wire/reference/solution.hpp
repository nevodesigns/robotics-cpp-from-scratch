#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <rc/core/compat.hpp>

// Big endian, because most sensor and network protocols are, and because a
// packet capture then reads left to right in the order the datasheet describes.
//
// Neither class allocates. Both work over a buffer the caller owns, which is
// what lets them run inside a control loop and inside an interrupt handler.

class ByteWriter {
 public:
  explicit ByteWriter(rc::span<std::uint8_t> destination) : out_(destination) {}

  bool put_u8(std::uint8_t value) {
    // The room check comes before the write, not after. After is one byte past
    // the end of somebody's buffer, and the value it lands on is whatever was
    // unlucky enough to be next.
    if (remaining() < 1) { ok_ = false; return false; }
    out_[written_++] = value;
    return true;
  }

  // Shifting rather than copying the memory of the value. A shift is arithmetic
  // on a number and means the same thing on every machine; copying the memory
  // means whatever this processor's byte order happens to be.
  bool put_u16(std::uint16_t value) {
    if (remaining() < 2) { ok_ = false; return false; }
    out_[written_++] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
    out_[written_++] = static_cast<std::uint8_t>((value     ) & 0xFFu);
    return true;
  }

  bool put_u32(std::uint32_t value) {
    if (remaining() < 4) { ok_ = false; return false; }
    out_[written_++] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
    out_[written_++] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
    out_[written_++] = static_cast<std::uint8_t>((value >>  8) & 0xFFu);
    out_[written_++] = static_cast<std::uint8_t>((value      ) & 0xFFu);
    return true;
  }

  // Signed values go on the wire as their two's complement bit pattern, which
  // is what every protocol means by a signed field. Getting at that pattern
  // through memcpy rather than a cast is deliberate: converting a negative
  // number to an unsigned type is well defined and does the right thing, but
  // saying it with memcpy makes it obvious that what travels is the bits.
  bool put_i16(std::int16_t value) {
    std::uint16_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return put_u16(bits);
  }

  bool put_i32(std::int32_t value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return put_u32(bits);
  }

  // memcpy, not reinterpret_cast. The cast is undefined behaviour, and on a
  // pointer into the middle of a buffer it is also misaligned, which faults on
  // some ARM cores. Every compiler turns this memcpy into the same instruction
  // the cast would have produced.
  bool put_f32(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return put_u32(bits);
  }

  std::size_t written() const { return written_; }
  std::size_t remaining() const { return out_.size() - written_; }

  // Sticky. A whole message can be written and checked once at the end, rather
  // than after every field, which is how this gets used in practice.
  bool ok() const { return ok_; }

 private:
  rc::span<std::uint8_t> out_;
  std::size_t written_ = 0;
  bool ok_ = true;
};

class ByteReader {
 public:
  explicit ByteReader(rc::span<const std::uint8_t> source) : in_(source) {}

  bool get_u8(std::uint8_t& out) {
    if (remaining() < 1) { ok_ = false; return false; }
    out = in_[read_++];
    return true;
  }

  bool get_u16(std::uint16_t& out) {
    if (remaining() < 2) { ok_ = false; return false; }
    // Each byte is widened before it is shifted. Without the cast the byte is
    // promoted to int, and on a platform where char is signed a byte above 127
    // would arrive with its sign extended and every high bit set.
    const std::uint16_t high = in_[read_++];
    const std::uint16_t low = in_[read_++];
    out = static_cast<std::uint16_t>((high << 8) | low);
    return true;
  }

  bool get_u32(std::uint32_t& out) {
    if (remaining() < 4) { ok_ = false; return false; }
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i) value = (value << 8) | in_[read_++];
    out = value;
    return true;
  }

  bool get_i16(std::int16_t& out) {
    std::uint16_t bits = 0;
    if (!get_u16(bits)) return false;
    std::memcpy(&out, &bits, sizeof(out));
    return true;
  }

  bool get_i32(std::int32_t& out) {
    std::uint32_t bits = 0;
    if (!get_u32(bits)) return false;
    std::memcpy(&out, &bits, sizeof(out));
    return true;
  }

  bool get_f32(float& out) {
    std::uint32_t bits = 0;
    if (!get_u32(bits)) return false;
    std::memcpy(&out, &bits, sizeof(out));
    return true;
  }

  std::size_t read() const { return read_; }
  std::size_t remaining() const { return in_.size() - read_; }
  bool ok() const { return ok_; }

 private:
  rc::span<const std::uint8_t> in_;
  std::size_t read_ = 0;
  bool ok_ = true;
};

#endif  // LESSON_SOLUTION_HPP
