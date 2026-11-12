#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const char* name(Endian e) { return e == Endian::little ? "little" : "big"; }

}  // namespace

RC_TEST("a triple is read into the word sizes and byte order of the machine, in either of its shapes") {
  const struct { const char* triple; int ptr; int lng; Endian e; const char* os; } rows[] = {
      {"x86_64-linux-gnu", 64, 64, Endian::little, "linux"},
      {"aarch64-linux-gnu", 64, 64, Endian::little, "linux"},
      {"arm-linux-gnueabihf", 32, 32, Endian::little, "linux"},
      {"i686-pc-linux-gnu", 32, 32, Endian::little, "linux"},
      {"x86_64-w64-mingw32", 64, 32, Endian::little, "mingw32"},
      {"x86_64-pc-windows-msvc", 64, 32, Endian::little, "windows"},
      {"mips-linux-gnu", 32, 32, Endian::big, "linux"},
      {"riscv64-unknown-linux-gnu", 64, 64, Endian::little, "linux"},
  };
  std::cout << "  triple                       pointer  long  order   os\n";
  for (const auto& r : rows) {
    const Target t = parse_triple(r.triple);
    std::cout << "  " << r.triple << std::string(28 - std::string(r.triple).size(), ' ') << std::setw(5) << t.pointer_bits << "  " << std::setw(5) << t.long_bits << "  " << name(t.endian) << (t.endian == Endian::little ? "   " : "      ") << t.os << "\n";
    RC_CHECK(t.valid);
    RC_CHECK(t.pointer_bits == r.ptr);
    RC_CHECK(t.long_bits == r.lng);
    RC_CHECK(t.endian == r.e);
    RC_CHECK(t.os == r.os);
  }
  std::cout << "\n";
  RC_CHECK(parse_triple("x86_64-linux-gnu").abi == "gnu" && parse_triple("x86_64-linux-gnu").vendor.empty());
  RC_CHECK(parse_triple("x86_64-w64-mingw32").vendor == "w64" && parse_triple("x86_64-w64-mingw32").abi.empty());
  RC_CHECK(parse_triple("i686-pc-linux-gnu").vendor == "pc");
}

RC_TEST("a triple that cannot be read is invalid, not a guess") {
  const char* bad[] = {"", "x86_64", "x86_64-linux", "x86_64--gnu", "foo-linux-gnu", "x86_64-plan9-gnu", "x86_64-linux-gnu-extra-more", "x86_64-pc-plan9-gnu", "-linux-gnu"};
  for (const char* b : bad) {
    const Target t = parse_triple(b);
    RC_CHECK(!t.valid);
    RC_CHECK(t.pointer_bits == 0);
  }
  std::cout << "  nine malformed triples, none of them valid\n\n";
}

RC_TEST("the same struct is a different size on each target, because of long, pointers and the alignment of an eight byte field") {
  const std::vector<Field> s = {Field::i8, Field::i64, Field::ptr, Field::long_, Field::f64};
  const char* triples[] = {"x86_64-linux-gnu", "aarch64-linux-gnu", "x86_64-w64-mingw32", "arm-linux-gnueabihf", "i686-pc-linux-gnu"};
  std::cout << "  struct { char flag; int64 stamp; void* next; long count; double x; }\n  target                       size\n";
  std::size_t sizes[5];
  for (int i = 0; i < 5; ++i) {
    sizes[i] = struct_size(s, parse_triple(triples[i]));
    std::cout << "  " << triples[i] << std::string(28 - std::string(triples[i]).size(), ' ') << sizes[i] << "\n";
  }
  std::cout << "\n";
  RC_CHECK(sizes[0] == 40);
  RC_CHECK(sizes[1] == 40);
  RC_CHECK(sizes[2] == 40);          // long is 4 on Windows, and it still fills the same padding
  RC_CHECK(sizes[3] == 32);
  RC_CHECK(sizes[4] == 28);
  RC_CHECK(struct_size({}, parse_triple("x86_64-linux-gnu")) == 0);
  RC_CHECK(struct_size({Field::i8}, parse_triple("x86_64-linux-gnu")) == 1);
  RC_CHECK(struct_size({Field::i8, Field::i32}, parse_triple("x86_64-linux-gnu")) == 8);
  RC_CHECK(struct_size({Field::i32, Field::i8}, parse_triple("x86_64-linux-gnu")) == 8);      // padded at the end to keep an array aligned
  RC_CHECK(struct_size({Field::i32, Field::i32, Field::i8}, parse_triple("x86_64-linux-gnu")) == 12);
  RC_CHECK(align_of(Field::i64, parse_triple("i686-pc-linux-gnu")) == 4);
  RC_CHECK(align_of(Field::i64, parse_triple("arm-linux-gnueabihf")) == 8);
  RC_CHECK(size_of(Field::long_, parse_triple("x86_64-w64-mingw32")) == 4);
  RC_CHECK(size_of(Field::size_t_, parse_triple("arm-linux-gnueabihf")) == 4);
}

