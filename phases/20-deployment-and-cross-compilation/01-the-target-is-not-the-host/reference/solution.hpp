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
  Target t;
  const std::vector<std::string> p = detail::split_dashes(triple);
  if (p.size() < 3 || p.size() > 4) return t;
  for (const std::string& s : p)
    if (s.empty()) return t;
  t.arch = p[0];
  if (p.size() == 4) {
    t.vendor = p[1];
    t.os = p[2];
    t.abi = p[3];
  } else if (detail::known_os(p[1])) {
    t.os = p[1];
    t.abi = p[2];
  } else {
    t.vendor = p[1];
    t.os = p[2];
  }
  if (!detail::known_os(t.os)) return Target();
  const std::string& a = t.arch;
  if (a == "x86_64" || a == "aarch64" || a == "riscv64" || a == "s390x" || a == "powerpc64") t.pointer_bits = 64;
  else if (a == "i686" || a == "i386" || a == "arm" || a == "armv7" || a == "riscv32" || a == "mips") t.pointer_bits = 32;
  else return Target();
  t.endian = (a == "mips" || a == "s390x" || a == "powerpc64") ? Endian::big : Endian::little;
  t.long_bits = (t.os == "windows" || t.os == "mingw32") ? 32 : t.pointer_bits;
  t.valid = true;
  return t;
}

enum class Field { i8, i16, i32, i64, f32, f64, ptr, long_, size_t_ };

// The size in bytes of a field on a target: i8 1, i16 2, i32 4, i64 8, f32 4, f64 8, a pointer and a size_t as wide as a pointer, a long as wide as a long.
inline std::size_t size_of(Field f, const Target& t) {
  switch (f) {
    case Field::i8: return 1;
    case Field::i16: return 2;
    case Field::i32: return 4;
    case Field::i64: return 8;
    case Field::f32: return 4;
    case Field::f64: return 8;
    case Field::ptr: return static_cast<std::size_t>(t.pointer_bits / 8);
    case Field::long_: return static_cast<std::size_t>(t.long_bits / 8);
    case Field::size_t_: return static_cast<std::size_t>(t.pointer_bits / 8);
  }
  return 0;
}

// The alignment of a field inside a struct: its size, except that on 32 bit x86 (i686 and i386) an eight byte i64 or f64 is aligned to 4.
inline std::size_t align_of(Field f, const Target& t) {
  if ((t.arch == "i686" || t.arch == "i386") && (f == Field::i64 || f == Field::f64)) return 4;
  return size_of(f, t);
}

// The size of a struct whose fields are these, in order, with each field placed at the next multiple of its alignment, and the whole padded to a multiple of the largest alignment. An empty struct is 0 bytes.
inline std::size_t struct_size(const std::vector<Field>& fields, const Target& t) {
  std::size_t offset = 0;
  std::size_t biggest = 1;
  for (Field f : fields) {
    const std::size_t a = align_of(f, t);
    biggest = a > biggest ? a : biggest;
    offset = (offset + a - 1) / a * a;
    offset += size_of(f, t);
  }
  if (fields.empty()) return 0;
  return (offset + biggest - 1) / biggest * biggest;
}

// The four bytes of a 32 bit value as they are in memory on a machine of this byte order, first byte first.
inline std::array<std::uint8_t, 4> encode_u32(std::uint32_t v, Endian e) {
  std::array<std::uint8_t, 4> b{};
  for (int i = 0; i < 4; ++i) {
    const int shift = e == Endian::little ? 8 * i : 8 * (3 - i);
    b[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>((v >> shift) & 0xFFu);
  }
  return b;
}

// Can this struct be sent between all of these targets as it lies in memory? Only if it has the same size on every one of them and contains no pointer, long or size_t, whose width differs by target. Fewer than one target: false.
inline bool layout_equal(const std::vector<Field>& fields, const std::vector<Target>& targets) {
  if (targets.empty()) return false;
  for (Field f : fields)
    if (f == Field::ptr || f == Field::long_ || f == Field::size_t_) return false;
  const std::size_t first = struct_size(fields, targets[0]);
  for (const Target& t : targets)
    if (struct_size(fields, t) != first) return false;
  return true;
}

#endif  // LESSON_SOLUTION_HPP
