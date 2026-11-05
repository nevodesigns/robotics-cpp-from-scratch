#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/link.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr int kFrames = 10000;
constexpr std::uint32_t kSeed = 7;

// One frame of the kind the rest of the phase sends: a sequence number and two
// values derived from it, so a frame that arrives wrong is recognisable as wrong.
std::vector<std::uint8_t> wire_frame(std::uint16_t sequence) {
  std::uint8_t body[8] = {};
  rc::io::ByteWriter writer{rc::span<std::uint8_t>(body, sizeof(body))};
  writer.put_u16(sequence);
  writer.put_u16(static_cast<std::uint16_t>(sequence * 3u));
  writer.put_u32(0x11223344u * sequence);

  std::vector<std::uint8_t> out{rc::io::Link::kStartByte, static_cast<std::uint8_t>(sizeof(body) + 1)};
  out.insert(out.end(), body, body + sizeof(body));
  out.push_back(rc::io::crc8(rc::span<const std::uint8_t>(body, sizeof(body))));
  return out;
}

bool content_is_right(rc::span<const std::uint8_t> message) {
  rc::io::ByteReader reader{message};
  std::uint16_t sequence = 0;
  std::uint16_t triple = 0;
  std::uint32_t mixed = 0;
  reader.get_u16(sequence);
  reader.get_u16(triple);
  reader.get_u32(mixed);
  return reader.ok() && triple == static_cast<std::uint16_t>(sequence * 3u) &&
         mixed == 0x11223344u * sequence;
}

struct Outcome {
  int delivered = 0;
  int wrong = 0;          // delivered, checksum passed, and the content is not what was sent
  std::size_t dropped = 0;
  std::size_t flipped = 0;
  std::size_t discarded = 0;
  std::size_t checksum_failures = 0;
  double lost_per_fault = 0.0;
};

// Sends kFrames frames through a damaged port into a Link and counts what
// arrives. Time is not involved, so the answer is the same on every run.
Outcome run_stream(std::uint32_t drop, std::uint32_t flip) {
  MemoryPort device;
  for (int i = 0; i < kFrames; ++i) {
    const std::vector<std::uint8_t> frame = wire_frame(static_cast<std::uint16_t>(i));
    device.feed(rc::span<const std::uint8_t>(frame.data(), frame.size()));
  }

  FaultOptions options;
  options.seed = kSeed;
  options.drop_per_million = drop;
  options.flip_per_million = flip;
  FaultPort damaged(device, options);

  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(damaged, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e12);

  Outcome out;
  double now = 0.0;

  // Bounded, so a port that never hands anything over, which is what an empty
  // implementation is, ends the loop with nothing delivered rather than
  // spinning until somebody notices.
  for (int polls = 0; polls < 50000 && device.pending() > 0; ++polls) {
    now += 1.0;
    while (link.poll(now)) {
      ++out.delivered;
      if (!content_is_right(link.message())) ++out.wrong;
    }
  }
  while (link.poll(now + 1.0)) {
    ++out.delivered;
    if (!content_is_right(link.message())) ++out.wrong;
  }

  out.dropped = damaged.dropped();
  out.flipped = damaged.flipped();
  out.discarded = link.discarded();
  out.checksum_failures = link.checksum_failures();
  const std::size_t faults = out.dropped + out.flipped;
  out.lost_per_fault =
      faults == 0 ? 0.0 : static_cast<double>(kFrames - out.delivered) / static_cast<double>(faults);
  return out;
}

int popcount8(std::uint8_t value) {
  int bits = 0;
  for (int i = 0; i < 8; ++i) bits += (value >> i) & 1;
  return bits;
}

std::vector<std::uint8_t> read_everything(BytePort& port, std::size_t room) {
  std::vector<std::uint8_t> out;
  std::vector<std::uint8_t> scratch(room, 0);
  for (int attempt = 0; attempt < 1000000; ++attempt) {
    const int got = port.read(rc::span<std::uint8_t>(scratch.data(), scratch.size()));
    if (got <= 0) break;
    out.insert(out.end(), scratch.begin(), scratch.begin() + got);
  }
  return out;
}

}  // namespace

