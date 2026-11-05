#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/fault_port.hpp>
#include <rc/io/serial.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "solution.hpp"

#if !defined(_WIN32)
#  include <cstdlib>
#  include <fcntl.h>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace {

constexpr double kHz = 100.0;
constexpr double kMinLatency = 0.002;

std::vector<std::uint8_t> move_by(int delta) {
  std::vector<std::uint8_t> out(2, 0);
  rc::io::ByteWriter writer{rc::span<std::uint8_t>(out.data(), out.size())};
  writer.put_u16(static_cast<std::uint16_t>(static_cast<std::int16_t>(delta)));
  return out;
}

// The host's end of a SimDevice's cable, optionally made hostile.
class HostEnd : public rc::io::BytePort {
 public:
  HostEnd(rc::io::SimDevice& device, const rc::io::FaultOptions& faults)
      : plain_(device), damaged_(plain_, faults) {}

  int read(rc::span<std::uint8_t> into) override { return damaged_.read(into); }
  int write(rc::span<const std::uint8_t> from) override { return damaged_.write(from); }

 private:
  rc::io::SimDevicePort plain_;
  rc::io::FaultPort damaged_;
};

class DeviceOpener : public rc::io::PortOpener {
 public:
  DeviceOpener(rc::io::SimDevice& device, const rc::io::FaultOptions& faults)
      : device_(device), faults_(faults) {}

  std::unique_ptr<rc::io::BytePort> open(rc::io::OpenStatus& status) override {
    if (!device_.connected()) {
      status = rc::io::OpenStatus::NotFound;
      return nullptr;
    }
    status = rc::io::OpenStatus::Ok;
    return std::unique_ptr<rc::io::BytePort>(new HostEnd(device_, faults_));
  }

 private:
  rc::io::SimDevice& device_;
  rc::io::FaultOptions faults_;
};

// A device, a cable, and a Session on the host, stepped together a millisecond at
// a time on a clock that is a counter.
struct Rig {
  explicit Rig(const rc::io::SimDeviceOptions& options = rc::io::SimDeviceOptions{},
               const rc::io::FaultOptions& faults = rc::io::FaultOptions{})
      : device(options), opener(device, faults), session(opener) {}

  void step() {
    now += 0.001;
    device.step(now);
    session.tick(now);
  }

  void run(double seconds) {
    const long steps = static_cast<long>(seconds * 1000.0);
    for (long i = 0; i < steps; ++i) step();
  }

  rc::io::SimDevice device;
  DeviceOpener opener;
  Session session;
  double now = 0.0;
};

}  // namespace

RC_TEST("with nothing connected a session is quiet, says so, and refuses to ask") {
  rc::io::SimDevice device;
  device.set_connected(false);
  DeviceOpener opener(device, rc::io::FaultOptions{});
  Session session(opener);

  for (int i = 0; i < 2000; ++i) {
    device.step(i * 0.001);
    session.tick(i * 0.001);
  }

  Telemetry t;
  RC_CHECK(!session.latest(t));
  const Health h = session.health(2.0);
  RC_CHECK(!h.connected);
  RC_CHECK(h.stale);
  RC_CHECK_EQ(h.frames, static_cast<std::size_t>(0));
  RC_CHECK(!session.ask(rc::io::kOpPing, rc::span<const std::uint8_t>(), 2.0));
  RC_CHECK(session.request_state() == rc::io::RequestState::Idle);

  // And when the device appears, the session finds it and the conversation begins,
  // with nobody having told it to.
  device.set_connected(true);
  for (int i = 2000; i < 6000; ++i) {
    device.step(i * 0.001);
    session.tick(i * 0.001);
  }
  const Health found = session.health(6.0);
  RC_CHECK(found.connected);
  RC_CHECK(!found.stale);
  RC_CHECK(found.frames > 100);
  RC_CHECK(session.latest(t));
}

