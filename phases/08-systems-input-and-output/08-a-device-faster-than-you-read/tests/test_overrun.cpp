#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/link.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr int kBaud = 115200;
constexpr std::size_t kBuffer = 4096;      // a common size for the receive buffer of a serial driver
constexpr int kFramesPerSecond = 500;
constexpr std::size_t kFrameBytes = 11;    // start, length, eight payload, checksum
constexpr int kSeconds = 20;

std::vector<std::uint8_t> wire_frame(std::uint16_t sequence) {
  std::uint8_t body[8] = {};
  rc::io::ByteWriter writer{rc::span<std::uint8_t>(body, sizeof(body))};
  writer.put_u16(sequence);
  for (int i = 2; i < 8; ++i) writer.put_u8(static_cast<std::uint8_t>(i));

  std::vector<std::uint8_t> out{rc::io::Link::kStartByte, static_cast<std::uint8_t>(sizeof(body) + 1)};
  out.insert(out.end(), body, body + sizeof(body));
  out.push_back(rc::io::crc8(rc::span<const std::uint8_t>(body, sizeof(body))));
  return out;
}

struct Run {
  long sent = 0;
  long delivered = 0;
  std::size_t overflowed_bytes = 0;
  std::size_t lost16 = 0;     // what a sixteen bit counter says was lost
  std::size_t lost8 = 0;      // what the low eight bits of the same counter say
};

// A device sending at a steady rate into a port with a fixed buffer, read by a
// loop that comes round every period_ms. Time is a counter, so the answer is
// the same on every run and on every machine.
Run run(int period_ms) {
  BoundedPort port(kBuffer);
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e12);
  SequenceTracker<std::uint16_t> wide;
  SequenceTracker<std::uint8_t> narrow;

  Run out;
  std::uint16_t sequence = 0;
  double next_frame = 0.0;

  const auto drain = [&](long now_ms) {
    while (link.poll(static_cast<double>(now_ms))) {
      rc::io::ByteReader reader{link.message()};
      std::uint16_t seen = 0;
      reader.get_u16(seen);
      if (!reader.ok()) continue;
      ++out.delivered;
      wide.accept(seen);
      narrow.accept(static_cast<std::uint8_t>(seen & 0xFF));
    }
  };

  const long total_ms = static_cast<long>(kSeconds) * 1000;
  for (long ms = 0; ms < total_ms; ++ms) {
    while (next_frame <= static_cast<double>(ms)) {
      const std::vector<std::uint8_t> f = wire_frame(sequence++);
      port.deliver(rc::span<const std::uint8_t>(f.data(), f.size()));
      ++out.sent;
      next_frame += 1000.0 / kFramesPerSecond;
    }
    if (ms % period_ms == 0) drain(ms);
  }

  // A loss at the very end shows only when a later message arrives, because a
  // sequence number can only reveal a gap behind it. Let a few more through, read
  // promptly, so the count is complete.
  for (long ms = total_ms; ms < total_ms + 200; ++ms) {
    while (next_frame <= static_cast<double>(ms)) {
      const std::vector<std::uint8_t> f = wire_frame(sequence++);
      port.deliver(rc::span<const std::uint8_t>(f.data(), f.size()));
      ++out.sent;
      next_frame += 1000.0 / kFramesPerSecond;
    }
    drain(ms);
  }

  out.overflowed_bytes = port.overflowed();
  out.lost16 = wide.lost();
  out.lost8 = narrow.lost();
  return out;
}

}  // namespace

RC_TEST("a byte on the wire is ten bits, so 115200 baud carries 11520 bytes a second") {
  RC_CHECK_NEAR(wire_bytes_per_second(115200), 11520.0, 1e-9);
  RC_CHECK_NEAR(wire_bytes_per_second(9600), 960.0, 1e-9);
  RC_CHECK_NEAR(wire_bytes_per_second(115200, 11), 115200.0 / 11.0, 1e-9);   // two stop bits
  RC_CHECK_NEAR(wire_bytes_per_second(0), 0.0, 1e-12);
  RC_CHECK_NEAR(wire_bytes_per_second(-5), 0.0, 1e-12);

  std::cout << "  115200 baud: 14400 if a byte were eight bits, " << std::fixed << std::setprecision(0)
            << wire_bytes_per_second(115200) << " because it is ten.\n\n";
}

