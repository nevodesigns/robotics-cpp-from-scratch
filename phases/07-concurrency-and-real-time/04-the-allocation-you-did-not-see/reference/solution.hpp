#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <array>
#include <cstddef>

// Fixed storage, handed out and taken back.
//
// A control loop that allocates is not usually slow. Measured on an ordinary
// Linux desktop, a loop body building a small vector each tick cost 0.400
// microseconds against 0.150 for a fixed array, while the worst case of both
// was over 20 microseconds and belonged to the scheduler rather than to either
// of them. You cannot find an allocation with a stopwatch on a machine like
// this, because the thing you are listening for is a hundred times quieter than
// the room.
//
// So you do not listen for it. You forbid it, and you assert the count.
//
// This pool is what a loop uses instead: N slots built once, an index handed
// out on request, and nothing at all from the heap after construction. When it
// runs out it says so and counts the refusal, because a pool that quietly grows
// has given up the only property it had, and one that quietly returns nothing
// is a fault nobody will find until it matters.
template <class T, std::size_t N>
class Pool {
 public:
  Pool() {
    // The free list is built once, here, and never touched by the heap again.
    for (std::size_t i = 0; i < N; ++i) {
      free_[i] = N - 1 - i;   // hand out slot 0 first, which is easier to read
      used_[i] = false;
    }
    available_ = N;
  }

  // A slot, or nullptr when there are none left. Never allocates.
  T* acquire() {
    if (available_ == 0) {
      ++exhaustions_;
      return nullptr;
    }
    const std::size_t slot = free_[--available_];
    used_[slot] = true;
    return &storage_[slot];
  }

  // Give one back. A pointer this pool did not hand out, or one already
  // returned, is refused rather than corrupting the free list: a double release
  // would put the same slot on the list twice and two callers would then be
  // handed the same object, which is a bug that appears somewhere else entirely.
  bool release(T* item) {
    if (item == nullptr) return false;
    if (item < storage_.data() || item >= storage_.data() + N) {
      ++foreign_releases_;
      return false;
    }
    const std::size_t slot = static_cast<std::size_t>(item - storage_.data());
    if (!used_[slot]) {
      ++double_releases_;
      return false;
    }
    used_[slot] = false;
    free_[available_++] = slot;
    return true;
  }

  std::size_t capacity() const { return N; }
  std::size_t available() const { return available_; }
  std::size_t in_use() const { return N - available_; }

  // How many times this pool has had to say no, and how many times somebody has
  // handed it something it did not own. Both are zero on a healthy loop, and
  // both are worth reporting rather than only checking in a test: a pool sized
  // for the worst case last year is the first thing a new feature outgrows.
  std::size_t exhaustions() const { return exhaustions_; }
  std::size_t double_releases() const { return double_releases_; }
  std::size_t foreign_releases() const { return foreign_releases_; }

 private:
  std::array<T, N> storage_{};
  std::array<std::size_t, N> free_{};
  std::array<bool, N> used_{};
  std::size_t available_ = 0;
  std::size_t exhaustions_ = 0;
  std::size_t double_releases_ = 0;
  std::size_t foreign_releases_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
