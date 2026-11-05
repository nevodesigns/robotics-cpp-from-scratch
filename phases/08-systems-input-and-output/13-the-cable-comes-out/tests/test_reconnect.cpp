#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/link.hpp>
#include <rc/io/sequence.hpp>

#include <cmath>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include "solution.hpp"

namespace {

// A cable. While it is plugged in, bytes from the device are waiting for the host.
// When it is pulled out they are lost, and any read or write through it fails.
struct Cable {
  bool plugged = true;
  std::deque<std::uint8_t> incoming;

  void from_device(rc::span<const std::uint8_t> bytes) {
    if (!plugged) return;
    for (std::size_t i = 0; i < bytes.size(); ++i) incoming.push_back(bytes[i]);
  }
};

class CablePort : public BytePort {
 public:
  explicit CablePort(Cable& cable) : cable_(cable) {}

  int read(rc::span<std::uint8_t> into) override {
    if (!cable_.plugged) return -1;
    std::size_t count = 0;
    while (count < into.size() && !cable_.incoming.empty()) {
      into[count++] = cable_.incoming.front();
      cable_.incoming.pop_front();
    }
    return static_cast<int>(count);
  }

  int write(rc::span<const std::uint8_t> from) override {
    if (!cable_.plugged) return -1;
    return static_cast<int>(from.size());
  }

 private:
  Cable& cable_;
};

class CableOpener : public PortOpener {
 public:
  explicit CableOpener(Cable& cable) : cable_(cable) {}

  // What to answer instead of looking at the cable, for the cases a cable
  // cannot produce on its own.
  void force(OpenStatus status) { forced_ = status; forcing_ = true; }

  std::unique_ptr<BytePort> open(OpenStatus& status) override {
    attempt_times.push_back(now);
    if (forcing_) {
      status = forced_;
      return nullptr;
    }
    if (!cable_.plugged) {
      status = OpenStatus::NotFound;
      return nullptr;
    }
    status = OpenStatus::Ok;
    return std::unique_ptr<BytePort>(new CablePort(cable_));
  }

  double now = 0.0;
  std::vector<double> attempt_times;

 private:
  Cable& cable_;
  OpenStatus forced_ = OpenStatus::Ok;
  bool forcing_ = false;
};

std::vector<std::uint8_t> wire_frame(std::uint8_t sequence) {
  std::uint8_t body[8] = {sequence, 1, 2, 3, 4, 5, 6, 7};
  std::vector<std::uint8_t> out{rc::io::Link::kStartByte, static_cast<std::uint8_t>(sizeof(body) + 1)};
  out.insert(out.end(), body, body + sizeof(body));
  out.push_back(rc::io::crc8(rc::span<const std::uint8_t>(body, sizeof(body))));
  return out;
}

// Runs a port with the cable out between two times, ticking every millisecond,
// and says how many times it tried and when it was connected again.
struct Wait {
  std::size_t attempts = 0;
  double reconnected_at = -1.0;
};

Wait unplugged_for(const BackoffPolicy& policy, double unplugged_until, double run_for) {
  Cable cable;
  cable.plugged = false;
  CableOpener opener(cable);
  ReconnectOptions options;
  options.backoff = policy;
  ReconnectingPort port(opener, options);

  Wait out;
  for (long ms = 0; ms <= static_cast<long>(run_for * 1000.0); ++ms) {
    const double now = static_cast<double>(ms) / 1000.0;
    if (now >= unplugged_until) cable.plugged = true;
    opener.now = now;
    port.tick(now);
    if (port.connected() && out.reconnected_at < 0.0) {
      out.reconnected_at = now;
      break;
    }
  }
  out.attempts = port.attempts();
  return out;
}

}  // namespace

