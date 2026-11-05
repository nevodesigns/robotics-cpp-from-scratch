#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/fault_port.hpp>
#include <rc/io/link.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr int kFrames = 50000;
constexpr std::uint32_t kSeed = 7;
constexpr std::size_t kWindow = 257;

std::vector<std::uint8_t> frame_of(const std::vector<std::uint8_t>& payload, Check check) {
  const std::size_t extra = check == Check::Crc16 ? 2 : 1;
  std::vector<std::uint8_t> out{0xAA, static_cast<std::uint8_t>(payload.size() + extra)};
  out.insert(out.end(), payload.begin(), payload.end());
  const rc::span<const std::uint8_t> bytes(payload.data(), payload.size());
  if (check == Check::Crc8) {
    out.push_back(rc::io::crc8(bytes));
  } else {
    const std::uint16_t crc = rc::io::crc16_ccitt(bytes);
    out.push_back(static_cast<std::uint8_t>(crc >> 8));
    out.push_back(static_cast<std::uint8_t>(crc & 0xFF));
  }
  return out;
}

// Eight bytes derived from a sequence number, so a frame that arrives wrong is
// recognisable as wrong.
std::vector<std::uint8_t> payload_for(std::uint16_t sequence) {
  std::uint8_t body[8] = {};
  rc::io::ByteWriter writer{rc::span<std::uint8_t>(body, sizeof(body))};
  writer.put_u16(sequence);
  writer.put_u16(static_cast<std::uint16_t>(sequence * 3u));
  writer.put_u32(0x11223344u * sequence);
  return std::vector<std::uint8_t>(body, body + sizeof(body));
}

bool content_is_right(rc::span<const std::uint8_t> message) {
  rc::io::ByteReader reader{message};
  std::uint16_t sequence = 0;
  std::uint16_t triple = 0;
  std::uint32_t mixed = 0;
  reader.get_u16(sequence);
  reader.get_u16(triple);
  reader.get_u32(mixed);
  return reader.ok() && message.size() == 8 && triple == static_cast<std::uint16_t>(sequence * 3u) &&
         mixed == 0x11223344u * sequence;
}

// Everything the parser delivers from a run of bytes, copied out because the
// parser's own view is only good until the next call.
std::vector<std::vector<std::uint8_t>> parse_all(RescanParser& parser,
                                                  const std::vector<std::uint8_t>& bytes) {
  std::vector<std::vector<std::uint8_t>> frames;
  for (const std::uint8_t byte : bytes) {
    bool ready = parser.push(byte);
    while (ready) {
      const rc::span<const std::uint8_t> view = parser.payload();
      frames.emplace_back(view.data(), view.data() + view.size());
      ready = parser.next();
    }
  }
  return frames;
}

struct Outcome {
  int delivered = 0;
  int wrong = 0;
  std::size_t dropped = 0;
  std::size_t checksum_failures = 0;
  double lost_per_drop = 0.0;
};

void tally(Outcome& out, rc::span<const std::uint8_t> message) {
  ++out.delivered;
  if (!content_is_right(message)) ++out.wrong;
}

void finish(Outcome& out, std::size_t dropped, std::size_t failures) {
  out.dropped = dropped;
  out.checksum_failures = failures;
  out.lost_per_drop =
      dropped == 0 ? 0.0 : static_cast<double>(kFrames - out.delivered) / static_cast<double>(dropped);
}

// The parser from lesson 08-02 inside the Link from 08-04, as the baseline.
Outcome run_old_link(std::uint32_t drop) {
  rc::io::MemoryPort device;
  for (int i = 0; i < kFrames; ++i) {
    const std::vector<std::uint8_t> f = frame_of(payload_for(static_cast<std::uint16_t>(i)), Check::Crc8);
    device.feed(rc::span<const std::uint8_t>(f.data(), f.size()));
  }
  rc::io::FaultOptions options;
  options.seed = kSeed;
  options.drop_per_million = drop;
  rc::io::FaultPort damaged(device, options);

  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(damaged, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e12);
  Outcome out;
  double now = 0.0;
  for (int polls = 0; polls < 200000 && device.pending() > 0; ++polls) {
    now += 1.0;
    while (link.poll(now)) tally(out, link.message());
  }
  while (link.poll(now + 1.0)) tally(out, link.message());
  finish(out, damaged.dropped(), link.checksum_failures());
  return out;
}

