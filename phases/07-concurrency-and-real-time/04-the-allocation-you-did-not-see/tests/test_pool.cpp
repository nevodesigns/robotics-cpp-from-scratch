#include <rc/test/rc_test.hpp>
#include <rc/test/leak_check.hpp>

#include <rc/rt/histogram.hpp>

#include <chrono>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

using rc::test::AllocationCount;

// A message a control loop might pass around: a name and some numbers.
struct Reading {
  char name[24] = {};
  double values[8] = {};
  int count = 0;
};

// How long a body takes, per call, at three percentiles and at its worst.
template <class Body>
rc::rt::Histogram time_body(int iterations, Body body) {
  rc::rt::Histogram histogram(0.05, 4000);   // 50 ns buckets out to 200 us
  for (int i = 0; i < iterations; ++i) {
    const auto start = std::chrono::steady_clock::now();
    body(i);
    const auto end = std::chrono::steady_clock::now();
    histogram.record(std::chrono::duration<double, std::micro>(end - start).count());
  }
  return histogram;
}

// The longest string this toolchain keeps inside the string object, or -1 if
// there is no such length because every string allocates.
//
// Found rather than assumed. It is 15 on libstdc++ and on the Microsoft library
// in a release build, 22 on libc++, and there is no window at all in a
// Microsoft debug build, where each string gets a bookkeeping object of its own
// whatever its length.
int small_string_limit() {
  for (std::size_t length = 1; length < 128; ++length) {
    AllocationCount count;
    std::string subject(length, 'x');
    if (count.total() > 0) return static_cast<int>(length) - 1;
    // Touching it keeps the optimiser from deleting the string entirely.
    if (subject.size() == 12345) return static_cast<int>(length);
  }
  return 127;
}

}  // namespace

RC_TEST("the allocation counter counts what the leak counter cannot") {
  // Allocating and freeing leaves no leak and is still an allocation. These are
  // two different questions and a control loop cares about the second.
  rc::test::LeakCheck leaks;
  AllocationCount allocations;

  for (int i = 0; i < 100; ++i) {
    std::vector<double> scratch(64, 1.0);
    if (scratch.size() != 64) return;
  }

  RC_CHECK(leaks.balanced());          // nothing leaked
  RC_CHECK(allocations.total() >= 100);  // and it allocated a hundred times

  // The counter can be restarted, so one test can ask about several regions.
  allocations.reset();
  double fixed[64] = {};
  for (int i = 0; i < 64; ++i) fixed[i] = i;
  RC_CHECK(allocations.none());
  RC_CHECK_NEAR(fixed[63], 63.0, 1e-12);
}

RC_TEST("what a loop body costs, and what it allocates") {
  std::cout << "\n    " << std::left << std::setw(52) << "one pass of the loop body"
            << std::right << std::setw(14) << "allocations" << "\n\n";

  const auto report = [](const char* name, std::size_t allocations) {
    std::cout << "    " << std::left << std::setw(52) << name << std::right
              << std::setw(14) << allocations << "\n";
  };

  std::size_t fixed_array = 0, reserved_doubles = 0, reserved_strings = 0;
  std::size_t short_strings = 0, grown = 0;

  {
    AllocationCount count;
    double buffer[64] = {};
    for (int i = 0; i < 64; ++i) buffer[i] = i;
    fixed_array = count.total();
    if (buffer[0] == 12345.0) return;
  }
  report("a fixed array", fixed_array);

  {
    std::vector<double> buffer;
    buffer.reserve(64);
    AllocationCount count;
    buffer.clear();
    for (int i = 0; i < 64; ++i) buffer.push_back(i);
    reserved_doubles = count.total();
  }
  report("a reserved vector<double>, cleared and refilled", reserved_doubles);

  {
    std::vector<std::string> buffer;
    buffer.reserve(64);
    AllocationCount count;
    buffer.clear();
    for (int i = 0; i < 64; ++i) buffer.push_back("a twenty character str");
    reserved_strings = count.total();
  }
  report("a reserved vector<string>, the same thing", reserved_strings);

  {
    std::vector<std::string> buffer;
    buffer.reserve(64);
    AllocationCount count;
    buffer.clear();
    for (int i = 0; i < 64; ++i) buffer.push_back("short");
    short_strings = count.total();
  }
  report("the same, with strings that fit inside the object", short_strings);

  {
    std::vector<double> buffer;
    AllocationCount count;
    for (int i = 0; i < 4096; ++i) buffer.push_back(i);
    grown = count.total();
  }
  report("an unreserved vector grown to 4096", grown);

  std::cout << "\n    reserve on a vector of strings reserves the vector and\n";
  std::cout << "    not the strings\n";

  // True on every library: nothing on the stack goes to the heap, and putting
  // doubles into a vector that already has room for them allocates nothing.
  RC_CHECK_EQ(fixed_array, static_cast<std::size_t>(0));
  RC_CHECK_EQ(reserved_doubles, static_cast<std::size_t>(0));

  // At least one allocation per string, from a loop with reserve written in it.
  RC_CHECK(reserved_strings >= 64);

  // And shorter strings cost strictly less, which is the whole of the small
  // string optimisation. How much less is a property of the library:
  // libstdc++ makes this row zero, and the Microsoft library in a debug build
  // does not, because there every string allocates whatever its length. That
  // is not a footnote, it is the reason this is measured rather than recited.
  RC_CHECK(short_strings < reserved_strings);

  // Growth is logarithmic rather than free: a handful of reallocations, each
  // copying everything written so far.
  RC_CHECK(grown > 5);
  RC_CHECK(grown < 40);
}

