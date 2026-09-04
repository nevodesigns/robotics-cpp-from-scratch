#include <rc/test/rc_test.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "solution.hpp"

// Several structures here are deliberately over-aligned, and MSVC reports at
// /W4 that it padded them because of the alignment specifier. That is the
// request, not a mistake.
#ifdef _MSC_VER
#pragma warning(disable : 4324)
#endif

namespace {

using Clock = std::chrono::steady_clock;

constexpr int kIncrements = 1500000;
constexpr int kItems = 1000000;
constexpr int kOrderedItems = 200000;

// How many times each measurement is repeated. The number reported is the
// smallest of them, which is the only statistic a microbenchmark on a shared
// machine can defend: noise can only ever add time, so the minimum is the
// closest any run came to measuring the thing itself. A mean or a single sample
// measures whatever else the machine was doing.
constexpr int kRepeats = 5;

// The queue measurement needs more repeats than the counters do. Its effect is
// large and its variance is larger, and a gate that fails one run in six is a
// gate people learn to rerun.
constexpr int kQueueRepeats = 9;
constexpr int kThreads = 4;

double milliseconds_since(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// Every spin in this file is bounded. A queue that never accepts anything is a
// perfectly ordinary thing for a half finished implementation to be, and a test
// that hangs on it is worse than one that fails: nobody sees the message.
constexpr int kSpinLimit = 100000000;

template <class Run>
double best_of(int repeats, Run run) {
  double best = -1.0;
  for (int i = 0; i < repeats; ++i) {
    const double ms = run();
    if (best < 0.0 || ms < best) best = ms;
  }
  return best;
}

bool has_two_cores() { return std::thread::hardware_concurrency() >= 2; }

// Two atomic counters `gap` bytes apart, hammered by two threads.
template <std::size_t Gap>
double contend() {
  struct Pair {
    alignas(kCacheLine) std::atomic<long> first{0};
    char spacer[Gap];
    std::atomic<long> second{0};
  };
  Pair pair;

  const auto start = Clock::now();
  std::thread one([&pair] {
    for (int i = 0; i < kIncrements; ++i) pair.first.fetch_add(1, std::memory_order_relaxed);
  });
  std::thread two([&pair] {
    for (int i = 0; i < kIncrements; ++i) pair.second.fetch_add(1, std::memory_order_relaxed);
  });
  one.join();
  two.join();
  return milliseconds_since(start);
}

// The queue, with the padding and the caching each turned on or off.
template <bool Separated, bool Cached>
class Queue {
 public:
  explicit Queue(std::size_t capacity) : buffer_(capacity + 1) {}

  bool push(int value) {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t next = (tail + 1) % buffer_.size();
    if (Cached) {
      if (next == cached_head_) {
        cached_head_ = head_.load(std::memory_order_acquire);
        if (next == cached_head_) return false;
      }
    } else if (next == head_.load(std::memory_order_acquire)) {
      return false;
    }
    buffer_[tail] = value;
    tail_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(int& out) {
    const std::size_t head = head_.load(std::memory_order_relaxed);
    if (Cached) {
      if (head == cached_tail_) {
        cached_tail_ = tail_.load(std::memory_order_acquire);
        if (head == cached_tail_) return false;
      }
    } else if (head == tail_.load(std::memory_order_acquire)) {
      return false;
    }
    out = buffer_[head];
    head_.store((head + 1) % buffer_.size(), std::memory_order_release);
    return true;
  }

 private:
  std::vector<int> buffer_;
  alignas(kCacheLine) std::atomic<std::size_t> head_{0};
  std::size_t cached_tail_ = 0;
  char spacer_[Separated ? kCacheLine - sizeof(std::atomic<std::size_t>) - sizeof(std::size_t)
                         : 1];
  std::atomic<std::size_t> tail_{0};
  std::size_t cached_head_ = 0;
};

template <bool Separated, bool Cached>
double drain() {
  Queue<Separated, Cached> queue(1024);
  const auto start = Clock::now();
  std::atomic<bool> stalled{false};
  std::thread producer([&queue, &stalled] {
    for (int i = 0; i < kItems && !stalled; ++i) {
      int spins = 0;
      while (!queue.push(i)) {
        if (++spins > kSpinLimit) { stalled = true; return; }
      }
    }
  });
  std::thread consumer([&queue, &stalled] {
    int value = 0;
    for (int i = 0; i < kItems && !stalled; ++i) {
      int spins = 0;
      while (!queue.pop(value)) {
        if (++spins > kSpinLimit) { stalled = true; return; }
      }
    }
  });
  producer.join();
  consumer.join();
  return milliseconds_since(start);
}

}  // namespace

RC_TEST("a padded value takes a whole line, and keeps it inside an array") {
  RC_CHECK_EQ(sizeof(Padded<std::atomic<long>>), kCacheLine);
  RC_CHECK_EQ(alignof(Padded<std::atomic<long>>), kCacheLine);

  // The point of putting alignas on the type rather than on a member: a vector
  // of these has one per line. A vector of the bare type packs eight into one.
  std::vector<Padded<std::atomic<long>>> padded(4);
  std::vector<std::atomic<long>> packed(4);

  const char* first = reinterpret_cast<const char*>(&padded[0]);
  const char* second = reinterpret_cast<const char*>(&padded[1]);
  RC_CHECK_EQ(static_cast<std::size_t>(second - first), kCacheLine);

  const char* packed_first = reinterpret_cast<const char*>(&packed[0]);
  const char* packed_second = reinterpret_cast<const char*>(&packed[1]);
  RC_CHECK(static_cast<std::size_t>(packed_second - packed_first) < kCacheLine);

  // And it still behaves like the thing it holds.
  padded[0].value.store(7);
  RC_CHECK_EQ((*padded[0]).load(), 7L);
  RC_CHECK_EQ(padded[0]->load(), 7L);
}

RC_TEST("two counters, and the distance between them") {
  if (!has_two_cores()) {
    std::cout << "\n    one core reported, so there is nothing to contend\n";
    RC_CHECK(true);
    return;
  }

  std::cout << "\n    two threads, " << kIncrements
            << " atomic increments each, on two counters\n\n";
  std::cout << "    " << std::right << std::setw(22) << "bytes between them"
            << std::setw(12) << "ms" << std::setw(12) << "relative" << "\n";

  const double same_line = best_of(kRepeats, [] { return contend<0>(); });
  const double still_same = best_of(kRepeats, [] { return contend<8>(); });
  const double next_line = best_of(kRepeats, [] { return contend<kCacheLine - 8>(); });
  const double far = best_of(kRepeats, [] { return contend<2 * kCacheLine - 8>(); });

  const auto row = [same_line](std::size_t bytes, double ms) {
    std::cout << "    " << std::right << std::setw(22) << bytes << std::fixed
              << std::setprecision(1) << std::setw(12) << ms << std::setprecision(2)
              << std::setw(12) << ms / same_line << "\n";
  };
  row(8, same_line);
  row(16, still_same);
  row(kCacheLine, next_line);
  row(2 * kCacheLine, far);

  std::cout << "\n    nothing changes until the two cross a line boundary, and\n";
  std::cout << "    then everything does\n";

  // Eight bytes apart and sixteen bytes apart are the same situation: both are
  // inside one line, and both are far worse than a line apart.
  RC_CHECK(still_same > next_line * 1.6);
  RC_CHECK(same_line > next_line * 1.6);

  // And there is nothing further to buy by going further away than one line.
  RC_CHECK(far > next_line * 0.6);
}

RC_TEST("a slot per thread, packed into one array") {
  if (!has_two_cores()) {
    std::cout << "\n    one core reported, so there is nothing to contend\n";
    RC_CHECK(true);
    return;
  }

  double packed_ms = 0.0, padded_ms = 0.0, local_ms = 0.0;

  const auto packed_run = [] {
    std::vector<std::atomic<long>> slots(kThreads);
    for (auto& slot : slots) slot.store(0);
    const auto start = Clock::now();
    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t)
      workers.emplace_back([&slots, t] {
        for (int i = 0; i < kIncrements; ++i)
          slots[t].fetch_add(1, std::memory_order_relaxed);
      });
    for (auto& worker : workers) worker.join();
    return milliseconds_since(start);
  };
  const auto padded_run = [] {
    std::vector<Padded<std::atomic<long>>> slots(kThreads);
    const auto start = Clock::now();
    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t)
      workers.emplace_back([&slots, t] {
        for (int i = 0; i < kIncrements; ++i)
          slots[t].value.fetch_add(1, std::memory_order_relaxed);
      });
    for (auto& worker : workers) worker.join();
    return milliseconds_since(start);
  };
  const auto local_run = [] {
    std::vector<std::atomic<long>> slots(kThreads);
    for (auto& slot : slots) slot.store(0);
    const auto start = Clock::now();
    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t)
      workers.emplace_back([&slots, t] {
        volatile long local = 0;
        for (int i = 0; i < kIncrements; ++i) local = local + 1;
        slots[t].store(local, std::memory_order_relaxed);
      });
    for (auto& worker : workers) worker.join();
    return milliseconds_since(start);
  };

  packed_ms = best_of(3, packed_run);
  padded_ms = best_of(3, padded_run);
  local_ms = best_of(3, local_run);

  std::cout << "\n    " << kThreads << " threads, " << kIncrements
            << " increments each into a slot of their own\n\n";
  const auto row = [packed_ms](const char* name, double ms) {
    std::cout << "    " << std::left << std::setw(40) << name << std::right
              << std::fixed << std::setprecision(1) << std::setw(10) << ms
              << std::setprecision(2) << std::setw(12) << ms / packed_ms << "\n";
  };
  row("a vector of atomics, one per thread", packed_ms);
  row("the same, one per cache line", padded_ms);
  row("a local on the stack, stored once", local_ms);

  std::cout << "\n    a per-thread array is the most natural thing to write and\n";
  std::cout << "    it puts every thread's counter in one line\n";

  // Four threads need four cores for this to be about cache lines rather than
  // about timeslicing, so the claim is only made where there are four.
  if (std::thread::hardware_concurrency() >= 4) {
    RC_CHECK(padded_ms < packed_ms * 0.75);
  }

  // Not sharing at all beats sharing carefully, on any number of cores. The
  // stack is per thread by construction, and a counter published only at the
  // end is never contended.
  RC_CHECK(local_ms < padded_ms);
}