RC_TEST("a clean conversation: every frame, in order, with the device's time on our clock") {
  Rig rig;
  rig.run(120.0);

  Telemetry t;
  RC_REQUIRE(rig.session.latest(t));
  const Health h = rig.session.health(rig.now);

  // The device's own schedule is known to the test: frame k was sent at k / 100 s,
  // and the least a message takes is kMinLatency, which is what a clock map can know.
  const double truth = static_cast<double>(t.sequence) / kHz + kMinLatency;
  const double clock_error = std::fabs(t.sent_at - truth);

  std::cout << "  two minutes, a 100 Hz device, a 50 ppm fast clock, jittery link:\n"
            << "    delivered " << h.frames << ", lost " << h.lost << ", checksum failures "
            << h.checksum_failures << ", clock drift found " << std::fixed << std::setprecision(1)
            << h.drift_ppm << " ppm\n"
            << "    the newest frame, number " << t.sequence << ", is placed on our clock to within "
            << std::setprecision(2) << clock_error * 1000.0 << " ms\n\n";

  RC_CHECK(h.connected);
  RC_CHECK(!h.stale);
  RC_CHECK(h.frames >= 11990 && h.frames <= 12001);
  RC_CHECK_EQ(h.lost, static_cast<std::size_t>(0));
  RC_CHECK_EQ(h.checksum_failures, static_cast<std::size_t>(0));
  RC_CHECK_EQ(h.reconnects, static_cast<std::size_t>(0));
  RC_CHECK(clock_error < 0.003);
  RC_CHECK_NEAR(h.drift_ppm, 50.0, 15.0);
}

RC_TEST("requests and telemetry share one link and neither takes the other's messages") {
  Rig rig;
  rig.run(1.0);

  int done = 0;
  for (int i = 0; i < 50; ++i) {
    const std::vector<std::uint8_t> args = move_by(1);
    RC_REQUIRE(rig.session.ask(rc::io::kOpMove, rc::span<const std::uint8_t>(args.data(), args.size()), rig.now));
    for (int guard = 0; guard < 500 && rig.session.request_state() == rc::io::RequestState::Waiting; ++guard) {
      rig.step();
    }
    if (rig.session.request_state() == rc::io::RequestState::Done) ++done;
  }
  rig.run(0.5);

  const Health h = rig.session.health(rig.now);
  RC_CHECK_EQ(done, 50);
  RC_CHECK_EQ(rig.device.position(), 50L);
  RC_CHECK_EQ(h.request_timeouts, static_cast<std::size_t>(0));
  RC_CHECK_EQ(h.lost, static_cast<std::size_t>(0));            // the requests did not cost a single telemetry frame
  RC_CHECK(h.frames > 150);
}

RC_TEST("a request that waits by polling the link itself throws telemetry away, and a session does not") {
  // The same fifty requests, made by a loop that waits for its answer by polling
  // the link and discarding everything that is not it.
  rc::io::SimDevice device;
  rc::io::SimDevicePort port(device);
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e12);
  rc::io::SequenceTracker<std::uint16_t> tracker;
  std::size_t delivered = 0;
  std::uint8_t scratch[64] = {};
  double now = 0.0;

  const auto take_telemetry = [&](rc::span<const std::uint8_t> m) {
    if (m.size() == 9 && m[0] == rc::io::kTelemetryType) {
      rc::io::ByteReader reader{rc::span<const std::uint8_t>(m.data() + 1, 2)};
      std::uint16_t sequence = 0;
      reader.get_u16(sequence);
      tracker.accept(sequence);
      ++delivered;
    }
  };

  const auto step = [&]() {
    now += 0.001;
    device.step(now);
    while (link.poll(now)) take_telemetry(link.message());
  };
  for (int i = 0; i < 1000; ++i) step();

  std::uint8_t id = 0;
  for (int request = 0; request < 50; ++request) {
    ++id;
    const std::vector<std::uint8_t> args = move_by(1);
    std::vector<std::uint8_t> body{rc::io::kRequestType, id, rc::io::kOpMove, args[0], args[1]};
    link.send(rc::span<const std::uint8_t>(body.data(), body.size()), rc::span<std::uint8_t>(scratch, sizeof(scratch)));

    // The wait: poll until a response with this id turns up, and discard the rest.
    for (int guard = 0; guard < 500; ++guard) {
      now += 0.001;
      device.step(now);
      bool answered = false;
      while (link.poll(now)) {
        const rc::span<const std::uint8_t> m = link.message();
        if (m.size() >= 2 && m[0] == rc::io::kResponseType && m[1] == id) answered = true;
      }
      if (answered) break;
    }
    for (int i = 0; i < 100; ++i) step();
  }
  for (int i = 0; i < 500; ++i) step();

  Rig rig;
  rig.run(1.0);
  for (int i = 0; i < 50; ++i) {
    const std::vector<std::uint8_t> args = move_by(1);
    RC_REQUIRE(rig.session.ask(rc::io::kOpMove, rc::span<const std::uint8_t>(args.data(), args.size()), rig.now));
    for (int guard = 0; guard < 500 && rig.session.request_state() == rc::io::RequestState::Waiting; ++guard) rig.step();
    rig.run(0.1);
  }
  rig.run(0.5);
  const Health h = rig.session.health(rig.now);

  std::cout << "  fifty requests made while a 100 Hz telemetry stream runs on the same link:\n\n"
            << "    waiting by polling the link and discarding the rest    telemetry lost " << std::setw(4)
            << tracker.lost() << "\n"
            << "    a session, which routes each message to who wants it   telemetry lost " << std::setw(4)
            << h.lost << "\n\n";

  RC_CHECK(tracker.lost() > 20);
  RC_CHECK_EQ(h.lost, static_cast<std::size_t>(0));
}

