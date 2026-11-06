#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <vector>

// The rule every compiler follows for where a member goes.
//
// Each member sits at an address that is a multiple of its own alignment, which is usually its size, so a
// double sits at a multiple of eight. If the next free byte is not such an address, the gap is skipped and
// stays unused: padding. Round `offset` up to the next multiple of `alignment`.
//
// Add alignment minus one, divide, and multiply back: the integer division throws the remainder away, which is
// the rounding down that makes the whole thing a rounding up.
inline std::size_t align_up(std::size_t offset, std::size_t alignment) {
  if (alignment <= 1) return offset;
  return (offset + alignment - 1) / alignment * alignment;
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
//
// Members are laid out one after another, each at the next offset its alignment allows. The struct as a
// whole is then padded at the end to a multiple of its own alignment, which is the largest of its members',
// because an array of them has to keep every element aligned: the second element starts where the first
// ends.
inline Layout layout_of(const std::vector<FieldSpec>& fields) {
  Layout out;
  std::size_t at = 0;
  std::size_t used = 0;

  for (const FieldSpec& f : fields) {
    at = align_up(at, f.alignment);
    out.offsets.push_back(at);
    at += f.size;
    used += f.size;
    out.alignment = std::max(out.alignment, f.alignment);
  }

  out.size = align_up(at, out.alignment);
  out.padding = out.size - used;
  return out;
}

// The smallest a struct of these members can be, by choosing the order.
//
// Placing the most strictly aligned members first leaves no gaps before any of them, and what is left is
// members of smaller alignment, which fit wherever the previous one ended. For alignments that are powers
// of two, which they always are, that order is the best there is.
inline std::size_t smallest_size(std::vector<FieldSpec> fields) {
  std::stable_sort(fields.begin(), fields.end(),
                   [](const FieldSpec& a, const FieldSpec& b) { return a.alignment > b.alignment; });
  return layout_of(fields).size;
}

#endif  // LESSON_SOLUTION_HPP
