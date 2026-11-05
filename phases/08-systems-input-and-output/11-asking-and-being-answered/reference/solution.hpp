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
// Asking a device something is two messages, and the answer is only an answer if
// it is to the question that was asked. On a link that can delay and lose, the
// next message to arrive is not necessarily the reply: it can be the reply to a
// request that has already been given up on, and a requester that takes it has
// acted on a stale answer without any error anywhere.
//
// A request carries an id, the device copies it into the response, and the
// requester accepts only the response whose id is the one it is waiting for.
class Requester {
 public:
  // The timeout is how long to wait for an answer before sending again, and
  // max_attempts is how many times to send in all, so the longest a request can
  // take is timeout times max_attempts. A caller with a deadline checks that
  // against it.
  Requester(Link& link, double timeout, int max_attempts)
      : link_(link), timeout_(timeout), max_attempts_(max_attempts < 1 ? 1 : max_attempts) {}

  // Frames and sends a request. False if one is still outstanding, if the
  // arguments are too long, or if the link could not send it. An id is used up
  // only by a request that was sent.
  bool begin(std::uint8_t operation, rc::span<const std::uint8_t> arguments, double now) {
    if (state_ == RequestState::Waiting) return false;
    if (arguments.size() + 3 > sizeof(command_)) return false;

    std::uint8_t next = static_cast<std::uint8_t>(id_ + 1u);
    if (next == 0) next = 1;   // zero means no request, so the id skips it when it wraps

    command_[0] = kRequestType;
    command_[1] = next;
    command_[2] = operation;
    if (arguments.size() != 0) std::memcpy(command_ + 3, arguments.data(), arguments.size());
    command_size_ = arguments.size() + 3;

    if (!link_.send(rc::span<const std::uint8_t>(command_, command_size_),
                    rc::span<std::uint8_t>(scratch_, sizeof(scratch_)))) {
      ++send_failures_;
      return false;
    }

    id_ = next;
    attempts_ = 1;
    sent_at_ = now;
    response_size_ = 0;
    state_ = RequestState::Waiting;
    ++sent_;
    return true;
  }

  // Offers a message the link produced. True if it was a response, which means
  // it has been dealt with and the caller must not route it anywhere else.
  // False for anything else, such as telemetry, which is the caller's to route.
  bool handle(rc::span<const std::uint8_t> message) {
    if (message.size() < 2 || message[0] != kResponseType) return false;

    // A response to anything but the request being waited for is stale. That
    // includes a second answer to a request already completed, which is what a
    // retry produces when the original was only late.
    if (state_ != RequestState::Waiting || message[1] != id_) {
      ++stale_;
      return true;
    }

    const std::size_t body = message.size() - 2;
    if (body > kMaxBody) {
      ++stale_;
      return true;
    }
    if (body != 0) std::memcpy(response_, message.data() + 2, body);
    response_size_ = body;
    state_ = RequestState::Done;
    return true;
  }

  // Call every tick. Sends again when the timeout has passed, and gives up when
  // the attempts are used.
  void tick(double now) {
    if (state_ != RequestState::Waiting) return;
    if (now - sent_at_ < timeout_) return;

    if (attempts_ >= max_attempts_) {
      state_ = RequestState::TimedOut;
      ++timeouts_;
      return;
    }

    // The same bytes and the same id, so a device that answered the first time
    // can recognise this as a repeat.
    sent_at_ = now;   // even if the send fails, so a dead port is not hammered every tick
    ++attempts_;
    if (link_.send(rc::span<const std::uint8_t>(command_, command_size_),
                   rc::span<std::uint8_t>(scratch_, sizeof(scratch_)))) {
      ++retries_;
    } else {
      ++send_failures_;
    }
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
// id lets the device tell, so it remembers the last id it answered and what it
// said, and replays the answer instead of repeating the act.
//
// One slot is enough while the requester has one request in flight. More in
// flight needs more slots, and an id that has been forgotten is an id that gets
// executed again.
class ReplyCache {
 public:
  // True, and the response filled in, if this id is the last one answered.
  bool lookup(std::uint8_t id, rc::span<const std::uint8_t>& response) const {
    if (!has_ || id != id_) return false;
    response = rc::span<const std::uint8_t>(bytes_, size_);
    return true;
  }

  void remember(std::uint8_t id, rc::span<const std::uint8_t> response) {
    id_ = id;
    size_ = response.size() > sizeof(bytes_) ? sizeof(bytes_) : response.size();
    if (size_ != 0) std::memcpy(bytes_, response.data(), size_);
    has_ = true;
  }

 private:
  bool has_ = false;
  std::uint8_t id_ = 0;
  std::uint8_t bytes_[kMaxBody + 4] = {};
  std::size_t size_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
