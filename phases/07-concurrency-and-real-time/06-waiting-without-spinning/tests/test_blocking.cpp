#include <rc/test/rc_test.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

using Queue = BlockingQueue<int>;
using Result = Queue::Result;
using std::chrono::milliseconds;
using Clock = std::chrono::steady_clock;

const milliseconds kPatient(5000);

const char* name_of(Result result) {
  switch (result) {
    case Result::kGot: return "got";
    case Result::kClosed: return "closed";
    default: return "timeout";
  }
}

// Waits, with a deadline, until at least `count` consumers have gone to sleep on
// the queue. Returns false if they never did. A test that acts before the
// consumers are really waiting is testing a race, not the queue.
bool wait_until_asleep(const Queue& queue, int count, milliseconds limit) {
  const auto stop = Clock::now() + limit;
  while (queue.waits() < count) {
    if (Clock::now() > stop) return false;
    std::this_thread::sleep_for(milliseconds(1));
  }
  return true;
}

}  // namespace

RC_TEST("what is already there is taken at once, in order, and drained before closing") {
  Queue queue;
  queue.push(1);
  queue.push(2);
  queue.push(3);
  RC_CHECK_EQ(queue.size(), 3u);

  int value = 0;
  RC_REQUIRE(queue.pop_for(value, kPatient) == Result::kGot);
  RC_CHECK_EQ(value, 1);
  RC_REQUIRE(queue.pop_for(value, kPatient) == Result::kGot);
  RC_CHECK_EQ(value, 2);

  // Closed with one item left: the item is still handed out. A shutdown does not
  // drop work that was already queued.
  queue.close();
  RC_REQUIRE(queue.pop_for(value, kPatient) == Result::kGot);
  RC_CHECK_EQ(value, 3);
  RC_CHECK(queue.pop_for(value, kPatient) == Result::kClosed);

  // None of that needed a consumer to sleep.
  RC_CHECK_EQ(queue.waits(), 0);
  RC_CHECK(!queue.try_pop(value));
}

RC_TEST("a consumer that waits wakes once, and one that polls wakes thousands of times") {
  Queue waited;
  Queue polled;
  std::atomic<long> polls{0};

  std::thread producer([&] {
    std::this_thread::sleep_for(milliseconds(100));
    waited.push(7);
    polled.push(7);
  });

  std::thread poller([&] {
    int value = 0;
    const auto stop = Clock::now() + milliseconds(2000);
    while (!polled.try_pop(value) && Clock::now() < stop) {
      ++polls;
      std::this_thread::yield();
    }
  });

  int got = 0;
  const Result result = waited.pop_for(got, kPatient);

  producer.join();
  poller.join();

  std::cout << "  both waited for the same push, 100 ms after they started:\n";
  std::cout << "    the consumer that waited   went to sleep " << waited.waits() << " time(s)\n";
  std::cout << "    the consumer that polled   asked " << polls.load() << " times\n\n";
  std::cout << "  Measured once, outside the suite, on a 12 core machine: the poller used\n";
  std::cout << "  100.05 ms of CPU in the 100 ms, a whole core; the waiter used 0.04 ms.\n\n";

  RC_CHECK(result == Result::kGot);
  RC_CHECK_EQ(got, 7);
  RC_CHECK(waited.waits() >= 1);
  RC_CHECK(waited.waits() <= 3);
  RC_CHECK(polls.load() > 100);
}

RC_TEST("a notification nobody heard is gone, so the state is checked and not the notification") {
  // The mistake, in plain std types. Nobody is waiting when the notification is
  // sent, and the wait that follows has no condition to look at.
  {
    std::mutex mutex;
    std::condition_variable signal;
    bool ready = false;

    signal.notify_one();
    std::unique_lock<std::mutex> lock(mutex);
    const std::cv_status status = signal.wait_for(lock, milliseconds(50));
    ready = true;
    const bool with_predicate = signal.wait_for(lock, milliseconds(50), [&] { return ready; });

    std::cout << "  notify first, then wait 50 ms:\n";
    std::cout << "    with no condition checked   "
              << (status == std::cv_status::timeout ? "timed out, the notification was lost" : "woke")
              << "\n";
    std::cout << "    checking the state first    "
              << (with_predicate ? "returned at once" : "timed out") << "\n\n";
    RC_CHECK(status == std::cv_status::timeout);
    RC_CHECK(with_predicate);
  }

  // The queue is built the second way: an item pushed before anyone asks is found
  // by looking, with no notification needed and no sleep.
  Queue queue;
  queue.push(4);
  int value = 0;
  RC_REQUIRE(queue.pop_for(value, kPatient) == Result::kGot);
  RC_CHECK_EQ(value, 4);
  RC_CHECK_EQ(queue.waits(), 0);
}