RC_TEST("a struct can be sent as it lies in memory only if it has one size on every target and no field whose width changes") {
  const std::vector<Target> all = {parse_triple("x86_64-linux-gnu"), parse_triple("aarch64-linux-gnu"), parse_triple("arm-linux-gnueabihf"), parse_triple("i686-pc-linux-gnu")};
  const std::vector<Field> fixed = {Field::i32, Field::i32, Field::f32};                          // 12 everywhere
  const std::vector<Field> gap = {Field::i8, Field::i64};                                         // 16 on most, 12 on 32 bit x86
  const std::vector<Field> wide = {Field::i32, Field::long_};
  const std::vector<Field> pointer = {Field::ptr};
  std::cout << "  { int32, int32, float }: " << (layout_equal(fixed, all) ? "same everywhere" : "differs") << "; { char, int64 }: " << (layout_equal(gap, all) ? "same everywhere" : "differs (")
            << (layout_equal(gap, all) ? "" : std::to_string(struct_size(gap, all[0])) + " and " + std::to_string(struct_size(gap, all[3])) + " bytes)") << "; { int32, long }: "
            << (layout_equal(wide, all) ? "same" : "refused") << "; { pointer }: " << (layout_equal(pointer, all) ? "same" : "refused") << "\n\n";
  RC_CHECK(layout_equal(fixed, all));
  RC_CHECK(!layout_equal(gap, all));
  RC_CHECK(struct_size(gap, all[0]) == 16);
  RC_CHECK(struct_size(gap, all[3]) == 12);
  RC_CHECK(!layout_equal(wide, all));
  RC_CHECK(!layout_equal(pointer, all));
  RC_CHECK(!layout_equal(fixed, {}));
  RC_CHECK(layout_equal(gap, {all[0], all[1]}));                                                // between two 64 bit machines it is fine
}

RC_TEST("a number is laid out in the byte order of the machine, and the same four bytes mean different numbers on the other") {
  const std::array<std::uint8_t, 4> le = encode_u32(0x01020304u, Endian::little);
  const std::array<std::uint8_t, 4> be = encode_u32(0x01020304u, Endian::big);
  std::cout << "  0x01020304 in memory, little: " << int(le[0]) << " " << int(le[1]) << " " << int(le[2]) << " " << int(le[3]) << "; big: " << int(be[0]) << " " << int(be[1]) << " " << int(be[2]) << " " << int(be[3]) << "\n";
  RC_CHECK(le[0] == 4 && le[1] == 3 && le[2] == 2 && le[3] == 1);
  RC_CHECK(be[0] == 1 && be[1] == 2 && be[2] == 3 && be[3] == 4);
  // The bytes one machine wrote, read as little endian by the other.
  const std::uint32_t read_wrong = static_cast<std::uint32_t>(be[0]) | (static_cast<std::uint32_t>(be[1]) << 8) | (static_cast<std::uint32_t>(be[2]) << 16) | (static_cast<std::uint32_t>(be[3]) << 24);
  std::cout << "  the big endian bytes of 16909060 read as little endian: " << read_wrong << "\n\n";
  RC_CHECK(read_wrong == 0x04030201u);
  RC_CHECK(encode_u32(0u, Endian::big) == encode_u32(0u, Endian::little));
}
