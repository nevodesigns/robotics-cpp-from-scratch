#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <rc/core/compat.hpp>
#include <rc/io/link.hpp>

using rc::io::Link;

// The two message types this lesson puts on a link that also carries telemetry.
// The first byte of every message says which it is, so the code that polls the
// link can route a message without knowing the protocol above it.
constexpr std::uint8_t kRequestType = 0x01;
constexpr std::uint8_t kResponseType = 0x81;
constexpr std::size_t kMaxBody = 32;

enum class RequestState { Idle, Waiting, Done, TimedOut };

// One request in flight at a time, matched to its answer by an id.
//
// A request carries an id, the device copies it into the response, and the
// requester accepts only the response whose id is the one it is waiting for.
class Requester {
 public:
  Requester(Link& link, double timeout, int max_attempts)
      : link_(link), timeout_(timeout), max_attempts_(max_attempts < 1 ? 1 : max_attempts) {}

  // Frames and sends a request. False if one is still outstanding, if the
  // arguments are too long, or if the link could not send it.
  bool begin(std::uint8_t operation, rc::span<const std::uint8_t> arguments, double now) {
    // TODO: refuse while Waiting, and refuse arguments that do not fit.
    //
    // The next id is id_ plus one, and wraps past zero without ever being zero,
    // because zero means no request. Build the body in command_ as
    //   kRequestType, id, operation, arguments...
    // and send it with link_.send, using scratch_.
    //
    // Take the new id, set attempts_ to one, remember the time, clear the old
    // response and become Waiting ONLY if the send succeeded. A send that fails
    // is counted in send_failures_ and leaves everything as it was, including
    // the id, which a request that never went out has not used.
    (void)operation;
    (void)arguments;
    (void)now;
    (void)id_;
    (void)attempts_;
    (void)sent_at_;
    (void)command_;
    (void)command_size_;
    (void)scratch_;
    return false;
  }

  // Offers a message the link produced. True if it was a response, which means
  // it has been dealt with. False for anything else, which is the caller's.
  bool handle(rc::span<const std::uint8_t> message) {
    // TODO: a message is a response if it has at least two bytes and starts with
    // kResponseType. Anything else returns false and is left alone.
    //
    // A response is the answer if you are Waiting and its id (the second byte)
    // is id_. Copy its body, the bytes after the type and the id, into response_
    // and become Done. Any other response, including a second answer to a
    // request that is already Done, is stale: count it in stale_ and carry on.
    (void)message;
    return false;
  }

  // Call every tick. Sends again when the timeout has passed, and gives up when
  // the attempts are used.
  void tick(double now) {
    // TODO: only while Waiting, and only once now - sent_at_ reaches timeout_.
    //
    // With attempts left: send command_ again, the SAME bytes and the same id so
    // that a device can recognise a repeat, count it in retries_, and restart the
    // clock even if the send failed, so a dead port is not tried every tick.
    // Without: become TimedOut and count it in timeouts_.
    (void)now;
    (void)link_;
    (void)timeout_;
    (void)max_attempts_;
  }

  RequestState state() const { return state_; }
  std::uint8_t id() const { return id_; }

  // The body of the answer, after the type and the id. Valid when Done.
  rc::span<const std::uint8_t> response() const {
    return rc::span<const std::uint8_t>(response_, response_size_);
  }

  std::size_t sent() const { return sent_; }
  std::size_t retries() const { return retries_; }
  std::size_t stale() const { return stale_; }
  std::size_t timeouts() const { return timeouts_; }
  std::size_t send_failures() const { return send_failures_; }

 private:
  Link& link_;
  double timeout_;
  int max_attempts_;
  RequestState state_ = RequestState::Idle;
  std::uint8_t id_ = 0;
  int attempts_ = 0;
  double sent_at_ = 0.0;
  std::uint8_t command_[kMaxBody + 4] = {};
  std::size_t command_size_ = 0;
  std::uint8_t scratch_[kMaxBody + 12] = {};
  std::uint8_t response_[kMaxBody] = {};
  std::size_t response_size_ = 0;
  std::size_t sent_ = 0;
  std::size_t retries_ = 0;
  std::size_t stale_ = 0;
  std::size_t timeouts_ = 0;
  std::size_t send_failures_ = 0;
};

// The device's half of the same promise.
//
// A request that is sent again is a request the device may already have done. If
// the answer was what was lost, doing it a second time moves the arm twice. The
// device remembers the last id it answered and what it said, and replays the
// answer instead of repeating the act.
class ReplyCache {
 public:
  // True, and the response filled in, if this id is the last one answered.
  bool lookup(std::uint8_t id, rc::span<const std::uint8_t>& response) const {
    // TODO: nothing is remembered until remember() has been called, even for id
    // zero. Only the id from the last remember() matches.
    (void)id;
    (void)response;
    (void)has_;
    (void)id_;
    (void)bytes_;
    (void)size_;
    return false;
  }

  void remember(std::uint8_t id, rc::span<const std::uint8_t> response) {
    // TODO: keep the id and a copy of the bytes, replacing what was there. The
    // bytes the span points at are only valid for this call.
    (void)id;
    (void)response;
  }

 private:
  bool has_ = false;
  std::uint8_t id_ = 0;
  std::uint8_t bytes_[kMaxBody + 4] = {};
  std::size_t size_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
