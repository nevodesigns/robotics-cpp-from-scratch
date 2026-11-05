#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/fault_port.hpp>
#include <rc/io/sequence.hpp>
#include <rc/rt/scoped_thread.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::size_t kBuffer = 4096;       // 372 ms of a 1000 frame a second device
constexpr int kFramesPerSecond = 1000;
constexpr int kFrames = 1000;
constexpr int kStallMs = 900;               // the main thread is busy for longer than the buffer lasts

std::vector<std::uint8_t> wire_frame(std::uint16_t sequence, std::size_t payload_size = 8) {
  std::vector<std::uint8_t> body(payload_size, 0);
  if (payload_size >= 8) {
    rc::io::ByteWriter writer{rc::span<std::uint8_t>(body.data(), body.size())};
    writer.put_u16(sequence);
    writer.put_u16(static_cast<std::uint16_t>(sequence * 3u));
    writer.put_u32(0x11223344u * sequence);
  }
  std::vector<std::uint8_t> out{rc::io::Link::kStartByte, static_cast<std::uint8_t>(body.size() + 1)};
  out.insert(out.end(), body.begin(), body.end());
  out.push_back(rc::io::crc8(rc::span<const std::uint8_t>(body.data(), body.size())));
  return out;
}

bool content_is_right(const Reading& reading) {
  rc::io::ByteReader reader{rc::span<const std::uint8_t>(reading.bytes, reading.size)};
  std::uint16_t sequence = 0;
  std::uint16_t triple = 0;
  std::uint32_t mixed = 0;
  reader.get_u16(sequence);
  reader.get_u16(triple);
  reader.get_u32(mixed);
  return reader.ok() && reading.size == 8 && triple == static_cast<std::uint16_t>(sequence * 3u) &&
         mixed == 0x11223344u * sequence;
}

std::uint16_t sequence_of(const Reading& reading) {
  rc::io::ByteReader reader{rc::span<const std::uint8_t>(reading.bytes, reading.size)};
  std::uint16_t sequence = 0;
  reader.get_u16(sequence);
  return sequence;
}

// A device on a thread of its own. It sends frame i at i / rate seconds after it
// starts and, when it is late, sends at once rather than slowing down, so the
// rate holds on a machine whose sleeps are coarse.
class Device {
 public:
  Device(SharedPort& port, int frames, int per_second)
      : thread_([&port, frames, per_second, this] {
          const Clock::time_point start = Clock::now();
          for (int i = 0; i < frames; ++i) {
            std::this_thread::sleep_until(start + std::chrono::microseconds(1000000LL * i / per_second));
            const std::vector<std::uint8_t> f = wire_frame(static_cast<std::uint16_t>(i));
            port.deliver(rc::span<const std::uint8_t>(f.data(), f.size()));
            sent_.store(i + 1);
          }
        }) {}

  int sent() const { return sent_.load(); }
  void join() { thread_.join(); }

 private:
  std::atomic<int> sent_{0};
  rc::rt::ScopedThread thread_;
};

template <class Predicate>
bool wait_until(Predicate done, int timeout_ms) {
  const Clock::time_point give_up = Clock::now() + std::chrono::milliseconds(timeout_ms);
  while (!done()) {
    if (Clock::now() > give_up) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return true;
}

}  // namespace

RC_TEST("a reader on its own thread takes what the device sends, whole and in order") {
  SharedPort port(kBuffer);
  ReaderOptions options;
  options.queue_capacity = 512;   // room for every one of the 300, so nothing here is about a full queue
  LinkReader reader(port, options);
  Device device(port, 300, kFramesPerSecond);

  RC_REQUIRE(wait_until([&] { return reader.frames() == 300; }, 8000));
  device.join();

  std::vector<Reading> taken;
  Reading r;
  while (reader.pop(r)) taken.push_back(r);

  RC_REQUIRE_EQ(taken.size(), static_cast<std::size_t>(300));
  bool in_order = true;
  bool whole = true;
  for (std::size_t i = 0; i < taken.size(); ++i) {
    if (sequence_of(taken[i]) != i) in_order = false;
    if (!content_is_right(taken[i])) whole = false;
  }
  RC_CHECK(in_order);
  RC_CHECK(whole);
  RC_CHECK_EQ(reader.checksum_failures(), static_cast<std::size_t>(0));
  RC_CHECK_EQ(reader.queue_dropped(), static_cast<std::size_t>(0));

  Reading newest;
  RC_REQUIRE(reader.latest(newest));
  RC_CHECK_EQ(newest.count, static_cast<std::uint64_t>(300));
  RC_CHECK_EQ(static_cast<int>(sequence_of(newest)), 299);
}

