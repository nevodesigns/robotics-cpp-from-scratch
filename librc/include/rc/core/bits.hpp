// rc/core/bits.hpp
//
// Named operations on a status register, from lesson 01-10, graduated.
//
// Every motor driver, IMU and encoder chip has a register where each bit means
// a different thing has gone wrong. Reading it is the first thing a driver
// does, and the arithmetic has three traps in it that produce a working
// program which reports the wrong faults.
//
// The first is precedence, and it is why these functions exist at all.
//
//     if (status & kOverTemp != 0)
//
// == and != bind tighter than &, so that parses as
// status & (kOverTemp != 0), which is status & 1, which answers a question
// about bit zero. Measured with over temperature and stalled both set, so the
// fault is plainly there: the correct form answered true and the mistaken form
// evaluated to 0 and answered false. A fault check that reports no fault.
//
// It is hard to spot because it is not always wrong. When the flag being
// looked for happens to be bit zero the two forms agree, which is how it
// passes a test written against the first flag in the list. gcc and clang both
// refuse it under -Werror through -Wparentheses, which is worth having and is
// not a substitute for a name with no precedence in it.
//
// The second is width. A narrow register promotes to int before anything
// happens to it, so expressions are wider than the variable they came from.
// Measured on a std::uint8_t holding 0x80:
//
//     narrow << 1 as an expression   256, sizeof 4
//     stored back into the uint8_t     0, sizeof 1
//     ~narrow_mask for mask 0x02      -3, sizeof 4, and signed
//
// So the complement of a narrow mask has the whole top of the word set. It
// gives the right answer when immediately masked and the wrong one when
// stored. Fixing the register type at 32 bits unsigned removes that question
// rather than answering it at each use, which is why Bits is not generic.
//
// The third is the shift count, which lesson 01-09 covered: shifting by the
// width of the type or further is undefined, and nothing diagnoses it once the
// count arrives at runtime. bit() checks first.

#ifndef RC_CORE_BITS_HPP
#define RC_CORE_BITS_HPP

#include <cstdint>

namespace rc {
namespace core {

// A status register. Fixed width on purpose: a narrower register widens into
// it without losing anything, and every expression below is already the width
// it looks like.
using Bits = std::uint32_t;

inline constexpr int kBitCount = 32;

// Is this a bit index that exists.
inline bool is_valid_bit_index(int index) { return index >= 0 && index < kBitCount; }

// The mask with one bit set, counting from zero.
//
// No bits at all for an index that does not exist. Zero is a deliberate choice
// rather than a shrug: it is the identity for or, so value | bit(99) leaves
// value alone rather than corrupting it, and it is absorbing for and, so
// has_any(value, bit(99)) is false rather than true. A wrong index produces a
// flag that is never set and never clears anything, which fails visibly.
inline Bits bit(int index) {
  if (!is_valid_bit_index(index)) return 0u;
  return Bits{1u} << index;
}

// Is any of these bits set.
//
// A named function has no precedence to get wrong. That is the argument for
// this header, and the measurement above is the evidence.
inline bool has_any(Bits value, Bits mask) { return (value & mask) != 0u; }

// Are all of these bits set.
//
// Not the same question as has_any, and a one bit mask cannot tell the two
// apart, which is how the wrong one of them reaches a release.
//
// An empty mask asks whether every one of no bits is set, which is vacuously
// true and falls out without a special case.
inline bool has_all(Bits value, Bits mask) { return (value & mask) == mask; }

// The same register with these bits set.
//
// A new value rather than a modification: a register read from a device is a
// snapshot, and a caller usually wants the original to compare against the
// next one.
inline Bits with_set(Bits value, Bits mask) { return value | mask; }

// The same register with these bits cleared, and everything else untouched.
inline Bits with_cleared(Bits value, Bits mask) { return value & ~mask; }

// How many bits are set, which is how many faults are live.
inline int count_set(Bits value) {
  int count = 0;
  for (Bits remaining = value; remaining != 0u; remaining >>= 1)
    count += static_cast<int>(remaining & 1u);
  return count;
}

// Is this exactly one bit.
//
// Worth checking the flag constants with, once, at startup. A constant
// transcribed from a data sheet is meant to be a single bit and the common typo
// turns it into two, so a mask of 0x03 that was meant to be 0x02 reports two
// faults for one and clears a neighbour when acknowledged.
//
// Subtracting one turns the lowest set bit off and everything below it on, so
// the and is zero exactly when there was one bit.
inline bool is_single_bit(Bits value) {
  return value != 0u && (value & (value - 1u)) == 0u;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_BITS_HPP
