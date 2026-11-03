#include <rc/test/rc_test.hpp>

#include <array>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

constexpr std::size_t kCount = 1024;

std::vector<double> readings() { return std::vector<double>(kCount, 1.0); }

const char* verdict(bool same) { return same ? "the same buffer" : "a new buffer"; }

}  // namespace

RC_TEST("by value copies the buffer, by reference and by span do not") {
  const std::vector<double> samples = readings();
  const Address caller = address_of(samples.data());

  struct Row {
    const char* name;
    Address seen;
  };
  const Row rows[] = {
      {"by value", passed_by_value(samples)},
      {"by const reference", passed_by_const_reference(samples)},
      {"as a span", passed_as_span(rc::span<const double>(samples.data(), samples.size()))},
  };

  std::cout << "\n  a vector of " << kCount << " doubles, and what the callee sees\n\n";
  std::cout << "  " << std::left << std::setw(26) << "how it is passed" << "what arrives\n"
            << std::right;
  for (const Row& row : rows)
    std::cout << "  " << std::left << std::setw(26) << row.name
              << verdict(row.seen == caller) << "\n" << std::right;
  std::cout << "\n";

  // The one that costs something, and the two that do not.
  RC_CHECK(rows[0].seen != caller);
  RC_CHECK(rows[1].seen == caller);
  RC_CHECK(rows[2].seen == caller);

  // And the caller's vector is untouched in every case, which is the whole
  // reason by value is ever chosen.
  RC_CHECK(samples.size() == kCount);
  RC_CHECK(address_of(samples.data()) == caller);
}

RC_TEST("each call that takes its own copy allocates another buffer") {
  const std::vector<double> samples = readings();
  const Address caller = address_of(samples.data());

  const ChainAddresses by_value = passed_by_value_twice(samples);
  const ChainAddresses by_reference = passed_by_reference_twice(samples);

  int copies_by_value = 0;
  if (by_value.first != caller) ++copies_by_value;
  if (by_value.second != by_value.first) ++copies_by_value;

  int copies_by_reference = 0;
  if (by_reference.first != caller) ++copies_by_reference;
  if (by_reference.second != by_reference.first) ++copies_by_reference;

  std::cout << "  through two calls\n\n";
  std::cout << "  " << std::left << std::setw(26) << "by value" << copies_by_value
            << " buffers allocated\n";
  std::cout << "  " << std::left << std::setw(26) << "by const reference"
            << copies_by_reference << " buffers allocated\n\n" << std::right;

  // Two calls, two copies. It is per call, not once.
  RC_CHECK(copies_by_value == 2);
  RC_CHECK(copies_by_reference == 0);
  RC_CHECK(by_reference.first == caller);
  RC_CHECK(by_reference.second == caller);
}

RC_TEST("returning a container hands the buffer over rather than rebuilding it") {
  Address inside = 0;
  const std::vector<double> returned = built_and_returned(kCount, &inside);

  std::cout << "  building " << kCount << " doubles inside a function and returning them\n\n";
  std::cout << "  " << std::left << std::setw(34) << "the buffer the local held"
            << verdict(address_of(returned.data()) == inside) << " arrived\n\n" << std::right;

  RC_REQUIRE(inside != 0);
  RC_CHECK(returned.size() == kCount);

  // The buffer travelled out. Whether the compiler elided the local or moved
  // it, nothing was copied, and this holds on every toolchain this curriculum
  // claims and at every optimisation level.
  RC_CHECK(address_of(returned.data()) == inside);

  // The out parameter version does the same work and produces the same result.
  std::vector<double> out;
  built_into_an_out_parameter(kCount, out);
  RC_CHECK(out.size() == kCount);
  for (std::size_t i = 0; i < out.size(); ++i) RC_CHECK_NEAR(out[i], returned[i], 1e-12);

  // What it cost was not performance. It cost a declaration on a separate
  // line, which cannot be const, and a function whose result is not its
  // return value.
  std::cout << "  the out parameter version computes the same thing, and needs the\n";
  std::cout << "  vector declared first and left non const to do it\n\n";
}

