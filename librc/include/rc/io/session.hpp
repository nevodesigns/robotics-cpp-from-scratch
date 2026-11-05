// rc/io/session.hpp
//
// The conversation from lesson 08-14, graduated: everything in phase 08 in one
// object.
//
// A Session owns the port that reopens itself, the link over it, the requests and
// their answers, the sequence numbers and the map from the device's clock to ours,
// and routes each message to the part that wants it. Exactly one place reads the link:
// a request that waited by polling it itself, and discarded what was not its answer,
// lost 38 telemetry frames in fifty requests, and a session lost none.
//
// When the connection is made again everything that learned something from the old
// one starts over: the parser, the sequence counter, the clock map and a request in
// flight, which is counted as abandoned. What was lost while the cable was out is in
// no sequence counter, so Health reports reconnects beside the loss.
//
// Tested against rc/io/sim_device.hpp, on a virtual clock and, on Linux, over a
// pseudo terminal with the real SerialPort at the near end.

#ifndef RC_IO_SESSION_HPP
#define RC_IO_SESSION_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <rc/core/compat.hpp>
#include <rc/io/bytes.hpp>
#include <rc/io/link.hpp>
#include <rc/io/reconnect.hpp>
#include <rc/io/request.hpp>
#include <rc/io/sequence.hpp>
#include <rc/io/sim_device.hpp>
#include <rc/io/timesync.hpp>

namespace rc {
namespace io {

// One reading from the device, with its time on the robot's clock.
struct Telemetry {
  std::uint16_t sequence = 0;
  double sent_at = 0.0;        // when the device sent it, by the host's clock, from the clock map
  double arrived_at = 0.0;     // when it got here
  std::int32_t position = 0;
};

struct SessionOptions {
  ReconnectOptions reconnect;
  double stale_after = 0.5;        // seconds without a valid message before the link is called stale
  double request_timeout = 0.05;   // and how long to wait for an answer before asking again
  int request_attempts = 3;
};

// Everything worth knowing about the conversation, in one place, since a link
// that cannot say how it is doing can only be diagnosed by watching the robot.
struct Health {
  bool connected = false;
  bool stale = true;                      // no valid message lately, which is not the same as no bytes
  std::size_t frames = 0;                 // telemetry delivered
  std::size_t lost = 0;                   // telemetry the sequence numbers say never arrived
  std::size_t checksum_failures = 0;
  std::size_t reconnects = 0;             // times the connection was made again
  std::size_t requests_sent = 0;
  std::size_t requests_retried = 0;
  std::size_t request_timeouts = 0;       // gave up
  std::size_t requests_abandoned = 0;     // in flight when the connection was lost
  std::size_t stale_responses = 0;        // answers to a request that was not the one waiting
  double drift_ppm = 0.0;                 // how much faster the device's clock runs than ours
  double seconds_since_message = 0.0;
};

// A conversation with a device: everything in phase 08 in one object.
//
// It owns the port that reopens itself, the link over it, the requests and their
// answers, the sequence numbers, and the map from the device's clock to ours, and
// it routes each message to the part that wants it. Telemetry and answers share
// one link, and a message goes to exactly one of them.
//
// When the connection is made again, everything that learned something from the
// old one starts over. That is the part that is easy to leave out and expensive to
// miss: a parser holding half a frame, a sequence counter the device restarted,
// a clock map built before a reset, and a request whose answer will never come.
class Session {
 public:
  explicit Session(PortOpener& opener, SessionOptions options = SessionOptions{})
      : options_(options), port_(opener, options.reconnect), storage_(64, 0) {}

  // Call once each time round the loop.
  void tick(double now) {
    port_.tick(now);
    if (port_.generation() != generation_) start_over();
    if (!port_.connected() || !link_) return;

    // Bounded, so a device that never stops cannot hold the loop.
    for (int taken = 0; taken < 256 && link_->poll(now); ++taken) {
      const rc::span<const std::uint8_t> message = link_->message();
      any_message_ = true;
      last_message_ = now;

      // A response belongs to the requester, whether or not it is the one being
      // waited for. Anything else is for us.
      if (requester_->handle(message)) continue;

      if (message.size() == 9 && message[0] == kTelemetryType) accept_telemetry(message, now);
    }
    requester_->tick(now);
  }

  // The newest telemetry. False before the first.
  bool latest(Telemetry& out) const {
    if (!have_latest_) return false;
    out = latest_;
    return true;
  }

