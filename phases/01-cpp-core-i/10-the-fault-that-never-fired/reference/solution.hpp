#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstdint>

// Every motor driver, IMU and encoder chip has a status register: one word
// where each bit means a different thing has gone wrong. Reading it is the
// first thing a driver does and the last thing anybody checks, and the
// arithmetic for it has three traps in it that produce a working program which
// reports the wrong faults.
//
// The register type is fixed at 32 bits here rather than being made generic.
// A narrower register widens into it without losing anything, and pinning the
// width removes the promotion trap that this lesson measures: every expression
// below is already the width it looks like.

using Bits = std::uint32_t;

// How many bits there are to index. Named rather than written as 32 in four
// places, because this is the number the shift guard is about.
inline constexpr int kBitCount = 32;

// Is this a bit index that exists.
//
// Lesson 01-09 established that shifting by the width of the type or further is
// undefined behaviour, and that nothing diagnoses it when the shift count
// arrives at runtime. This is the check that has to happen first.
inline bool is_valid_bit_index(int index) { return index >= 0 && index < kBitCount; }

// The mask with one bit set, counting from zero.
//
// Returns no bits at all for an index that does not exist. Zero is a
// deliberate choice and not a shrug: it is the identity for the or operation,
// so `value | bit(99)` leaves value alone rather than corrupting it, and it is
// absorbing for and, so `has_any(value, bit(99))` is false rather than true.
// A wrong index then produces a flag that is never set and never clears
// anything, which is a visible failure rather than a silent corruption.
//
// Call is_valid_bit_index first when the index came from outside.
inline Bits bit(int index) {
  if (!is_valid_bit_index(index)) return 0u;
  return Bits{1u} << index;
}

// Is any of these bits set.
//
// This function exists because of how its body reads when written inline.
//
//     if (status & kOverTemp != 0)
//
// is not a mistake about bits, it is a mistake about precedence: == and != bind
// tighter than &, so that parses as `status & (kOverTemp != 0)`, which is
// `status & 1`, which answers a question about bit zero. A fault check written
// that way reports the wrong fault, or no fault at all.
//
// A named function has no precedence to get wrong. That is the whole argument
// for this header.
inline bool has_any(Bits value, Bits mask) { return (value & mask) != 0u; }

// Are all of these bits set.
//
// Not the same question as has_any, and the difference matters as soon as a
// mask covers more than one bit. Comparing against the mask rather than
// against zero is what makes it all rather than any.
//
// An empty mask asks whether every one of no bits is set, which is vacuously
// true, and falls out of the comparison without a special case.
inline bool has_all(Bits value, Bits mask) { return (value & mask) == mask; }

// The same register with these bits set.
//
// Returning a new value rather than modifying in place: a status register read
// from a device is a snapshot, and the caller usually wants to keep the
// original to compare against the next one.
inline Bits with_set(Bits value, Bits mask) { return value | mask; }

// The same register with these bits cleared.
//
// The complement is where the width matters. On a narrow type, ~mask promotes
// to int first, so the result has the whole top of the word set and is a
// signed value; it happens to give the right answer when immediately masked
// and the wrong one when stored. Keeping everything at a fixed 32 bit unsigned
// width is what removes that question rather than answering it each time.
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
// Worth having because a flag constant transcribed from a data sheet is
// supposed to be a single bit, and the common typo turns it into two. Checking
// the constants once at startup catches a mask of 0x03 that was meant to be
// 0x02, which otherwise reports two faults for one.
//
// Subtracting one from a value with a single bit set turns that bit off and
// every bit below it on, so the and is zero exactly when there was one bit.
inline bool is_single_bit(Bits value) { return value != 0u && (value & (value - 1u)) == 0u; }

#endif  // LESSON_SOLUTION_HPP
