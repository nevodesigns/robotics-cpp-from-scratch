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
  // TODO: xor each byte into the register, then, eight times, shift it left by
  // one and xor in the polynomial 0x07 whenever the bit that was shifted out
  // was set.
  //
  // Keep the register a uint8_t and cast back to it after every shift.
  // Shifting a uint8_t promotes it to int first, so without the cast the value
  // grows past eight bits and the high bit test stops meaning what it says.
  //
  // The check value is what tells you this is right: crc8 of the nine bytes
  // "123456789" is 0xF4. Round tripping against yourself passes for every
  // wrong variant.
  (void)bytes;
  return 0;
}

// CRC-16, polynomial 0x1021, starting value 0xFFFF. Check value 0x29B1.
//
// The byte is xored into the top of the register rather than the bottom, which
// is what makes this the unreflected form. Getting that backwards produces a
// checksum that is perfectly self consistent and agrees with nobody.
inline std::uint16_t crc16_ccitt(rc::span<const std::uint8_t> bytes) {
  // TODO: the same shape, sixteen bits wide, polynomial 0x1021, starting value
  // 0xFFFF. Check value 0x29B1.
  //
  // The byte is xored into the top of the register, not the bottom. Getting
  // that backwards gives a checksum that is perfectly self consistent and
  // agrees with nobody.
  (void)bytes;
  return 0;
}

// A payload whose last byte is the CRC-8 of everything before it. Reports
// whether it survived and hands back a view of the body.
//
// The body is returned rather than copied, so this stays allocation free, and
// it is valid for as long as the payload is.
inline bool verify_and_strip(rc::span<const std::uint8_t> payload,
                             rc::span<const std::uint8_t>& body) {
  // TODO: the last byte is the CRC-8 of everything before it.
  //
  // Compute the checksum over the body only. Both ends have to agree about
  // which bytes are covered, and computing it over the checksum byte as well
  // is the version that never matches.
  //
  // A payload with no room for a checksum has not lost one, it never had one,
  // and treating that as a pass is how an empty frame gets trusted.
  (void)payload;
  (void)body;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