RC_TEST("a span borrows from anything laid out in a row") {
  const std::vector<double> from_vector = readings();
  std::array<double, 4> from_array = {1.0, 2.0, 3.0, 4.0};
  const double from_driver[3] = {5.0, 6.0, 7.0};

  // The same function, three kinds of caller, no copies anywhere.
  RC_CHECK(passed_as_span(rc::span<const double>(from_vector.data(), from_vector.size())) ==
           address_of(from_vector.data()));
  RC_CHECK(passed_as_span(rc::span<const double>(from_array.data(), from_array.size())) ==
           address_of(from_array.data()));
  RC_CHECK(passed_as_span(rc::span<const double>(from_driver, 3)) ==
           address_of(&from_driver[0]));

  // A const reference to std::vector cannot take the other two at all without
  // building a vector first, which is the copy this signature avoids.
  std::cout << "  a span took a vector, a std::array and a plain buffer, and copied\n";
  std::cout << "  none of them. A const vector reference would have needed a vector\n";
  std::cout << "  built from the last two.\n\n";

  // An empty span is a legitimate thing to pass and is not a null pointer
  // dressed up: it has a size of zero.
  const rc::span<const double> nothing;
  RC_CHECK(nothing.size() == 0u);
  RC_CHECK(nothing.empty());

  // Three containers, three different places. Without this, a version that
  // reported nothing at all would agree with itself and pass the three checks
  // above.
  RC_CHECK(passed_as_span(rc::span<const double>(from_vector.data(), from_vector.size())) != 0);
  RC_CHECK(passed_as_span(rc::span<const double>(from_array.data(), from_array.size())) !=
           passed_as_span(rc::span<const double>(from_driver, 3)));
}

RC_TEST("a short string keeps its characters inside itself") {
  std::cout << "  sizeof(std::string) is " << sizeof(std::string) << " on this library\n\n";
  std::cout << "  " << std::left << std::setw(10) << "length" << std::setw(26)
            << "characters live" << "copying it allocates\n" << std::right;

  const std::size_t lengths[] = {0, 1, 8, 15, 16, 23, 24, 64};
  int inline_count = 0;
  int heap_count = 0;
  for (std::size_t length : lengths) {
    const std::string text(length, 'x');
    const bool inside = stored_inside_the_object(text);
    if (inside) ++inline_count;
    else ++heap_count;
    std::cout << "  " << std::left << std::setw(10) << length << std::setw(26)
              << (inside ? "inside the object" : "on the heap")
              << (inside ? "nothing" : "a buffer") << "\n" << std::right;
  }
  std::cout << "\n  the length where that changes is your standard library's choice,\n";
  std::cout << "  not a rule of the language, so the table above is the one you have\n\n";

  // Both kinds occur, and the short end is inline while the long end is not.
  // The exact threshold is deliberately not asserted.
  RC_CHECK(inline_count > 0);
  RC_CHECK(heap_count > 0);
  RC_CHECK(stored_inside_the_object(std::string(1, 'x')));
  RC_CHECK(!stored_inside_the_object(std::string(64, 'x')));

  // Which is why "pass strings by const reference" is advice about long
  // strings. Copying a short one allocates nothing, and the copy is a handful
  // of bytes moved inside one object.
  RC_CHECK(stored_inside_the_object(std::string()));
}

RC_TEST("passing a long string by value copies its characters, by reference does not") {
  const std::string long_text(256, 'x');
  const Address caller = address_of(long_text.data());

  // Same measurement as the vector, on the other container everybody passes.
  const auto by_value = [](std::string text) { return address_of(text.data()); };
  const auto by_reference = [](const std::string& text) { return address_of(text.data()); };

  RC_CHECK(by_value(long_text) != caller);
  RC_CHECK(by_reference(long_text) == caller);

  std::cout << "  a 256 character string passed by value arrives as "
            << verdict(by_value(long_text) == caller) << "\n";
  std::cout << "  the same string by const reference arrives as "
            << verdict(by_reference(long_text) == caller) << "\n\n";

  // And this is the string whose characters are certainly not inside it, which
  // is why passing it by value cost a buffer.
  RC_CHECK(!stored_inside_the_object(long_text));
  RC_CHECK(stored_inside_the_object(std::string(2, 'x')));
}
