#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <vector>

// The rule every compiler follows for where a member goes: at an address that is a multiple of its own
// alignment. If the next free byte is not such an address, the gap is skipped and stays unused: padding.
// Round `offset` up to the next multiple of `alignment`.
inline std::size_t align_up(std::size_t offset, std::size_t alignment) {
  // TODO: an alignment of one or nothing leaves it as it is. Otherwise add alignment - 1, divide by
  // alignment (integer division throws the remainder away) and multiply back.
  (void)offset;
  (void)alignment;
  return 0;
}

// What the compiler needs to know about a member to place it.
struct FieldSpec {
  std::size_t size = 0;
  std::size_t alignment = 1;
};

struct Layout {
  std::vector<std::size_t> offsets;   // where each member starts
  std::size_t size = 0;               // the size of the whole struct
  std::size_t padding = 0;            // bytes in it that belong to no member
  std::size_t alignment = 1;          // the struct's own alignment: the largest of its members'
};

// Where each member of a struct goes, in the order declared.
inline Layout layout_of(const std::vector<FieldSpec>& fields) {
  // TODO: walk the members in order with a position `at`. Each one starts at align_up(at, its
  // alignment): record that offset, move `at` past its size, and add its size to `used`. The struct's
  // alignment is the largest of its members'.
  //
  // At the end the whole struct is padded to a multiple of its own alignment, because an array of them
  // has to keep every element aligned: the second starts where the first ends. That is the size. The
  // padding is the size less `used`.
  (void)fields;
  return Layout{};
}

// The smallest a struct of these members can be, by choosing the order.
inline std::size_t smallest_size(std::vector<FieldSpec> fields) {
  // TODO: put the most strictly aligned members first and take layout_of that order. It leaves no gap
  // before any of them. Keep members of equal alignment in their given order (std::stable_sort with a
  // comparison on alignment, largest first).
  (void)fields;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