RC_TEST("where the small string ends, on this toolchain") {
  const int limit = small_string_limit();

  if (limit < 0) {
    std::cout << "\n    this library keeps no string inside the object: even a\n";
    std::cout << "    single character allocates, because each string is given\n";
    std::cout << "    a bookkeeping object of its own. That is what a Microsoft\n";
    std::cout << "    debug build does, and it is the same source that is\n";
    std::cout << "    allocation free elsewhere\n";
    AllocationCount count;
    std::string one(1, 'x');
    RC_CHECK(count.total() >= 1);
    if (one.size() == 999999) return;
    return;
  }

  std::cout << "\n    the longest string this library keeps inside the object: "
            << limit << " characters\n";
  std::cout << "    sizeof(std::string) = " << sizeof(std::string) << "\n";

  std::size_t at_limit = 0, past_limit = 0;
  {
    AllocationCount count;
    std::string inside(static_cast<std::size_t>(limit), 'x');
    at_limit = count.total();
    if (inside.size() == 999999) return;
  }
  {
    AllocationCount count;
    std::string outside(static_cast<std::size_t>(limit) + 1, 'x');
    past_limit = count.total();
    if (outside.size() == 999999) return;
  }

  std::cout << "\n    which is not a number to memorise. It is 15 on libstdc++\n";
  std::cout << "    and on the Microsoft library in a release build, 22 on\n";
  std::cout << "    libc++, and 0 in a Microsoft debug build, where every\n";
  std::cout << "    string allocates whatever its length. A loop that is\n";
  std::cout << "    allocation free on your machine may not be on the robot's,\n";
  std::cout << "    and may not be in a different build of the same program\n";

  // The boundary the search found is a real boundary, whatever its value.
  RC_CHECK_EQ(at_limit, static_cast<std::size_t>(0));
  RC_CHECK(past_limit >= 1);
  RC_CHECK(limit <= 40);
}

RC_TEST("a stopwatch cannot find an allocation on a general purpose machine") {
  const int iterations = 100000;

  const rc::rt::Histogram fixed = time_body(iterations, [](int i) {
    double buffer[64];
    for (int k = 0; k < 64; ++k) buffer[k] = i * 0.001 + k;
    double total = 0.0;
    for (int k = 0; k < 64; ++k) total += buffer[k];
    if (total == 12345.0) std::cout << "";
  });

  const rc::rt::Histogram allocating = time_body(iterations, [](int i) {
    std::vector<double> buffer;
    for (int k = 0; k < 64; ++k) buffer.push_back(i * 0.001 + k);
    double total = 0.0;
    for (double value : buffer) total += value;
    if (total == 12345.0) std::cout << "";
  });

  std::cout << "\n    " << std::left << std::setw(26) << "loop body" << std::right
            << std::setw(11) << "p50 us" << std::setw(11) << "p99"
            << std::setw(11) << "p99.9" << std::setw(11) << "worst" << "\n";
  const auto row = [](const char* name, const rc::rt::Histogram& h) {
    std::cout << "    " << std::left << std::setw(26) << name << std::right
              << std::fixed << std::setprecision(3) << std::setw(11) << h.percentile(50)
              << std::setw(11) << h.percentile(99) << std::setw(11)
              << h.percentile(99.9) << std::setw(11) << h.worst() << "\n";
  };
  row("a fixed array", fixed);
  row("a vector each tick", allocating);

  std::cout << "\n    this suite builds unoptimised, which widens the gap; with\n";
  std::cout << "    optimisation the same two bodies measured 0.150 and 0.400\n";
  std::cout << "    microseconds. Either way the difference is in the median\n";
  std::cout << "    and the worst case of both belongs to the scheduler, which\n";
  std::cout << "    is why this lesson counts allocations instead of timing them\n";

  // The allocating body is slower on average. That much is reliable.
  RC_CHECK(allocating.percentile(50) >= fixed.percentile(50));

  // And the worst case of the body that allocates nothing is far above its own
  // hundredth-worst sample, because something else on the machine took the
  // processor away. No timing threshold can separate an allocation from that.
  RC_CHECK(fixed.worst() > fixed.percentile(99.9) * 2.0);
}

