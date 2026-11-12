#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class Endian { little, big };

// What the machine a program will run on is like, as far as layout goes. `valid` is false for a triple that could not be read.
struct Target {
  bool valid = false;
  std::string arch;
  std::string vendor;
  std::string os;
  std::string abi;
  int pointer_bits = 0;
  int long_bits = 0;            // the width of `long`: 64 bits on a 64 bit Linux, 32 on 64 bit Windows
  Endian endian = Endian::little;
};

namespace detail {
inline std::vector<std::string> split_dashes(const std::string& s) {
  std::vector<std::string> parts;
  std::string cur;
  for (char c : s) {
    if (c == '-') {
      parts.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  parts.push_back(cur);
  return parts;
}
inline bool known_os(const std::string& o) { return o == "linux" || o == "windows" || o == "mingw32" || o == "none"; }
}  // namespace detail

// Read a target triple: arch-os-abi (x86_64-linux-gnu), arch-vendor-os-abi (x86_64-pc-linux-gnu) or arch-vendor-os (x86_64-w64-mingw32).
// With three parts, if the second is an operating system it is arch-os-abi, and otherwise arch-vendor-os. Any other number of parts, an empty part, an arch that is not one of
// x86_64, i686, i386, aarch64, arm, armv7, riscv64, riscv32, mips, s390x, powerpc64, or an os that is not one of linux, windows, mingw32, none, is not valid.
// Pointers are 64 bits for x86_64, aarch64, riscv64, s390x and powerpc64, and 32 for the rest. Byte order is big for mips, s390x and powerpc64, little for the rest.
// `long` is as wide as a pointer, except on windows and mingw32, where it is 32 bits.
inline Target parse_triple(const std::string& triple) {
  // TODO: Split on '-'. 3 or 4 parts, none empty, else invalid (return Target()).
  // 4 parts: arch vendor os abi. 3 parts: if the second is a known os (detail::known_os) it is arch os abi, else arch vendor os.
  // The os must be known and the arch one of the listed ones, else invalid. Set pointer_bits, endian, long_bits (32 on windows and mingw32) and valid
  (void)triple;
  return Target();
}

enum class Field { i8, i16, i32, i64, f32, f64, ptr, long_, size_t_ };

// The size in bytes of a field on a target: i8 1, i16 2, i32 4, i64 8, f32 4, f64 8, a pointer and a size_t as wide as a pointer, a long as wide as a long.
inline std::size_t size_of(Field f, const Target& t) {
  // TODO: the sizes in the comment; a pointer and size_t from pointer_bits / 8, a long from long_bits / 8
  (void)f;
  (void)t;
  return 0;
}

// The alignment of a field inside a struct: its size, except that on 32 bit x86 (i686 and i386) an eight byte i64 or f64 is aligned to 4.
inline std::size_t align_of(Field f, const Target& t) {
  // TODO: the size, except 4 for i64 and f64 on i686 and i386
  (void)f;
  (void)t;
  return 1;
}

// The size of a struct whose fields are these, in order, with each field placed at the next multiple of its alignment, and the whole padded to a multiple of the largest alignment. An empty struct is 0 bytes.
inline std::size_t struct_size(const std::vector<Field>& fields, const Target& t) {
  // TODO: place each field at the next multiple of its alignment, track the largest alignment, pad the end to a multiple of it; empty is 0
  (void)fields;
  (void)t;
  return 0;
}

// The four bytes of a 32 bit value as they are in memory on a machine of this byte order, first byte first.
inline std::array<std::uint8_t, 4> encode_u32(std::uint32_t v, Endian e) {
  // TODO: little: least significant byte first; big: most significant first
  (void)v;
  (void)e;
  return std::array<std::uint8_t, 4>();
}

// Can this struct be sent between all of these targets as it lies in memory? Only if it has the same size on every one of them and contains no pointer, long or size_t, whose width differs by target. Fewer than one target: false.
inline bool layout_equal(const std::vector<Field>& fields, const std::vector<Target>& targets) {
  // TODO: false for no targets or for any ptr, long_ or size_t_ field; otherwise true only if struct_size is the same on every target
  (void)fields;
  (void)targets;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
