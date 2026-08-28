#include <rc/test/rc_test.hpp>
#include <rc/test/leak_check.hpp>

#include <type_traits>
#include <utility>

#include "solution.hpp"

namespace {

using rc::test::LeakCheck;

SampleBuffer counting(std::size_t size) {
  SampleBuffer buffer(size);
  for (std::size_t i = 0; i < size; ++i) buffer.set(i, static_cast<double>(i));
  return buffer;
}

}  // namespace

RC_TEST("a buffer holds what it is given") {
  SampleBuffer buffer(3);
  buffer.set(0, 1.5);
  RC_CHECK_EQ(buffer.size(), std::size_t{3});
  RC_CHECK_NEAR(buffer.at(0), 1.5, 1e-9);
}

RC_TEST("a new buffer starts zeroed") {
  const SampleBuffer buffer(4);
  for (std::size_t i = 0; i < 4; ++i) RC_CHECK_NEAR(buffer.at(i), 0.0, 1e-9);
}

RC_TEST("an empty buffer allocates nothing and releases nothing") {
  const LeakCheck check;
  { const SampleBuffer buffer(0); }
  RC_CHECK(check.balanced());
}

RC_TEST("a buffer releases its memory") {
  const LeakCheck check;
  { const SampleBuffer buffer(64); }
  RC_CHECK(check.balanced());
}

RC_TEST("a copy has the same contents") {
  const SampleBuffer source = counting(5);
  const SampleBuffer copy(source);
  RC_REQUIRE_EQ(copy.size(), std::size_t{5});
  for (std::size_t i = 0; i < 5; ++i) RC_CHECK_NEAR(copy.at(i), source.at(i), 1e-9);
}

RC_TEST("a copy is independent of the original") {
  // The check that catches a copied pointer. If both objects share one buffer,
  // changing either is visible through the other.
  SampleBuffer source = counting(3);
  SampleBuffer copy(source);
  copy.set(0, 99.0);
  RC_CHECK_NEAR(source.at(0), 0.0, 1e-9);
  RC_CHECK_NEAR(copy.at(0), 99.0, 1e-9);
}

RC_TEST("copying leaks nothing and frees nothing twice") {
  const LeakCheck check;
  {
    const SampleBuffer source = counting(8);
    const SampleBuffer copy(source);
  }
  RC_CHECK(check.balanced());
}

RC_TEST("copy assignment replaces the contents") {
  SampleBuffer target(2);
  const SampleBuffer source = counting(5);
  target = source;
  RC_REQUIRE_EQ(target.size(), std::size_t{5});
  RC_CHECK_NEAR(target.at(4), 4.0, 1e-9);
}

RC_TEST("copy assignment survives assigning an object to itself") {
  // Looks absurd written out, arrives in real code as a[i] = a[j] with equal
  // indices, or through two references to one object.
  SampleBuffer buffer = counting(4);
  SampleBuffer& alias = buffer;
  buffer = alias;
  RC_REQUIRE_EQ(buffer.size(), std::size_t{4});
  RC_CHECK_NEAR(buffer.at(2), 2.0, 1e-9);
}

RC_TEST("copy assignment leaks nothing") {
  const LeakCheck check;
  {
    SampleBuffer target(2);
    const SampleBuffer source = counting(5);
    target = source;
  }
  RC_CHECK(check.balanced());
}

RC_TEST("moving transfers the contents") {
  SampleBuffer source = counting(6);
  const SampleBuffer moved(std::move(source));
  RC_REQUIRE_EQ(moved.size(), std::size_t{6});
  RC_CHECK_NEAR(moved.at(5), 5.0, 1e-9);
}

RC_TEST("a moved from buffer is left empty and safe to destroy") {
  SampleBuffer source = counting(6);
  const SampleBuffer moved(std::move(source));
  RC_CHECK(source.empty());
  RC_CHECK_EQ(source.size(), std::size_t{0});
}

RC_TEST("moving allocates nothing at all") {
  const LeakCheck check;
  {
    SampleBuffer source = counting(32);
    const SampleBuffer moved(std::move(source));
    // A move that allocates is a copy wearing the wrong name.
    RC_CHECK_EQ(check.leaked_arrays(), std::size_t{1});
  }
  RC_CHECK(check.balanced());
}

RC_TEST("move assignment transfers and leaks nothing") {
  const LeakCheck check;
  {
    SampleBuffer target(3);
    SampleBuffer source = counting(7);
    target = std::move(source);
    RC_CHECK_EQ(target.size(), std::size_t{7});
    RC_CHECK(source.empty());
  }
  RC_CHECK(check.balanced());
}

RC_TEST("move assignment survives assigning an object to itself") {
  SampleBuffer buffer = counting(4);
  SampleBuffer& alias = buffer;
  buffer = std::move(alias);
  // The requirement is only that it is still valid and destructible. A buffer
  // that released its own memory and then adopted the freed pointer would fail
  // here or crash on the way out.
  RC_CHECK(buffer.size() == 4 || buffer.empty());
}

RC_TEST("the move operations promise not to throw") {
  // std::vector only moves its elements while growing when this promise is
  // made, and copies them otherwise, so the absence of noexcept is a silent
  // slowdown rather than an error.
  RC_CHECK(std::is_nothrow_move_constructible<SampleBuffer>::value);
  RC_CHECK(std::is_nothrow_move_assignable<SampleBuffer>::value);
}
