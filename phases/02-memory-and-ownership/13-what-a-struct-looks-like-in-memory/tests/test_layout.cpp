#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// Real structs, laid out by the compiler running this test. The code under test has to agree with it.
struct Spread {            // a byte, a double, a byte: the order a person writes
  std::uint8_t flag;
  double value;
  std::uint8_t count;
};

struct Packed {            // the same three members, largest first
  double value;
  std::uint8_t flag;
  std::uint8_t count;
};

struct Tail {              // the end of a struct is padded too
  std::uint64_t big;
  std::uint8_t small;
};

struct Mixed {
  std::uint16_t a;
  std::uint32_t b;
  std::uint8_t c;
  std::uint64_t d;
  std::uint16_t e;
};

// A telemetry record in the order the fields occurred to somebody, and in the order that wastes least.
struct TelemetryAsWritten {
  bool valid;
  double timestamp;
  std::uint8_t sensor;
  float x;
  bool fresh;
  std::uint32_t sequence;
  std::int16_t temperature;
  std::uint64_t counter;
};

struct TelemetrySorted {
  double timestamp;
  std::uint64_t counter;
  float x;
  std::uint32_t sequence;
  std::int16_t temperature;
  bool valid;
  std::uint8_t sensor;
  bool fresh;
};

struct alignas(16) Vector4 {
  float v[4];
};

struct WithWide {
  std::uint8_t tag;
  Vector4 vec;
  std::uint8_t other;
};

FieldSpec spec(std::size_t size, std::size_t alignment) {
  FieldSpec f;
  f.size = size;
  f.alignment = alignment;
  return f;
}

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
};

}  // namespace

RC_TEST("rounding up to a multiple of the alignment") {
  RC_CHECK_EQ(align_up(0, 8), static_cast<std::size_t>(0));
  RC_CHECK_EQ(align_up(1, 8), static_cast<std::size_t>(8));
  RC_CHECK_EQ(align_up(7, 8), static_cast<std::size_t>(8));
  RC_CHECK_EQ(align_up(8, 8), static_cast<std::size_t>(8));
  RC_CHECK_EQ(align_up(9, 4), static_cast<std::size_t>(12));
  RC_CHECK_EQ(align_up(17, 16), static_cast<std::size_t>(32));
  RC_CHECK_EQ(align_up(5, 1), static_cast<std::size_t>(5));
  RC_CHECK_EQ(align_up(5, 0), static_cast<std::size_t>(5));   // no alignment is no gap
  RC_CHECK_EQ(align_up(0, 0), static_cast<std::size_t>(0));
}

// An offset that a stub with no answer cannot make the test read past the end of.
static std::size_t offset_at(const Layout& layout, std::size_t i) {
  return i < layout.offsets.size() ? layout.offsets[i] : 0;
}

