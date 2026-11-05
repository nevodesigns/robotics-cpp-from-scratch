#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <rc/core/compat.hpp>
#include <rc/io/link.hpp>
#include <rc/io/reconnect.hpp>
#include <rc/io/request.hpp>
#include <rc/io/sequence.hpp>
#include <rc/io/sim_device.hpp>
#include <rc/io/timesync.hpp>

// One reading from the device, with its time on the robot's clock.
struct Telemetry {
  std::uint16_t sequence = 0;
  double sent_at = 0.0;        // when the device sent it, by the host's clock, from the clock map
  double arrived_at = 0.0;     // when it got here
  std::int32_t position = 0;
};

struct SessionOptions {
  rc::io::ReconnectOptions reconnect;
  double stale_after = 0.5;        // seconds without a valid message before the link is called stale
  double request_timeout = 0.05;   // and how long to wait for an answer before asking again
  int request_attempts = 3;
};

// Everything worth knowing about the conversation, in one place.
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

// A conversation with a device: everything in phase 08 in one object. Every piece
// is something you built, in rc::io, and this routes messages between them.
class Session {
 public:
  explicit Session(rc::io::PortOpener& opener, SessionOptions options = SessionOptions{})
      : options_(options), port_(opener, options.reconnect), storage_(64, 0) {}

  // Call once each time round the loop.
  void tick(double now) {
    // TODO, in this order:
    //   1. tick the port. If its generation() is not the one you last saw, this is
    //      a new connection: call start_over().
    //   2. If the port is not connected, or there is no link yet, stop.
    //   3. Poll the link for up to 256 messages. For each one note that a message
    //      arrived and when. Offer it to requester_->handle() first: a response
    //      is the requester's whether or not it is the one being waited for, and
    //      handle() returns true if it took it. Anything left of nine bytes that
    //      starts with rc::io::kTelemetryType is telemetry: accept_telemetry().
    //   4. tick the requester so a silent device is asked again.
    (void)now;
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
    // TODO: forward to requester_->begin(), but only when there is a requester and
    // the port is connected.
    (void)operation;
    (void)arguments;
    (void)now;
    return false;
  }

  rc::io::RequestState request_state() const {
    return requester_ ? requester_->state() : rc::io::RequestState::Idle;
  }
  rc::span<const std::uint8_t> response() const {
    return requester_ ? requester_->response() : rc::span<const std::uint8_t>();
  }

  Health health(double now) const {
    Health h;
    // TODO: fill it in. The totals are what past connections counted, plus what
    // this one has counted so far. reconnects is the number of connections made
    // after the first. stale is true if nothing valid has ever arrived or the last
    // message was more than options_.stale_after ago.
    h.connected = port_.connected();
    (void)now;
    (void)options_;
    (void)frames_;
    (void)any_message_;
    (void)last_message_;
    (void)lost_total_;
    (void)checksum_total_;
    (void)sent_total_;
    (void)retried_total_;
    (void)timeouts_total_;
    (void)stale_total_;
    (void)abandoned_;
    return h;
  }

 private:
  // Telemetry is nine bytes: the type, a sequence number (u16), the device's tick
  // count (u16 milliseconds) and a position (u32, signed).
  void accept_telemetry(rc::span<const std::uint8_t> message, double now) {
    // TODO: decode it with rc::io::ByteReader. Then
    //   - tracker_.accept(sequence)
    //   - the stamp through unwrapper_; if it says restarted, clock_.reset()
    //   - clock_.add(device seconds, now), and sent_at is clock_.to_host() of it
    //   - keep it as latest_, and count it in frames_
    (void)message;
    (void)now;
  }

  // A new connection. Fold what the old one counted into the totals, then give
  // everything that held state a fresh start.
  void start_over() {
    // TODO: remember the generation. If there is an old link and requester, add
    // their counters (checksum failures, sent, retries, timeouts, stale) and the
    // tracker's lost into the totals, and count a request still Waiting as
    // abandoned. Then make a new Link over port_ with storage_, a new Requester
    // over it, and start tracker_, unwrapper_ and clock_ again.
    (void)generation_;
  }

  SessionOptions options_;
  rc::io::ReconnectingPort port_;
  std::vector<std::uint8_t> storage_;
  std::unique_ptr<rc::io::Link> link_;
  std::unique_ptr<rc::io::Requester> requester_;
  rc::io::SequenceTracker<std::uint16_t> tracker_;
  rc::io::TickUnwrapper<std::uint16_t> unwrapper_;
  rc::io::ClockMap clock_;
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

#endif  // LESSON_SOLUTION_HPP
