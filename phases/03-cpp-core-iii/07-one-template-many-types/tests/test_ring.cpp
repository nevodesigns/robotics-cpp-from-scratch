#include <rc/test/rc_test.hpp>

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <rc/test/leak_check.hpp>

#include "solution.hpp"

namespace {

// A type of the learner's own, to show the template does not care.
struct Pose {
  double x = 0.0;
  double y = 0.0;
};

// At file scope because rule L022 says so: a local constexpr used inside a
// lambda is rejected by MSVC as C3493, and the auditor does not try to work out
// whether this particular one is used in one.
constexpr std::size_t kRoom = Ring<double, 64>::capacity();

std::string shape(const Ring<double, 4>& ring) {
  std::string text;
  for (std::size_t i = 0; i < ring.size(); ++i) {
    if (i != 0) text += " ";
    text += std::to_string(static_cast<int>(ring[i]));
  }
  return text;
}

}  // namespace

RC_TEST("a ring fills up, then forgets the oldest") {
  Ring<double, 4> ring;

  RC_CHECK(ring.capacity() == 4u);
  RC_CHECK(ring.empty());
  RC_CHECK(!ring.full());
  RC_CHECK(ring.size() == 0u);

  std::cout << "\n  pushing 1 to 7 into a ring of 4\n\n";
  std::cout << "  " << std::left << std::setw(10) << "pushed" << std::setw(8) << "size"
            << std::setw(16) << "holds" << "oldest  newest\n" << std::right;

  for (int value = 1; value <= 7; ++value) {
    ring.push(static_cast<double>(value));
    std::cout << "  " << std::left << std::setw(10) << value << std::setw(8)
              << ring.size() << std::setw(16) << shape(ring);
    // oldest() and newest() have a precondition, so the suite respects it even
    // when a half written push leaves the ring empty.
    if (!ring.empty())
      std::cout << std::setw(8) << static_cast<int>(ring.oldest())
                << static_cast<int>(ring.newest());
    std::cout << "\n" << std::right;
  }
  std::cout << "\n";

  // It never grows past its capacity, and the newest is always the last push.
  RC_CHECK(ring.size() == 4u);
  RC_CHECK(ring.full());
  RC_CHECK_NEAR(ring.newest(), 7.0, 1e-12);

  // And the oldest is the oldest still held, which is 4, not 1.
  RC_CHECK_NEAR(ring.oldest(), 4.0, 1e-12);

  // Position zero counts from the oldest, so the window reads in order.
  RC_CHECK_NEAR(ring[0], 4.0, 1e-12);
  RC_CHECK_NEAR(ring[1], 5.0, 1e-12);
  RC_CHECK_NEAR(ring[2], 6.0, 1e-12);
  RC_CHECK_NEAR(ring[3], 7.0, 1e-12);
}

RC_TEST("a ring of one holds the newest and nothing else") {
  // The smallest coherent ring, which is where off by one errors live.
  Ring<int, 1> ring;
  RC_CHECK(ring.empty());

  ring.push(10);
  RC_REQUIRE(!ring.empty());
  RC_CHECK(ring.size() == 1u);
  RC_CHECK(ring.full());
  RC_CHECK(ring.oldest() == 10);
  RC_CHECK(ring.newest() == 10);
  RC_CHECK(ring[0] == 10);

  ring.push(20);
  RC_CHECK(ring.size() == 1u);
  RC_CHECK(ring.oldest() == 20);
  RC_CHECK(ring.newest() == 20);

  // And clearing puts it back to the start rather than leaving stale state.
  ring.clear();
  RC_CHECK(ring.empty());
  RC_CHECK(ring.size() == 0u);
  ring.push(30);
  RC_CHECK(ring.newest() == 30);
  RC_CHECK(ring[0] == 30);
}

RC_TEST("the slot arithmetic wraps and keeps wrapping") {
  Ring<int, 3> ring;

  // Pushed well past capacity, many times round, so a wrap that works once
  // and drifts is caught.
  for (int value = 0; value < 100; ++value) ring.push(value);

  RC_REQUIRE(ring.size() == 3u);
  RC_CHECK(ring[0] == 97);
  RC_CHECK(ring[1] == 98);
  RC_CHECK(ring[2] == 99);
  RC_CHECK(ring.oldest() == 97);
  RC_CHECK(ring.newest() == 99);

  // Every slot is a real slot, every time round.
  for (std::size_t i = 0; i < ring.size(); ++i) RC_CHECK(ring.slot(i) < ring.capacity());

  std::cout << "  after 100 pushes into a ring of 3 it holds " << ring[0] << " "
            << ring[1] << " " << ring[2] << "\n\n";
}