Outcome run_rescan(Check check, std::uint32_t drop) {
  rc::io::MemoryPort device;
  for (int i = 0; i < kFrames; ++i) {
    const std::vector<std::uint8_t> f = frame_of(payload_for(static_cast<std::uint16_t>(i)), check);
    device.feed(rc::span<const std::uint8_t>(f.data(), f.size()));
  }
  rc::io::FaultOptions options;
  options.seed = kSeed;
  options.drop_per_million = drop;
  rc::io::FaultPort damaged(device, options);

  std::vector<std::uint8_t> window(kWindow, 0);
  RescanParser parser(check, rc::span<std::uint8_t>(window.data(), window.size()));
  Outcome out;
  std::uint8_t byte = 0;
  for (long reads = 0; reads < 5000000; ++reads) {
    if (damaged.read(rc::span<std::uint8_t>(&byte, 1)) <= 0) {
      if (device.pending() == 0) break;
      continue;
    }
    bool ready = parser.push(byte);
    while (ready) {
      tally(out, parser.payload());
      ready = parser.next();
    }
  }
  finish(out, damaged.dropped(), parser.checksum_failures());
  return out;
}

}  // namespace

RC_TEST("a clean stream of frames comes out whole, and nothing is counted as damage") {
  std::vector<std::uint8_t> wire;
  for (int i = 0; i < 20; ++i) {
    const std::vector<std::uint8_t> f = frame_of(payload_for(static_cast<std::uint16_t>(i)), Check::Crc8);
    wire.insert(wire.end(), f.begin(), f.end());
  }
  std::vector<std::uint8_t> window(kWindow, 0);
  RescanParser parser(Check::Crc8, rc::span<std::uint8_t>(window.data(), window.size()));
  const auto frames = parse_all(parser, wire);

  RC_REQUIRE_EQ(frames.size(), static_cast<std::size_t>(20));
  RC_CHECK(frames[7] == payload_for(7));
  RC_CHECK(frames[19] == payload_for(19));
  RC_CHECK_EQ(parser.frames(), static_cast<std::size_t>(20));
  RC_CHECK_EQ(parser.checksum_failures(), static_cast<std::size_t>(0));
  RC_CHECK_EQ(parser.rescans(), static_cast<std::size_t>(0));
  RC_CHECK_EQ(parser.discarded(), static_cast<std::size_t>(0));
}

RC_TEST("a frame sent by the Link from the last phase is read by this parser") {
  rc::io::MemoryPort wire;
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(wire, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);
  const std::vector<std::uint8_t> body = payload_for(4242);
  std::uint8_t scratch[64] = {};
  RC_REQUIRE(link.send(rc::span<const std::uint8_t>(body.data(), body.size()),
                       rc::span<std::uint8_t>(scratch, sizeof(scratch))));

  std::vector<std::uint8_t> sent(wire.written().begin(), wire.written().end());
  std::vector<std::uint8_t> window(kWindow, 0);
  RescanParser parser(Check::Crc8, rc::span<std::uint8_t>(window.data(), window.size()));
  const auto frames = parse_all(parser, sent);

  RC_REQUIRE_EQ(frames.size(), static_cast<std::size_t>(1));
  RC_CHECK(frames[0] == body);
}

RC_TEST("a frame that lost a byte does not take the next frame with it") {
  // The case lesson 08-06 priced at nearly two frames. Frame A has a payload
  // byte missing, so it is one short, and frame B follows it whole.
  std::vector<std::uint8_t> a = frame_of(payload_for(1), Check::Crc8);
  a.erase(a.begin() + 5);
  const std::vector<std::uint8_t> b = frame_of(payload_for(2), Check::Crc8);
  const std::vector<std::uint8_t> c = frame_of(payload_for(3), Check::Crc8);

  std::vector<std::uint8_t> wire = a;
  wire.insert(wire.end(), b.begin(), b.end());
  wire.insert(wire.end(), c.begin(), c.end());

  std::vector<std::uint8_t> window(kWindow, 0);
  RescanParser parser(Check::Crc8, rc::span<std::uint8_t>(window.data(), window.size()));
  const auto frames = parse_all(parser, wire);

  std::cout << "  frame 1 with a byte missing, then frames 2 and 3 whole:\n"
            << "    delivered " << frames.size() << ", checksum failures " << parser.checksum_failures()
            << ", rescans " << parser.rescans() << "\n\n";

  RC_REQUIRE_EQ(frames.size(), static_cast<std::size_t>(2));
  RC_CHECK(frames[0] == payload_for(2));
  RC_CHECK(frames[1] == payload_for(3));
  RC_CHECK(parser.rescans() >= 1);

  // The parser this replaces, on the same bytes, for the contrast.
  rc::io::MemoryPort port;
  port.feed(rc::span<const std::uint8_t>(wire.data(), wire.size()));
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link old_link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);
  int old_delivered = 0;
  for (int i = 0; i < 10; ++i) {
    while (old_link.poll(static_cast<double>(i))) ++old_delivered;
  }
  RC_CHECK(old_delivered < static_cast<int>(frames.size()));
}

