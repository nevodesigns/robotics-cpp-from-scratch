// rc/test/leak_check.hpp
//
// Proving that a piece of code gave back everything it took.
//
// Include this in a test and every allocation the binary makes is counted. A
// LeakCheck records the counts when it is created, and balanced() reports
// whether they have returned to where they were.
//
// Counting is deterministic, needs no tooling, and behaves identically on every
// supported platform, which is why the memory lessons use it rather than relying
// on a sanitizer being switched on. Continuous integration runs the sanitizers
// as well, and the two catch different things: this catches a leak inside one
// function, the sanitizer catches use after free and mismatched pairs.
//
// Single objects and arrays are counted separately on purpose. A new[] released
// with a plain delete leaves one counter wrong even though the total number of
// allocations and releases matches, which is exactly the bug in E-MEM-0005.
//
// Replacing operator new is the one place in this curriculum where malloc and
// free are the correct tools, because calling new inside operator new would
// recurse forever.

#ifndef RC_TEST_LEAK_CHECK_HPP
#define RC_TEST_LEAK_CHECK_HPP

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>

namespace rc {
namespace test {

// Atomic because a test may allocate on more than one thread, and the counters
// are touched from every one of them. They were plain integers until the thread
// sanitizer reported the race in lesson 07-02, where a threaded test includes
// this header: the harness that proves other code is correct was itself the
// only racy thing in the binary.
inline std::atomic<std::size_t>& live_blocks() {
  static std::atomic<std::size_t> count{0};
  return count;
}

inline std::atomic<std::size_t>& live_arrays() {
  static std::atomic<std::size_t> count{0};
  return count;
}

// Every allocation ever made, rather than the ones still outstanding.
//
// Leaks and allocations are different questions. A control loop that allocates
// and frees a hundred blocks every tick leaks nothing and is still unfit to run
// at 500 Hz, and the live counts above cannot see it: they come back to where
// they started. Lesson 07-04 is about the second question, and it needs a
// number that only goes up.
inline std::atomic<std::size_t>& total_blocks() {
  static std::atomic<std::size_t> count{0};
  return count;
}

inline std::atomic<std::size_t>& total_arrays() {
  static std::atomic<std::size_t> count{0};
  return count;
}

// Records the counts on creation so a test can ask whether they came back.
struct LeakCheck {
  std::size_t blocks_before = live_blocks();
  std::size_t arrays_before = live_arrays();

  bool balanced() const {
    return live_blocks() == blocks_before && live_arrays() == arrays_before;
  }

  std::size_t leaked_blocks() const { return live_blocks() - blocks_before; }
  std::size_t leaked_arrays() const { return live_arrays() - arrays_before; }
};

// Records the running totals on creation so a test can ask how many
// allocations a piece of code made, which is a thing you assert rather than
// time: on a general purpose operating system the scheduler's own worst case is
// a hundred times the cost of an allocation, so a stopwatch cannot find one.
struct AllocationCount {
  std::size_t blocks_before = total_blocks();
  std::size_t arrays_before = total_arrays();

  void reset() {
    blocks_before = total_blocks();
    arrays_before = total_arrays();
  }

  std::size_t blocks() const { return total_blocks() - blocks_before; }
  std::size_t arrays() const { return total_arrays() - arrays_before; }
  std::size_t total() const { return blocks() + arrays(); }
  bool none() const { return total() == 0; }
};

}  // namespace test
}  // namespace rc

// Not inline, and deliberately so. A replacement allocation function must not
// be inline: the standard says so, and Clang says it too, once per function per
// translation unit. The consequence is that this header belongs in exactly one
// translation unit per binary, which is what a test file already is. Two files
// in one test including it will not link, and that failure is the correct
// answer rather than a limitation, because two copies of the counters would
// report leaks that are not there.
void* operator new(std::size_t size) {
  ++rc::test::live_blocks();
  ++rc::test::total_blocks();
  void* memory = std::malloc(size == 0 ? 1 : size);
  if (memory == nullptr) throw std::bad_alloc();
  return memory;
}

void* operator new[](std::size_t size) {
  ++rc::test::live_arrays();
  ++rc::test::total_arrays();
  void* memory = std::malloc(size == 0 ? 1 : size);
  if (memory == nullptr) throw std::bad_alloc();
  return memory;
}

void operator delete(void* memory) noexcept {
  if (memory != nullptr) --rc::test::live_blocks();
  std::free(memory);
}

void operator delete[](void* memory) noexcept {
  if (memory != nullptr) --rc::test::live_arrays();
  std::free(memory);
}

// The sized forms. A compiler is allowed to call either, so both must keep the
// counts straight or the harness reports leaks that are not there.
void operator delete(void* memory, std::size_t) noexcept { operator delete(memory); }
void operator delete[](void* memory, std::size_t) noexcept { operator delete[](memory); }

#endif  // RC_TEST_LEAK_CHECK_HPP
