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
// Trying at once is wrong for a cable that is out for seconds. A fixed pause is a
// compromise between wasting work while it is out and waiting after it is back.
// Doubling the wait up to a cap is quick for a glitch, patient for an absence,
// and never longer than the cap after the cable returns.
struct BackoffPolicy {
  double initial = 0.01;   // seconds before the first retry
  double factor = 2.0;     // each wait is this much longer than the one before
  double cap = 2.0;        // and never longer than this

  // The wait before retry number `attempt`, counting from zero.
  double delay(unsigned attempt) const {
    // TODO: start at `initial` and multiply by `factor` once for each attempt, but
    // stop at `cap`.
    //
    //   - never less than a millisecond to start with: a wait of nothing retries
    //     as fast as the loop runs, which is what this exists to prevent
    //   - a factor under one is not a backoff, so treat it as one
    //   - multiply one step at a time and stop when you reach the cap. Computing
    //     initial times factor to the power of attempt overflows to infinity for a
    //     long enough absence
    //   - a factor of exactly one never reaches the cap, so that loop would run
    //     once for every failure so far, and this is called on every failure.
    //     Answer a constant wait at once
    //   - the answer is never more than cap, even for the first attempt
    (void)attempt;
    return 0.0;
  }
};

enum class OpenStatus {
  Ok,
  NotFound,           // the device is not there, which is what an unplugged cable looks like
  Busy,               // something else has it, and may let go
  PermissionDenied,   // usually a setting, and sometimes the moment between a device appearing and its permissions being set
  Fatal,              // a mistake in the program, which no amount of trying will fix
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
  unsigned permission_attempts = 5;   // refused this many times in a row is a setting, not a race
  double silence_limit = 0.0;         // seconds without a byte before the port is dropped and reopened, or zero for never
};

// A port that survives the cable coming out. It is a BytePort, so a Link goes
// over it unchanged.
class ReconnectingPort : public BytePort {
 public:
  ReconnectingPort(PortOpener& opener, ReconnectOptions options = ReconnectOptions{})
      : opener_(opener), options_(options) {}

  // Call once each time round the loop, before reading.
  void tick(double now) {
    // TODO: remember `now`, which read() and write() will need.
    //
    // Connected: with a silence_limit set, a port that has produced no bytes for
    // longer than that is dropped (count it in silences_) and will be reopened.
    //
    // Waiting, and the wait is over: call opener_.open(). Count the attempt and
    // remember the status.
    //   Ok: keep the port, become Connected, add one to generation_, reset the
    //       failure counts, and start the silence clock.
    //   Fatal: become Failed. Retrying a mistake hides it behind a loop that
    //       looks like patience.
    //   PermissionDenied: Failed once it has happened permission_attempts times
    //       in a row, because a device that is refused that often is a setting
    //       and not a race with the system setting its permissions.
    //   otherwise: the next try is options_.backoff.delay() from now, counting
    //       the failures in a row.
    // Failed does nothing at all.
    (void)now;
    (void)opener_;
    (void)options_;
  }

  int read(rc::span<std::uint8_t> into) override {
    // TODO: no port reads zero, which to the Link above is an ordinary quiet
    // link. A negative from the port is a cable that came out: drop it and
    // return -1 so the caller sees the failure once. Bytes feed the silence clock.
    (void)into;
    return 0;
  }

  int write(rc::span<const std::uint8_t> from) override {
    // TODO: no port refuses with -1, which is true: nothing can be sent. A
    // negative from the port drops it, and is passed up.
    (void)from;
    return 0;
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
  // Throws the port away and schedules the first retry at the first wait of the
  // backoff, not at once: a port that has just failed will not open this instant.
  // Counts the disconnect. Used by read(), write() and tick().
  void drop(double now) {
    // TODO
    (void)now;
    (void)now_;
    (void)next_attempt_;
    (void)last_data_;
    (void)failures_in_a_row_;
    (void)permission_failures_;
    (void)inner_;
    (void)disconnects_;
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