RC_TEST("two whole frames hidden inside one false claim are both found") {
  // A start byte and a length of 36, followed by two real frames that are
  // together shorter than that. The parser takes 38 bytes for a frame that
  // does not exist, fails the checksum, and must go back through what it took.
  const std::vector<std::uint8_t> x = frame_of(payload_for(10), Check::Crc8);
  const std::vector<std::uint8_t> y = frame_of(payload_for(11), Check::Crc8);
  std::vector<std::uint8_t> wire{0xAA, 36};
  wire.insert(wire.end(), x.begin(), x.end());
  wire.insert(wire.end(), y.begin(), y.end());
  while (wire.size() < 38) wire.push_back(0x00);   // the false frame is as long as it claimed

  std::vector<std::uint8_t> window(kWindow, 0);
  RescanParser parser(Check::Crc8, rc::span<std::uint8_t>(window.data(), window.size()));
  const auto frames = parse_all(parser, wire);

  RC_REQUIRE_EQ(frames.size(), static_cast<std::size_t>(2));
  RC_CHECK(frames[0] == payload_for(10));
  RC_CHECK(frames[1] == payload_for(11));
}

RC_TEST("a delivered frame can be read in place until the next byte arrives") {
  const std::vector<std::uint8_t> f = frame_of(payload_for(9), Check::Crc8);
  std::vector<std::uint8_t> window(kWindow, 0);
  RescanParser parser(Check::Crc8, rc::span<std::uint8_t>(window.data(), window.size()));

  bool ready = false;
  for (const std::uint8_t byte : f) ready = parser.push(byte);
  RC_REQUIRE(ready);

  const std::vector<std::uint8_t> expected = payload_for(9);
  RC_REQUIRE_EQ(parser.payload().size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) RC_CHECK_EQ(parser.payload()[i], expected[i]);
  RC_CHECK(!parser.next());   // nothing else is waiting inside
}

RC_TEST("a length that cannot be a frame is counted and does not stop the stream") {
  std::vector<std::uint8_t> wire;
  wire.push_back(0xAA);
  wire.push_back(0);                      // a frame with no room for its own checksum
  wire.push_back(0xAA);
  wire.push_back(200);                    // more than this window can hold
  const std::vector<std::uint8_t> good = frame_of(payload_for(5), Check::Crc8);
  wire.insert(wire.end(), good.begin(), good.end());

  std::vector<std::uint8_t> window(64, 0);   // holds a frame of at most 62 bytes after the start and length
  RescanParser parser(Check::Crc8, rc::span<std::uint8_t>(window.data(), window.size()));
  const auto frames = parse_all(parser, wire);

  RC_REQUIRE_EQ(frames.size(), static_cast<std::size_t>(1));
  RC_CHECK(frames[0] == payload_for(5));
  RC_CHECK(parser.malformed() >= 2);
}

RC_TEST("endless noise never overruns the window and never produces a frame") {
  std::vector<std::uint8_t> window(48, 0);
  RescanParser parser(Check::Crc8, rc::span<std::uint8_t>(window.data(), window.size()));

  // Every byte is a start byte, which is the worst input for a parser that
  // rescans: every position looks like the beginning of something.
  int delivered = 0;
  for (int i = 0; i < 5000; ++i) {
    if (parser.push(0xAA)) ++delivered;
  }
  for (int i = 0; i < 5000; ++i) {
    if (parser.push(static_cast<std::uint8_t>(i * 31))) ++delivered;
  }

  RC_CHECK_EQ(delivered, 0);
  RC_CHECK(parser.malformed() + parser.discarded() > 0);
}

