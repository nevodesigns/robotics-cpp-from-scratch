#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>

#include <rc/core/compat.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/framing.hpp>

// Anything that speaks bytes: a serial port, a socket, a radio.
//
// Both calls return a count. Zero means nothing happened, which on a link that
// is merely idle is most calls and is not news. Negative means the port itself
// failed, which is the only one of the three that is an error.
class BytePort {
 public:
  virtual ~BytePort() = default;
  virtual int read(rc::span<std::uint8_t> into) = 0;
  virtual int write(rc::span<const std::uint8_t> from) = 0;
};

// A framed, checksummed link over any BytePort, with the counters that turn a
// dead link into a sentence about why.
class Link {
 public:
  static constexpr std::uint8_t kStartByte = 0xAA;

  // The timeout is how long without a valid message before the link is
  // considered stale. Storage bounds the largest frame accepted.
  Link(BytePort& port, rc::span<std::uint8_t> storage, double timeout)
      : port_(port), parser_(kStartByte, storage), timeout_(timeout) {}

  // Drains whatever has arrived. True when a valid message is ready, which the
  // caller must take before polling again.
  //
  // Takes the time rather than reading a clock, the same seam as lesson 03-05,
  // so a test can make an hour pass instantly.
  bool poll(double now) {
    if (!started_) {
      started_ = true;
      last_message_ = now;   // the clock starts when the link does
    }

    for (int attempt = 0; attempt < kMaxReadsPerPoll; ++attempt) {
      // Whatever is left from the previous poll is consumed first.
      //
      // A read takes bytes out of the port and they are gone from it. Returning
      // as soon as a frame completes, without keeping the rest of the chunk,
      // throws away every message after the first one in a read, which on a
      // busy link is most of them and looks like a device that has halved its
      // rate.
      while (position_ < filled_) {
        const std::uint8_t byte = chunk_[position_++];
        ++bytes_read_;
        if (!parser_.push(byte)) continue;

        rc::span<const std::uint8_t> body;
        if (!rc::io::verify_and_strip(parser_.payload(), body)) {
          // Framed correctly and still not a message. Counted separately from a
          // discard, because the two say different things about the cable.
          ++checksum_failures_;
          continue;
        }

        ++frames_;

        // The watchdog is fed here and nowhere else. Bytes are not information:
        // a crashed device holding its line produces bytes all day, and a
        // watchdog fed by them reports a healthy link for ever.
        last_message_ = now;
        message_ = body;
        return true;
      }

      const int count = port_.read(rc::span<std::uint8_t>(chunk_, sizeof(chunk_)));

      // Zero is not an error and not an end. Nothing had arrived yet, which on
      // a link polled faster than it talks is most of the time.
      if (count == 0) return false;

      if (count < 0) {
        ++port_errors_;
        return false;
      }

      position_ = 0;
      filled_ = static_cast<std::size_t>(count);
    }
    return false;
  }

  // Valid until the next poll.
  rc::span<const std::uint8_t> message() const { return message_; }

  // Frames, checksums and writes the whole thing.
  bool send(rc::span<const std::uint8_t> body, rc::span<std::uint8_t> scratch) {
    const std::size_t needed = body.size() + 3;   // start, length, payload, crc
    if (body.size() + 1 > 255 || scratch.size() < needed) return false;

    std::size_t at = 0;
    scratch[at++] = kStartByte;
    scratch[at++] = static_cast<std::uint8_t>(body.size() + 1);
    for (std::size_t i = 0; i < body.size(); ++i) scratch[at++] = body[i];
    scratch[at++] = rc::io::crc8(body);

    return write_all(rc::span<const std::uint8_t>(scratch.data(), at));
  }

  // A link is stale when nothing valid has arrived for the timeout, which is
  // a different question from whether the port is returning bytes.
  bool stale(double now) const {
    if (!started_) return false;
    return (now - last_message_) > timeout_;
  }

  double last_message_at() const { return last_message_; }
  std::size_t frames() const { return frames_; }
  std::size_t checksum_failures() const { return checksum_failures_; }
  std::size_t discarded() const { return parser_.discarded(); }
  std::size_t oversized() const { return parser_.oversized(); }
  std::size_t port_errors() const { return port_errors_; }
  std::size_t bytes_read() const { return bytes_read_; }

 private:
  // A write is a request, not an instruction: it is allowed to take fewer bytes
  // than it was given. What goes out otherwise is a truncated frame, which is
  // worse than no frame, because the far end resynchronises onto the wrong byte
  // and stays out of step.
  bool write_all(rc::span<const std::uint8_t> bytes) {
    std::size_t sent = 0;

    for (int attempt = 0; attempt < kMaxWriteAttempts && sent < bytes.size(); ++attempt) {
      const int wrote = port_.write(
          rc::span<const std::uint8_t>(bytes.data() + sent, bytes.size() - sent));

      if (wrote < 0) { ++port_errors_; return false; }
      if (wrote == 0) continue;   // the buffer was full, try again
      sent += static_cast<std::size_t>(wrote);
    }

    // Running out of attempts is a failure worth reporting, not a partial
    // success worth ignoring.
    return sent == bytes.size();
  }

  // Bounded so that a port with an endless supply of bytes cannot hold the
  // control loop past its deadline. A poll returns and the next one continues.
  static constexpr int kMaxReadsPerPoll = 16;
  static constexpr int kMaxWriteAttempts = 64;

  BytePort& port_;
  rc::io::FrameParser parser_;
  double timeout_ = 0.0;
  double last_message_ = 0.0;
  bool started_ = false;

  // Bytes taken from the port and not yet parsed, carried between polls.
  std::uint8_t chunk_[64] = {};
  std::size_t position_ = 0;
  std::size_t filled_ = 0;

  rc::span<const std::uint8_t> message_;
  std::size_t frames_ = 0;
  std::size_t checksum_failures_ = 0;
  std::size_t port_errors_ = 0;
  std::size_t bytes_read_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
