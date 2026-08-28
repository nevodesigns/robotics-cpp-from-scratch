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

#include <cstddef>
#include <cstdlib>
#include <new>

namespace rc {
namespace test {

inline std::size_t& live_blocks() {
  static std::size_t count = 0;
  return count;
}

inline std::size_t& live_arrays() {
  static std::size_t count = 0;
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

}  // namespace test
}  // namespace rc

inline void* operator new(std::size_t size) {
  ++rc::test::live_blocks();
  void* memory = std::malloc(size == 0 ? 1 : size);
  if (memory == nullptr) throw std::bad_alloc();
  return memory;
}

inline void* operator new[](std::size_t size) {
  ++rc::test::live_arrays();
  void* memory = std::malloc(size == 0 ? 1 : size);
  if (memory == nullptr) throw std::bad_alloc();
  return memory;
}

inline void operator delete(void* memory) noexcept {
  if (memory != nullptr) --rc::test::live_blocks();
  std::free(memory);
}

inline void operator delete[](void* memory) noexcept {
  if (memory != nullptr) --rc::test::live_arrays();
  std::free(memory);
}

// The sized forms. A compiler is allowed to call either, so both must keep the
// counts straight or the harness reports leaks that are not there.
inline void operator delete(void* memory, std::size_t) noexcept { operator delete(memory); }
inline void operator delete[](void* memory, std::size_t) noexcept { operator delete[](memory); }

#endif  // RC_TEST_LEAK_CHECK_HPP
