#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <memory>

#include <rc/core/compat.hpp>
#include <rc/io/link.hpp>

using rc::io::BytePort;

// How long to wait before trying again, for each failure in a row.
//
// Trying again at once is the wrong answer to a cable that is out, because the
// cable is out for seconds and the loop comes round in microseconds. Trying again
// after a fixed pause is a compromise between two bad answers: short wastes work
// for as long as the cable is out, and long makes the robot wait after it is back.
// Doubling the wait, up to a cap, gets both: quick for a glitch, patient for an
// absence, and never longer than the cap after the cable returns.
struct BackoffPolicy {
  double initial = 0.01;   // seconds before the first retry
  double factor = 2.0;     // each wait is this much longer than the one before
  double cap = 2.0;        // and never longer than this

  // The wait before retry number `attempt`, counting from zero.
  double delay(unsigned attempt) const {
    // A wait of nothing retries as fast as the loop runs, which is the problem
    // this exists to prevent, so a millisecond is the least it will give.
    const double start = initial < 0.001 ? 0.001 : initial;
    const double grow = factor < 1.0 ? 1.0 : factor;

    // A wait that does not grow never reaches the cap, so the loop below would
    // run once for every failure so far, and this is called on every failure:
    // fifty thousand of them is more than a billion steps. It is answered at once.
    if (grow == 1.0) return start < cap ? start : cap;

    // Multiplied up one step at a time and stopped at the cap, rather than
    // computed as initial times factor to the power of attempt, which overflows
    // to infinity for a long enough absence and then compares as nothing.
    double wait = start;
    for (unsigned i = 0; i < attempt && wait < cap; ++i) wait *= grow;
    return wait < cap ? wait : cap;
  }
};

enum class OpenStatus {
  Ok,
  NotFound,           // the device is not there, which is what an unplugged cable looks like
  Busy,               // something else has it, and may let go
  PermissionDenied,   // usually a setting, and sometimes the moment between a device appearing and its permissions being set
  Fatal,              // a mistake in the program, such as a baud rate that does not exist, which no amount of trying will fix
};

// Whatever knows how to open the port. A serial port, a socket, a test double.
class PortOpener {
 public:
  virtual ~PortOpener() = default;
  virtual std::unique_ptr<BytePort> open(OpenStatus& status) = 0;
};

enum class LinkState { Connected, Waiting, Failed };

struct ReconnectOptions {
  BackoffPolicy backoff;
  unsigned permission_attempts = 5;   // a device that is refused this many times in a row is a setting, not a race
  double silence_limit = 0.0;         // seconds without a byte before the port is dropped and reopened, or zero for never
};

// A port that survives the cable coming out.
//
// It is a BytePort, so a Link goes over it unchanged. While there is no port it
// reads zero and refuses writes, and the loop above sees an ordinary quiet link
// that cannot send, which is true. When the cable returns it opens again, and says
// so: generation() goes up, because everything built on the old connection has to
// start again. A parser holding half a frame would eat the start of the first
// frame on the new one.
class ReconnectingPort : public BytePort {
 public:
  ReconnectingPort(PortOpener& opener, ReconnectOptions options = ReconnectOptions{})
      : opener_(opener), options_(options) {}

  // Call once each time round the loop, before reading. This is where it opens
  // when the wait is over and where it decides a quiet port is gone.
  void tick(double now) {
    now_ = now;

    if (state_ == LinkState::Connected) {
      if (options_.silence_limit > 0.0 && now - last_data_ > options_.silence_limit) {
        ++silences_;
        drop(now);
      }
      return;
    }
    if (state_ == LinkState::Failed || now < next_attempt_) return;

    OpenStatus status = OpenStatus::Fatal;
    std::unique_ptr<BytePort> opened = opener_.open(status);
    ++attempts_;
    last_status_ = status;

    if (status == OpenStatus::Ok && opened) {
      inner_ = std::move(opened);
      state_ = LinkState::Connected;
      ++generation_;
      failures_in_a_row_ = 0;
      permission_failures_ = 0;
      last_data_ = now;
      return;
    }

    // Some failures are not going to get better. Retrying a baud rate that does
    // not exist hides a mistake behind a loop that looks like patience.
    if (status == OpenStatus::Fatal) {
      state_ = LinkState::Failed;
      return;
    }
    if (status == OpenStatus::PermissionDenied && ++permission_failures_ >= options_.permission_attempts) {
      state_ = LinkState::Failed;
      return;
    }

    next_attempt_ = now + options_.backoff.delay(failures_in_a_row_++);
  }

  int read(rc::span<std::uint8_t> into) override {
    if (!inner_) return 0;
    const int count = inner_->read(into);
    if (count < 0) {
      drop(now_);
      return -1;   // the caller sees the failure once, and then a quiet link
    }
    if (count > 0) last_data_ = now_;
    return count;
  }

  int write(rc::span<const std::uint8_t> from) override {
    if (!inner_) return -1;
    const int count = inner_->write(from);
    if (count < 0) drop(now_);
    return count;
  }

  LinkState state() const { return state_; }
  bool connected() const { return state_ == LinkState::Connected; }

  // Goes up each time a connection is made. The first is one.
  unsigned generation() const { return generation_; }

  std::size_t attempts() const { return attempts_; }
  std::size_t disconnects() const { return disconnects_; }
  std::size_t silences() const { return silences_; }
  OpenStatus last_status() const { return last_status_; }

 private:
  void drop(double now) {
    inner_.reset();
    state_ = LinkState::Waiting;
    ++disconnects_;

    // The first retry is at the first wait of the backoff, not at once: a port
    // that has just failed is not going to open this instant.
    failures_in_a_row_ = 0;
    next_attempt_ = now + options_.backoff.delay(failures_in_a_row_++);
  }

  PortOpener& opener_;
  ReconnectOptions options_;
  std::unique_ptr<BytePort> inner_;
  LinkState state_ = LinkState::Waiting;
  double now_ = 0.0;
  double next_attempt_ = 0.0;
  double last_data_ = 0.0;
  unsigned failures_in_a_row_ = 0;
  unsigned permission_failures_ = 0;
  unsigned generation_ = 0;
  std::size_t attempts_ = 0;
  std::size_t disconnects_ = 0;
  std::size_t silences_ = 0;
  OpenStatus last_status_ = OpenStatus::Ok;
};

#endif  // LESSON_SOLUTION_HPP
