// rc/core/init.hpp
//
// Fills storage with a pattern that is loud when it is read, and counts how
// much of it a function failed to write, from lesson 02-09, graduated.
//
// Declaring a variable reserves storage and puts nothing in it. What is in it
// is the previous call's locals, or the last owner's data plus the
// allocator's bookkeeping, and reading it is undefined behaviour, so the
// compiler may assume it does not happen. Measured in the lesson, the same
// function returning an unwritten int gave 1405118043 at -O0 and 0 at -O2.
//
// The diagnostics do not cover it. Measured with gcc 11 and clang 14 at both
// levels: a local int read in the same function warns on both; a struct
// member read in the same function warns on gcc and is silent on clang; a
// struct member read inside a function it was passed to is silent on both.
// That third row is the one that reaches a robot.
//
// Neither do this repository's sanitizer lanes. The address sanitizer finds
// overflows, leaks and use after free and is blind to this. The memory
// sanitizer is the tool that finds it, and it is clang only, cannot be
// combined with the address sanitizer, and needs the whole program including
// the standard library instrumented to be reliable: tried here, it caught a
// trivial local and missed the struct member.
//
// So this header exists to cover the gap by hand, in two moves.
//
// poison_bytes makes an unwritten value loud. Note what it does to the rules
// and not only to the bytes: afterwards every byte has been written, so the
// read is ordinary defined behaviour that produces a NaN. It does not make
// the bug legal. It turns a read the optimiser was entitled to invent an
// answer for into one that shows up in a log.
//
// bytes_still_poisoned proves a function filled its output. The C shape every
// driver uses, bool read_pose(Pose* out), has a hole in it: the return value
// says whether the call succeeded and nothing says whether the output was
// completely written. Poison it, call, and count. Measured on a 24 byte Pose
// of three doubles: a filler that writes all three left 0 bytes, one that
// wrote two and returned true left 8, which is sizeof(double) and names the
// field.
//
// The honest limit, measured rather than mentioned: a byte a function wrote
// to the pattern on purpose is indistinguishable from one it never touched.
// On an eight byte frame where every byte was written and two happened to be
// 0xff, the counter reported two unwritten. For a struct of doubles the answer
// is exact, because the pattern repeated is a NaN and no correct code writes
// that. For raw bytes, read the count as a lower bound on what was written.
// Padding counts as unwritten, because it is.
//
// This belongs in tests, not in a shipping code path. The point is to prove
// once that a function fills what it claims to, so the call sites can go on
// trusting the return value.

#ifndef RC_CORE_INIT_HPP
#define RC_CORE_INIT_HPP

#include <cstddef>
#include <cstring>

namespace rc {
namespace core {

// Every byte 0xff, for three reasons that are all about being noticed.
//
// A double of all ones is a NaN, so arithmetic on an unwritten heading
// produces something visibly not a number rather than a plausible angle, NaN
// propagates through every operation after it, and every comparison with it
// is false. A pointer of all ones is not a mappable address on any platform
// this curriculum claims, so a dereference faults at once. An integer of all
// ones is -1, which is not loud, and that is the honest limit of the idea.
inline unsigned char poison_pattern() { return 0xff; }

// Writes the pattern over storage. Does nothing for a null pointer or an
// empty range, so a caller need not special case either.
inline void poison_bytes(void* storage, std::size_t bytes) {
  if (storage == nullptr || bytes == 0) return;
  std::memset(storage, poison_pattern(), bytes);
}

// How many of those bytes are still the pattern.
//
// unsigned char is the only type you may inspect an arbitrary object
// representation through, which is what makes this loop defined where the
// equivalent read of an int or a double would not be.
inline std::size_t bytes_still_poisoned(const void* storage, std::size_t bytes) {
  if (storage == nullptr) return 0;
  const unsigned char* const first = static_cast<const unsigned char*>(storage);
  std::size_t count = 0;
  for (std::size_t i = 0; i < bytes; ++i)
    if (first[i] == poison_pattern()) ++count;
  return count;
}

// The question most callers actually have.
inline bool all_bytes_written(const void* storage, std::size_t bytes) {
  return bytes_still_poisoned(storage, bytes) == 0;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_INIT_HPP