RC_TEST("a pool hands out storage and never goes to the heap") {
  Pool<Reading, 8> pool;
  RC_CHECK_EQ(pool.capacity(), static_cast<std::size_t>(8));
  RC_CHECK_EQ(pool.available(), static_cast<std::size_t>(8));
  RC_CHECK_EQ(pool.in_use(), static_cast<std::size_t>(0));

  AllocationCount count;

  Reading* first = pool.acquire();
  RC_REQUIRE(first != nullptr);
  RC_CHECK_EQ(pool.in_use(), static_cast<std::size_t>(1));

  Reading* second = pool.acquire();
  RC_REQUIRE(second != nullptr);
  RC_CHECK(first != second);

  RC_CHECK(pool.release(first));
  RC_CHECK_EQ(pool.in_use(), static_cast<std::size_t>(1));
  RC_CHECK(pool.release(second));
  RC_CHECK_EQ(pool.in_use(), static_cast<std::size_t>(0));

  // A million acquires and releases, and the heap was never touched.
  for (int i = 0; i < 1000000; ++i) {
    Reading* item = pool.acquire();
    if (item == nullptr) break;
    item->count = i;
    pool.release(item);
  }
  RC_CHECK(count.none());
  RC_CHECK_EQ(pool.exhaustions(), static_cast<std::size_t>(0));
}

RC_TEST("a pool that runs out says so, and counts it") {
  Pool<Reading, 4> pool;
  Reading* held[4] = {};

  for (int i = 0; i < 4; ++i) {
    held[i] = pool.acquire();
    RC_REQUIRE(held[i] != nullptr);
  }
  RC_CHECK_EQ(pool.available(), static_cast<std::size_t>(0));

  // The fifth request is refused rather than served by growing, which would
  // give up the only property the pool had.
  AllocationCount count;
  RC_CHECK(pool.acquire() == nullptr);
  RC_CHECK(pool.acquire() == nullptr);
  RC_CHECK(count.none());
  RC_CHECK_EQ(pool.exhaustions(), static_cast<std::size_t>(2));

  // Give one back and it serves again, and the count of refusals stays, because
  // it is the thing worth alarming on. A pool sized for last year's worst case
  // is the first thing a new feature outgrows.
  RC_CHECK(pool.release(held[0]));
  RC_CHECK(pool.acquire() != nullptr);
  RC_CHECK_EQ(pool.exhaustions(), static_cast<std::size_t>(2));
}

RC_TEST("a pool refuses what it did not hand out") {
  Pool<Reading, 4> pool;

  Reading* item = pool.acquire();
  RC_REQUIRE(item != nullptr);

  // Releasing the same slot twice would put it on the free list twice, and two
  // later callers would be handed the same object. The bug then appears
  // somewhere else entirely, which is what makes it expensive.
  RC_CHECK(pool.release(item));
  RC_CHECK(!pool.release(item));
  RC_CHECK_EQ(pool.double_releases(), static_cast<std::size_t>(1));

  // Something from somewhere else, and a null.
  Reading stranger;
  RC_CHECK(!pool.release(&stranger));
  RC_CHECK_EQ(pool.foreign_releases(), static_cast<std::size_t>(1));
  RC_CHECK(!pool.release(nullptr));

  // None of that disturbed the pool.
  RC_CHECK_EQ(pool.available(), static_cast<std::size_t>(4));
  for (int i = 0; i < 4; ++i) RC_CHECK(pool.acquire() != nullptr);
  RC_CHECK(pool.acquire() == nullptr);
}
