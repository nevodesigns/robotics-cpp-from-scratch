#include <rc/test/rc_test.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

// Four fields that are always equal, so a read that mixes two writes can be seen.
struct Pose {
  double a;
  double b;
  double c;
  double d;
};

Pose pose_of(double x) { return Pose{x, x, x, x}; }
bool whole(const Pose& p) { return p.a == p.b && p.b == p.c && p.c == p.d; }

}  // namespace

RC_TEST("a value written is read back whole, and an unwritten one reads as zero") {
  Seqlock<Pose> cell;
  const Pose empty = cell.read();
  RC_CHECK_NEAR(empty.a, 0.0, 1e-12);
  RC_CHECK(whole(empty));

  cell.write(pose_of(3.5));
  const Pose got = cell.read();
  RC_CHECK_NEAR(got.a, 3.5, 1e-12);
  RC_CHECK(whole(got));

  cell.write(pose_of(-8.0));
  RC_CHECK_NEAR(cell.read().d, -8.0, 1e-12);
  RC_CHECK_EQ(cell.retries(), 0L);
}

RC_TEST("any size of value round-trips, including ones that are not a whole number of words") {
  struct OneByte { unsigned char value; };
  struct ThreeBytes { unsigned char value[3]; };
  struct Big { double values[13]; };

  Seqlock<OneByte> one;
  one.write(OneByte{200});
  RC_CHECK_EQ(static_cast<int>(one.read().value), 200);

  Seqlock<ThreeBytes> three;
  three.write(ThreeBytes{{7, 8, 9}});
  const ThreeBytes t = three.read();
  RC_CHECK_EQ(static_cast<int>(t.value[0]), 7);
  RC_CHECK_EQ(static_cast<int>(t.value[1]), 8);
  RC_CHECK_EQ(static_cast<int>(t.value[2]), 9);

  Seqlock<Big> big;
  Big in{};
  for (int i = 0; i < 13; ++i) in.values[i] = i * 1.5;
  big.write(in);
  const Big out = big.read();
  for (int i = 0; i < 13; ++i) RC_CHECK_NEAR(out.values[i], i * 1.5, 1e-12);
}

RC_TEST("a read overtaken by a write throws its copy away and returns the new value") {
  // Forced, on one thread: between reading the data and checking the counter, a write
  // lands. The hook runs inside read(), at the point where the reader has a copy that
  // is now stale.
  Seqlock<Pose> cell;
  cell.write(pose_of(1.0));

  bool fired = false;
  Pose got{};
  {
    const rc::test::RaceHook hook("seqlock.read.middle", [&] {
      if (!fired) {
        fired = true;
        cell.write(pose_of(2.0));
      }
    });
    got = cell.read();
  }

  std::cout << "  a write landed in the middle of a read:\n";
  std::cout << "    hook fired      " << (fired ? "yes" : "no") << "\n";
  std::cout << "    value returned  " << got.a << " (the old one was 1, the new one is 2)\n";
  std::cout << "    retries         " << cell.retries() << "\n\n";

  RC_CHECK(fired);
  RC_CHECK_NEAR(got.a, 2.0, 1e-12);
  RC_CHECK(whole(got));
  RC_CHECK_EQ(cell.retries(), 1L);
}

RC_TEST("a reader that meets a write in progress waits for it, and does not return half of it") {
  // Two threads, with the writer stopped halfway: the counter is odd and the data is
  // written. A reader arriving now must not return it. The writer waits, with a limit,
  // until the reader has retried at least once, then finishes.
  Seqlock<Pose> cell;
  cell.write(pose_of(1.0));

  std::atomic<bool> writer_is_halfway{false};
  std::atomic<bool> hook_ran{false};
  Pose seen{};

  {
    const rc::test::RaceHook hook("seqlock.write.middle", [&] {
      hook_ran = true;
      writer_is_halfway = true;
      const auto stop = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
      while (cell.retries() < 1 && std::chrono::steady_clock::now() < stop) std::this_thread::yield();
    });

    std::thread writer([&] { cell.write(pose_of(9.0)); });
    std::thread reader([&] {
      const auto stop = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
      while (!writer_is_halfway.load() && std::chrono::steady_clock::now() < stop) std::this_thread::yield();
      seen = cell.read();
    });
    writer.join();
    reader.join();
  }

  std::cout << "  a reader arrived while a write was halfway done:\n";
  std::cout << "    it retried " << cell.retries() << " time(s) and returned " << seen.a << ", whole: "
            << (whole(seen) ? "yes" : "no") << "\n\n";

  RC_CHECK(hook_ran.load());
  RC_CHECK(cell.retries() >= 1);
  RC_CHECK(whole(seen));
  RC_CHECK_NEAR(seen.a, 9.0, 1e-12);
}

RC_TEST("readers running against a writer never see a mixed value, and see the values advance") {
  Seqlock<Pose> cell;
  cell.write(pose_of(0.0));

  const int kWrites = 50000;
  std::atomic<bool> done{false};
  std::atomic<long> torn{0};
  std::atomic<long> reads{0};
  std::atomic<long> went_backwards{0};
  std::atomic<long> distinct{0};

  std::vector<std::thread> readers;
  for (int r = 0; r < 3; ++r) {
    readers.emplace_back([&] {
      double last = -1.0;
      long mine = 0;
      long changes = 0;
      while (!done.load()) {
        const Pose p = cell.read();
        ++mine;
        if (!whole(p)) ++torn;
        if (p.a < last) ++went_backwards;
        if (p.a != last) ++changes;
        last = p.a;
      }
      reads += mine;
      distinct += changes;
    });
  }

  std::thread writer([&] {
    for (int i = 1; i <= kWrites; ++i) cell.write(pose_of(static_cast<double>(i)));
    done = true;
  });
  writer.join();
  for (std::thread& reader : readers) reader.join();

  const Pose final_value = cell.read();
  std::cout << "  " << kWrites << " writes against 3 readers: " << reads.load() << " reads, " << torn.load()
            << " mixed, " << went_backwards.load() << " went backwards, " << cell.retries() << " retries\n\n";

  RC_CHECK_EQ(torn.load(), 0L);
  RC_CHECK_EQ(went_backwards.load(), 0L);
  RC_CHECK(distinct.load() > 3);
  RC_CHECK_NEAR(final_value.a, static_cast<double>(kWrites), 1e-12);
}