RC_TEST("a sixteen bit check closes a frame the same way") {
  std::vector<std::uint8_t> wire;
  for (int i = 0; i < 5; ++i) {
    const std::vector<std::uint8_t> f = frame_of(payload_for(static_cast<std::uint16_t>(i)), Check::Crc16);
    wire.insert(wire.end(), f.begin(), f.end());
  }
  std::vector<std::uint8_t> window(kWindow, 0);
  RescanParser parser(Check::Crc16, rc::span<std::uint8_t>(window.data(), window.size()));
  const auto frames = parse_all(parser, wire);

  RC_REQUIRE_EQ(frames.size(), static_cast<std::size_t>(5));
  RC_CHECK(frames[3] == payload_for(3));
  RC_CHECK_EQ(frames[3].size(), static_cast<std::size_t>(8));   // the check is not part of the payload
}

RC_TEST("what rescanning buys: a dropped byte costs one frame, not two") {
  const Outcome old_light = run_old_link(2000);
  const Outcome rescan_light = run_rescan(Check::Crc8, 2000);
  const Outcome old_heavy = run_old_link(10000);
  const Outcome rescan_heavy = run_rescan(Check::Crc8, 10000);

  const auto row = [](const char* what, const Outcome& o) {
    std::cout << "    " << std::left << std::setw(26) << what << std::right << std::setw(7) << o.dropped
              << std::setw(11) << o.delivered << std::setw(8) << (kFrames - o.delivered) << std::setw(12)
              << std::fixed << std::setprecision(2) << o.lost_per_drop << std::setw(8) << o.wrong << "\n";
  };

  std::cout << "  " << kFrames << " frames through a port dropping bytes, CRC-8 throughout:\n\n";
  std::cout << "    " << std::left << std::setw(26) << "parser, bytes dropped" << std::right << std::setw(7)
            << "drops" << std::setw(11) << "delivered" << std::setw(8) << "lost" << std::setw(12)
            << "lost/drop" << std::setw(8) << "wrong" << "\n";
  row("old, 1 byte in 500", old_light);
  row("rescan, 1 byte in 500", rescan_light);
  row("old, 1 byte in 100", old_heavy);
  row("rescan, 1 byte in 100", rescan_heavy);
  std::cout << "\n";

  RC_CHECK(old_light.lost_per_drop > 1.6 && old_light.lost_per_drop < 2.2);
  RC_CHECK(old_heavy.lost_per_drop > 1.5 && old_heavy.lost_per_drop < 2.2);

  // One byte gone costs the frame it was in, and nothing else.
  RC_CHECK(rescan_light.lost_per_drop > 0.9 && rescan_light.lost_per_drop < 1.1);
  RC_CHECK(rescan_heavy.lost_per_drop > 0.85 && rescan_heavy.lost_per_drop < 1.1);

  // Rescanning gets the frames back. It does not remove the wrong ones, and
  // it can add a few, because it examines more candidate frames and each one is
  // another chance for a checksum to pass by luck.
  RC_CHECK(rescan_heavy.delivered > old_heavy.delivered + kFrames / 20);
  RC_CHECK(rescan_heavy.wrong >= 1);
}

RC_TEST("what a wider check buys: sixteen bits let no wrong frame through") {
  const Outcome crc8 = run_rescan(Check::Crc8, 10000);
  const Outcome crc16 = run_rescan(Check::Crc16, 10000);

  std::cout << "  the same rescanning parser, one byte in 100 dropped, " << kFrames << " frames:\n\n"
            << "    check     misaligned frames rejected   passed with the wrong content\n"
            << "    CRC-8     " << std::setw(26) << crc8.checksum_failures << std::setw(30) << crc8.wrong << "\n"
            << "    CRC-16    " << std::setw(26) << crc16.checksum_failures << std::setw(30) << crc16.wrong
            << "\n\n"
            << "  Eight bits let one in 256 through and sixteen let one in 65536. The price is one\n"
            << "  more byte on every frame, ten percent more on a ten byte frame.\n\n";

  RC_CHECK(crc8.wrong >= 1);
  RC_CHECK_EQ(crc16.wrong, 0);
  RC_CHECK(crc16.delivered > 0);
  RC_CHECK(crc16.checksum_failures > crc8.checksum_failures / 2);   // it rejected as much; it let nothing through
}
