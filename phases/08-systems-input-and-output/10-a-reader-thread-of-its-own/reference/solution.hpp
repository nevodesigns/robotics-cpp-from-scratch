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

// A port that two threads can use: a device thread delivers into it and the
// reader takes out of it. A fixed buffer, which drops what arrives when full,
// as a serial driver does.
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
// hold the reading.
//
// Lesson 08-08 measured the cliff: stop reading for longer than the buffer
// takes to fill and the device starts losing bytes. A control loop, a redraw and
// a log flush are all things that can run for longer than that. A thread that
// does nothing but read has a stall budget that only its own code can spend.
//
// Two ways to take what it reads. latest() is the newest message, for a state.
// pop() is every message in order, for a change, and counts what it had to drop
// when nobody was taking them.
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

  // Stops and waits. The thread is the last member, so it is joined before
  // anything it uses is destroyed.
  ~LinkReader() { stop(); }

  LinkReader(const LinkReader&) = delete;
  LinkReader& operator=(const LinkReader&) = delete;

  // Asks the thread to stop and waits for it. Safe to call more than once.
  void stop() {
    stopper_.request_stop();
    thread_.join();
  }

  // The newest message, whole. False before the first has arrived.
  bool latest(Reading& out) const {
    const Reading seen = newest_.read();
    if (seen.count == 0) return false;
    out = seen;
    return true;
  }

  // The oldest message nobody has taken yet. One thread may call this.
  bool pop(Reading& out) { return queue_.pop(out); }

  std::size_t frames() const { return frames_.load(std::memory_order_relaxed); }
  std::size_t queue_dropped() const { return queue_dropped_.load(std::memory_order_relaxed); }
  std::size_t checksum_failures() const { return checksum_failures_.load(std::memory_order_relaxed); }
  std::size_t port_errors() const { return port_errors_.load(std::memory_order_relaxed); }
  std::size_t oversized() const { return oversized_.load(std::memory_order_relaxed); }
  std::size_t loops() const { return loops_.load(std::memory_order_relaxed); }

  // The longest the thread went between looking at the port, in microseconds.
  // This is the number the stall budget is spent from.
  long long longest_gap_us() const { return longest_gap_us_.load(std::memory_order_relaxed); }

  // Seconds since a valid message arrived, which is a different thing from
  // bytes arriving. Infinity until the first.
  double seconds_since_message() const {
    const long long at = last_message_ns_.load(std::memory_order_relaxed);
    if (at < 0) return std::numeric_limits<double>::infinity();
    const long long now = elapsed_ns();
    return static_cast<double>(now - at) / 1e9;
  }

 private:
  long long elapsed_ns() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started_)
        .count();
  }

  void run() {
    auto last_look = std::chrono::steady_clock::now();
    std::uint64_t count = 0;

    while (!stopper_.get_token().stop_requested()) {
      const auto now = std::chrono::steady_clock::now();
      const long long gap =
          std::chrono::duration_cast<std::chrono::microseconds>(now - last_look).count();
      if (gap > longest_gap_us_.load(std::memory_order_relaxed)) {
        longest_gap_us_.store(gap, std::memory_order_relaxed);
      }
      last_look = now;
      loops_.fetch_add(1, std::memory_order_relaxed);

      const double seconds = std::chrono::duration<double>(now - started_).count();
      const std::size_t errors_before = link_.port_errors();
      bool any = false;

      // Bounded, so the stop request is looked at even when the device never
      // stops sending.
      for (int taken = 0; taken < 256 && link_.poll(seconds); ++taken) {
        any = true;
        const rc::span<const std::uint8_t> message = link_.message();
        if (message.size() > kReadingBytes) {
          oversized_.fetch_add(1, std::memory_order_relaxed);
          continue;
        }

        Reading reading;
        reading.count = ++count;
        reading.size = static_cast<std::uint32_t>(message.size());
        if (message.size() != 0) std::memcpy(reading.bytes, message.data(), message.size());

        newest_.write(reading);
        if (!queue_.push(reading)) queue_dropped_.fetch_add(1, std::memory_order_relaxed);
        frames_.store(static_cast<std::size_t>(count), std::memory_order_relaxed);
        last_message_ns_.store(elapsed_ns(), std::memory_order_relaxed);
      }

      checksum_failures_.store(link_.checksum_failures(), std::memory_order_relaxed);
      port_errors_.store(link_.port_errors(), std::memory_order_relaxed);

      // Rest when there was nothing to do, and also when the port is failing:
      // a read that errors straight away would otherwise be retried as fast as
      // the processor can go, and a dead cable would cost a whole core.
      if (!any || link_.port_errors() != errors_before) std::this_thread::sleep_for(options_.idle_sleep);
    }
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