RC_TEST("the wait doubles up to a cap, and never overflows however long the absence") {
  const BackoffPolicy policy;   // 10 ms, doubling, capped at 2 s
  const double expected[] = {0.01, 0.02, 0.04, 0.08, 0.16, 0.32, 0.64, 1.28, 2.0, 2.0};
  for (unsigned i = 0; i < 10; ++i) RC_CHECK_NEAR(policy.delay(i), expected[i], 1e-12);

  // A million failures in a row is a robot left in a cupboard. The answer is the
  // cap, not infinity and not a number that has wrapped.
  RC_CHECK_NEAR(policy.delay(1000000u), 2.0, 1e-12);

  BackoffPolicy flat;
  flat.initial = 0.5;
  flat.factor = 1.0;
  for (unsigned i = 0; i < 20; ++i) RC_CHECK_NEAR(flat.delay(i), 0.5, 1e-12);

  BackoffPolicy shrinking;
  shrinking.initial = 0.5;
  shrinking.factor = 0.5;   // a wait that gets shorter is not a backoff
  RC_CHECK_NEAR(shrinking.delay(5), 0.5, 1e-12);

  BackoffPolicy none;
  none.initial = 0.0;       // retrying with no wait is the problem this exists to prevent
  RC_CHECK(none.delay(0) >= 0.001);

  BackoffPolicy over;
  over.initial = 5.0;
  RC_CHECK_NEAR(over.delay(0), 2.0, 1e-12);   // the cap is a promise
}

RC_TEST("a port that is there is opened at the first tick and reads and writes through") {
  Cable cable;
  CableOpener opener(cable);
  ReconnectingPort port(opener);

  RC_CHECK(port.state() == LinkState::Waiting);
  port.tick(0.0);
  RC_REQUIRE(port.connected());
  RC_CHECK_EQ(port.generation(), 1u);
  RC_CHECK_EQ(port.attempts(), static_cast<std::size_t>(1));

  const std::uint8_t bytes[3] = {7, 8, 9};
  cable.from_device(rc::span<const std::uint8_t>(bytes, 3));
  std::uint8_t got[8] = {};
  RC_CHECK_EQ(port.read(rc::span<std::uint8_t>(got, sizeof(got))), 3);
  RC_CHECK_EQ(static_cast<int>(got[2]), 9);
  RC_CHECK_EQ(port.write(rc::span<const std::uint8_t>(bytes, 3)), 3);
}

RC_TEST("while the device is not there it is tried on the backoff schedule, and found as soon as it is") {
  Cable cable;
  cable.plugged = false;
  CableOpener opener(cable);
  ReconnectingPort port(opener);

  double connected_at = -1.0;
  for (long ms = 0; ms <= 600; ++ms) {
    const double now = static_cast<double>(ms) / 1000.0;
    if (now >= 0.2) cable.plugged = true;
    opener.now = now;
    port.tick(now);
    if (port.connected() && connected_at < 0.0) connected_at = now;
  }

  // Tries at 0, 10, 30, 70, 150 ms, which all fail, and at 310 ms, which does not.
  const double expected[] = {0.000, 0.010, 0.030, 0.070, 0.150, 0.310};
  RC_REQUIRE_EQ(opener.attempt_times.size(), static_cast<std::size_t>(6));
  for (std::size_t i = 0; i < 6; ++i) RC_CHECK_NEAR(opener.attempt_times[i], expected[i], 0.0015);
  RC_CHECK_NEAR(connected_at, 0.310, 0.0015);
  RC_CHECK(port.last_status() == OpenStatus::Ok);
  RC_CHECK_EQ(port.generation(), 1u);
}

