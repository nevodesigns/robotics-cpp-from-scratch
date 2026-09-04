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
  // TODO 1: build the free list once, here.
  //
  // Every slot starts unused and available. Fill free_ with the indices 0 to
  // N-1, set every entry of used_ to false, and set available_ to N.
  //
  // Push them in reverse so that acquire(), which takes from the end, hands out
  // slot 0 first. That is not required and it makes a failure much easier to
  // read.
  Pool() {}

  // TODO 2: hand out a slot, or nothing.
  //
  // When available_ is zero, count the refusal in exhaustions_ and return
  // nullptr. Do not grow: growing gives up the only property this class has.
  //
  // Otherwise take the last index off free_, mark it used, and return the
  // address of that slot.
  T* acquire() { return nullptr; }

  // TODO 3: take one back, and refuse anything this pool did not hand out.
  //
  // Return false, without touching the free list, for:
  //
  //   a null pointer;
  //   a pointer outside storage_, counted in foreign_releases_;
  //   a slot that is not currently in use, counted in double_releases_.
  //
  // Otherwise mark the slot unused, put its index back on free_, and return
  // true.
  //
  // The double release is the one worth the code. Putting the same slot on the
  // free list twice means two later callers are handed the same object, and the
  // bug then appears somewhere else entirely, long after the mistake.
  bool release(T* item) {
    (void)item;
    return false;
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
