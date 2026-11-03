#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <vector>

// Lesson 01-11 ended on a rule: borrow what you read, return what you make.
// Borrowing is the right default and it has a price, which is this lesson.
//
// A std::vector keeps its elements in one buffer. When it runs out of room it
// allocates a bigger one, copies everything across and frees the old one. Every
// pointer, reference, iterator and span that referred to the old buffer now
// refers to memory that has been handed back.
//
// Nothing warns. The code that borrowed and the code that grew are usually in
// different functions written by different people on different days.

// Where a vector keeps its elements, and how much room it has.
//
// The capacity is here as well as the address because it is the part you can
// act on. An address tells you where the buffer is now; the capacity tells you
// how many more elements can arrive before it moves.
struct StorageId {
  const void* data = nullptr;
  std::size_t capacity = 0;
};

// TODO 1: record where this vector keeps its elements, and how much room it
// has.
//
// data() and capacity(). Two lines, and the second one is the one people leave
// out, which is what the next function explains.
inline StorageId storage_of(const std::vector<double>& samples) {
  (void)samples;
  return StorageId{};
}

// Is this the same buffer it was.
//
// Comparing the capacity as well as the address is not belt and braces. A
// freed buffer can be handed straight back by the allocator for the next
// request, so two different buffers can share an address, and a vector that
// shrank and grew again can land where it started.
// TODO 2: is this the same buffer it was.
//
// Compare both fields, not just the address. An allocator is free to hand a
// freed buffer straight back for the next request, so two genuinely different
// buffers can share an address, and comparing addresses alone would call that
// unchanged.
inline bool same_storage(const StorageId& before, const StorageId& after) {
  (void)before;
  (void)after;
  return false;
}

// Will adding this many elements move the buffer.
//
// The question worth asking, and it has to be asked before the insertion
// rather than after, because afterwards every pointer that could have told you
// is already stale. This is the whole artifact.
// TODO 3: will adding this many elements move the buffer.
//
// How many it will hold afterwards, against how many it has room for. Strictly
// greater than, not greater or equal: filling the capacity exactly is the last
// insertion that fits, and reporting that one as a move would make the
// prediction wrong on every container that is exactly full.
//
// The suite checks this against what actually happened on every one of a
// thousand insertions, so an off by one will not hide.
inline bool would_reallocate(const std::vector<double>& samples, std::size_t adding) {
  (void)samples;
  (void)adding;
  return false;
}

// Make room for this many more, so that adding them will not move anything.
//
// Returns the capacity afterwards. Does nothing when there is already room,
// because reserve with a smaller number than the current capacity is a no op
// and asking for exactly the current size would throw away the slack.
// TODO 4: make room for this many more, and report the capacity afterwards.
//
// Only reserve when there is not already room, which is what would_reallocate
// is for. Reserve for the total the vector will hold, size plus adding, not
// for adding alone.
//
// Calling this when the room already exists must change nothing, so that a
// caller can use it defensively without cost. The suite checks that the
// storage is untouched in that case.
inline std::size_t reserve_for(std::vector<double>& samples, std::size_t adding) {
  (void)adding;
  return samples.capacity();
}

#endif  // LESSON_SOLUTION_HPP