RC_TEST("a cable pulled out is noticed once, then reads quiet and refuses writes, and comes back as a new generation") {
  Cable cable;
  CableOpener opener(cable);
  ReconnectingPort port(opener);
  port.tick(0.0);
  RC_REQUIRE(port.connected());

  cable.plugged = false;
  std::uint8_t got[8] = {};
  RC_CHECK_EQ(port.read(rc::span<std::uint8_t>(got, sizeof(got))), -1);   // the failure, once
  RC_CHECK(port.state() == LinkState::Waiting);
  RC_CHECK_EQ(port.disconnects(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(port.read(rc::span<std::uint8_t>(got, sizeof(got))), 0);    // and then a quiet link
  const std::uint8_t out[2] = {1, 2};
  RC_CHECK_EQ(port.write(rc::span<const std::uint8_t>(out, 2)), -1);      // which cannot send

  cable.plugged = true;
  for (long ms = 1; ms <= 500; ++ms) {
    opener.now = static_cast<double>(ms) / 1000.0;
    port.tick(opener.now);
  }
  RC_REQUIRE(port.connected());
  RC_CHECK_EQ(port.generation(), 2u);

  const std::uint8_t bytes[1] = {5};
  cable.from_device(rc::span<const std::uint8_t>(bytes, 1));
  RC_CHECK_EQ(port.read(rc::span<std::uint8_t>(got, sizeof(got))), 1);
}

RC_TEST("a mistake in the program is reported once and not retried for ever") {
  Cable cable;
  CableOpener opener(cable);
  opener.force(OpenStatus::Fatal);   // for instance a baud rate that does not exist
  ReconnectingPort port(opener);

  for (long ms = 0; ms <= 60000; ms += 10) {
    opener.now = static_cast<double>(ms) / 1000.0;
    port.tick(opener.now);
  }

  RC_CHECK(port.state() == LinkState::Failed);
  RC_CHECK(port.last_status() == OpenStatus::Fatal);
  RC_CHECK_EQ(port.attempts(), static_cast<std::size_t>(1));
}

RC_TEST("a device that is refused permission is tried a few times and then called a setting") {
  Cable cable;
  CableOpener opener(cable);
  opener.force(OpenStatus::PermissionDenied);
  ReconnectOptions options;
  options.permission_attempts = 5;
  ReconnectingPort port(opener, options);

  for (long ms = 0; ms <= 60000; ms += 10) {
    opener.now = static_cast<double>(ms) / 1000.0;
    port.tick(opener.now);
  }

  RC_CHECK(port.state() == LinkState::Failed);
  RC_CHECK(port.last_status() == OpenStatus::PermissionDenied);
  RC_CHECK_EQ(port.attempts(), static_cast<std::size_t>(5));
}

RC_TEST("a port that goes quiet is dropped and opened again, when a silence limit is set") {
  Cable cable;
  CableOpener opener(cable);
  ReconnectOptions options;
  options.silence_limit = 1.0;
  ReconnectingPort port(opener, options);

  std::uint8_t got[8] = {};
  bool saw_second_generation = false;
  for (long ms = 0; ms <= 3000; ++ms) {
    const double now = static_cast<double>(ms) / 1000.0;
    opener.now = now;
    port.tick(now);
    port.read(rc::span<std::uint8_t>(got, sizeof(got)));
    if (port.generation() == 2u) saw_second_generation = true;
  }
  RC_CHECK(port.silences() >= 1);
  RC_CHECK(saw_second_generation);

  // The same port with a byte every half second is never called quiet.
  Cable steady;
  CableOpener steady_opener(steady);
  ReconnectingPort steady_port(steady_opener, options);
  for (long ms = 0; ms <= 3000; ++ms) {
    const double now = static_cast<double>(ms) / 1000.0;
    steady_opener.now = now;
    steady_port.tick(now);
    if (ms % 500 == 0) {
      const std::uint8_t b = 1;
      steady.from_device(rc::span<const std::uint8_t>(&b, 1));
    }
    steady_port.read(rc::span<std::uint8_t>(got, sizeof(got)));
  }
  RC_CHECK_EQ(steady_port.silences(), static_cast<std::size_t>(0));
  RC_CHECK_EQ(steady_port.generation(), 1u);
}

RC_TEST("a Link over it sees a failed send while the cable is out and carries on when it is back") {
  Cable cable;
  CableOpener opener(cable);
  ReconnectingPort port(opener);
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);
  std::uint8_t scratch[32] = {};
  const std::uint8_t body[2] = {1, 2};

  port.tick(0.0);
  RC_CHECK(link.send(rc::span<const std::uint8_t>(body, 2), rc::span<std::uint8_t>(scratch, sizeof(scratch))));

  cable.plugged = false;
  link.poll(0.1);   // the read fails, once, and the port drops
  RC_CHECK(!link.send(rc::span<const std::uint8_t>(body, 2), rc::span<std::uint8_t>(scratch, sizeof(scratch))));
  RC_CHECK(link.port_errors() >= 1);
}

RC_TEST("how often to try: the backoff is patient while it is out and prompt once it is back") {
  BackoffPolicy every_ms;
  every_ms.initial = 0.001;
  every_ms.factor = 1.0;
  BackoffPolicy every_10ms;
  every_10ms.initial = 0.01;
  every_10ms.factor = 1.0;
  BackoffPolicy every_second;
  every_second.initial = 1.0;
  every_second.factor = 1.0;
  every_second.cap = 1.0;
  const BackoffPolicy doubling;   // 10 ms, doubling, capped at two seconds

  struct Summary {
    std::size_t tries = 0;        // for a cable that came back at 60.3 s
    double worst_wait = 0.0;      // the longest after it came back, over eleven different moments
  };

  const auto measure = [](const BackoffPolicy& policy) {
    Summary out;
    for (int step = 0; step <= 10; ++step) {
      const double back = 60.0 + static_cast<double>(step) * 0.1;
      const Wait w = unplugged_for(policy, back, 70.0);
      if (w.reconnected_at < 0.0) {
        out.worst_wait = 1e9;
        continue;
      }
      if (step == 3) out.tries = w.attempts;
      if (w.reconnected_at - back > out.worst_wait) out.worst_wait = w.reconnected_at - back;
    }
    return out;
  };

  const Summary a = measure(every_ms);
  const Summary b = measure(every_10ms);
  const Summary c = measure(every_second);
  const Summary d = measure(doubling);

  const auto row = [](const char* what, const Summary& s) {
    std::cout << "    " << std::left << std::setw(30) << what << std::right << std::setw(10) << s.tries
              << std::setw(18) << std::fixed << std::setprecision(0) << s.worst_wait * 1000.0 << "\n";
  };

  std::cout << "  the cable is out for about a minute. Tries made while it was out, and the longest it\n"
            << "  took to be open again once the cable was back, over eleven different moments:\n\n";
  std::cout << "    " << std::left << std::setw(30) << "wait between tries" << std::right << std::setw(10)
            << "tries" << std::setw(18) << "worst wait, ms" << "\n";
  row("every millisecond", a);
  row("every 10 ms", b);
  row("every second", c);
  row("10 ms doubling to 2 s", d);
  std::cout << "\n";

  // Retrying fast costs a try every tick for as long as the cable is out.
  RC_CHECK(a.tries > 50000);
  RC_CHECK(b.tries > 5000);

  // Doubling is as patient as the slow one while the cable is out: tens of tries.
  RC_CHECK(d.tries < 100);
  RC_CHECK(d.tries < c.tries);

  // And it is never later than its cap once the cable is back, which a fixed
  // second is, by construction, and a fast retry is by a millisecond or so.
  RC_CHECK(d.worst_wait <= 2.0 + 0.002);
  RC_CHECK(c.worst_wait <= 1.0 + 0.002);
  RC_CHECK(a.worst_wait <= 0.002);
  RC_CHECK(b.worst_wait <= 0.012);
  RC_CHECK(d.worst_wait > 0.5);   // the price of patience: up to the cap
}

RC_TEST("what has to start again on a new connection: a half frame, and a counter that restarted") {
  // The device sends a frame every ten milliseconds, numbered. At 105 ms the cable
  // comes out halfway through frame 10, and at 500 ms it goes back in. The device
  // was power cycled by being unplugged, so its counter starts again at 100.
  struct Result {
    int frames_after = 0;      // delivered after the reconnect
    int expected_after = 0;    // what the device sent after the reconnect
    std::size_t phantom_lost = 0;
  };

  const auto run = [](bool start_again) {
    Cable cable;
    CableOpener opener(cable);
    ReconnectingPort port(opener);
    std::vector<std::uint8_t> storage(64, 0);
    std::unique_ptr<rc::io::Link> link(new rc::io::Link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9));
    std::unique_ptr<rc::io::SequenceTracker<std::uint8_t>> tracker(new rc::io::SequenceTracker<std::uint8_t>());
    unsigned seen_generation = 0;
    Result out;

    std::uint8_t next_sequence = 0;
    bool restarted_device = false;
    for (long ms = 0; ms <= 900; ++ms) {
      const double now = static_cast<double>(ms) / 1000.0;
      opener.now = now;

      if (ms == 105) {
        // Half of frame 10 reaches the host, and then the cable is pulled.
        const std::vector<std::uint8_t> f = wire_frame(10);
        cable.from_device(rc::span<const std::uint8_t>(f.data(), 5));
        port.tick(now);
        link->poll(now);
        cable.plugged = false;
        continue;
      }
      if (ms == 500) cable.plugged = true;

      port.tick(now);

      if (start_again && port.generation() != seen_generation) {
        seen_generation = port.generation();
        link.reset(new rc::io::Link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9));
        tracker.reset(new rc::io::SequenceTracker<std::uint8_t>());
      }

      if (ms % 10 == 0 && ms < 100) {
        const std::vector<std::uint8_t> f = wire_frame(next_sequence++);
        cable.from_device(rc::span<const std::uint8_t>(f.data(), f.size()));
      }
      if (ms % 10 == 0 && ms >= 500 && port.connected()) {
        if (!restarted_device) {
          restarted_device = true;
          next_sequence = 100;
        }
        const std::vector<std::uint8_t> f = wire_frame(next_sequence++);
        cable.from_device(rc::span<const std::uint8_t>(f.data(), f.size()));
        ++out.expected_after;
      }

      while (link->poll(now)) {
        rc::io::ByteReader reader{link->message()};
        std::uint8_t sequence = 0;
        reader.get_u8(sequence);
        const std::size_t before = tracker->lost();
        tracker->accept(sequence);
        if (sequence >= 100) {
          ++out.frames_after;
          if (out.frames_after == 1) out.phantom_lost = tracker->lost() - before;
        }
      }
    }
    return out;
  };

  const Result kept = run(false);
  const Result fresh = run(true);

  std::cout << "  the cable is out halfway through a frame and comes back with the device restarted:\n\n"
            << "    on reconnecting             frames sent after   delivered   lost   phantom losses\n"
            << "    keep the parser and counter " << std::setw(13) << kept.expected_after << std::setw(14)
            << kept.frames_after << std::setw(7) << (kept.expected_after - kept.frames_after) << std::setw(12)
            << kept.phantom_lost << "\n"
            << "    start both again            " << std::setw(13) << fresh.expected_after << std::setw(14)
            << fresh.frames_after << std::setw(7) << (fresh.expected_after - fresh.frames_after) << std::setw(12)
            << fresh.phantom_lost << "\n\n"
            << "  The half frame still held by the old parser consumes the start of the first\n"
            << "  frame on the new connection, and a counter that restarted ahead of the old one\n"
            << "  is counted as a loss of the difference.\n\n";

  RC_CHECK(fresh.frames_after > 0);
  RC_CHECK_EQ(fresh.frames_after, fresh.expected_after);
  RC_CHECK_EQ(fresh.phantom_lost, static_cast<std::size_t>(0));

  RC_CHECK(kept.frames_after < kept.expected_after);
  RC_CHECK(kept.phantom_lost > 0);
}
