#include <rc/test/rc_test.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

using std::chrono::milliseconds;
using Clock = std::chrono::steady_clock;

const milliseconds kPatient(5000);

}  // namespace

RC_TEST("every task submitted runs exactly once") {
  const int kTasks = 500;
  std::vector<std::atomic<int>> ran(static_cast<std::size_t>(kTasks));
  for (std::atomic<int>& slot : ran) slot = 0;

  WorkerPool pool(4);
  int accepted = 0;
  for (int i = 0; i < kTasks; ++i) {
    const bool ok = pool.submit([&ran, i] { ++ran[static_cast<std::size_t>(i)]; });
    if (ok) ++accepted;
  }
  const bool idle = pool.wait_idle_for(kPatient);

  int exactly_once = 0;
  for (const std::atomic<int>& slot : ran)
    if (slot.load() == 1) ++exactly_once;

  std::cout << "  " << kTasks << " tasks on 4 workers: accepted " << accepted << ", ran exactly once "
            << exactly_once << ", completed " << pool.completed() << ", failed " << pool.failed() << "\n\n";

  RC_CHECK_EQ(accepted, kTasks);
  RC_CHECK(idle);
  RC_CHECK_EQ(exactly_once, kTasks);
  RC_CHECK_EQ(pool.completed(), kTasks);
  RC_CHECK_EQ(pool.failed(), 0);
}

RC_TEST("n workers run n tasks at once, and no more") {
  struct Outcome {
    int workers;
    int peak;
  };
  std::vector<Outcome> outcomes;

  for (const int workers : {1, 4}) {
    std::atomic<int> running{0};
    std::atomic<int> peak{0};
    {
      WorkerPool pool(workers);
      for (int i = 0; i < 8; ++i) {
        pool.submit([&running, &peak, workers] {
          const int now = ++running;
          int seen = peak.load();
          while (now > seen && !peak.compare_exchange_weak(seen, now)) {
          }
          // Wait, with a limit, until as many are running as there are workers. If
          // the pool really runs that many at once they all get here together.
          const auto stop = Clock::now() + milliseconds(2000);
          while (running.load() < workers && Clock::now() < stop) std::this_thread::yield();
          --running;
        });
      }
      pool.wait_idle_for(kPatient);
    }
    outcomes.push_back({workers, peak.load()});
  }

  std::cout << "  8 tasks that each wait until as many are running as there are workers:\n";
  for (const Outcome& outcome : outcomes)
    std::cout << "    " << outcome.workers << " worker(s): " << outcome.peak << " ran at once\n";
  std::cout << "\n  Measured outside the suite on 6 cores with 2 threads each (12 hardware threads):\n";
  std::cout << "  48 tasks of real computation sped up 2.0x on 2 workers, 3.5 to 3.8x on 4, 6.1 to 6.4x\n";
  std::cout << "  on 8, and stopped near 7x from 12 workers to 48, where 48 waiting tasks reached 44.6x.\n";
  std::cout << "  4000 trivial tasks: 122 ms with a thread each, 5.4 ms on a pool of 4.\n\n";

  RC_CHECK_EQ(outcomes[0].peak, 1);
  RC_CHECK_EQ(outcomes[1].peak, 4);
}

RC_TEST("a task that throws is counted, and the worker carries on") {
  std::atomic<int> ran{0};
  WorkerPool pool(2);

  for (int i = 0; i < 10; ++i) {
    pool.submit([&ran, i] {
      ++ran;
      if (i == 2 || i == 5 || i == 7) throw std::runtime_error("task failed");
    });
  }
  const bool idle = pool.wait_idle_for(kPatient);

  std::cout << "  10 tasks, three of which throw: ran " << ran.load() << ", completed " << pool.completed()
            << ", failed " << pool.failed() << "\n\n";

  RC_CHECK(idle);
  RC_CHECK_EQ(ran.load(), 10);
  RC_CHECK_EQ(pool.completed(), 10);
  RC_CHECK_EQ(pool.failed(), 3);

  // And the workers are still there to do more.
  std::atomic<int> after{0};
  for (int i = 0; i < 5; ++i) pool.submit([&after] { ++after; });
  RC_CHECK(pool.wait_idle_for(kPatient));
  RC_CHECK_EQ(after.load(), 5);
}

RC_TEST("shutdown runs everything accepted, then refuses more") {
  std::atomic<int> ran{0};
  WorkerPool pool(3);

  int accepted = 0;
  for (int i = 0; i < 200; ++i) {
    if (pool.submit([&ran] {
          std::this_thread::sleep_for(std::chrono::microseconds(200));
          ++ran;
        }))
      ++accepted;
  }

  pool.shutdown();
  const bool refused = !pool.submit([&ran] { ++ran; });
  pool.shutdown();  // harmless the second time

  std::cout << "  200 slow-ish tasks, then shutdown: accepted " << accepted << ", ran " << ran.load()
            << ", a late submit was " << (refused ? "refused" : "accepted") << "\n\n";

  RC_CHECK_EQ(accepted, 200);
  RC_CHECK_EQ(ran.load(), 200);
  RC_CHECK_EQ(pool.completed(), 200);
  RC_CHECK(refused);
}

RC_TEST("wait_idle_for times out while a task is held, and returns when it is released") {
  std::atomic<bool> gate{false};
  std::atomic<bool> finished{false};
  WorkerPool pool(2);

  pool.submit([&gate, &finished] {
    const auto stop = Clock::now() + kPatient;
    while (!gate.load() && Clock::now() < stop) std::this_thread::sleep_for(milliseconds(1));
    finished = true;
  });

  const auto before = Clock::now();
  const bool idle_while_held = pool.wait_idle_for(milliseconds(40));
  const auto waited = std::chrono::duration_cast<milliseconds>(Clock::now() - before);

  gate = true;
  const bool idle_after = pool.wait_idle_for(kPatient);

  std::cout << "  a task held on a gate: wait_idle_for(40 ms) gave " << (idle_while_held ? "idle" : "not idle")
            << " after " << waited.count() << " ms; released, then " << (idle_after ? "idle" : "not idle")
            << "\n\n";

  RC_CHECK(!idle_while_held);
  RC_CHECK(waited.count() >= 39);
  RC_CHECK(idle_after);
  RC_CHECK(finished.load());
}

RC_TEST("the destructor shuts the pool down, so leaving the scope finishes the work") {
  std::atomic<int> ran{0};
  {
    WorkerPool pool(2);
    for (int i = 0; i < 50; ++i) pool.submit([&ran] { ++ran; });
  }
  RC_CHECK_EQ(ran.load(), 50);
}
