#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstring>

// A variable you did not write holds whatever the memory held before.
//
// That is not a metaphor and it is not random. It is the previous function's
// locals, or the allocator's bookkeeping, or a page the operating system just
// handed over, and which of those it is decides whether your program looks
// fine today.
//
// Reading one is undefined behaviour, so the compiler is allowed to assume it
// never happens, which is why the same read can give one answer in a debug
// build and a different one in a release build of the same source.

// The pattern. Every byte 0xff, for three reasons that are all about being
// noticed rather than about the number itself.
//
// A double of all ones is a NaN. Any arithmetic that touches it produces NaN,
// NaN propagates through every operation after it, and every comparison with
// it is false, so a poisoned heading does not quietly steer the robot: it
// makes the answer visibly not a number.
//
// A pointer of all ones is not a mappable address on any platform this
// curriculum claims, so dereferencing it faults immediately rather than
// reading something that happens to be there.
//
// An integer of all ones is -1, which is not loud, and that is the honest
// limit of this tool. It is an aid, not a proof.
inline unsigned char poison_pattern() { return 0xff; }

// Writes the pattern over storage.
//
// Note what this does to the standard's rules rather than only to the bytes.
// Before the call, reading that storage is undefined behaviour and the
// compiler may assume you will not. After it, every byte has been written, so
// reading it is ordinary defined behaviour that produces a NaN or a -1.
//
// Poisoning does not make the bug legal. It turns an undefined read into a
// defined and loud one, which is the difference between a value you can see
// in a log and a value the optimiser was entitled to invent.
inline void poison_bytes(void* storage, std::size_t bytes) {
  if (storage == nullptr || bytes == 0) return;
  std::memset(storage, poison_pattern(), bytes);
}

// How many of those bytes are still the pattern.
//
// Poison an output parameter, call the function that is supposed to fill it,
// and ask. Zero means every byte was written. Anything else is the part the
// function forgot, measured in bytes rather than guessed at.
//
// The false negative is real and worth saying out loud: a byte the function
// deliberately wrote to 0xff is indistinguishable from one it never touched.
// For a struct of doubles that does not happen. For a buffer of raw bytes it
// certainly can, and the lesson's suite measures exactly that case rather
// than leaving it as a warning.
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

#endif  // LESSON_SOLUTION_HPP
