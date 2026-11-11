// rc/sim/transcript.hpp
//
// Golden transcripts of a device session, from lesson 18-10, graduated.
//
// A transcript is everything said in a session, in order, one message to a line in text: an arrow for the direction (> to the device, < from it) and the bytes as two digit hexadecimal numbers. Reading the text back refuses a bad line with its number and leaves nothing behind. A comparison reports the
// first difference (direction, length, content, extra or missing) with where, and a mask lets chosen bytes of every message in one direction differ.
//
// A session of 3 requests was 6 lines. The same session at two times of day first differed at message 1 byte 2 without a mask and not at all with the time masked; a reading changed one byte past the mask was still caught at message 1 byte 6. New firmware adding a byte to every reply was a length difference at message 1, offset 8;
// a reading one too high on request 3 was a content difference at message 5, byte 6.

#ifndef RC_SIM_TRANSCRIPT_HPP
#define RC_SIM_TRANSCRIPT_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rc {
namespace sim {

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
    const char* digits = "0123456789ABCDEF";
    std::string out;
    for (const Message& m : messages_) {
      out += m.dir == Direction::to_device ? ">" : "<";
      for (std::uint8_t b : m.bytes) {
        out += ' ';
        out += digits[b >> 4];
        out += digits[b & 15];
      }
      out += '\n';
    }
    return out;
  }

  // Read the text form back. Blank lines and lines starting with # are skipped. A line must start with > or <, and the rest must be pairs of hexadecimal
  // digits separated by spaces. The first bad line stops the read, and `error` says which line and why. On failure the transcript is left empty.
  static bool from_text(const std::string& text, Transcript& out, std::string& error) {
    out.messages_.clear();
    error.clear();
    Transcript parsed;
    int line_number = 0;
    std::size_t start = 0;
    while (start < text.size()) {
      std::size_t stop = text.find('\n', start);
      if (stop == std::string::npos) stop = text.size();
      const std::string line = text.substr(start, stop - start);
      start = stop + 1;
      ++line_number;
      std::size_t i = 0;
      while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) ++i;
      if (i == line.size() || line[i] == '#') continue;
      Message m;
      if (line[i] == '>') m.dir = Direction::to_device;
      else if (line[i] == '<') m.dir = Direction::from_device;
      else {
        error = "line " + std::to_string(line_number) + ": a line starts with > or <";
        return false;
      }
      ++i;
      while (i < line.size()) {
        if (line[i] == ' ' || line[i] == '\t' || line[i] == '\r') {
          ++i;
          continue;
        }
        if (i + 1 >= line.size() || !is_hex(line[i]) || !is_hex(line[i + 1]) || (i + 2 < line.size() && line[i + 2] != ' ' && line[i + 2] != '\t' && line[i + 2] != '\r')) {
          error = "line " + std::to_string(line_number) + ": bytes are two hexadecimal digits each";
          return false;
        }
        m.bytes.push_back(static_cast<std::uint8_t>(value_of(line[i]) * 16 + value_of(line[i + 1])));
        i += 2;
      }
      parsed.messages_.push_back(m);
    }
    out.messages_ = parsed.messages_;
    return true;
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
  for (const Mask& m : masks)
    if (m.dir == dir && offset >= m.offset && offset < m.offset + m.length) return true;
  return false;
}

// Compare a new session with the golden one, message by message. The first difference is reported, and the rest are not looked for.
//   direction: the same message number was sent the other way
//   length:    the messages differ in length (the offset is where the shorter ended)
//   content:   a byte differs, and is not inside a mask
//   extra:     the new session has more messages than the golden one, from this one on
//   missing:   the new session ended early
inline Difference compare(const Transcript& golden, const Transcript& actual, const std::vector<Mask>& masks) {
  Difference d;
  const std::size_t n = golden.size() < actual.size() ? golden.size() : actual.size();
  for (std::size_t i = 0; i < n; ++i) {
    const Message& g = golden.at(i);
    const Message& a = actual.at(i);
    if (g.dir != a.dir) {
      d.kind = Difference::Kind::direction;
      d.message = i;
      return d;
    }
    if (g.bytes.size() != a.bytes.size()) {
      d.kind = Difference::Kind::length;
      d.message = i;
      d.offset = g.bytes.size() < a.bytes.size() ? g.bytes.size() : a.bytes.size();
      return d;
    }
    for (std::size_t k = 0; k < g.bytes.size(); ++k)
      if (g.bytes[k] != a.bytes[k] && !masked(masks, g.dir, k)) {
        d.kind = Difference::Kind::content;
        d.message = i;
        d.offset = k;
        return d;
      }
  }
  if (actual.size() > golden.size()) {
    d.kind = Difference::Kind::extra;
    d.message = golden.size();
  } else if (actual.size() < golden.size()) {
    d.kind = Difference::Kind::missing;
    d.message = actual.size();
  }
  return d;
}

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_TRANSCRIPT_HPP