RC_TEST("one template, three types, and no code written twice") {
  Ring<double, 3> volts;
  Ring<int, 3> counts;
  Ring<Pose, 3> trail;

  volts.push(1.5);
  volts.push(2.5);
  counts.push(7);
  counts.push(8);
  trail.push(Pose{1.0, 2.0});
  trail.push(Pose{3.0, 4.0});

  RC_REQUIRE(!volts.empty());
  RC_REQUIRE(!counts.empty());
  RC_REQUIRE(!trail.empty());
  RC_CHECK_NEAR(volts.newest(), 2.5, 1e-12);
  RC_CHECK(counts.newest() == 8);
  RC_CHECK_NEAR(trail.newest().x, 3.0, 1e-12);
  RC_CHECK_NEAR(trail.newest().y, 4.0, 1e-12);

  // The one that matters: a type the template author never heard of works,
  // because a template is checked against what the body actually uses rather
  // than against a declared interface. Pose has no operator<, no arithmetic
  // and no default constructor argument, and the ring needs none of those.
  RC_CHECK(trail.size() == 2u);
  RC_CHECK(trail[0].x == 1.0);

  std::cout << "  the same Ring served double, int and a struct of this file's own\n\n";
}

RC_TEST("each instantiation is separate code with its own statics") {
  Instantiated<double>::uses = 0;
  Instantiated<int>::uses = 0;

  for (int i = 0; i < 5; ++i) Instantiated<double>::note();
  for (int i = 0; i < 2; ++i) Instantiated<int>::note();

  std::cout << "  Instantiated<double>::uses " << Instantiated<double>::uses << "\n";
  std::cout << "  Instantiated<int>::uses    " << Instantiated<int>::uses << "\n\n";

  // Two different classes, not one class used twice. Which is also why a
  // template in a header costs compile time per type and per translation unit.
  RC_CHECK(Instantiated<double>::uses == 5);
  RC_CHECK(Instantiated<int>::uses == 2);

  // The same is true of the ring. Ring<double, 4> and Ring<int, 4> are two
  // classes that share a definition and nothing else, so pushing into one
  // leaves the other alone, and Ring<double, 8> is a third class again.
  Ring<double, 4> volts;
  Ring<int, 4> counts;
  volts.push(1.0);
  RC_CHECK(volts.size() == 1u);
  RC_CHECK(counts.size() == 0u);
  RC_CHECK(sizeof(Ring<double, 8>) > sizeof(Ring<double, 4>));

  std::cout << "  sizeof(Ring<double, 4>) " << sizeof(Ring<double, 4>)
            << ", sizeof(Ring<double, 8>) " << sizeof(Ring<double, 8>) << "\n\n";
}

RC_TEST("a ring allocates nothing, ever") {
  // The capacity is a compile time constant, so the storage is an array inside
  // the object. That is the reason to write this rather than reach for a
  // std::vector, and it is why a control loop can hold one.
  rc::test::AllocationCount counted;

  Ring<double, 64> ring;
  for (int i = 0; i < 1000; ++i) ring.push(static_cast<double>(i));

  const std::size_t blocks = counted.blocks();
  const std::size_t arrays = counted.arrays();

  std::cout << "  1000 pushes into a Ring<double, 64>\n\n";
  std::cout << "    allocations: " << blocks << " blocks, " << arrays << " arrays\n";
  std::cout << "    sizeof(Ring<double, 64>) is " << sizeof(Ring<double, 64>)
            << ", and 64 doubles are " << (64 * sizeof(double)) << " of it\n\n";

  RC_CHECK(blocks == 0u);
  RC_CHECK(arrays == 0u);
  RC_CHECK(counted.none());
  RC_CHECK(ring.size() == 64u);
  RC_CHECK(ring.full());

  // All the storage is inside the object, with room only for the two indices
  // of bookkeeping on top.
  RC_CHECK(sizeof(Ring<double, 64>) >= 64 * sizeof(double));
  RC_CHECK(sizeof(Ring<double, 64>) < 64 * sizeof(double) + 4 * sizeof(std::size_t));

  // And the capacity really is compile time, usable where a constant must be:
  // kRoom at the top of this file is initialised from it, and a static_assert
  // on it is checked while this file compiles rather than while it runs.
  static_assert(kRoom == 64, "capacity() must be usable as a constant");
  RC_CHECK(kRoom == 64u);
}

RC_TEST("averaging a window, through a function template over both parameters") {
  Ring<double, 4> volts;
  RC_CHECK_NEAR(average(volts), 0.0, 1e-12);     // empty, and does not divide by zero

  volts.push(1.0);
  volts.push(2.0);
  volts.push(3.0);
  RC_CHECK_NEAR(average(volts), 2.0, 1e-12);

  volts.push(4.0);
  volts.push(5.0);                                // 1 has been forgotten
  RC_CHECK_NEAR(average(volts), 3.5, 1e-12);

  // The same function, a different capacity and a different type, with neither
  // written at the call site.
  Ring<int, 8> counts;
  for (int i = 1; i <= 4; ++i) counts.push(i);
  RC_CHECK_NEAR(average(counts), 2.5, 1e-12);

  Ring<double, 2> pair;
  pair.push(10.0);
  pair.push(20.0);
  RC_CHECK_NEAR(average(pair), 15.0, 1e-12);

  std::cout << "  average() took Ring<double,4>, Ring<int,8> and Ring<double,2>\n";
  std::cout << "  and the capacities were never written at the call site\n\n";
}