RC_TEST("the answer carries where the device is now") {
  Rig rig;
  rig.run(0.5);
  const std::vector<std::uint8_t> args = move_by(7);
  RC_REQUIRE(rig.session.ask(rc::io::kOpMove, rc::span<const std::uint8_t>(args.data(), args.size()), rig.now));
  for (int guard = 0; guard < 500 && rig.session.request_state() == rc::io::RequestState::Waiting; ++guard) rig.step();

  RC_REQUIRE(rig.session.request_state() == rc::io::RequestState::Done);
  RC_REQUIRE_EQ(rig.session.response().size(), static_cast<std::size_t>(5));
  rc::io::ByteReader reader{rc::span<const std::uint8_t>(rig.session.response().data() + 1, 4)};
  std::uint32_t position = 0;
  reader.get_u32(position);
  RC_CHECK_EQ(static_cast<int>(position), 7);
}

RC_TEST("a request in flight when the cable comes out is abandoned, and counted") {
  rc::io::SimDeviceOptions options;
  options.late_every = 1;     // every answer is slow, so the cable can come out while it waits
  options.late_by = 0.2;
  Rig rig(options);
  rig.run(0.5);

  const std::vector<std::uint8_t> args = move_by(1);
  RC_REQUIRE(rig.session.ask(rc::io::kOpMove, rc::span<const std::uint8_t>(args.data(), args.size()), rig.now));
  rig.run(0.02);
  RC_REQUIRE(rig.session.request_state() == rc::io::RequestState::Waiting);

  rig.device.set_connected(false);
  rig.run(1.0);
  rig.device.reboot(rig.now);
  rig.device.set_connected(true);
  rig.run(3.0);

  const Health h = rig.session.health(rig.now);
  RC_CHECK(h.connected);
  RC_CHECK_EQ(h.reconnects, static_cast<std::size_t>(1));
  RC_CHECK_EQ(h.requests_abandoned, static_cast<std::size_t>(1));
  RC_CHECK(rig.session.request_state() == rc::io::RequestState::Idle);
}

RC_TEST("a new connection starts the clock map over, so a device that was off does not put time out by the downtime") {
  Rig rig;
  rig.run(30.0);
  rig.device.set_connected(false);
  rig.run(5.0);                      // the device is off for five seconds
  rig.device.reboot(rig.now);
  rig.device.set_connected(true);
  const double rebooted_at = rig.now;

  double worst = 0.0;
  for (int i = 0; i < 20000; ++i) {
    rig.step();
    Telemetry t;
    if (rig.session.latest(t) && t.arrived_at > rebooted_at) {
      // After a reboot the device counts from zero again, and sent the frame
      // numbered k at (k + 1) / 100 seconds after it came back. Frames from before
      // the cable came out are not what is being judged.
      const double truth = rebooted_at + (static_cast<double>(t.sequence) + 1.0) / kHz + kMinLatency;
      worst = std::max(worst, std::fabs(t.sent_at - truth));
    }
  }

  std::cout << "  five seconds off and a reboot at thirty: the worst error placing a frame on our clock\n"
            << "  over the next twenty seconds was " << std::fixed << std::setprecision(1) << worst * 1000.0
            << " ms. Lesson 08-12 measured a map that was kept from before at 5011 ms.\n\n";

  RC_CHECK(rig.session.health(rig.now).reconnects >= 1);
  RC_CHECK(worst < 0.02);
}

