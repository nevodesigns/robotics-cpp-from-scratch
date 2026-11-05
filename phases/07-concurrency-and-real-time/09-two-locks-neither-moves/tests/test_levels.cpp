#include <rc/test/rc_test.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "solution.hpp"

namespace {

using std::chrono::milliseconds;
using Clock = std::chrono::steady_clock;

// Waits for a count to reach a target, but not for ever: a handshake that depends
// on a thread that never arrives would hang the suite, and a half finished lock
// class must fail rather than hang.
void wait_for_count(const std::atomic<int>& counter, int target) {
  const auto stop = Clock::now() + milliseconds(1000);
  while (counter.load() < target && Clock::now() < stop) std::this_thread::yield();
}

int violations_now() { return order_violations().load(); }

// Takes a lock, giving up after a limit. This is try_lock polled against a deadline
// and not std::timed_mutex::try_lock_for, on purpose: try_lock_for calls
// pthread_mutex_clocklock, which neither gcc 11's nor clang 14's thread sanitizer
// intercepts, so under either of them a lock that is successfully taken is reported
// as "unlock of an unlocked mutex". Measured: 15 of 15 runs. The program is correct
// and the tool cannot see it. A plain try_lock is seen by both.
bool lock_within(std::mutex& mutex, milliseconds limit) {
  const auto stop = Clock::now() + limit;
  while (!mutex.try_lock()) {
    if (Clock::now() > stop) return false;
    std::this_thread::yield();
  }
  return true;
}

}  // namespace

RC_TEST("locks in increasing level are taken, and one at or below a held level is refused") {
  LevelMutex drive(1);
  LevelMutex arm(2);

  const int before = violations_now();
  {
    const OrderedLock first(drive);
    const OrderedLock second(arm);
    RC_CHECK(first.ok());
    RC_CHECK(second.ok());
  }
  RC_CHECK_EQ(violations_now(), before);

  // The other way round: arm first, then drive. The second is refused and taken by
  // nobody, so it cannot block and there is nothing to release.
  {
    const OrderedLock first(arm);
    const OrderedLock second(drive);
    RC_CHECK(first.ok());
    RC_CHECK(!second.ok());
  }
  RC_CHECK_EQ(violations_now(), before + 1);

  // The refused lock was never taken, so another thread can take it straight away.
  std::atomic<int> took{0};
  std::thread other([&] {
    const OrderedLock again(drive);
    if (again.ok()) ++took;
  });
  other.join();
  RC_CHECK_EQ(took.load(), 1);
}

RC_TEST("the same level twice is refused, which is a thread deadlocking with itself") {
  LevelMutex gripper(5);

  const int before = violations_now();
  {
    const OrderedLock held(gripper);
    // A plain std::mutex locked twice by one thread is undefined behaviour and in
    // practice a hang. Here it is a refusal.
    const OrderedLock again(gripper);
    RC_CHECK(held.ok());
    RC_CHECK(!again.ok());
  }
  RC_CHECK_EQ(violations_now(), before + 1);

  // A different mutex at the same level is refused for the same reason: the order
  // between them is not defined, and an order that is not defined is not safe.
  LevelMutex sibling(5);
  {
    const OrderedLock one(gripper);
    const OrderedLock two(sibling);
    RC_CHECK(one.ok());
    RC_CHECK(!two.ok());
  }
}

RC_TEST("a wrong order is found even when nothing ever deadlocked") {
  LevelMutex drive(1);
  LevelMutex arm(2);
  const int before = violations_now();

  // Thread one takes drive then arm. It finishes before thread two starts, so the
  // two never overlap, and no interleaving could ever have hung them.
  std::thread one([&] {
    const OrderedLock first(drive);
    const OrderedLock second(arm);
  });
  one.join();
  const int after_first = violations_now();

  // Thread two takes arm then drive. The order is the bug, whether or not the
  // timing ever lines up.
  std::atomic<bool> refused{false};
  std::thread two([&] {
    const OrderedLock first(arm);
    const OrderedLock second(drive);
    refused = !second.ok();
  });
  two.join();

  std::cout << "  two threads, one after the other, never running together:\n";
  std::cout << "    drive then arm   violations " << (after_first - before) << "\n";
  std::cout << "    arm then drive   violations " << (violations_now() - after_first) << "\n";
  std::cout << "  Nothing deadlocked and nothing could have. The cycle was found in the order.\n\n";

  RC_CHECK_EQ(after_first, before);
  RC_CHECK(refused.load());
  RC_CHECK_EQ(violations_now(), before + 1);
}