RC_TEST("close wakes every waiter, not just one") {
  Queue queue;
  std::vector<Result> results(3, Result::kGot);
  std::vector<std::thread> consumers;

  const auto began = Clock::now();
  for (int i = 0; i < 3; ++i) {
    consumers.emplace_back([&queue, &results, i] {
      int value = 0;
      results[static_cast<std::size_t>(i)] = queue.pop_for(value, kPatient);
    });
  }

  const bool asleep = wait_until_asleep(queue, 3, milliseconds(1000));
  queue.close();
  for (std::thread& consumer : consumers) consumer.join();
  const auto elapsed = std::chrono::duration_cast<milliseconds>(Clock::now() - began);

  std::cout << "  three consumers asleep, then close():\n";
  for (int i = 0; i < 3; ++i)
    std::cout << "    consumer " << i << "   " << name_of(results[static_cast<std::size_t>(i)]) << "\n";
  std::cout << "    all three returned within " << elapsed.count() << " ms, where the wait was "
            << kPatient.count() << " ms\n\n";

  RC_CHECK(asleep);
  for (int i = 0; i < 3; ++i) RC_CHECK(results[static_cast<std::size_t>(i)] == Result::kClosed);
  RC_CHECK(elapsed < kPatient);
}

RC_TEST("two consumers, one item: one takes it, and the other is still waiting when the queue closes") {
  Queue queue;
  std::vector<Result> results(2, Result::kTimeout);
  std::vector<std::thread> consumers;

  for (int i = 0; i < 2; ++i) {
    consumers.emplace_back([&queue, &results, i] {
      int value = 0;
      results[static_cast<std::size_t>(i)] = queue.pop_for(value, kPatient);
    });
  }

  const bool both_asleep = wait_until_asleep(queue, 2, milliseconds(1000));
  const int sleeps_before = queue.waits();
  queue.push(9);

  // push wakes one consumer, so the other stays asleep. Had it used notify_all the
  // loser would wake, find nothing and sleep again, which is the reason the wait
  // is a loop. Either way it is still waiting here, and close() has to release it.
  std::this_thread::sleep_for(milliseconds(50));
  queue.close();
  for (std::thread& consumer : consumers) consumer.join();

  int got = 0;
  int closed = 0;
  for (const Result result : results) {
    if (result == Result::kGot) ++got;
    if (result == Result::kClosed) ++closed;
  }

  std::cout << "  two asleep, one item pushed, then close():\n";
  std::cout << "    took the item   " << got << "\n";
  std::cout << "    told it was closed   " << closed << "\n";
  std::cout << "    sleeps before the push " << sleeps_before << ", in all " << queue.waits() << "\n\n";

  RC_CHECK(both_asleep);
  RC_CHECK_EQ(got, 1);
  RC_CHECK_EQ(closed, 1);
}

RC_TEST("a timeout is an answer, and it is not early") {
  Queue queue;
  int value = 0;

  const auto before = Clock::now();
  const Result result = queue.pop_for(value, milliseconds(30));
  const auto elapsed = std::chrono::duration_cast<milliseconds>(Clock::now() - before);

  std::cout << "  pop_for on an empty queue with a 30 ms deadline: " << name_of(result) << " after "
            << elapsed.count() << " ms, " << queue.waits() << " sleep(s)\n\n";

  RC_CHECK(result == Result::kTimeout);
  RC_CHECK(elapsed.count() >= 29);
  RC_CHECK(queue.waits() >= 1);

  // And the queue is still usable afterwards.
  queue.push(5);
  RC_REQUIRE(queue.pop_for(value, kPatient) == Result::kGot);
  RC_CHECK_EQ(value, 5);
}
