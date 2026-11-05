#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/fault_port.hpp>
#include <rc/io/link.hpp>

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr std::size_t kBuffer = 4096;
constexpr int kLoopMs = 10;          // a 100 Hz control loop
constexpr int kLoopOffsetMs = 3;     // not on the same instant as a frame
constexpr int kFrameEveryMs = 2;     // a 500 Hz device
constexpr int kWorkMs = 3;           // what acting on one message costs, of the ten a loop has
constexpr int kSeconds = 20;

std::vector<std::uint8_t> wire_frame(std::uint32_t stamp, std::uint16_t sequence, std::int16_t delta) {
  std::uint8_t body[8] = {};
  rc::io::ByteWriter writer{rc::span<std::uint8_t>(body, sizeof(body))};
  writer.put_u32(stamp);
  writer.put_u16(sequence);
  writer.put_u16(static_cast<std::uint16_t>(delta));

  std::vector<std::uint8_t> out{rc::io::Link::kStartByte, static_cast<std::uint8_t>(sizeof(body) + 1)};
  out.insert(out.end(), body, body + sizeof(body));
  out.push_back(rc::io::crc8(rc::span<const std::uint8_t>(body, sizeof(body))));
  return out;
}

struct Reading {
  std::uint32_t stamp = 0;
  std::uint16_t sequence = 0;
  std::int16_t delta = 0;
};

Reading decode(rc::span<const std::uint8_t> message) {
  Reading r;
  rc::io::ByteReader reader{message};
  std::uint16_t raw = 0;
  reader.get_u32(r.stamp);
  reader.get_u16(r.sequence);
  reader.get_u16(raw);
  r.delta = static_cast<std::int16_t>(raw);
  return r;
}

enum class Policy { kOnePerTick, kEveryOneThatFits, kNewest };

struct Result {
  long acted = 0;
  double mean_age = 0.0;
  double worst_age = 0.0;
  std::size_t overflowed = 0;
};

// A device at 500 frames a second, a loop at 100 Hz, and a driver buffer of
// 4096 bytes that keeps what it already holds and drops what arrives when full.
// Time is a counter, so the result is the same on every run.
Result run_policy(Policy policy) {
  BoundedPort port(kBuffer);
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e12);

  Result result;
  double total_age = 0.0;
  double next_frame = 0.0;
  std::uint16_t sequence = 0;

  const long total_ms = static_cast<long>(kSeconds) * 1000;
  for (long ms = 0; ms < total_ms; ++ms) {
    while (next_frame <= static_cast<double>(ms)) {
      const std::vector<std::uint8_t> f = wire_frame(static_cast<std::uint32_t>(ms), sequence++, 1);
      port.deliver(rc::span<const std::uint8_t>(f.data(), f.size()));
      next_frame += kFrameEveryMs;
    }
    if (ms % kLoopMs != kLoopOffsetMs) continue;

    const auto act = [&](rc::span<const std::uint8_t> message) {
      const Reading r = decode(message);
      const double age = static_cast<double>(ms) - static_cast<double>(r.stamp);
      total_age += age;
      if (age > result.worst_age) result.worst_age = age;
      ++result.acted;
    };

    if (policy == Policy::kOnePerTick) {
      if (link.poll(static_cast<double>(ms))) act(link.message());
    } else if (policy == Policy::kEveryOneThatFits) {
      // Handles each message that is waiting, but a tick has only so much time
      // in it and acting on one takes kWorkMs of it.
      for (int i = 0; i < kLoopMs / kWorkMs; ++i) {
        if (!link.poll(static_cast<double>(ms))) break;
        act(link.message());
      }
    } else {
      std::uint8_t newest[64] = {};
      const Drained d = drain_latest(link, static_cast<double>(ms),
                                     rc::span<std::uint8_t>(newest, sizeof(newest)), 1000);
      if (d.got) act(rc::span<const std::uint8_t>(newest, d.size));
    }
  }

  result.mean_age = result.acted == 0 ? 0.0 : total_age / static_cast<double>(result.acted);
  result.overflowed = port.overflowed();
  return result;
}

void feed_frames(rc::io::MemoryPort& port, int first_sequence, int count) {
  for (int i = 0; i < count; ++i) {
    const std::vector<std::uint8_t> f =
        wire_frame(0, static_cast<std::uint16_t>(first_sequence + i), 1);
    port.feed(rc::span<const std::uint8_t>(f.data(), f.size()));
  }
}

}  // namespace

