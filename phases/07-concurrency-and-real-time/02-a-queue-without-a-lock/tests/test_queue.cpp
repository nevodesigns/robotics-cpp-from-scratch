#include <rc/test/rc_test.hpp>
#include <rc/test/leak_check.hpp>

#include <atomic>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

using rc::test::LeakCheck;

Reading numbered(long n) { return Reading{static_cast<double>(n), static_cast<double>(n) * 2.0, n}; }

constexpr long kExchanged = 200000;

// No retry loop in a test may spin without a limit. An unimplemented push
// always answers false, and an unbounded retry would hang the whole suite
// rather than reporting a failure, which is worse than useless to a learner.
constexpr long kMaxSpins = 50000000;

}  // namespace

RC_TEST("a new queue is empty and knows its capacity") {
  const SpscQueue queue(8);
  RC_CHECK_EQ(queue.capacity(), std::size_t{8});
  RC_CHECK_EQ(queue.size(), std::size_t{0});
  RC_CHECK(queue.empty());
}

RC_TEST("a pushed reading comes back out") {
  SpscQueue queue(8);
  RC_CHECK(queue.push(numbered(7)));

  Reading got;
  RC_REQUIRE(queue.pop(got));
  RC_CHECK_EQ(got.sequence, 7L);
  RC_CHECK_NEAR(got.y, 14.0, 1e-12);
}

RC_TEST("popping an empty queue answers false and changes nothing") {
  SpscQueue queue(4);
  Reading got = numbered(99);
  RC_CHECK(!queue.pop(got));
  RC_CHECK_EQ(got.sequence, 99L);
}

RC_TEST("readings come out in the order they went in") {
  SpscQueue queue(8);
  for (long n = 0; n < 5; ++n) RC_CHECK(queue.push(numbered(n)));

  for (long n = 0; n < 5; ++n) {
    Reading got;
    RC_REQUIRE(queue.pop(got));
    RC_CHECK_EQ(got.sequence, n);
  }
}

RC_TEST("the queue holds exactly its capacity, then refuses") {
  // The wasted slot has to be invisible from outside: a queue of four holds
  // four, not three.
  SpscQueue queue(4);
  for (long n = 0; n < 4; ++n) RC_CHECK(queue.push(numbered(n)));

  RC_CHECK_EQ(queue.size(), std::size_t{4});
  RC_CHECK(!queue.push(numbered(99)));
}

RC_TEST("a full queue accepts again once something is taken") {
  SpscQueue queue(2);
  RC_CHECK(queue.push(numbered(1)));
  RC_CHECK(queue.push(numbered(2)));
  RC_CHECK(!queue.push(numbered(3)));

  Reading got;
  RC_REQUIRE(queue.pop(got));
  RC_CHECK(queue.push(numbered(3)));
}

RC_TEST("the indices wrap many times without losing anything") {
  // Where an off by one in the modulus shows up: never on the first pass, only
  // after the indices have gone round.
  SpscQueue queue(3);
  for (long n = 0; n < 1000; ++n) {
    RC_REQUIRE(queue.push(numbered(n)));
    Reading got;
    RC_REQUIRE(queue.pop(got));
    RC_CHECK_EQ(got.sequence, n);
  }
  RC_CHECK(queue.empty());
}

RC_TEST("size is right at every point around a wrap") {
  SpscQueue queue(4);
  for (long round = 0; round < 20; ++round) {
    RC_CHECK_EQ(queue.size(), std::size_t{0});
    for (long n = 0; n < 4; ++n) {
      queue.push(numbered(n));
      RC_CHECK_EQ(queue.size(), static_cast<std::size_t>(n + 1));
    }
    for (long n = 0; n < 4; ++n) {
      Reading got;
      queue.pop(got);
      RC_CHECK_EQ(queue.size(), static_cast<std::size_t>(3 - n));
    }
  }
}

RC_TEST("the queue allocates once and never again") {
  SpscQueue queue(64);
  const LeakCheck check;

  for (long n = 0; n < 100000; ++n) {
    queue.push(numbered(n));
    Reading got;
    queue.pop(got);
  }
  RC_CHECK(check.balanced());
}

RC_TEST("a producer and a consumer exchange everything, in order, losing none") {
  // The test that carries the lesson. Two hundred thousand readings across two
  // real threads, every one of which must arrive exactly once and in sequence.
  SpscQueue queue(1024);

  std::atomic<bool> gave_up{false};

  std::thread producer([&queue, &gave_up] {
    for (long n = 0; n < kExchanged; ++n) {
      long spins = 0;
      while (!queue.push(numbered(n))) {
        if (++spins > kMaxSpins) { gave_up.store(true); return; }
        std::this_thread::yield();
      }
    }
  });

  long received = 0;
  bool out_of_order = false;
  bool wrong_contents = false;

  std::thread consumer([&queue, &received, &out_of_order, &wrong_contents, &gave_up] {
    long spins = 0;
    while (received < kExchanged) {
      Reading got;
      if (!queue.pop(got)) {
        if (gave_up.load() || ++spins > kMaxSpins) { gave_up.store(true); return; }
        std::this_thread::yield();
        continue;
      }
      spins = 0;
      if (got.sequence != received) out_of_order = true;

      // The contents must match the sequence number. A reading assembled from
      // a slot the producer had not finished writing fails here, which is what
      // catches relaxed ordering where release was needed.
      if (got.y != got.x * 2.0) wrong_contents = true;
      ++received;
    }
  });

  producer.join();
  consumer.join();

  RC_CHECK(!gave_up.load());
  RC_CHECK_EQ(received, kExchanged);
  RC_CHECK(!out_of_order);
  RC_CHECK(!wrong_contents);
  RC_CHECK(queue.empty());
}

RC_TEST("a small queue still exchanges everything, with both threads waiting often") {
  // A capacity of two forces the producer to wait for the consumer constantly,
  // which exercises the full and empty boundaries far harder than a large one.
  SpscQueue queue(2);
  constexpr long kSmall = 20000;

  std::atomic<bool> gave_up{false};

  std::thread producer([&queue, &gave_up] {
    for (long n = 0; n < kSmall; ++n) {
      long spins = 0;
      while (!queue.push(numbered(n))) {
        if (++spins > kMaxSpins) { gave_up.store(true); return; }
        std::this_thread::yield();
      }
    }
  });

  long received = 0;
  bool ordered = true;
  std::thread consumer([&queue, &received, &ordered, &gave_up] {
    long spins = 0;
    while (received < kSmall) {
      Reading got;
      if (!queue.pop(got)) {
        if (gave_up.load() || ++spins > kMaxSpins) { gave_up.store(true); return; }
        std::this_thread::yield();
        continue;
      }
      spins = 0;
      if (got.sequence != received) ordered = false;
      ++received;
    }
  });

  producer.join();
  consumer.join();
  RC_CHECK(!gave_up.load());
  RC_CHECK_EQ(received, kSmall);
  RC_CHECK(ordered);
}
