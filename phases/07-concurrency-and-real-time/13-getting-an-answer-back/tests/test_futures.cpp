#include <rc/test/rc_test.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

using std::chrono::milliseconds;
using Clock = std::chrono::steady_clock;

const milliseconds kPatient(5000);

struct Gauge {
  std::atomic<int> running{0};
  std::atomic<int> peak{0};
};

// A task that waits, with a limit, until four are running at once. If four can run
// together they all get here together, and peak is 4. If they are made to run one after
// another, peak stays 1 and each one waits out its limit.
void rendezvous_of_four(Gauge& gauge, milliseconds limit) {
  const int now = ++gauge.running;
  int seen = gauge.peak.load();
  while (now > seen && !gauge.peak.compare_exchange_weak(seen, now)) {
  }
  const auto stop = Clock::now() + limit;
  while (gauge.running.load() < 4 && Clock::now() < stop) std::this_thread::yield();
  --gauge.running;
}

}  // namespace

RC_TEST("a result comes back, whatever its type, including one that cannot be copied") {
  WorkerPool pool(2);

  std::future<int> number = submit_future(pool, [] { return 6 * 7; });
  std::future<std::string> text = submit_future(pool, [] { return std::string("done"); });
  std::future<std::unique_ptr<int>> owned = submit_future(pool, [] { return std::make_unique<int>(99); });
  std::future<void> nothing = submit_future(pool, [] {});

  RC_REQUIRE(ready_within(number, kPatient));
  RC_CHECK_EQ(number.get(), 42);
  RC_REQUIRE(ready_within(text, kPatient));
  RC_CHECK_EQ(text.get(), std::string("done"));
  RC_REQUIRE(ready_within(owned, kPatient));
  const std::unique_ptr<int> pointer = owned.get();
  RC_REQUIRE(pointer != nullptr);
  RC_CHECK_EQ(*pointer, 99);
  RC_REQUIRE(ready_within(nothing, kPatient));
  nothing.get();
}

RC_TEST("an exception in the task comes out of get(), and the pool is not told") {
  WorkerPool pool(2);
  std::future<int> failing =
      submit_future(pool, []() -> int { throw std::runtime_error("sensor lost"); });
  std::future<int> fine = submit_future(pool, [] { return 1; });

  RC_REQUIRE(ready_within(failing, kPatient));
  std::string message;
  try {
    failing.get();
  } catch (const std::runtime_error& e) {
    message = e.what();
  }
  RC_REQUIRE(ready_within(fine, kPatient));
  const int other = fine.get();
  const bool idle = pool.wait_idle_for(kPatient);

  std::cout << "  a task that threw runtime_error(\"sensor lost\"):\n";
  std::cout << "    get() threw           \"" << message << "\"\n";
  std::cout << "    pool completed        " << pool.completed() << "\n";
  std::cout << "    pool failed           " << pool.failed()
            << "   (the future caught it and kept it for the caller)\n\n";

  RC_CHECK_EQ(message, std::string("sensor lost"));
  RC_CHECK_EQ(other, 1);
  RC_CHECK(idle);
  RC_CHECK_EQ(pool.completed(), 2);
  RC_CHECK_EQ(pool.failed(), 0);
}

RC_TEST("a task the pool refuses surfaces as broken_promise, not a wait for ever") {
  WorkerPool pool(2);
  pool.shutdown();

  std::future<int> refused = submit_future(pool, [] { return 1; });
  RC_REQUIRE(ready_within(refused, kPatient));

  bool broken = false;
  std::string what;
  try {
    refused.get();
  } catch (const std::future_error& e) {
    broken = e.code() == std::future_errc::broken_promise;
    what = e.what();
  }

  std::cout << "  a task submitted after shutdown: get() threw future_error \"" << what << "\"\n\n";
  RC_CHECK(broken);
}

RC_TEST("many results arrive, and a pool of four runs four tasks at once") {
  WorkerPool pool(4);
  Gauge gauge;

  std::vector<std::future<int>> results;
  for (int i = 0; i < 8; ++i) {
    results.push_back(submit_future(pool, [&gauge, i] {
      rendezvous_of_four(gauge, milliseconds(2000));
      return i * i;
    }));
  }

  long sum = 0;
  for (std::future<int>& result : results) {
    RC_REQUIRE(ready_within(result, kPatient));
    sum += result.get();
  }

  std::cout << "  8 tasks on a pool of 4, results kept: peak running at once " << gauge.peak.load() << ", sum "
            << sum << "\n\n";
  RC_CHECK_EQ(sum, 140L);
  RC_CHECK_EQ(gauge.peak.load(), 4);
}

RC_TEST("std::async with the futures discarded runs one after another, and kept it does not") {
  const milliseconds kLimit(100);

  Gauge discarded;
  const auto began_discarded = Clock::now();
  for (int i = 0; i < 4; ++i) {
    // static_cast<void> says the discard is on purpose, and keeps the build free of the
    // nodiscard warning that both gcc 11 and clang 14 give for exactly this mistake.
    static_cast<void>(std::async(std::launch::async, [&discarded, kLimit] { rendezvous_of_four(discarded, kLimit); }));
  }
  const auto took_discarded =
      std::chrono::duration_cast<milliseconds>(Clock::now() - began_discarded);

  Gauge kept;
  const auto began_kept = Clock::now();
  {
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 4; ++i)
      futures.push_back(std::async(std::launch::async, [&kept, kLimit] { rendezvous_of_four(kept, kLimit); }));
  }
  const auto took_kept = std::chrono::duration_cast<milliseconds>(Clock::now() - began_kept);

  // The same four tasks through the learner's submit_future on a pool of four.
  Gauge pooled;
  const auto began_pooled = Clock::now();
  {
    WorkerPool pool(4);
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 4; ++i)
      futures.push_back(submit_future(pool, [&pooled, kLimit] { rendezvous_of_four(pooled, kLimit); }));
    for (std::future<void>& future : futures) ready_within(future, kPatient);
  }
  const auto took_pooled = std::chrono::duration_cast<milliseconds>(Clock::now() - began_pooled);

  std::cout << "  four tasks that each wait up to 100 ms for the others:\n";
  std::cout << "                        peak running at once   took\n";
  std::cout << "    futures discarded         " << discarded.peak.load() << "               " << took_discarded.count()
            << " ms\n";
  std::cout << "    futures kept              " << kept.peak.load() << "               " << took_kept.count()
            << " ms\n";
  std::cout << "    submit_future, kept       " << pooled.peak.load() << "               " << took_pooled.count()
            << " ms\n\n";

  RC_CHECK_EQ(discarded.peak.load(), 1);
  RC_CHECK_EQ(pooled.peak.load(), 4);
  RC_CHECK_EQ(kept.peak.load(), 4);
}

RC_TEST("ready_within gives up at its limit and answers yes once the task is done") {
  WorkerPool pool(1);
  std::atomic<bool> gate{false};

  std::future<int> held = submit_future(pool, [&gate] {
    const auto stop = Clock::now() + kPatient;
    while (!gate.load() && Clock::now() < stop) std::this_thread::sleep_for(milliseconds(1));
    return 5;
  });

  const auto before = Clock::now();
  const bool early = ready_within(held, milliseconds(40));
  const auto waited = std::chrono::duration_cast<milliseconds>(Clock::now() - before);

  gate = true;
  const bool late = ready_within(held, kPatient);

  RC_CHECK(!early);
  RC_CHECK(waited.count() >= 39);
  RC_CHECK(late);
  RC_CHECK_EQ(held.get(), 5);
}