RC_TEST("with several messages waiting, the newest is returned and the rest are counted") {
  rc::io::MemoryPort port;
  feed_frames(port, 1, 5);

  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);
  std::uint8_t out[64] = {};
  const Drained d = drain_latest(link, 1.0, rc::span<std::uint8_t>(out, sizeof(out)), 100);

  RC_REQUIRE(d.got);
  RC_CHECK_EQ(d.size, static_cast<std::size_t>(8));
  RC_CHECK_EQ(d.skipped, static_cast<std::size_t>(4));
  RC_CHECK(!d.more);
  RC_CHECK_EQ(static_cast<int>(decode(rc::span<const std::uint8_t>(out, d.size)).sequence), 5);
}

RC_TEST("nothing waiting is not an error, and it leaves the buffer alone") {
  rc::io::MemoryPort port;
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);

  std::uint8_t out[16];
  for (std::uint8_t& b : out) b = 0x5A;
  const Drained d = drain_latest(link, 1.0, rc::span<std::uint8_t>(out, sizeof(out)), 100);

  RC_CHECK(!d.got);
  RC_CHECK_EQ(d.size, static_cast<std::size_t>(0));
  RC_CHECK_EQ(d.skipped, static_cast<std::size_t>(0));
  RC_CHECK(!d.more);
  RC_CHECK_EQ(static_cast<int>(out[0]), 0x5A);
  RC_CHECK_EQ(static_cast<int>(out[15]), 0x5A);

  // And one message is neither skipped nor missing.
  feed_frames(port, 42, 1);
  const Drained one = drain_latest(link, 2.0, rc::span<std::uint8_t>(out, sizeof(out)), 100);
  RC_REQUIRE(one.got);
  RC_CHECK_EQ(one.skipped, static_cast<std::size_t>(0));
  RC_CHECK_EQ(static_cast<int>(decode(rc::span<const std::uint8_t>(out, one.size)).sequence), 42);
}

RC_TEST("a damaged frame in between is not a message and is not counted as one") {
  rc::io::MemoryPort port;
  feed_frames(port, 1, 2);
  std::vector<std::uint8_t> bad = wire_frame(0, 99, 1);
  bad[4] ^= 0xFF;   // the checksum will not match
  port.feed(rc::span<const std::uint8_t>(bad.data(), bad.size()));
  feed_frames(port, 3, 1);

  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);
  std::uint8_t out[64] = {};
  const Drained d = drain_latest(link, 1.0, rc::span<std::uint8_t>(out, sizeof(out)), 100);

  RC_REQUIRE(d.got);
  RC_CHECK_EQ(d.skipped, static_cast<std::size_t>(2));   // 1 and 2 were replaced, the damaged one never counted
  RC_CHECK_EQ(static_cast<int>(decode(rc::span<const std::uint8_t>(out, d.size)).sequence), 3);
}

RC_TEST("the limit stops the drain, says there is more, and the next call carries on") {
  rc::io::MemoryPort port;
  feed_frames(port, 1, 100);
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);
  std::uint8_t out[64] = {};

  const Drained first = drain_latest(link, 1.0, rc::span<std::uint8_t>(out, sizeof(out)), 10);
  RC_REQUIRE(first.got);
  RC_CHECK(first.more);
  RC_CHECK_EQ(first.skipped, static_cast<std::size_t>(9));
  RC_CHECK_EQ(static_cast<int>(decode(rc::span<const std::uint8_t>(out, first.size)).sequence), 10);

  const Drained second = drain_latest(link, 2.0, rc::span<std::uint8_t>(out, sizeof(out)), 10);
  RC_REQUIRE(second.got);
  RC_CHECK_EQ(static_cast<int>(decode(rc::span<const std::uint8_t>(out, second.size)).sequence), 20);

  const Drained none = drain_latest(link, 3.0, rc::span<std::uint8_t>(out, sizeof(out)), 0);
  RC_CHECK(!none.got);
  RC_CHECK(!none.more);   // a limit of nothing takes nothing
}

RC_TEST("a device that never stops cannot hold the loop") {
  // Two hundred thousand frames waiting stands in for a device faster than the
  // loop can ever drain. The call must return having taken only its limit.
  rc::io::MemoryPort port;
  for (int block = 0; block < 10; ++block) feed_frames(port, 0, 20000);

  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);
  std::uint8_t out[64] = {};
  const Drained d = drain_latest(link, 1.0, rc::span<std::uint8_t>(out, sizeof(out)), 64);

  RC_CHECK(d.got);
  RC_CHECK(d.more);
  RC_CHECK_EQ(d.skipped, static_cast<std::size_t>(63));
  RC_CHECK(port.pending() > 190000);   // almost all of it is still waiting
}

