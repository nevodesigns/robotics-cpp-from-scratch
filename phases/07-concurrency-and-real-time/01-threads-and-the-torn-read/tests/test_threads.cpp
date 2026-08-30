#include <rc/test/rc_test.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

// The readings written by the producer have a fixed relationship between their
// fields. Any reader that sees the relationship broken has assembled a value
// from two different writes, which is a torn read.
Reading consistent(double seed) { return Reading{seed, seed * 2.0, seed * 3.0}; }

bool is_consistent(const Reading& r) {
  return r.y == r.x * 2.0 && r.theta == r.x * 3.0;
}

}  // namespace

RC_TEST("a published reading can be read back") {
  LatestReading shared;
  shared.publish(consistent(1.5));

  const Reading got = shared.latest();
  RC_CHECK_NEAR(got.x, 1.5, 1e-12);
  RC_CHECK_NEAR(got.y, 3.0, 1e-12);
  RC_CHECK_NEAR(got.theta, 4.5, 1e-12);
}

RC_TEST("latest means the most recent") {
  LatestReading shared;
  shared.publish(consistent(1.0));
  shared.publish(consistent(2.0));
  RC_CHECK_NEAR(shared.latest().x, 2.0, 1e-12);
}

RC_TEST("publications are counted") {
  LatestReading shared;
  for (int i = 0; i < 10; ++i) shared.publish(consistent(i));
  RC_CHECK_EQ(shared.count(), 10L);
}

RC_TEST("a reading is never seen half written") {
  // The test that carries the lesson. One thread writes readings whose fields
  // have a fixed relationship, another checks the relationship on every read.
  // Without synchronisation the reader assembles values from two different
  // writes, and the relationship breaks.
  LatestReading shared;
  shared.publish(consistent(1.0));

  std::atomic<bool> running{true};
  std::atomic<long> torn{0};
  std::atomic<long> reads{0};

  std::thread writer([&shared, &running] {
    double seed = 1.0;
    while (running.load()) {
      shared.publish(consistent(seed));
      seed += 1.0;
    }
  });

  std::thread reader([&shared, &running, &torn, &reads] {
    while (running.load()) {
      if (!is_consistent(shared.latest())) torn.fetch_add(1);
      reads.fetch_add(1);
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  running.store(false);
  writer.join();
  reader.join();

  RC_CHECK(reads.load() > 1000);       // the test actually exercised something
  RC_CHECK_EQ(torn.load(), 0L);
}

RC_TEST("many writers and many readers still never tear") {
  LatestReading shared;
  shared.publish(consistent(1.0));

  std::atomic<bool> running{true};
  std::atomic<long> torn{0};
  std::vector<std::thread> threads;

  for (int i = 0; i < 3; ++i) {
    threads.emplace_back([&shared, &running, i] {
      double seed = 1.0 + i;
      while (running.load()) {
        shared.publish(consistent(seed));
        seed += 3.0;
      }
    });
  }
  for (int i = 0; i < 3; ++i) {
    threads.emplace_back([&shared, &running, &torn] {
      while (running.load()) {
        if (!is_consistent(shared.latest())) torn.fetch_add(1);
      }
    });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  running.store(false);
  for (std::thread& t : threads) t.join();

  RC_CHECK_EQ(torn.load(), 0L);
}

RC_TEST("every publication is counted under contention") {
  // A correctness check on the finished implementation, not a trap. Measured on
  // this machine, the unsynchronised version passes it every time: the lost
  // update it is meant to describe does not reproduce here. That is the point
  // made in the lesson about why testing cannot find races.
  LatestReading shared;
  std::vector<std::thread> writers;
  constexpr long kEach = 20000;

  for (int i = 0; i < 4; ++i) {
    writers.emplace_back([&shared] {
      for (long n = 0; n < kEach; ++n) shared.publish(consistent(1.0));
    });
  }
  for (std::thread& t : writers) t.join();

  RC_CHECK_EQ(shared.count(), 4 * kEach);
}

RC_TEST("a stop flag starts unset") {
  const StopFlag flag;
  RC_CHECK(!flag.stopped());
}

RC_TEST("a stop flag can be set") {
  StopFlag flag;
  flag.request_stop();
  RC_CHECK(flag.stopped());
}

RC_TEST("a worker thread notices the stop flag") {
  // A plain bool here is a data race, and the compiler is entitled to keep it
  // in a register so the loop never sees the change. It is entitled to, and on
  // this machine it does not: the unsynchronised version passes this every
  // time. The race is real and the test cannot see it, which is why the thread
  // sanitizer is not optional for threaded code.
  StopFlag flag;
  std::atomic<long> spins{0};

  std::thread worker([&flag, &spins] {
    while (!flag.stopped()) spins.fetch_add(1);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  flag.request_stop();
  worker.join();

  RC_CHECK(spins.load() > 0);
  RC_CHECK(flag.stopped());
}
