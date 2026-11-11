#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class Direction { to_device, from_device };

struct Message {
  Direction dir = Direction::to_device;
  std::vector<std::uint8_t> bytes;
};

// A part of every message in one direction that is allowed to differ: a timestamp, a counter.
struct Mask {
  Direction dir = Direction::to_device;
  std::size_t offset = 0;
  std::size_t length = 0;
};

// Everything said in a session, in order.
class Transcript {
 public:
  void add(Direction dir, const std::vector<std::uint8_t>& bytes) { messages_.push_back(Message{dir, bytes}); }
  std::size_t size() const { return messages_.size(); }
  const Message& at(std::size_t i) const { return messages_[i]; }

  // One message to a line: ">" for what was sent to the device and "<" for what came back, then the bytes as two digit hexadecimal numbers
  // separated by spaces. A message with no bytes is a line of just the arrow.
  std::string to_text() const {
    // TODO: For each message: a line that starts with \">\" (to the device) or \"<\" (from it), then for each byte a space and its two hexadecimal digits in capitals, then a newline. A message with no bytes is just the arrow.
    // Digits for 0 to 15 are \"0123456789ABCDEF\": the high four bits first, then the low four.
    return std::string();
  }

  // Read the text form back. Blank lines and lines starting with # are skipped. A line must start with > or <, and the rest must be pairs of hexadecimal
  // digits separated by spaces. The first bad line stops the read, and `error` says which line and why. On failure the transcript is left empty.
  static bool from_text(const std::string& text, Transcript& out, std::string& error) {
    // TODO: Clear out and error. Go through the text line by line, counting from 1. Skip blank lines and lines whose first non space character is #. A line must start with > or <: otherwise stop, set error to
    // \"line N: a line starts with > or <\" (N the line number) and return false. After the arrow, read bytes: skip spaces; each byte is exactly two hexadecimal digits (upper or lower case) followed by a space or the end of the line.
    // Anything else: error \"line N: bytes are two hexadecimal digits each\", return false. The messages go into a transcript of their own, and only when every line has been read are they put in `out` and true returned.
    (void)text;
    (void)out;
    (void)error;
    return false;
  }

 private:
  static bool is_hex(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }
  static int value_of(char c) { return c <= '9' ? c - '0' : (c <= 'F' ? c - 'A' + 10 : c - 'a' + 10); }

  std::vector<Message> messages_;
};

// Where two sessions first differ.
struct Difference {
  enum class Kind { none, direction, length, content, extra, missing };
  Kind kind = Kind::none;
  std::size_t message = 0;     // which message, counting from 0
  std::size_t offset = 0;      // for content, which byte of it; for length, where the shorter one ended
};

inline bool masked(const std::vector<Mask>& masks, Direction dir, std::size_t offset) {
  // TODO: true if some mask has this direction and offset >= its offset and offset < its offset + length
  (void)masks;
  (void)dir;
  (void)offset;
  return false;
}

// Compare a new session with the golden one, message by message. The first difference is reported, and the rest are not looked for.
//   direction: the same message number was sent the other way
//   length:    the messages differ in length (the offset is where the shorter ended)
//   content:   a byte differs, and is not inside a mask
//   extra:     the new session has more messages than the golden one, from this one on
//   missing:   the new session ended early
inline Difference compare(const Transcript& golden, const Transcript& actual, const std::vector<Mask>& masks) {
  // TODO: Walk the messages that both have, in order, and report the FIRST difference found:
  // direction (same number, other way: only the message number is filled in); length (different sizes: offset is the smaller size); content (a byte differs and masked() is false for it: offset is which byte).
  // If all of those match: the actual session having more messages is `extra` (message = the golden count), and having fewer is `missing` (message = the actual count). Otherwise the kind stays none.
  (void)golden;
  (void)actual;
  (void)masks;
  return Difference();
}

#endif  // LESSON_SOLUTION_HPP