RC_TEST("padding and caching are worth little apart and a lot together") {
  if (!has_two_cores()) {
    std::cout << "\n    one core reported, so there is nothing to contend\n";
    RC_CHECK(true);
    return;
  }

  const double plain = best_of(kQueueRepeats, [] { return drain<false, false>(); });
  const double separated = best_of(kQueueRepeats, [] { return drain<true, false>(); });
  const double cached = best_of(kQueueRepeats, [] { return drain<false, true>(); });
  const double both = best_of(kQueueRepeats, [] { return drain<true, true>(); });

  std::cout << "\n    " << kItems << " items through a 1024 slot queue\n\n";
  std::cout << "    " << std::left << std::setw(30) << "" << std::right
            << std::setw(12) << "ms" << std::setw(16) << "M items/s" << "\n";
  const auto row = [](const char* name, double ms) {
    std::cout << "    " << std::left << std::setw(30) << name << std::right
              << std::fixed << std::setprecision(1) << std::setw(12) << ms
              << std::setprecision(2) << std::setw(16) << kItems / ms / 1000.0 << "\n";
  };
  row("adjacent, read every time", plain);
  row("a line apart, read every time", separated);
  row("adjacent, cached", cached);
  row("a line apart, cached", both);

  std::cout << "\n    apart the two changes are worth " << std::setprecision(0)
            << (plain / separated - 1.0) * 100.0 << " and "
            << (plain / cached - 1.0) * 100.0 << " percent; together they are\n";
  std::cout << "    worth " << (plain / both - 1.0) * 100.0 << "\n";
  std::cout << "    caching the other side's index buys little while the two\n";
  std::cout << "    still share a line, because the cached copy is invalidated\n";
  std::cout << "    about as often as the real one would have been read\n";

  // Both together are worth a good deal more than either alone. Measured at
  // between 74 and 95 percent; the floor here is deliberately far below that,
  // because a gate that fails one run in six is a gate people learn to rerun.
  RC_CHECK(both < plain * 0.9);
  RC_CHECK(both < separated * 0.9);
}