RC_TEST("a full speed device fills a 4096 byte buffer in a third of a second") {
  const double full = stall_budget(kBuffer, wire_bytes_per_second(kBaud));
  const double half = stall_budget(kBuffer, wire_bytes_per_second(kBaud) / 2.0);

  std::cout << "  4096 bytes at 11520 bytes a second: " << std::fixed << std::setprecision(3) << full
            << " s from empty to full.\n\n";

  RC_CHECK_NEAR(full, 4096.0 / 11520.0, 1e-9);
  RC_CHECK_NEAR(half, 2.0 * full, 1e-9);
  RC_CHECK(stall_budget(kBuffer, 0.0) > 1e300);   // a silent device can be ignored for ever
}

RC_TEST("a sequence number in order reports nothing lost") {
  SequenceTracker<std::uint8_t> tracker;
  for (int i = 0; i < 50; ++i) {
    const Verdict v = tracker.accept(static_cast<std::uint8_t>(100 + i));
    RC_CHECK_EQ(v.lost, static_cast<std::size_t>(0));
    RC_CHECK(!v.duplicate);
    RC_CHECK(!v.late);
  }
  RC_CHECK_EQ(tracker.received(), static_cast<std::size_t>(50));
  RC_CHECK_EQ(tracker.lost(), static_cast<std::size_t>(0));

  // And it was really following the stream and not just reporting zero: one
  // message skipped straight afterwards is seen.
  const Verdict skipped = tracker.accept(151);   // 150 never arrived
  RC_CHECK_EQ(skipped.lost, static_cast<std::size_t>(1));
}

RC_TEST("a gap is counted as the messages missing from it") {
  SequenceTracker<std::uint8_t> tracker;
  tracker.accept(10);
  tracker.accept(11);
  const Verdict v = tracker.accept(17);   // 12 to 16 never arrived

  RC_CHECK_EQ(v.lost, static_cast<std::size_t>(5));
  RC_CHECK_EQ(tracker.lost(), static_cast<std::size_t>(5));

  const Verdict next = tracker.accept(18);   // and the stream carries on in step
  RC_CHECK_EQ(next.lost, static_cast<std::size_t>(0));
  RC_CHECK_EQ(tracker.lost(), static_cast<std::size_t>(5));
}

RC_TEST("the counter wrapping is not a loss, and a loss across the wrap is counted") {
  SequenceTracker<std::uint8_t> tracker;
  tracker.accept(254);
  tracker.accept(255);
  const Verdict wrapped = tracker.accept(0);
  tracker.accept(1);

  RC_CHECK_EQ(wrapped.lost, static_cast<std::size_t>(0));
  RC_CHECK(!wrapped.late);
  RC_CHECK_EQ(tracker.lost(), static_cast<std::size_t>(0));

  SequenceTracker<std::uint8_t> across;
  across.accept(253);
  const Verdict v = across.accept(2);   // 254, 255, 0 and 1 are missing
  RC_CHECK_EQ(v.lost, static_cast<std::size_t>(4));

  SequenceTracker<std::uint16_t> wide;
  wide.accept(65534);
  wide.accept(65535);
  wide.accept(0);
  RC_CHECK_EQ(wide.lost(), static_cast<std::size_t>(0));
}