RC_TEST("a byte, a double and a byte: the member order a person writes costs eight bytes in padding") {
  const Layout l = layout_of({spec(1, 1), spec(8, 8), spec(1, 1)});
  const Layout p = layout_of({spec(8, 8), spec(1, 1), spec(1, 1)});

  std::cout << "  the same three members, a byte, a double and a byte:\n"
            << "    in the order written      size " << sizeof(Spread) << ", padding " << l.padding << ", offsets "
            << offset_at(l, 0) << " " << offset_at(l, 1) << " " << offset_at(l, 2) << "\n"
            << "    largest first             size " << sizeof(Packed) << ", padding " << p.padding << ", offsets "
            << offset_at(p, 0) << " " << offset_at(p, 1) << " " << offset_at(p, 2) << "\n\n";

  // The arithmetic agrees with the compiler, member by member.
  RC_REQUIRE_EQ(l.offsets.size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(l.offsets[0], offsetof(Spread, flag));
  RC_CHECK_EQ(l.offsets[1], offsetof(Spread, value));
  RC_CHECK_EQ(l.offsets[2], offsetof(Spread, count));
  RC_CHECK_EQ(l.size, sizeof(Spread));
  RC_CHECK_EQ(l.alignment, alignof(Spread));

  RC_REQUIRE_EQ(p.offsets.size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(p.offsets[0], offsetof(Packed, value));
  RC_CHECK_EQ(p.offsets[1], offsetof(Packed, flag));
  RC_CHECK_EQ(p.offsets[2], offsetof(Packed, count));
  RC_CHECK_EQ(p.size, sizeof(Packed));

  RC_CHECK_EQ(l.size, static_cast<std::size_t>(24));
  RC_CHECK_EQ(p.size, static_cast<std::size_t>(16));
  RC_CHECK_EQ(l.padding, static_cast<std::size_t>(14));
  RC_CHECK_EQ(p.padding, static_cast<std::size_t>(6));
}

RC_TEST("the end of a struct is padded too, because an array of them has to stay aligned") {
  const Layout l = layout_of({spec(8, 8), spec(1, 1)});
  RC_CHECK_EQ(l.size, sizeof(Tail));
  RC_CHECK_EQ(l.size, static_cast<std::size_t>(16));   // nine bytes of members

  // The proof: in an array the second element starts where the first ends, and its double has to be
  // on a multiple of eight.
  Tail array[3] = {{1, 1}, {2, 2}, {3, 3}};
  const std::uintptr_t first = reinterpret_cast<std::uintptr_t>(&array[0].big);
  const std::uintptr_t second = reinterpret_cast<std::uintptr_t>(&array[1].big);
  RC_CHECK_EQ(second - first, l.size);
  RC_CHECK_EQ(second % 8, static_cast<std::uintptr_t>(0));
}

RC_TEST("the arithmetic agrees with the compiler on mixed members and on an over-aligned one") {
  const Layout m = layout_of({spec(2, 2), spec(4, 4), spec(1, 1), spec(8, 8), spec(2, 2)});
  RC_REQUIRE_EQ(m.offsets.size(), static_cast<std::size_t>(5));
  RC_CHECK_EQ(m.offsets[0], offsetof(Mixed, a));
  RC_CHECK_EQ(m.offsets[1], offsetof(Mixed, b));
  RC_CHECK_EQ(m.offsets[2], offsetof(Mixed, c));
  RC_CHECK_EQ(m.offsets[3], offsetof(Mixed, d));
  RC_CHECK_EQ(m.offsets[4], offsetof(Mixed, e));
  RC_CHECK_EQ(m.size, sizeof(Mixed));

  // alignas(16) pushes a member to a multiple of sixteen and the whole struct to match.
  const Layout w = layout_of({spec(1, 1), spec(16, 16), spec(1, 1)});
  RC_REQUIRE_EQ(w.offsets.size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(w.offsets[1], offsetof(WithWide, vec));
  RC_CHECK_EQ(w.size, sizeof(WithWide));
  RC_CHECK_EQ(w.alignment, alignof(WithWide));
  RC_CHECK_EQ(w.size, static_cast<std::size_t>(48));

  // No members is no size.
  const Layout none = layout_of({});
  RC_CHECK_EQ(none.size, static_cast<std::size_t>(0));
  RC_CHECK_EQ(none.padding, static_cast<std::size_t>(0));
}

RC_TEST("largest alignment first is the smallest the struct can be, checked against every order") {
  Rng rng{17};
  const std::size_t alignments[] = {1, 2, 4, 8, 16};
  int trials = 0;
  int sorted_was_best = 0;
  int worst_saved = 0;

  for (int t = 0; t < 400; ++t) {
    const std::size_t count = 3 + rng.next() % 4;   // three to six members
    std::vector<FieldSpec> fields;
    for (std::size_t i = 0; i < count; ++i) {
      const std::size_t a = alignments[rng.next() % 5];
      fields.push_back(spec(a * (1 + rng.next() % 2), a));   // a member is one or two alignments long
    }

    // The truth: try every order, and keep the smallest.
    std::vector<std::size_t> order(count);
    for (std::size_t i = 0; i < count; ++i) order[i] = i;
    std::size_t best = layout_of(fields).size;
    std::size_t first = best;
    do {
      std::vector<FieldSpec> permuted;
      for (const std::size_t i : order) permuted.push_back(fields[i]);
      best = std::min(best, layout_of(permuted).size);
    } while (std::next_permutation(order.begin(), order.end()));

    ++trials;
    if (smallest_size(fields) == best) ++sorted_was_best;
    worst_saved = std::max(worst_saved, static_cast<int>(first - best));
  }

  std::cout << "  " << trials << " random structs of three to six members, every order tried for each: putting the most\n"
            << "  strictly aligned first gave the smallest size in " << sorted_was_best << " of them. Reordering saved up to "
            << worst_saved << " bytes.\n\n";

  RC_CHECK_EQ(sorted_was_best, trials);
  RC_CHECK(worst_saved > 0);
}

RC_TEST("a telemetry record in the order it occurred to somebody, and in the order that wastes least") {
  const std::vector<FieldSpec> written = {spec(sizeof(bool), alignof(bool)),
                                          spec(sizeof(double), alignof(double)),
                                          spec(sizeof(std::uint8_t), alignof(std::uint8_t)),
                                          spec(sizeof(float), alignof(float)),
                                          spec(sizeof(bool), alignof(bool)),
                                          spec(sizeof(std::uint32_t), alignof(std::uint32_t)),
                                          spec(sizeof(std::int16_t), alignof(std::int16_t)),
                                          spec(sizeof(std::uint64_t), alignof(std::uint64_t))};

  const Layout as_written = layout_of(written);
  const std::size_t smallest = smallest_size(written);

  const std::size_t members = 1 + 8 + 1 + 4 + 1 + 4 + 2 + 8;
  std::cout << "  a record of eight members holding " << members << " bytes of data:\n"
            << "    in the order written   " << sizeof(TelemetryAsWritten) << " bytes, " << as_written.padding
            << " of them padding\n"
            << "    the best order         " << sizeof(TelemetrySorted) << " bytes\n"
            << "  A million of them is " << sizeof(TelemetryAsWritten) << " MB against " << sizeof(TelemetrySorted)
            << " MB, and every cache line holds " << std::fixed << std::setprecision(1)
            << 64.0 / static_cast<double>(sizeof(TelemetryAsWritten)) << " records against "
            << 64.0 / static_cast<double>(sizeof(TelemetrySorted)) << ".\n\n";

  RC_CHECK_EQ(as_written.size, sizeof(TelemetryAsWritten));
  RC_CHECK_EQ(smallest, sizeof(TelemetrySorted));
  RC_CHECK(smallest < as_written.size);
  RC_CHECK_EQ(as_written.size, static_cast<std::size_t>(48));
  RC_CHECK_EQ(smallest, static_cast<std::size_t>(32));
}