RC_TEST("the queue is still a queue") {
  FastQueue<int> queue(4);
  RC_CHECK_EQ(queue.capacity(), static_cast<std::size_t>(4));

  int value = 0;
  RC_CHECK(!queue.pop(value));   // empty

  for (int i = 0; i < 4; ++i) RC_CHECK(queue.push(i));
  RC_CHECK(!queue.push(99));     // full, refused rather than grown

  for (int i = 0; i < 4; ++i) {
    RC_REQUIRE(queue.pop(value));
    RC_CHECK_EQ(value, i);       // in order
  }
  RC_CHECK(!queue.pop(value));

  // And it wraps, which is where a cached index would go wrong if it were
  // refreshed in the wrong place.
  for (int round = 0; round < 100; ++round) {
    for (int i = 0; i < 3; ++i) RC_CHECK(queue.push(round * 10 + i));
    for (int i = 0; i < 3; ++i) {
      RC_REQUIRE(queue.pop(value));
      RC_CHECK_EQ(value, round * 10 + i);
    }
  }
}

RC_TEST("everything the producer sent arrives, in order") {
  if (!has_two_cores()) {
    std::cout << "\n    one core reported, so this runs on one\n";
  }

  FastQueue<int> queue(64);
  bool ordered = true;
  std::atomic<bool> stalled{false};
  std::atomic<int> received{0};

  std::thread producer([&queue, &stalled] {
    for (int i = 0; i < kOrderedItems && !stalled; ++i) {
      int spins = 0;
      while (!queue.push(i)) {
        if (++spins > kSpinLimit) { stalled = true; return; }
      }
    }
  });
  std::thread consumer([&queue, &ordered, &stalled, &received] {
    int value = 0;
    for (int i = 0; i < kOrderedItems && !stalled; ++i) {
      int spins = 0;
      while (!queue.pop(value)) {
        if (++spins > kSpinLimit) { stalled = true; return; }
      }
      if (value != i) ordered = false;
      received.store(i + 1);
    }
  });
  producer.join();
  consumer.join();

  RC_CHECK(!stalled);
  RC_CHECK(ordered);
  RC_CHECK_EQ(received.load(), kOrderedItems);
}
