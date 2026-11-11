#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>

// The chip's frame is seven bytes.
//   byte 0: status. bit 0 data ready, bit 1 overrun (a frame was lost), bits 4 and 5 the range code.
//   bytes 1 to 6: x, y and z, two bytes each, high byte first. Each is a 12 bit two's complement number
//   held in the top twelve bits of the 16, so the low four bits are not part of it.
// The range code 0, 1, 2 and 3 means plus or minus 2, 4, 8 and 16 g, over 2048 counts.

// Bits `width` wide starting at bit `shift` of `reg`, as an unsigned number.
inline unsigned extract_bits(unsigned reg, int shift, int width) {
  // TODO: shift right by `shift`, then keep the low `width` bits with a mask of (1 << width) - 1.
  // Zero for a negative shift, a width of zero or less, or a shift of 32 or more.
  (void)reg;
  (void)shift;
  (void)width;
  return 0;
}

// The low `bits` bits of `raw` as a two's complement number.
inline int sign_extend(unsigned raw, int bits) {
  // TODO: keep the low `bits` bits; if the top one of them (1 << (bits - 1)) is set, the number is
  // negative: subtract 1 << bits. For bits of zero or less, or 32 or more, return raw as an int.
  (void)raw;
  (void)bits;
  return 0;
}

// The full scale of a range code in g: 2, 4, 8 or 16. Anything else gives zero.
inline double range_in_g(int code) {
  // TODO: 2 << code for 0 to 3, otherwise 0.
  (void)code;
  return 0.0;
}

// What one frame said.
struct AccelSample {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  int range_code = 0;
  bool overrun = false;
};

enum class DecodeStatus { Ok, TooShort, NotReady, Overrun };

// Decode one frame. `Overrun` means the frame is good and an earlier one was lost, so it is filled in.
// TooShort and NotReady leave `out` as it was.
inline DecodeStatus decode_frame(const std::uint8_t* bytes, std::size_t size, AccelSample& out) {
  // TODO:
  //  - fewer than 7 bytes, or a null pointer: TooShort.
  //  - bit 0 of byte 0 clear: NotReady.
  //  - the range code is bits 4 and 5 of byte 0; g per count = range_in_g(code) / 2048.
  //  - each axis: (high byte << 8) | low byte, shifted right by 4, sign extended from 12 bits, times g per count.
  //  - the overrun flag is bit 1. Fill in x, y, z, range_code, overrun; return Overrun if it is set, else Ok.
  (void)bytes;
  (void)size;
  (void)out;
  return DecodeStatus::TooShort;
}

#endif  // LESSON_SOLUTION_HPP