RC_TEST("latest is never torn while the device is sending") {
  SharedPort port(kBuffer);
  LinkReader reader(port);
  Device device(port, 1200, 1200);

  long looks = 0;
  long torn = 0;
  std::uint64_t last_count = 0;
  bool went_backwards = false;

  const Clock::time_point give_up = Clock::now() + std::chrono::seconds(8);
  while (device.sent() < 1200 && Clock::now() < give_up) {
    Reading seen;
    if (reader.latest(seen)) {
      ++looks;
      if (!content_is_right(seen)) ++torn;
      if (seen.count < last_count) went_backwards = true;
      last_count = seen.count;
    }
    std::this_thread::yield();
  }
  device.join();
  RC_REQUIRE(wait_until([&] { return reader.frames() == 1200; }, 4000));

  std::cout << "  " << looks << " looks at latest() while 1200 frames arrived: " << torn << " torn, "
            << (went_backwards ? "some went backwards" : "none went backwards") << ".\n\n";

  RC_CHECK(looks > 10);
  RC_CHECK_EQ(torn, 0L);
  RC_CHECK(!went_backwards);
  Reading final_value;
  RC_REQUIRE(reader.latest(final_value));
  RC_CHECK_EQ(final_value.count, static_cast<std::uint64_t>(1200));
}

RC_TEST("a full queue drops the newest, counts it, and latest still moves") {
  rc::io::MemoryPort port;
  for (int i = 0; i < 50; ++i) {
    const std::vector<std::uint8_t> f = wire_frame(static_cast<std::uint16_t>(i));
    port.feed(rc::span<const std::uint8_t>(f.data(), f.size()));
  }

  ReaderOptions options;
  options.queue_capacity = 4;
  LinkReader reader(port, options);
  RC_REQUIRE(wait_until([&] { return reader.frames() == 50; }, 5000));

  std::vector<Reading> kept;
  Reading r;
  while (reader.pop(r)) kept.push_back(r);

  RC_REQUIRE_EQ(kept.size(), static_cast<std::size_t>(4));
  RC_CHECK_EQ(reader.queue_dropped(), static_cast<std::size_t>(46));
  RC_CHECK_EQ(static_cast<int>(sequence_of(kept[0])), 0);    // the oldest are kept
  RC_CHECK_EQ(static_cast<int>(sequence_of(kept[3])), 3);

  Reading newest;
  RC_REQUIRE(reader.latest(newest));
  RC_CHECK_EQ(static_cast<int>(sequence_of(newest)), 49);   // and the newest is still the newest
}

RC_TEST("stopping is prompt, is safe twice, and leaves the port alone afterwards") {
  SharedPort port(kBuffer);
  LinkReader reader(port);
  std::this_thread::sleep_for(std::chrono::milliseconds(30));

  const Clock::time_point before = Clock::now();
  reader.stop();
  reader.stop();
  const auto took = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - before).count();

  const std::size_t loops_at_stop = reader.loops();
  std::this_thread::sleep_for(std::chrono::milliseconds(40));

  RC_CHECK(took < 1000);
  RC_CHECK(loops_at_stop > 0);
  RC_CHECK_EQ(reader.loops(), loops_at_stop);   // nothing is looking at the port any more
}

RC_TEST("a port that fails is retried gently, and the reader recovers when it comes back") {
  SharedPort port(kBuffer);
  port.fail_next_reads(-1);   // every read fails until told otherwise

  LinkReader reader(port);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  const std::size_t errors = reader.port_errors();
  const std::size_t loops = reader.loops();
  std::cout << "  200 ms with the cable out: " << errors << " port errors in " << loops << " loops.\n\n";

  RC_CHECK(errors > 0);
  RC_CHECK(loops < 2000);   // it rests between tries, instead of spinning at the speed of the processor
  RC_CHECK_EQ(reader.frames(), static_cast<std::size_t>(0));

  port.fail_next_reads(0);
  const std::vector<std::uint8_t> f = wire_frame(7);
  port.deliver(rc::span<const std::uint8_t>(f.data(), f.size()));
  RC_CHECK(wait_until([&] { return reader.frames() == 1; }, 3000));
}

RC_TEST("a message too big for a Reading is counted and does not stop the stream") {
  SharedPort port(kBuffer);
  LinkReader reader(port);

  const std::vector<std::uint8_t> big = wire_frame(1, 60);   // 60 bytes of payload, and a Reading holds 48
  port.deliver(rc::span<const std::uint8_t>(big.data(), big.size()));
  const std::vector<std::uint8_t> small = wire_frame(2);
  port.deliver(rc::span<const std::uint8_t>(small.data(), small.size()));

  RC_REQUIRE(wait_until([&] { return reader.frames() == 1; }, 3000));
  RC_CHECK_EQ(reader.oversized(), static_cast<std::size_t>(1));

  Reading r;
  RC_REQUIRE(reader.latest(r));
  RC_CHECK_EQ(static_cast<int>(sequence_of(r)), 2);
}

