#include <rc/test/rc_test.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

using Clock = std::chrono::steady_clock;

constexpr int kIncrements = 4000000;
constexpr int kItems = 4000000;

double milliseconds_since(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// Every spin in this file is bounded. A queue that never accepts anything is a
// perfectly ordinary thing for a half finished implementation to be, and a test
// that hangs on it is worse than one that fails: nobody sees the message.
constexpr int kSpinLimit = 100000000;

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

  const double same_line = contend<0>();
  const double still_same = contend<8>();
  const double next_line = contend<kCacheLine - 8>();
  const double far = contend<2 * kCacheLine - 8>();

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
  // inside one line.
  RC_CHECK(still_same < same_line * 1.5);
  RC_CHECK(still_same > same_line * 0.5);

  // A line apart is a different situation entirely. Measured at eight times
  // faster on the machine this was written on; two is a floor that leaves room
  // for a busy shared runner.
  RC_CHECK(next_line < same_line * 0.5);

  // And there is nothing further to buy by going further away.
  RC_CHECK(far > next_line * 0.5);
}

RC_TEST("a slot per thread, packed into one array") {
  if (!has_two_cores()) {
    std::cout << "\n    one core reported, so there is nothing to contend\n";
    RC_CHECK(true);
    return;
  }

  const int threads = 4;
  double packed_ms = 0.0, padded_ms = 0.0, local_ms = 0.0;

  {
    std::vector<std::atomic<long>> slots(threads);
    for (auto& slot : slots) slot.store(0);
    const auto start = Clock::now();
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t)
      workers.emplace_back([&slots, t] {
        for (int i = 0; i < kIncrements; ++i)
          slots[t].fetch_add(1, std::memory_order_relaxed);
      });
    for (auto& worker : workers) worker.join();
    packed_ms = milliseconds_since(start);
  }
  {
    std::vector<Padded<std::atomic<long>>> slots(threads);
    const auto start = Clock::now();
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t)
      workers.emplace_back([&slots, t] {
        for (int i = 0; i < kIncrements; ++i)
          slots[t].value.fetch_add(1, std::memory_order_relaxed);
      });
    for (auto& worker : workers) worker.join();
    padded_ms = milliseconds_since(start);
  }
  {
    std::vector<std::atomic<long>> slots(threads);
    for (auto& slot : slots) slot.store(0);
    const auto start = Clock::now();
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t)
      workers.emplace_back([&slots, t] {
        volatile long local = 0;
        for (int i = 0; i < kIncrements; ++i) local = local + 1;
        slots[t].store(local, std::memory_order_relaxed);
      });
    for (auto& worker : workers) worker.join();
    local_ms = milliseconds_since(start);
  }

  std::cout << "\n    " << threads << " threads, " << kIncrements
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

  RC_CHECK(padded_ms < packed_ms * 0.6);

  // Not sharing at all beats sharing carefully. The stack is per thread by
  // construction, and a counter that is only published at the end is never
  // contended at all.
  RC_CHECK(local_ms < padded_ms);
}

RC_TEST("padding and caching are worth little apart and a lot together") {
  if (!has_two_cores()) {
    std::cout << "\n    one core reported, so there is nothing to contend\n";
    RC_CHECK(true);
    return;
  }

  const double plain = drain<false, false>();
  const double separated = drain<true, false>();
  const double cached = drain<false, true>();
  const double both = drain<true, true>();

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

  // Both together are worth a good deal more than either alone.
  RC_CHECK(both < plain * 0.75);
  RC_CHECK(both < cached * 0.75);

  // Caching alone, with the indices still adjacent, is not an improvement.
  RC_CHECK(cached > plain * 0.75);
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
  constexpr int items = 200000;
  bool ordered = true;
  std::atomic<bool> stalled{false};
  std::atomic<int> received{0};

  std::thread producer([&queue, &stalled] {
    for (int i = 0; i < items && !stalled; ++i) {
      int spins = 0;
      while (!queue.push(i)) {
        if (++spins > kSpinLimit) { stalled = true; return; }
      }
    }
  });
  std::thread consumer([&queue, &ordered, &stalled, &received] {
    int value = 0;
    for (int i = 0; i < items && !stalled; ++i) {
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
  RC_CHECK_EQ(received.load(), items);
}