RC_TEST("a hostile minute: loss, noise, late answers, an unplug and a power cycle, and a robot that is right at the end") {
  rc::io::SimDeviceOptions options;
  options.late_every = 10;
  options.drop_every = 15;
  rc::io::FaultOptions faults;
  faults.seed = 7;
  faults.drop_per_million = 400;
  faults.flip_per_million = 400;
  Rig rig(options, faults);

  int done_after_reboot = 0;
  bool counted = true;           // the answer to the last request, counted once
  bool rebooted = false;
  double rebooted_at = 0.0;
  double worst_clock = 0.0;
  std::size_t timeouts_at_reboot = 0;
  std::size_t abandoned_at_reboot = 0;
  const long steps = 60000;

  for (long i = 0; i < steps; ++i) {
    rig.step();

    if (i == 20000) rig.device.set_connected(false);
    if (i == 23000) {
      const Health before = rig.session.health(rig.now);
      timeouts_at_reboot = before.request_timeouts;
      abandoned_at_reboot = before.requests_abandoned;
      rig.device.reboot(rig.now);
      rebooted_at = rig.now;
      rebooted = true;
      rig.device.set_connected(true);
    }

    // An answer is counted when it arrives, once, and only those after the
    // power cycle are compared with the device, whose position went to zero.
    if (!counted && rig.session.request_state() == rc::io::RequestState::Done) {
      counted = true;
      if (rebooted) ++done_after_reboot;
    }

    // Ask for one more every 200 ms, when nothing is outstanding.
    if (i % 200 == 0 && rig.session.request_state() != rc::io::RequestState::Waiting) {
      const std::vector<std::uint8_t> args = move_by(1);
      if (rig.session.ask(rc::io::kOpMove, rc::span<const std::uint8_t>(args.data(), args.size()), rig.now)) {
        counted = false;
      }
    }

    Telemetry t;
    if (rebooted && i > 40000 && rig.session.latest(t) && t.arrived_at > rebooted_at) {
      const double truth = rebooted_at + (static_cast<double>(t.sequence) + 1.0) / kHz + kMinLatency;
      worst_clock = std::max(worst_clock, std::fabs(t.sent_at - truth));
    }
  }
  if (!counted && rig.session.request_state() == rc::io::RequestState::Done) ++done_after_reboot;

  const Health h = rig.session.health(rig.now);
  const std::size_t unanswered_after =
      (h.request_timeouts - timeouts_at_reboot) + (h.requests_abandoned - abandoned_at_reboot);

  std::cout << "  sixty seconds: 0.04 percent of bytes dropped and 0.04 percent flipped, one answer in\n"
            << "  ten late and one in fifteen lost, the cable out for three seconds at twenty, and the\n"
            << "  device power cycled while it was out.\n\n"
            << "    telemetry sent " << rig.device.telemetry_sent() << ", delivered " << h.frames
            << ", counted lost " << h.lost << ", checksum failures " << h.checksum_failures << "\n"
            << "    connections made again " << h.reconnects << ", requests in flight when the cable came out "
            << h.requests_abandoned << "\n"
            << "    requests sent " << h.requests_sent << ", resent " << h.requests_retried << ", gave up on "
            << h.request_timeouts << ", answers to something else " << h.stale_responses << "\n"
            << "    after the power cycle " << done_after_reboot << " moves were answered and the device did "
            << rig.device.position() << ", replaying " << rig.device.replays() << " repeats\n"
            << "    worst error placing a frame on our clock once the reconnect had settled: " << std::fixed
            << std::setprecision(1) << worst_clock * 1000.0 << " ms\n\n";

  RC_CHECK(h.connected);
  RC_CHECK(!h.stale);
  RC_CHECK_EQ(h.reconnects, static_cast<std::size_t>(1));
  RC_CHECK(h.frames > 3000);                  // most of the minute got through
  RC_CHECK(h.lost > 0);                       // and the loss was seen, by the sequence numbers
  RC_CHECK(h.checksum_failures > 0);
  RC_CHECK(h.stale_responses > 0);            // the late answers were recognised as late
  RC_CHECK(rig.device.replays() > 0);         // and repeats were replayed, not done again

  // A move is done once, however many times it was sent. The device cannot be
  // behind what was answered, and it is ahead only by moves that reached it and
  // whose answers never came back.
  RC_CHECK(rig.device.position() >= done_after_reboot);
  RC_CHECK(static_cast<std::size_t>(rig.device.position() - done_after_reboot) <= unanswered_after + 1);

  RC_CHECK(worst_clock < 0.02);
}

#if !defined(_WIN32)

