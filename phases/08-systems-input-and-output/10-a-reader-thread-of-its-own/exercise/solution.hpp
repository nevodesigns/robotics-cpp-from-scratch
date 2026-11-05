#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include <rc/core/compat.hpp>
#include <rc/io/link.hpp>
#include <rc/rt/scoped_thread.hpp>
#include <rc/rt/seqlock.hpp>
#include <rc/rt/spsc_queue.hpp>
#include <rc/rt/stop.hpp>

using rc::io::BytePort;
using rc::io::Link;

// A port that two threads can use. Given: a device thread delivers into it and
// the reader takes out of it. A fixed buffer, which drops what arrives when full.
class SharedPort : public BytePort {
 public:
  explicit SharedPort(std::size_t capacity) : capacity_(capacity) {}

  void deliver(rc::span<const std::uint8_t> bytes) {
    const std::lock_guard<std::mutex> lock(mutex_);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      if (held_.size() >= capacity_) {
        ++overflowed_;
        continue;
      }
      held_.push_back(bytes[i]);
    }
  }

  std::size_t overflowed() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return overflowed_;
  }

  // The next read fails, for as many reads as asked. A cable that comes out.
  void fail_next_reads(int count) {
    const std::lock_guard<std::mutex> lock(mutex_);
    failing_ = count;
  }

  int read(rc::span<std::uint8_t> into) override {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (failing_ != 0) {
      if (failing_ > 0) --failing_;
      return -1;
    }
    std::size_t count = 0;
    while (count < into.size() && !held_.empty()) {
      into[count++] = held_.front();
      held_.pop_front();
    }
    return static_cast<int>(count);
  }

  int write(rc::span<const std::uint8_t> from) override { return static_cast<int>(from.size()); }

 private:
  mutable std::mutex mutex_;
  std::size_t capacity_;
  std::deque<std::uint8_t> held_;
  std::size_t overflowed_ = 0;
  int failing_ = 0;
};

constexpr std::size_t kReadingBytes = 48;

// One message, as the reader hands it over. Plain data, so that it can be
// copied through a Seqlock and a queue without either needing to know what is
// in it.
struct Reading {
  std::uint64_t count = 0;                   // which message this is, from one
  std::uint32_t size = 0;                    // how many of the bytes are real
  std::uint8_t bytes[kReadingBytes] = {};
};

struct ReaderOptions {
  std::chrono::microseconds idle_sleep{500};   // how long to rest when nothing was waiting
  std::size_t queue_capacity = 256;            // messages held for pop() before new ones are dropped
};

// Reads a port on a thread of its own, so that nothing else the program does can
// hold the reading. Yours.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324)   // the queue is padded to a cache line on purpose
#endif
class LinkReader {
 public:
  LinkReader(BytePort& port, ReaderOptions options = ReaderOptions{})
      : options_(options),
        queue_(options.queue_capacity),
        storage_(64, 0),
        link_(port, rc::span<std::uint8_t>(storage_.data(), storage_.size()), 1e12),
        started_(std::chrono::steady_clock::now()),
        thread_([this] { run(); }) {}

  ~LinkReader() { stop(); }

  LinkReader(const LinkReader&) = delete;
  LinkReader& operator=(const LinkReader&) = delete;

  // Asks the thread to stop and waits for it. Safe to call more than once.
  void stop() {
    // TODO: ask the thread to stop, then wait for it. Both halves, and in that
    // order: waiting without asking waits for ever.
  }

  // The newest message, whole. False before the first has arrived.
  bool latest(Reading& out) const {
    // TODO: read newest_ through the Seqlock. A count of zero means nothing has
    // arrived yet.
    (void)out;
    return false;
  }

  // The oldest message nobody has taken yet. One thread may call this.
  bool pop(Reading& out) {
    // TODO: take from queue_.
    (void)out;
    return false;
  }

  std::size_t frames() const { return frames_.load(std::memory_order_relaxed); }
  std::size_t queue_dropped() const { return queue_dropped_.load(std::memory_order_relaxed); }
  std::size_t checksum_failures() const { return checksum_failures_.load(std::memory_order_relaxed); }
  std::size_t port_errors() const { return port_errors_.load(std::memory_order_relaxed); }
  std::size_t oversized() const { return oversized_.load(std::memory_order_relaxed); }
  std::size_t loops() const { return loops_.load(std::memory_order_relaxed); }

  // The longest the thread went between looking at the port, in microseconds.
  long long longest_gap_us() const { return longest_gap_us_.load(std::memory_order_relaxed); }

  // Seconds since a valid message arrived, which is a different thing from bytes
  // arriving. Infinity until the first.
  double seconds_since_message() const {
    // TODO: last_message_ns_ is -1 until the first message, and otherwise the
    // time of the last one in nanoseconds since started_. Infinity for the
    // first case and the difference from now for the second.
    return 0.0;
  }

 private:
  long long elapsed_ns() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started_)
        .count();
  }

  void run() {
    // TODO: until stop is requested, loop:
    //   - note the gap since the last time round, and keep the longest in
    //     longest_gap_us_, and count the loop in loops_
    //   - poll link_ (seconds since started_) for every message waiting, at most
    //     256 at a time so a device that never stops cannot hide the stop request
    //   - a message longer than kReadingBytes is counted in oversized_ and skipped
    //   - otherwise make a Reading with the next count, write it to newest_, push
    //     it to queue_ (a push that returns false is counted in queue_dropped_),
    //     and update frames_ and last_message_ns_
    //   - copy link_'s checksum_failures and port_errors into their atomics
    //   - rest for options_.idle_sleep when nothing was waiting, AND when the port
    //     errored: a read that fails at once, retried as fast as the processor
    //     can go, costs a whole core for a dead cable
    (void)elapsed_ns();
    (void)options_;
    (void)link_;
  }

  ReaderOptions options_;
  rc::rt::SpscQueue<Reading> queue_;
  std::vector<std::uint8_t> storage_;
  Link link_;
  rc::rt::Seqlock<Reading> newest_;
  rc::rt::StopSource stopper_;
  std::chrono::steady_clock::time_point started_;
  std::atomic<std::size_t> frames_{0};
  std::atomic<std::size_t> queue_dropped_{0};
  std::atomic<std::size_t> checksum_failures_{0};
  std::atomic<std::size_t> port_errors_{0};
  std::atomic<std::size_t> oversized_{0};
  std::atomic<std::size_t> loops_{0};
  std::atomic<long long> longest_gap_us_{0};
  std::atomic<long long> last_message_ns_{-1};

  // Last, so it starts after everything above exists and is joined before any of
  // it goes away.
  rc::rt::ScopedThread thread_;
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif  // LESSON_SOLUTION_HPP
