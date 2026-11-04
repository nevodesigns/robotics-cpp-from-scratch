#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

#include <rc/core/clock.hpp>

#include "solution.hpp"

namespace {

using rc::core::Nanoseconds;
using rc::core::TestClock;

// One microsecond, written once.
constexpr Nanoseconds kMicrosecond = 1000;

// A clock that counts how many times it was asked, so the cost of the
// instrument can be reported as a number rather than as a feeling.
class CountingClock : public rc::core::Clock {
 public:
  explicit CountingClock(TestClock& inner) : inner_(&inner) {}

  Nanoseconds now() const override {
    ++reads;
    return inner_->now();
  }

  mutable int reads = 0;

 private:
  TestClock* inner_;
};

double microseconds(Nanoseconds value) { return static_cast<double>(value) / 1000.0; }

}  // namespace

RC_TEST("a section is attributed exactly the time that passed inside it") {
  TestClock clock;
  SectionTimer timer(clock);

  {
    const auto guard = timer.measure("read sensors");
    clock.advance(250 * kMicrosecond);
  }

  RC_REQUIRE(timer.size() == 1u);
  const Section& read = timer[0];
  RC_CHECK(read.calls == 1);
  RC_CHECK(read.total == 250 * kMicrosecond);
  RC_CHECK(read.worst == 250 * kMicrosecond);
  RC_CHECK(timer.accounted_for() == 250 * kMicrosecond);

  // Nothing is statistical here. The clock advanced by exactly that and the
  // section recorded exactly that, on every machine and every toolchain, which
  // is what an injectable clock buys a test.
  std::cout << "\n  one section, " << microseconds(read.total)
            << " microseconds, asserted exactly rather than within a tolerance\n\n";
}

RC_TEST("a whole iteration, broken down") {
  TestClock clock;
  SectionTimer timer(clock);

  // A control loop with four named parts and a gap nobody named.
  {
    const auto whole = timer.measure("whole iteration");

    {
      const auto guard = timer.measure("read sensors");
      clock.advance(250 * kMicrosecond);
    }
    {
      const auto guard = timer.measure("estimate");
      clock.advance(400 * kMicrosecond);
    }
    clock.advance(120 * kMicrosecond);     // the part nobody named
    {
      const auto guard = timer.measure("control");
      clock.advance(80 * kMicrosecond);
    }
    {
      const auto guard = timer.measure("write motors");
      clock.advance(150 * kMicrosecond);
    }
  }

  const Section* whole = timer.find("whole iteration");
  RC_REQUIRE(whole != nullptr);

  std::cout << "  " << std::left << std::setw(20) << "section" << std::right
            << std::setw(10) << "calls" << std::setw(14) << "total us"
            << std::setw(12) << "share" << "\n";
  for (const Section& section : timer) {
    if (section.name == whole->name) continue;
    std::cout << "  " << std::left << std::setw(20) << section.name << std::right
              << std::setw(10) << section.calls << std::setw(14) << std::fixed
              << std::setprecision(1) << microseconds(section.total) << std::setw(11)
              << (100.0 * static_cast<double>(section.total) /
                  static_cast<double>(whole->total))
              << "%\n";
  }

  // The four named parts, each exactly as the clock was advanced.
  RC_CHECK(timer.find("read sensors")->total == 250 * kMicrosecond);
  RC_CHECK(timer.find("estimate")->total == 400 * kMicrosecond);
  RC_CHECK(timer.find("control")->total == 80 * kMicrosecond);
  RC_CHECK(timer.find("write motors")->total == 150 * kMicrosecond);
  RC_CHECK(whole->total == 1000 * kMicrosecond);

  // And the part nobody named, which is the point of the lesson.
  const Nanoseconds named = 250 * kMicrosecond + 400 * kMicrosecond +
                            80 * kMicrosecond + 150 * kMicrosecond;
  const Nanoseconds missing = whole->total - named;

  std::cout << "\n  the four named parts come to " << microseconds(named)
            << " microseconds of a " << microseconds(whole->total)
            << " microsecond loop\n";
  std::cout << "  " << microseconds(missing)
            << " microseconds, " << std::setprecision(0)
            << (100.0 * static_cast<double>(missing) /
                static_cast<double>(whole->total))
            << "%, went somewhere nobody named\n\n" << std::setprecision(1);

  RC_CHECK(missing == 120 * kMicrosecond);
  RC_CHECK(missing > 0);
}

RC_TEST("nesting counts the inner time twice, and says so by going negative") {
  TestClock clock;
  SectionTimer timer(clock);

  {
    const auto outer = timer.measure("outer");
    clock.advance(100 * kMicrosecond);
    {
      const auto inner = timer.measure("inner");
      clock.advance(500 * kMicrosecond);
    }
    clock.advance(100 * kMicrosecond);
  }

  const Nanoseconds loop = 700 * kMicrosecond;

  std::cout << "  outer " << microseconds(timer.find("outer")->total)
            << " us, inner " << microseconds(timer.find("inner")->total)
            << " us, loop " << microseconds(loop) << " us\n";
  std::cout << "  accounted for " << microseconds(timer.accounted_for())
            << " us, which is more than the loop took\n";
  std::cout << "  unaccounted_for reports "
            << microseconds(timer.unaccounted_for(loop)) << " us\n\n";

  // The outer section measured wall time, so it includes the inner one.
  RC_CHECK(timer.find("outer")->total == 700 * kMicrosecond);
  RC_CHECK(timer.find("inner")->total == 500 * kMicrosecond);
  RC_CHECK(timer.accounted_for() == 1200 * kMicrosecond);

  // So the remainder is negative, by exactly the nested time. It is not
  // clamped to zero, because a negative answer is the only signal that the
  // sections overlap, and a percentage computed from overlapping sections adds
  // up to more than a hundred.
  RC_CHECK(timer.unaccounted_for(loop) == -500 * kMicrosecond);
  RC_CHECK(timer.unaccounted_for(loop) < 0);
}

