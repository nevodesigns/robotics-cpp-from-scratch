// rc/core/storage.hpp
//
// Asking whether a container is about to move its elements, from lesson 02-07,
// graduated.
//
// Lesson 01-11 ended on a rule: borrow what you read, return what you make.
// Borrowing is the right default and this is its price. A std::vector keeps its
// elements in one buffer, and when it runs out of room it allocates a bigger
// one, copies everything across and frees the old one. Every pointer,
// reference, iterator and span that referred to the old buffer now refers to
// memory that has been handed back, and nothing warns.
//
// Three failures, measured, in increasing order of how hard they are to find.
//
// One, the buffer moves. Three readings with no slack, capacity 3, one
// push_back: capacity 6 and the buffer somewhere else. Anything borrowed from
// the old one is dangling.
//
// Two, it does not happen every time. Twenty insertions into an empty vector
// caused six reallocations, with capacities 1, 2, 4, 8, 16, 32. Nineteen of
// twenty insertions are harmless, which is why borrowed pointers survive
// testing.
//
// Three, and this is the one with no diagnostic at all. erase does not move the
// buffer. Erasing index 1 of five readings left the storage identical and
// shifted everything down, so the address that held 30 now holds 40. Nothing
// was freed, so no sanitizer has anything to report, and the program carries
// on with a plausible wrong number. A live pointer to the wrong reading is
// worse than a dangling one that crashes.
//
// Which containers move their elements, measured over 100 insertions:
// std::vector moved, std::deque did not, std::list did not. The vector is the
// one that moves and is still the one to reach for first; the other two buy
// element stability with a pointer chase per element.
//
// would_reallocate was checked against what actually happened on every one of
// a thousand insertions, 11 of which reallocated, with no prediction wrong.

#ifndef RC_CORE_STORAGE_HPP
#define RC_CORE_STORAGE_HPP

#include <cstddef>
#include <vector>

namespace rc {
namespace core {

// Where a vector keeps its elements, and how much room it has.
//
// The capacity is here as well as the address because it is the part you can
// act on. The address says where the buffer is now; the capacity says how many
// more elements can arrive before it moves.
struct StorageId {
  const void* data = nullptr;
  std::size_t capacity = 0;
};

inline StorageId storage_of(const std::vector<double>& samples) {
  StorageId id;
  id.data = samples.data();
  id.capacity = samples.capacity();
  return id;
}

// Is this the same buffer it was.
//
// Comparing the capacity as well as the address is not belt and braces. An
// allocator may hand a freed buffer straight back for the next request, so two
// different buffers can share an address.
inline bool same_storage(const StorageId& before, const StorageId& after) {
  return before.data == after.data && before.capacity == after.capacity;
}

// Will adding this many elements move the buffer.
//
// The question has to be asked before the insertion, because afterwards every
// pointer that could have answered it is already stale.
inline bool would_reallocate(const std::vector<double>& samples, std::size_t adding) {
  return samples.size() + adding > samples.capacity();
}

// Make room for this many more, so adding them will not move anything.
//
// Returns the capacity afterwards, and does nothing when the room already
// exists, so it can be called defensively at no cost.
inline std::size_t reserve_for(std::vector<double>& samples, std::size_t adding) {
  if (would_reallocate(samples, adding)) samples.reserve(samples.size() + adding);
  return samples.capacity();
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_STORAGE_HPP