RC_TEST("releasing a lock puts its level back, in any order") {
  LevelMutex drive(1);
  LevelMutex arm(2);
  LevelMutex gripper(3);
  const int before = violations_now();

  // Hold drive and arm, release the lower one first, which is allowed.
  RC_REQUIRE(drive.lock());
  RC_REQUIRE(arm.lock());
  drive.unlock();

  // drive is released, so only arm is held, and gripper is above it.
  RC_CHECK(gripper.lock());
  gripper.unlock();
  arm.unlock();

  // Nothing held: any lock may be taken first.
  RC_CHECK(arm.lock());
  arm.unlock();
  RC_CHECK(drive.lock());
  drive.unlock();

  RC_CHECK_EQ(violations_now(), before);
}

RC_TEST("opposite orders deadlock for real, scoped_lock does not, and the level check never gets there") {
  // The real thing, with a timeout so that it can end. Each thread takes its first
  // lock, waits until the other has, then asks for the second.
  {
    std::mutex a;
    std::mutex b;
    std::atomic<int> holding{0};
    std::atomic<int> gave_up{0};
    auto grab = [&](std::mutex& first, std::mutex& second) {
      first.lock();
      ++holding;
      wait_for_count(holding, 2);
      if (lock_within(second, milliseconds(200)))
        second.unlock();
      else
        ++gave_up;
      first.unlock();
    };
    std::thread one(grab, std::ref(a), std::ref(b));
    std::thread two(grab, std::ref(b), std::ref(a));
    one.join();
    two.join();

    std::cout << "  opposite order, interleaving forced: " << gave_up.load()
              << " of 2 threads gave up waiting\n";
    std::cout << "  (1 or 2: when the first gives up and lets go, the other gets through)\n\n";
    RC_CHECK(gave_up.load() >= 1);
  }

  // scoped_lock takes both at once, in an order it chooses, so the order the caller
  // wrote does not matter.
  {
    std::mutex a;
    std::mutex b;
    std::atomic<int> finished{0};
    std::thread one([&] {
      for (int i = 0; i < 500; ++i) {
        const std::scoped_lock both(a, b);
      }
      ++finished;
    });
    std::thread two([&] {
      for (int i = 0; i < 500; ++i) {
        const std::scoped_lock both(b, a);
      }
      ++finished;
    });
    one.join();
    two.join();
    RC_CHECK_EQ(finished.load(), 2);
  }

  // The level check with the same opposite orders and the same forced overlap. The
  // wrong order is refused the moment it is tried, so nothing waits and nothing hangs.
  {
    LevelMutex drive(1);
    LevelMutex arm(2);
    std::atomic<int> holding{0};
    std::atomic<int> finished{0};
    const int before = violations_now();

    std::thread one([&] {
      const OrderedLock first(drive);
      ++holding;
      wait_for_count(holding, 2);
      const OrderedLock second(arm);
      ++finished;
    });
    std::thread two([&] {
      const OrderedLock first(arm);
      ++holding;
      wait_for_count(holding, 2);
      const OrderedLock second(drive);
      ++finished;
    });
    one.join();
    two.join();

    std::cout << "  the same opposite orders with levels: both threads finished (" << finished.load()
              << " of 2), " << (violations_now() - before) << " order violation(s) reported\n\n";
    RC_CHECK_EQ(finished.load(), 2);
    RC_CHECK_EQ(violations_now(), before + 1);
  }
}
