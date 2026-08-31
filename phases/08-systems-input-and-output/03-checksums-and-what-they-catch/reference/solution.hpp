#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>

#include <rc/core/compat.hpp>

// The two cheap schemes, here so the measurements have something to measure
// against. Both are commutative, which is the whole story: the order of the
// bytes cannot affect the result, so neither can ever see a reordering.
inline std::uint8_t sum8(rc::span<const std::uint8_t> bytes) {
  std::uint8_t total = 0;
  for (std::size_t i = 0; i < bytes.size(); ++i)
    total = static_cast<std::uint8_t>(total + bytes[i]);
  return total;
}

inline std::uint8_t xor8(rc::span<const std::uint8_t> bytes) {
  std::uint8_t total = 0;
  for (std::size_t i = 0; i < bytes.size(); ++i)
    total = static_cast<std::uint8_t>(total ^ bytes[i]);
  return total;
}

// CRC-8, polynomial 0x07, starting value 0x00. Its published check value over
// the nine bytes "123456789" is 0xF4, which is how you find out you agree with
// everybody else rather than only with yourself.
inline std::uint8_t crc8(rc::span<const std::uint8_t> bytes) {
  std::uint8_t crc = 0x00;

  for (std::size_t i = 0; i < bytes.size(); ++i) {
    crc = static_cast<std::uint8_t>(crc ^ bytes[i]);

    for (int bit = 0; bit < 8; ++bit) {
      // Every cast back to uint8_t is doing work. Shifting a uint8_t promotes
      // it to int first, so without the cast the value keeps growing past eight
      // bits and the high bit test stops meaning what it says.
      const bool high_bit_set = (crc & 0x80u) != 0;
      crc = static_cast<std::uint8_t>(crc << 1);
      if (high_bit_set) crc = static_cast<std::uint8_t>(crc ^ 0x07u);
    }
  }
  return crc;
}

// CRC-16, polynomial 0x1021, starting value 0xFFFF. Check value 0x29B1.
//
// The byte is xored into the top of the register rather than the bottom, which
// is what makes this the unreflected form. Getting that backwards produces a
// checksum that is perfectly self consistent and agrees with nobody.
inline std::uint16_t crc16_ccitt(rc::span<const std::uint8_t> bytes) {
  std::uint16_t crc = 0xFFFF;

  for (std::size_t i = 0; i < bytes.size(); ++i) {
    crc = static_cast<std::uint16_t>(crc ^ (static_cast<std::uint16_t>(bytes[i]) << 8));

    for (int bit = 0; bit < 8; ++bit) {
      const bool high_bit_set = (crc & 0x8000u) != 0;
      crc = static_cast<std::uint16_t>(crc << 1);
      if (high_bit_set) crc = static_cast<std::uint16_t>(crc ^ 0x1021u);
    }
  }
  return crc;
}

// A payload whose last byte is the CRC-8 of everything before it. Reports
// whether it survived and hands back a view of the body.
//
// The body is returned rather than copied, so this stays allocation free, and
// it is valid for as long as the payload is.
inline bool verify_and_strip(rc::span<const std::uint8_t> payload,
                             rc::span<const std::uint8_t>& body) {
  // A payload with no room for a checksum has not lost one, it never had one,
  // and treating that as a pass is how an empty frame gets trusted.
  if (payload.size() < 1) return false;

  const std::size_t body_size = payload.size() - 1;
  const rc::span<const std::uint8_t> candidate(payload.data(), body_size);

  if (crc8(candidate) != payload[body_size]) return false;

  body = candidate;
  return true;
}

#endif  // LESSON_SOLUTION_HPP