RC_TEST("silence is measured from valid messages, not from bytes") {
  SharedPort port(kBuffer);
  LinkReader reader(port);

  // Noise that never becomes a frame: bytes arrive, and none of them is a message.
  const std::vector<std::uint8_t> noise(200, 0x55);
  port.deliver(rc::span<const std::uint8_t>(noise.data(), noise.size()));
  std::this_thread::sleep_for(std::chrono::milliseconds(60));

  RC_CHECK_EQ(reader.frames(), static_cast<std::size_t>(0));
  RC_CHECK(reader.seconds_since_message() > 1e9);   // never, which is infinity

  const std::vector<std::uint8_t> f = wire_frame(3);
  port.deliver(rc::span<const std::uint8_t>(f.data(), f.size()));
  RC_REQUIRE(wait_until([&] { return reader.frames() == 1; }, 3000));
  RC_CHECK(reader.seconds_since_message() < 1.0);

  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  RC_CHECK(reader.seconds_since_message() > 0.1);
}

RC_TEST("while the main thread is busy for longer than the buffer lasts, a reader thread loses nothing") {
  const double budget_ms = 1000.0 * static_cast<double>(kBuffer) /
                           (static_cast<double>(kFramesPerSecond) * 11.0);

  // With a reader thread: the main thread does nothing for 900 ms.
  std::size_t lost_with = 0;
  long long gap_us = 0;
  std::size_t overflowed_with = 0;
  {
    SharedPort port(kBuffer);
    ReaderOptions options;
    options.queue_capacity = 2048;   // the reader's queue is not what is being tested
    LinkReader reader(port, options);
    Device device(port, kFrames, kFramesPerSecond);
    std::this_thread::sleep_for(std::chrono::milliseconds(kStallMs));
    device.join();
    wait_until([&] { return reader.frames() >= static_cast<std::size_t>(kFrames); }, 4000);

    rc::io::SequenceTracker<std::uint16_t> tracker;
    std::size_t received = 0;
    Reading r;
    while (reader.pop(r)) {
      tracker.accept(sequence_of(r));
      ++received;
    }

    // Lost is what was sent and did not arrive, counted two ways that must
    // agree: by the total, and by the gaps in the sequence numbers.
    lost_with = static_cast<std::size_t>(kFrames) - received;
    RC_CHECK_EQ(tracker.lost(), static_cast<std::size_t>(0));
    gap_us = reader.longest_gap_us();
    overflowed_with = port.overflowed();
  }

  // Without one: the same stall, then the main thread reads what the buffer kept.
  std::size_t lost_without = 0;
  std::size_t overflowed_without = 0;
  {
    SharedPort port(kBuffer);
    Device device(port, kFrames, kFramesPerSecond);
    std::this_thread::sleep_for(std::chrono::milliseconds(kStallMs));
    device.join();

    std::vector<std::uint8_t> storage(64, 0);
    rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e12);
    std::size_t delivered = 0;
    while (link.poll(0.0)) ++delivered;
    lost_without = static_cast<std::size_t>(kFrames) - delivered;
    overflowed_without = port.overflowed();
  }

  std::cout << "  a 1000 frame a second device, a 4096 byte buffer that lasts " << std::fixed
            << std::setprecision(0) << budget_ms << " ms,\n  and a main thread that does nothing for "
            << kStallMs << " ms:\n\n"
            << "    reading                      frames lost   bytes dropped\n"
            << "    on the main thread           " << std::setw(11) << lost_without << std::setw(16)
            << overflowed_without << "\n"
            << "    on a reader thread           " << std::setw(11) << lost_with << std::setw(16)
            << overflowed_with << "\n\n"
            << "  The reader's own longest gap between looks at the port was " << std::setprecision(2)
            << static_cast<double>(gap_us) / 1000.0 << " ms, against a budget of " << std::setprecision(0)
            << budget_ms << " ms.\n\n";

  RC_CHECK(lost_without > 200);
  RC_CHECK_EQ(lost_with, static_cast<std::size_t>(0));
  RC_CHECK_EQ(overflowed_with, static_cast<std::size_t>(0));
  RC_CHECK(static_cast<double>(gap_us) / 1000.0 < budget_ms);
}
