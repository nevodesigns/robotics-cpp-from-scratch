#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>

#include <rc/core/compat.hpp>

// Finds messages in a stream that has none.
//
// The frame is a start byte, a length, and that many payload bytes:
//
//   AA  05  48 65 6C 6C 6F
//
// One byte at a time, because that is what an interrupt handler hands you, and
// because a parser that cannot be given a whole message by accident has no
// special case for the awkward splits: a byte at a time and a thousand at a
// time go down the same path.
class FrameParser {
 public:
  FrameParser(std::uint8_t start_byte, rc::span<std::uint8_t> storage)
      : start_(start_byte), storage_(storage) {}

  // True when this byte completed a frame, which is then available from
  // payload() until the next call.
  bool push(std::uint8_t byte) {
    switch (state_) {
      case State::WaitStart:
        // TODO: a byte that is not the start byte is thrown away and counted,
        // and the parser keeps looking.
        //
        // Not an error, and not a reason to stop. A parser that gives up on the
        // unexpected is useless in the field, where the first plugged cable
        // ends the session. Getting back in step is the whole job.
        return false;

      case State::WaitLength:
        // TODO: check the length against the space that exists, before doing
        // anything with it.
        //
        // The length arrives from outside. It is a claim, not a fact, and it is
        // a claim that indexes memory. A frame that does not fit is not a frame
        // that was understood, so count it and go back to hunting for a start
        // byte rather than truncating it.
        //
        // A payload of exactly the storage size fits. A parser that refuses it
        // silently loses every largest frame.
        //
        // A length of zero is a complete frame on its own, and a parser that
        // waits for a byte that is not coming stalls the link.
        return false;

      case State::WaitPayload:
        // TODO: store the byte, and report the frame complete on the last one.
        return false;
    }
    return false;
  }

  // Valid until the next push. Returning a view rather than a copy is what
  // keeps this allocation free, and the cost is that the caller must use it
  // before feeding more bytes.
  rc::span<const std::uint8_t> payload() const {
    // TODO: a view over the bytes of the frame that just completed.
    return rc::span<const std::uint8_t>();
  }

  // Worth reporting rather than hiding. A link that discards a few bytes at
  // startup is normal; one discarding thousands is saying the baud rate is
  // wrong, and no amount of staring at payloads will say so.
  std::size_t discarded() const { return discarded_; }
  std::size_t oversized() const { return oversized_; }

  void reset() {
    state_ = State::WaitStart;
    expected_ = 0;
    filled_ = 0;
  }

 private:
  enum class State { WaitStart, WaitLength, WaitPayload };

  std::uint8_t start_ = 0xAA;
  rc::span<std::uint8_t> storage_;
  State state_ = State::WaitStart;
  std::size_t expected_ = 0;
  std::size_t filled_ = 0;
  std::size_t discarded_ = 0;
  std::size_t oversized_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