namespace {

// A pseudo terminal is a real terminal, so a Session over one runs the real
// SerialPort and the real termios settings, with the device at the other end.
class Pty {
 public:
  Pty() {
    master_ = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (master_ < 0) return;
    if (::grantpt(master_) != 0 || ::unlockpt(master_) != 0) { give_up(); return; }
    const int flags = ::fcntl(master_, F_GETFL, 0);
    if (flags < 0 || ::fcntl(master_, F_SETFL, flags | O_NONBLOCK) != 0) { give_up(); return; }
    const char* name = ::ptsname(master_);
    if (name == nullptr) { give_up(); return; }
    slave_name_ = name;
  }
  ~Pty() { if (master_ >= 0) ::close(master_); }
  Pty(const Pty&) = delete;
  Pty& operator=(const Pty&) = delete;

  bool ok() const { return master_ >= 0 && !slave_name_.empty(); }
  const std::string& slave_name() const { return slave_name_; }

  void send(const std::uint8_t* bytes, std::size_t count) {
    if (count == 0) return;
    const ssize_t written = ::write(master_, bytes, count);
    (void)written;
  }

  std::size_t receive(std::uint8_t* out, std::size_t room) {
    const ssize_t count = ::read(master_, out, room);
    return count > 0 ? static_cast<std::size_t>(count) : 0;
  }

 private:
  void give_up() { ::close(master_); master_ = -1; }
  int master_ = -1;
  std::string slave_name_;
};

class SerialOpener : public rc::io::PortOpener {
 public:
  explicit SerialOpener(std::string path) : path_(std::move(path)) {}

  std::unique_ptr<rc::io::BytePort> open(rc::io::OpenStatus& status) override {
    std::unique_ptr<rc::io::SerialPort> port(new rc::io::SerialPort());
    switch (port->open(path_, 115200)) {
      case rc::io::SerialError::Ok:               status = rc::io::OpenStatus::Ok; return std::unique_ptr<rc::io::BytePort>(port.release());
      case rc::io::SerialError::NotFound:         status = rc::io::OpenStatus::NotFound; break;
      case rc::io::SerialError::Busy:             status = rc::io::OpenStatus::Busy; break;
      case rc::io::SerialError::PermissionDenied: status = rc::io::OpenStatus::PermissionDenied; break;
      default:                                    status = rc::io::OpenStatus::Fatal; break;
    }
    return nullptr;
  }

 private:
  std::string path_;
};

}  // namespace

RC_TEST("over a real terminal: the real SerialPort, the real settings, and a device at the far end") {
  Pty pty;
  RC_REQUIRE(pty.ok());

  rc::io::SimDevice device;
  SerialOpener opener(pty.slave_name());
  Session session(opener);

  using Clock = std::chrono::steady_clock;
  const Clock::time_point start = Clock::now();
  int done = 0;
  int asked = 0;
  double last_ask = 0.0;
  std::uint8_t buffer[512];

  for (;;) {
    const double now = std::chrono::duration<double>(Clock::now() - start).count();
    if (now > 1.6) break;

    device.step(now);

    // The wire: the device's bytes go in at the master, the host's come out of it.
    const std::size_t from_host = pty.receive(buffer, sizeof(buffer));
    if (from_host > 0) device.put_rx(rc::span<const std::uint8_t>(buffer, from_host));
    const std::size_t to_host = device.take_tx(rc::span<std::uint8_t>(buffer, sizeof(buffer)));
    pty.send(buffer, to_host);

    session.tick(now);

    if (session.request_state() == rc::io::RequestState::Done) ++done;
    if (now > 0.4 && now - last_ask > 0.15 && session.request_state() != rc::io::RequestState::Waiting && asked < 5) {
      const std::vector<std::uint8_t> args = move_by(1);
      if (session.ask(rc::io::kOpMove, rc::span<const std::uint8_t>(args.data(), args.size()), now)) {
        ++asked;
        last_ask = now;
      }
    }
    ::usleep(1000);
  }

  const Health h = session.health(1.6);
  std::cout << "  1.6 seconds over a pseudo terminal: delivered " << h.frames << " telemetry frames, lost "
            << h.lost << ", checksum failures " << h.checksum_failures << ", " << asked << " requests asked, "
            << "the device at " << device.position() << ".\n\n";

  RC_CHECK(h.connected);
  RC_CHECK(h.frames > 100);
  RC_CHECK_EQ(h.checksum_failures, static_cast<std::size_t>(0));
  RC_CHECK_EQ(h.lost, static_cast<std::size_t>(0));
  RC_CHECK_EQ(asked, 5);
  RC_CHECK_EQ(device.position(), 5L);
  RC_CHECK_EQ(h.request_timeouts, static_cast<std::size_t>(0));
  RC_CHECK(done > 0);
}

#endif  // !_WIN32