RC_TEST("a repeat and a late arrival are named, and neither is a loss") {
  SequenceTracker<std::uint8_t> tracker;
  tracker.accept(20);
  tracker.accept(21);
  tracker.accept(22);

  const Verdict again = tracker.accept(22);    // the same one twice
  const Verdict early = tracker.accept(19);    // one from before
  const Verdict next = tracker.accept(23);     // and the stream goes on

  RC_CHECK(again.duplicate);
  RC_CHECK(!again.late);
  RC_CHECK(early.late);
  RC_CHECK(!early.duplicate);
  RC_CHECK_EQ(tracker.duplicates(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(tracker.late(), static_cast<std::size_t>(1));

  // Neither moved the expectation, so the next one in order is not a gap.
  RC_CHECK_EQ(next.lost, static_cast<std::size_t>(0));
  RC_CHECK_EQ(tracker.lost(), static_cast<std::size_t>(0));
}

RC_TEST("a counter can only see a gap shorter than half its range") {
  RC_CHECK_EQ(SequenceTracker<std::uint8_t>::widest_gap(), static_cast<std::size_t>(127));
  RC_CHECK_EQ(SequenceTracker<std::uint16_t>::widest_gap(), static_cast<std::size_t>(32767));

  SequenceTracker<std::uint8_t> seen;
  seen.accept(0);
  const Verdict wide_gap = seen.accept(128);   // 127 missing, the widest it can see
  RC_CHECK_EQ(wide_gap.lost, static_cast<std::size_t>(127));

  SequenceTracker<std::uint8_t> too_wide;
  too_wide.accept(0);
  const Verdict past = too_wide.accept(129);   // 128 missing, and it looks like going backwards
  RC_CHECK_EQ(past.lost, static_cast<std::size_t>(0));
  RC_CHECK(past.late);

  // Exactly one full turn of the counter is invisible.
  SequenceTracker<std::uint8_t> a_full_turn;
  a_full_turn.accept(5);
  const Verdict nothing = a_full_turn.accept(5);   // 255 missing, then the same number again
  RC_CHECK(nothing.duplicate);
  RC_CHECK_EQ(a_full_turn.lost(), static_cast<std::size_t>(0));

  SequenceTracker<std::uint16_t> wide;
  wide.accept(5);
  const Verdict seen_by_wide = wide.accept(static_cast<std::uint16_t>(5 + 256));
  RC_CHECK_EQ(seen_by_wide.lost, static_cast<std::size_t>(255));
}

RC_TEST("a reader that comes round slower than the buffer fills loses bytes, at the predicted rate") {
  const double rate = wire_bytes_per_second(kBaud);
  const double device_bytes = static_cast<double>(kFramesPerSecond) * static_cast<double>(kFrameBytes);
  const double budget = stall_budget(kBuffer, device_bytes);

  std::cout << "  a device sending " << kFramesPerSecond << " frames a second, " << kFrameBytes
            << " bytes each, into a " << kBuffer << " byte buffer, for " << kSeconds << " s.\n"
            << "  line speed " << std::fixed << std::setprecision(0) << rate << " bytes a second, device "
            << device_bytes << ", so the buffer holds " << std::setprecision(3) << budget << " s of it.\n\n";
  std::cout << "    read every   sent  delivered   lost   predicted   overflow bytes\n";

  const int periods[] = {100, 300, 700, 800, 1000, 2000};
  Run results[6];
  for (int i = 0; i < 6; ++i) {
    results[i] = run(periods[i]);

    // Each stall longer than the budget loses what the device sent after the
    // buffer filled. The run is kSeconds long, so it has that many stalls over.
    const double over = static_cast<double>(periods[i]) / 1000.0 - budget;
    const double predicted =
        over <= 0.0 ? 0.0 : over * kFramesPerSecond * (static_cast<double>(kSeconds) * 1000.0 / periods[i]);

    std::cout << "    " << std::setw(6) << periods[i] << " ms" << std::setw(8) << results[i].sent
              << std::setw(11) << results[i].delivered << std::setw(7) << (results[i].sent - results[i].delivered)
              << std::setw(12) << std::setprecision(0) << predicted << std::setw(16)
              << results[i].overflowed_bytes << "\n";
  }
  std::cout << "\n";

  // Inside the budget nothing is lost, and the sequence numbers agree.
  for (int i = 0; i < 3; ++i) {
    RC_CHECK_EQ(results[i].sent, results[i].delivered);
    RC_CHECK_EQ(results[i].overflowed_bytes, static_cast<std::size_t>(0));
    RC_CHECK_EQ(results[i].lost16, static_cast<std::size_t>(0));
  }

  // Past it, frames go, and the sixteen bit counter counts exactly the frames
  // that went: what was sent and did not arrive.
  for (int i = 3; i < 6; ++i) {
    const long lost = results[i].sent - results[i].delivered;
    RC_CHECK(lost > 0);
    RC_CHECK_EQ(results[i].lost16, static_cast<std::size_t>(lost));
    RC_CHECK(results[i].overflowed_bytes > 0);
  }

  // And the loss is about what the budget said it would be: the part of each
  // stall past the budget, times the rate the device sends.
  const double over_1000 = 1.0 - budget;
  const double predicted_1000 = over_1000 * kFramesPerSecond * (kSeconds * 1000.0 / 1000.0);
  const double actual_1000 = static_cast<double>(results[4].sent - results[4].delivered);
  RC_CHECK(actual_1000 > predicted_1000 * 0.8 && actual_1000 < predicted_1000 * 1.25);
}

RC_TEST("an eight bit counter reports a fraction of a long stall, a sixteen bit one all of it") {
  const Run long_stall = run(2000);
  const long truth = long_stall.sent - long_stall.delivered;

  std::cout << "  a reader that comes round every two seconds lost " << truth << " frames.\n"
            << "    sixteen bit counter says " << long_stall.lost16 << "\n"
            << "    eight bit counter says   " << long_stall.lost8 << "\n\n"
            << "  Each stall loses about 630 frames, and an eight bit counter can only say how\n"
            << "  many modulo 256, or nothing when the gap is past 127.\n\n";

  RC_CHECK_EQ(long_stall.lost16, static_cast<std::size_t>(truth));
  RC_CHECK(static_cast<long>(long_stall.lost8) < truth / 2);
}