  // Asks the device something. False if a request is still outstanding, or there
  // is no connection to send it on.
  bool ask(std::uint8_t operation, rc::span<const std::uint8_t> arguments, double now) {
    if (!requester_ || !port_.connected()) return false;
    return requester_->begin(operation, arguments, now);
  }

  RequestState request_state() const {
    return requester_ ? requester_->state() : RequestState::Idle;
  }
  rc::span<const std::uint8_t> response() const {
    return requester_ ? requester_->response() : rc::span<const std::uint8_t>();
  }

  Health health(double now) const {
    Health h;
    h.connected = port_.connected();
    h.frames = frames_;
    h.lost = lost_total_ + tracker_.lost();
    h.checksum_failures = checksum_total_ + (link_ ? link_->checksum_failures() : 0);
    h.reconnects = port_.generation() > 0 ? port_.generation() - 1 : 0;
    h.requests_sent = sent_total_ + (requester_ ? requester_->sent() : 0);
    h.requests_retried = retried_total_ + (requester_ ? requester_->retries() : 0);
    h.request_timeouts = timeouts_total_ + (requester_ ? requester_->timeouts() : 0);
    h.requests_abandoned = abandoned_;
    h.stale_responses = stale_total_ + (requester_ ? requester_->stale() : 0);
    h.drift_ppm = clock_.drift_ppm();
    h.seconds_since_message = any_message_ ? now - last_message_ : 1e9;
    h.stale = !any_message_ || h.seconds_since_message > options_.stale_after;
    return h;
  }

 private:
  void accept_telemetry(rc::span<const std::uint8_t> message, double now) {
    ByteReader reader{rc::span<const std::uint8_t>(message.data() + 1, 8)};
    std::uint16_t sequence = 0;
    std::uint16_t ticks = 0;
    std::uint32_t position = 0;
    reader.get_u16(sequence);
    reader.get_u16(ticks);
    reader.get_u32(position);
    if (!reader.ok()) return;

    tracker_.accept(sequence);

    // The device's stamp goes through the unwrapper, and a restart is told to the
    // map: its offset from our clock has just changed.
    const Unwrapped unwrapped = unwrapper_.accept(ticks);
    if (unwrapped.restarted) clock_.reset();
    const double device_seconds = static_cast<double>(unwrapped.ticks) / 1000.0;
    clock_.add(device_seconds, now);

    latest_.sequence = sequence;
    latest_.sent_at = clock_.to_host(device_seconds);
    latest_.arrived_at = now;
    latest_.position = static_cast<std::int32_t>(position);
    have_latest_ = true;
    ++frames_;
  }

  // A new connection. Fold what the old one counted into the totals, then give
  // everything that held state a fresh start.
  void start_over() {
    generation_ = port_.generation();

    if (link_) checksum_total_ += link_->checksum_failures();
    lost_total_ += tracker_.lost();
    if (requester_) {
      sent_total_ += requester_->sent();
      retried_total_ += requester_->retries();
      timeouts_total_ += requester_->timeouts();
      stale_total_ += requester_->stale();
      if (requester_->state() == RequestState::Waiting) ++abandoned_;
    }

    link_.reset(new Link(port_, rc::span<std::uint8_t>(storage_.data(), storage_.size()), 1e12));
    requester_.reset(new Requester(*link_, options_.request_timeout, options_.request_attempts));
    tracker_ = SequenceTracker<std::uint16_t>();
    unwrapper_ = TickUnwrapper<std::uint16_t>();
    clock_.reset();
  }

  SessionOptions options_;
  ReconnectingPort port_;
  std::vector<std::uint8_t> storage_;
  std::unique_ptr<Link> link_;
  std::unique_ptr<Requester> requester_;
  SequenceTracker<std::uint16_t> tracker_;
  TickUnwrapper<std::uint16_t> unwrapper_;
  ClockMap clock_;
  unsigned generation_ = 0;

  Telemetry latest_;
  bool have_latest_ = false;
  bool any_message_ = false;
  double last_message_ = 0.0;
  std::size_t frames_ = 0;
  std::size_t lost_total_ = 0;
  std::size_t checksum_total_ = 0;
  std::size_t sent_total_ = 0;
  std::size_t retried_total_ = 0;
  std::size_t timeouts_total_ = 0;
  std::size_t stale_total_ = 0;
  std::size_t abandoned_ = 0;
};

}  // namespace io
}  // namespace rc

#endif  // RC_IO_SESSION_HPP