RC_TEST("with no faults asked for, every byte goes through untouched and is counted") {
  MemoryPort device;
  std::vector<std::uint8_t> sent;
  for (int i = 0; i < 300; ++i) sent.push_back(static_cast<std::uint8_t>(i * 7));
  device.feed(rc::span<const std::uint8_t>(sent.data(), sent.size()));

  FaultPort port(device, FaultOptions{});
  const std::vector<std::uint8_t> got = read_everything(port, 64);

  RC_CHECK(got == sent);
  RC_CHECK_EQ(port.seen(), static_cast<std::size_t>(300));
  RC_CHECK_EQ(port.dropped(), static_cast<std::size_t>(0));
  RC_CHECK_EQ(port.flipped(), static_cast<std::size_t>(0));

  const std::uint8_t out[3] = {9, 8, 7};
  RC_CHECK_EQ(port.write(rc::span<const std::uint8_t>(out, 3)), 3);
  RC_REQUIRE_EQ(device.written().size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(static_cast<int>(device.written()[2]), 7);
}

RC_TEST("bytes are dropped about as often as asked, and every byte is accounted for") {
  MemoryPort device;
  const std::vector<std::uint8_t> sent(200000, 0x42);
  device.feed(rc::span<const std::uint8_t>(sent.data(), sent.size()));

  FaultOptions options;
  options.seed = kSeed;
  options.drop_per_million = 10000;   // one byte in a hundred
  FaultPort port(device, options);
  const std::vector<std::uint8_t> got = read_everything(port, 256);

  std::cout << "  200000 bytes, one in a hundred dropped: " << port.dropped() << " dropped, "
            << got.size() << " delivered\n\n";

  RC_CHECK(port.dropped() > 1700 && port.dropped() < 2300);
  RC_CHECK_EQ(got.size() + port.dropped(), sent.size());
  RC_CHECK_EQ(port.seen(), sent.size());
}

RC_TEST("a flip changes exactly one bit of exactly one byte") {
  MemoryPort device;
  const std::vector<std::uint8_t> sent(100000, 0x55);
  device.feed(rc::span<const std::uint8_t>(sent.data(), sent.size()));

  FaultOptions options;
  options.seed = kSeed;
  options.flip_per_million = 20000;
  FaultPort port(device, options);
  const std::vector<std::uint8_t> got = read_everything(port, 256);

  RC_REQUIRE_EQ(got.size(), sent.size());   // a flip damages a byte, it does not remove one
  std::size_t changed = 0;
  bool every_change_is_one_bit = true;
  for (std::size_t i = 0; i < got.size(); ++i) {
    if (got[i] == sent[i]) continue;
    ++changed;
    if (popcount8(static_cast<std::uint8_t>(got[i] ^ sent[i])) != 1) every_change_is_one_bit = false;
  }

  RC_CHECK(port.flipped() > 0);
  RC_CHECK_EQ(changed, port.flipped());
  RC_CHECK(every_change_is_one_bit);
}

RC_TEST("the same seed gives the same faults and another seed gives different ones") {
  const auto damage = [](std::uint32_t seed) {
    MemoryPort device;
    const std::vector<std::uint8_t> sent(20000, 0x10);
    device.feed(rc::span<const std::uint8_t>(sent.data(), sent.size()));
    FaultOptions options;
    options.seed = seed;
    options.drop_per_million = 20000;
    options.flip_per_million = 20000;
    FaultPort port(device, options);
    return read_everything(port, 128);
  };

  const std::vector<std::uint8_t> first = damage(5);
  const std::vector<std::uint8_t> again = damage(5);
  const std::vector<std::uint8_t> other = damage(6);

  RC_CHECK(first == again);
  RC_CHECK(first != other);
  RC_CHECK(first.size() < 20000);   // and something really was dropped
}

RC_TEST("a read limit splits what arrives and loses none of it") {
  MemoryPort device;
  std::vector<std::uint8_t> sent;
  for (int i = 0; i < 100; ++i) sent.push_back(static_cast<std::uint8_t>(i));
  device.feed(rc::span<const std::uint8_t>(sent.data(), sent.size()));

  FaultOptions options;
  options.read_chunk = 3;
  FaultPort port(device, options);

  std::vector<std::uint8_t> got;
  std::size_t largest = 0;
  std::uint8_t scratch[64] = {};
  for (int attempt = 0; attempt < 1000; ++attempt) {
    const int count = port.read(rc::span<std::uint8_t>(scratch, sizeof(scratch)));
    if (count <= 0) break;
    if (static_cast<std::size_t>(count) > largest) largest = static_cast<std::size_t>(count);
    got.insert(got.end(), scratch, scratch + count);
  }

  RC_CHECK_EQ(largest, static_cast<std::size_t>(3));
  RC_CHECK(got == sent);
}

RC_TEST("a write limit makes a short write, and the return value is how you find out") {
  MemoryPort device;
  FaultOptions options;
  options.write_chunk = 4;
  FaultPort port(device, options);

  const std::uint8_t out[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  const int accepted = port.write(rc::span<const std::uint8_t>(out, sizeof(out)));

  RC_CHECK_EQ(accepted, 4);
  RC_REQUIRE_EQ(device.written().size(), static_cast<std::size_t>(4));
  RC_CHECK_EQ(static_cast<int>(device.written()[3]), 4);
}

RC_TEST("what a fault costs: a dropped byte loses nearly two frames and a flipped bit loses one") {
  const Outcome clean = run_stream(0, 0);
  const Outcome light_drop = run_stream(2000, 0);
  const Outcome light_flip = run_stream(0, 2000);
  const Outcome heavy_drop = run_stream(10000, 0);
  const Outcome heavy_flip = run_stream(0, 10000);

  const auto row = [](const char* what, const Outcome& o) {
    std::cout << "    " << std::left << std::setw(24) << what << std::right << std::setw(7) << o.dropped
              << std::setw(8) << o.flipped << std::setw(11) << o.delivered << std::setw(8)
              << (kFrames - o.delivered) << std::setw(14) << std::fixed << std::setprecision(2)
              << o.lost_per_fault << std::setw(8) << o.wrong << "\n";
  };

  std::cout << "  " << kFrames << " frames of ten bytes through the Link from the last lesson:\n\n";
  std::cout << "    " << std::left << std::setw(24) << "damage" << std::right << std::setw(7) << "drops"
            << std::setw(8) << "flips" << std::setw(11) << "delivered" << std::setw(8) << "lost"
            << std::setw(14) << "lost/fault" << std::setw(8) << "wrong" << "\n";
  row("none", clean);
  row("1 byte in 500 dropped", light_drop);
  row("1 bit in 500 flipped", light_flip);
  row("1 byte in 100 dropped", heavy_drop);
  row("1 bit in 100 flipped", heavy_flip);
  std::cout << "\n";

  RC_CHECK_EQ(clean.delivered, kFrames);
  RC_CHECK_EQ(clean.wrong, 0);

  // The damage was really done, and every frame is accounted for as delivered
  // or lost: nothing is delivered that was not sent.
  RC_CHECK(light_drop.dropped > 0);
  RC_CHECK(light_flip.flipped > 0);
  RC_CHECK(heavy_drop.delivered <= kFrames);

  // A dropped byte costs nearly two frames, not one. The frame it was in is
  // short, and the parser, which was promised a length, goes on taking bytes to
  // make it up from the start of the next frame. It is under two because a drop
  // in the start byte costs only one, and because at the heavier rate two drops
  // sometimes land in the same pair of frames.
  RC_CHECK(light_drop.lost_per_fault > 1.6 && light_drop.lost_per_fault < 2.2);
  RC_CHECK(heavy_drop.lost_per_fault > 1.5 && heavy_drop.lost_per_fault < 2.2);

  // A flipped bit costs one, because the frame stays the length it was and the
  // checksum rejects it cleanly.
  RC_CHECK(light_flip.lost_per_fault > 0.9 && light_flip.lost_per_fault < 1.2);
  RC_CHECK(heavy_flip.lost_per_fault > 0.9 && heavy_flip.lost_per_fault < 1.2);
}

RC_TEST("a checksum that passes is not proof the frame is right") {
  const Outcome heavy_drop = run_stream(10000, 0);
  const Outcome heavy_flip = run_stream(0, 10000);

  // Every misaligned frame that reaches the checksum is, to the checksum, a
  // random byte compared with a random byte: one chance in 256 of passing.
  const double expected_wrong = static_cast<double>(heavy_drop.checksum_failures + heavy_drop.wrong) / 256.0;

  std::cout << "  at one dropped byte in a hundred, " << heavy_drop.checksum_failures
            << " misaligned frames failed the checksum and " << heavy_drop.wrong << " passed it,\n"
            << "  with the wrong content. One in 256 passes by luck, which predicts "
            << std::fixed << std::setprecision(1) << expected_wrong << ".\n"
            << "  A flipped bit is different: CRC-8 catches every single bit error, and "
            << heavy_flip.wrong << " of " << heavy_flip.flipped << " got through.\n\n";

  RC_CHECK(heavy_drop.delivered > 0);
  RC_CHECK(heavy_drop.wrong >= 1);
  RC_CHECK(heavy_drop.wrong * 100 < heavy_drop.delivered);   // rare, which is why it goes unnoticed
  RC_CHECK(static_cast<double>(heavy_drop.wrong) < expected_wrong * 3.0 + 3.0);
  RC_CHECK_EQ(heavy_flip.wrong, 0);
}
