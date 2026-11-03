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
// TODO 1: the mask with one bit set, counting from zero.
//
// Check the index before shifting, or you have written the undefined
// behaviour lesson 01-09 was about. Return no bits at all for an index that
// does not exist.
//
// Shift Bits{1u}, not 1, so the shift happens at the register width rather
// than at int width. bit(31) must come back as 0x80000000, which 1 << 31 on a
// signed int cannot give you.
inline Bits bit(int index) {
  (void)index;
  return 0u;
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
// TODO 2: is any of these bits set.
//
// One line, and the parentheses are the whole point. Write the and first, in
// brackets, then compare the result against zero. Without the brackets the
// comparison binds tighter and you have asked a question about bit zero.
inline bool has_any(Bits value, Bits mask) {
  (void)value;
  (void)mask;
  return false;
}

// Are all of these bits set.
//
// Not the same question as has_any, and the difference matters as soon as a
// mask covers more than one bit. Comparing against the mask rather than
// against zero is what makes it all rather than any.
//
// An empty mask asks whether every one of no bits is set, which is vacuously
// true, and falls out of the comparison without a special case.
// TODO 3: are all of these bits set.
//
// Almost the same line as has_any and not the same question. Compare the
// masked value against the mask rather than against zero.
//
// Check what your version answers for an empty mask. Every one of no bits is
// set, so it should be true, and the natural form gives that without a
// special case.
inline bool has_all(Bits value, Bits mask) {
  (void)value;
  (void)mask;
  return false;
}

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
// TODO 4: the same register with these bits cleared.
//
// And with the bits outside the mask left exactly as they were. The test sets
// bit 31 and checks it survives, because the complement is a full width
// operation and a version that loses the top of the word passes every test
// written with small flags.
inline Bits with_cleared(Bits value, Bits mask) {
  (void)mask;
  return value;
}

// How many bits are set, which is how many faults are live.
// TODO 5: how many bits are set.
//
// Shift a copy right until it is zero, adding the lowest bit each time. Work
// on a copy: the caller's register is a snapshot and modifying the parameter
// would be fine here but is a habit worth not forming.
//
// Loop while the value is not zero rather than a fixed thirty two passes. Both
// are correct; one stops early.
inline int count_set(Bits value) {
  (void)value;
  return 0;
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
// TODO 6: is this exactly one bit.
//
// Subtracting one from a value with a single bit set turns that bit off and
// every bit below it on, so anding the two together gives zero exactly when
// there was one bit. Zero needs answering separately, since it has no bits at
// all and this lesson's callers want false for it.
//
// You could also write count_set(value) == 1, which is clearer and slower.
// Either passes; the subtraction is the one worth having seen.
inline bool is_single_bit(Bits value) {
  (void)value;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