RC_TEST("calls accumulate across iterations and the worst one is kept") {
  TestClock clock;
  SectionTimer timer(clock);

  const Nanoseconds durations[] = {100 * kMicrosecond, 180 * kMicrosecond,
                                   120 * kMicrosecond, 900 * kMicrosecond,
                                   110 * kMicrosecond};
  for (Nanoseconds duration : durations) {
    const auto guard = timer.measure("estimate");
    clock.advance(duration);
  }

  const Section* estimate = timer.find("estimate");
  RC_REQUIRE(estimate != nullptr);
  RC_CHECK(estimate->calls == 5);
  RC_CHECK(estimate->total == 1410 * kMicrosecond);
  RC_CHECK(estimate->worst == 900 * kMicrosecond);

  const double mean = static_cast<double>(estimate->total) /
                      static_cast<double>(estimate->calls);
  std::cout << "  five iterations: mean " << microseconds(static_cast<Nanoseconds>(mean))
            << " us, worst " << microseconds(estimate->worst) << " us\n";
  std::cout << "  a mean of 282 hides a single pass that took 900, which is why\n";
  std::cout << "  the worst is kept alongside it\n\n";

  // The worst is the number that misses a deadline. A mean under budget with a
  // worst over it is a loop that fails occasionally, which phase 05 is about.
  RC_CHECK(estimate->worst > static_cast<Nanoseconds>(mean) * 3);

  // Resetting clears the counts and leaves the timer usable.
  timer.reset();
  RC_CHECK(timer.size() == 0u);
  RC_CHECK(timer.accounted_for() == 0);
  {
    const auto guard = timer.measure("estimate");
    clock.advance(50 * kMicrosecond);
  }
  RC_REQUIRE(timer.find("estimate") != nullptr);
  RC_CHECK(timer.find("estimate")->calls == 1);
}

RC_TEST("the guard closes the section on an early return") {
  TestClock clock;
  SectionTimer timer(clock);

  // A function that gives up halfway, which is what a begin and end pair gets
  // wrong: the end never runs and the section stays open.
  const auto gives_up = [&timer, &clock](bool bail) {
    const auto guard = timer.measure("read sensors");
    clock.advance(40 * kMicrosecond);
    if (bail) return false;
    clock.advance(60 * kMicrosecond);
    return true;
  };

  RC_CHECK(!gives_up(true));
  RC_CHECK(timer.find("read sensors")->calls == 1);
  RC_CHECK(timer.find("read sensors")->total == 40 * kMicrosecond);

  RC_CHECK(gives_up(false));
  RC_CHECK(timer.find("read sensors")->calls == 2);
  RC_CHECK(timer.find("read sensors")->total == 140 * kMicrosecond);

  // The next section starts clean rather than inheriting an open one's start
  // time, which is the failure a pair of calls produces.
  {
    const auto guard = timer.measure("control");
    clock.advance(10 * kMicrosecond);
  }
  RC_CHECK(timer.find("control")->total == 10 * kMicrosecond);
}

RC_TEST("the instrument costs exactly two clock reads a section") {
  TestClock inner;
  CountingClock counted(inner);
  SectionTimer timer(counted);

  {
    const auto guard = timer.measure("read sensors");
    inner.advance(10 * kMicrosecond);
  }
  RC_CHECK(counted.reads == 2);

  counted.reads = 0;
  for (int i = 0; i < 10; ++i) {
    const auto guard = timer.measure("estimate");
    inner.advance(kMicrosecond);
  }
  RC_CHECK(counted.reads == 20);

  // Which makes the cost arithmetic rather than opinion. A loop at 500 Hz with
  // ten sections asks the clock ten thousand times a second, and whether that
  // matters is a question about your clock rather than about this code.
  const long long per_second = 500LL * 10 * 2;
  std::cout << "  two reads per section, so a 500 Hz loop with ten sections asks\n";
  std::cout << "  the clock " << per_second << " times a second\n\n";
  RC_CHECK(per_second == 10000);
}

RC_TEST("the report is a container, so it sorts like one") {
  TestClock clock;
  SectionTimer timer(clock);

  {
    const auto guard = timer.measure("read sensors");
    clock.advance(250 * kMicrosecond);
  }
  {
    const auto guard = timer.measure("estimate");
    clock.advance(400 * kMicrosecond);
  }
  {
    const auto guard = timer.measure("control");
    clock.advance(80 * kMicrosecond);
  }

  // Walked with range-for, because the timer carries value_type, begin and end
  // from lesson 03-08.
  int walked = 0;
  for (const Section& section : timer) {
    RC_CHECK(section.calls == 1);
    ++walked;
  }
  RC_CHECK(walked == 3);

  // And sorted into a copy, slowest first, which is the order anybody reading
  // a report wants. A copy, because the report itself is read only: the
  // iterator hands out const references, which lesson 03-08 kept in checks/.
  std::vector<Section> ranked(timer.begin(), timer.end());
  std::sort(ranked.begin(), ranked.end(),
            [](const Section& a, const Section& b) { return a.total > b.total; });

  RC_REQUIRE(ranked.size() == 3u);
  RC_CHECK(ranked[0].total == 400 * kMicrosecond);
  RC_CHECK(ranked[2].total == 80 * kMicrosecond);

  std::cout << "  slowest first: ";
  for (const Section& section : ranked) std::cout << section.name << " ";
  std::cout << "\n\n";
}