RC_TEST("a message too big for the buffer is not copied and cannot overrun it") {
  rc::io::MemoryPort port;
  feed_frames(port, 1, 1);

  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);

  // Room for four bytes, and the message is eight. A guard byte after it shows
  // whether anything was written past the end.
  std::uint8_t out[5] = {0, 0, 0, 0, 0xEE};
  const Drained d = drain_latest(link, 1.0, rc::span<std::uint8_t>(out, 4), 100);

  RC_CHECK(!d.got);
  RC_CHECK_EQ(d.oversized, static_cast<std::size_t>(1));
  RC_CHECK_EQ(static_cast<int>(out[4]), 0xEE);
}

RC_TEST("acting on one message a tick: the data the robot acts on is seconds old") {
  const Result one = run_policy(Policy::kOnePerTick);
  const Result every = run_policy(Policy::kEveryOneThatFits);
  const Result newest = run_policy(Policy::kNewest);

  const auto row = [](const char* what, const Result& r) {
    std::cout << "    " << std::left << std::setw(36) << what << std::right << std::setw(8) << r.acted
              << std::setw(12) << std::fixed << std::setprecision(1) << r.mean_age << std::setw(12)
              << r.worst_age << std::setw(14) << r.overflowed << "\n";
  };

  std::cout << "  a 500 Hz device, a 100 Hz loop, a 4096 byte buffer, " << kSeconds << " s, and what each\n"
            << "  message the loop acts on is worth: age is how long ago the device sent it.\n\n";
  std::cout << "    " << std::left << std::setw(36) << "policy" << std::right << std::setw(8) << "acted"
            << std::setw(12) << "mean age ms" << std::setw(12) << "worst ms" << std::setw(14)
            << "bytes lost" << "\n";
  row("one message per tick", one);
  row("every message, 3 ms each, 3 a tick", every);
  row("drain and act on the newest", newest);
  std::cout << "\n";

  // The simple loop falls behind and never recovers. The buffer fills, and a
  // full buffer keeps what it has, which is the oldest data.
  RC_CHECK(one.mean_age > 1000.0);
  RC_CHECK(one.worst_age > 3000.0);
  RC_CHECK(one.overflowed > 0);

  // Acting on everything helps and does not fix it: the loop can handle 300 a
  // second and the device sends 500.
  RC_CHECK(every.mean_age > 500.0);
  RC_CHECK(every.mean_age < one.mean_age);
  RC_CHECK(every.overflowed > 0);

  // The newest message is at most one frame old, and nothing is lost.
  RC_REQUIRE(newest.acted > 0);
  RC_CHECK(newest.worst_age <= static_cast<double>(kFrameEveryMs) + 1.0);
  RC_CHECK_EQ(newest.overflowed, static_cast<std::size_t>(0));
  RC_CHECK(newest.mean_age * 100.0 < one.mean_age);
}

RC_TEST("the newest is right for a state and wrong for a change") {
  // The same 1000 messages, each saying the value moved by one. Drained a
  // hundred at a time, the newest of each hundred is all the loop sees.
  rc::io::MemoryPort port;
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);

  long acted_sum = 0;
  long skipped = 0;
  std::uint8_t out[64] = {};
  for (int block = 0; block < 10; ++block) {
    feed_frames(port, block * 100, 100);
    const Drained d = drain_latest(link, static_cast<double>(block), rc::span<std::uint8_t>(out, sizeof(out)), 1000);
    RC_REQUIRE(d.got);
    acted_sum += decode(rc::span<const std::uint8_t>(out, d.size)).delta;
    skipped += static_cast<long>(d.skipped);
  }

  std::cout << "  1000 messages each saying +1, drained 100 at a time:\n"
            << "    a loop that integrates the newest of each drain sees " << acted_sum << "\n"
            << "    the messages it skipped carried " << skipped << "\n"
            << "  The truth is 1000. Position, temperature and battery are states and the newest is\n"
            << "  enough. An encoder count delta, a gyro increment and a step is a change, and every\n"
            << "  one of them has to be added.\n\n";

  RC_CHECK_EQ(acted_sum, 10L);
  RC_CHECK_EQ(skipped, 990L);
  RC_CHECK_EQ(acted_sum + skipped, 1000L);   // nothing is unaccounted for, it was just not used
}
