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
  if (shift < 0 || width <= 0 || shift >= 32) return 0;
  const unsigned mask = width >= 32 ? 0xFFFFFFFFu : ((1u << width) - 1u);
  return (reg >> shift) & mask;
}

// The low `bits` bits of `raw` as a two's complement number.
inline int sign_extend(unsigned raw, int bits) {
  if (bits <= 0 || bits >= 32) return static_cast<int>(raw);
  const unsigned mask = (1u << bits) - 1u;
  raw &= mask;
  const unsigned sign = 1u << (bits - 1);
  return (raw & sign) ? static_cast<int>(raw) - static_cast<int>(1u << bits) : static_cast<int>(raw);
}

// The full scale of a range code in g: 2, 4, 8 or 16. Anything else gives zero.
inline double range_in_g(int code) {
  if (code < 0 || code > 3) return 0.0;
  return static_cast<double>(2 << code);
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
  if (bytes == nullptr || size < 7) return DecodeStatus::TooShort;
  const unsigned status = bytes[0];
  if (!extract_bits(status, 0, 1)) return DecodeStatus::NotReady;

  const int code = static_cast<int>(extract_bits(status, 4, 2));
  const double g_per_count = range_in_g(code) / 2048.0;
  double axis[3];
  for (int i = 0; i < 3; ++i) {
    const unsigned word = (static_cast<unsigned>(bytes[1 + 2 * i]) << 8) | bytes[2 + 2 * i];
    axis[i] = sign_extend(word >> 4, 12) * g_per_count;
  }
  out.x = axis[0];
  out.y = axis[1];
  out.z = axis[2];
  out.range_code = code;
  out.overrun = extract_bits(status, 1, 1) != 0;
  return out.overrun ? DecodeStatus::Overrun : DecodeStatus::Ok;
}

#endif  // LESSON_SOLUTION_HPP
