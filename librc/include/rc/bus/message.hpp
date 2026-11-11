// rc/bus/message.hpp
//
// Messages as bytes, from lesson 17-03, graduated.
//
// A writer that puts integers and doubles down low byte first with no padding, a reader that checks there is enough left before every read and keeps a flag that stays set once one fails,
// a type id worked out from the message's name (FNV-1a, 32 bit; "a" gives 0xE40C292C), and a frame of type id, length and payload that a receiver can check.
//
// The wire size is the fields added up: 1 + 8 + 2 = 11 bytes where the same struct in memory is 24 on the compiler that built it. A string whose length claimed 500 with 2 bytes behind it failed the read
// and returned nothing. A frame for another type, one cut short, one with an extra byte and one cut to 5 bytes were answered wrong_type, bad_length, bad_length and too_short.

#ifndef RC_BUS_MESSAGE_HPP
#define RC_BUS_MESSAGE_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace rc {
namespace bus {

// Builds a message a byte at a time, little endian, with no padding. The layout is what is written here and nothing else:
// the compiler's idea of a struct has no part in it.
class ByteWriter {
 public:
  void put_u8(std::uint8_t v) { bytes_.push_back(v); }
  void put_u16(std::uint16_t v) {
    put_u8(static_cast<std::uint8_t>(v & 0xFF));
    put_u8(static_cast<std::uint8_t>(v >> 8));
  }
  void put_u32(std::uint32_t v) {
    for (int i = 0; i < 4; ++i) put_u8(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
  }
  void put_f64(double v) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &v, sizeof bits);
    for (int i = 0; i < 8; ++i) put_u8(static_cast<std::uint8_t>((bits >> (8 * i)) & 0xFF));
  }
  // A length (two bytes), then the characters. Longer than 65535 is cut there.
  void put_string(const std::string& s) {
    const std::size_t n = s.size() > 65535 ? 65535 : s.size();
    put_u16(static_cast<std::uint16_t>(n));
    for (std::size_t i = 0; i < n; ++i) put_u8(static_cast<std::uint8_t>(s[i]));
  }
  const std::vector<std::uint8_t>& bytes() const { return bytes_; }

 private:
  std::vector<std::uint8_t> bytes_;
};

// Reads it back. Every read checks there is enough left; one that runs out sets a flag that stays set, and returns zero.
class ByteReader {
 public:
  ByteReader(const std::uint8_t* data, std::size_t size) : data_(data), size_(size) {}
  explicit ByteReader(const std::vector<std::uint8_t>& v) : data_(v.data()), size_(v.size()) {}

  std::uint8_t get_u8() {
    if (!take(1)) return 0;
    return data_[pos_ - 1];
  }
  std::uint16_t get_u16() {
    if (!take(2)) return 0;
    return static_cast<std::uint16_t>(data_[pos_ - 2] | (data_[pos_ - 1] << 8));
  }
  std::uint32_t get_u32() {
    if (!take(4)) return 0;
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(data_[pos_ - 4 + static_cast<std::size_t>(i)]) << (8 * i);
    return v;
  }
  double get_f64() {
    if (!take(8)) return 0.0;
    std::uint64_t bits = 0;
    for (int i = 0; i < 8; ++i) bits |= static_cast<std::uint64_t>(data_[pos_ - 8 + static_cast<std::size_t>(i)]) << (8 * i);
    double v = 0.0;
    std::memcpy(&v, &bits, sizeof v);
    return v;
  }
  // A length and that many characters. A length longer than what is left fails the whole read and gives an empty string.
  std::string get_string() {
    const std::uint16_t n = get_u16();
    if (!take(n)) return std::string();
    return std::string(reinterpret_cast<const char*>(data_ + pos_ - n), n);
  }

  bool ok() const { return ok_; }
  std::size_t remaining() const { return size_ - pos_; }

 private:
  bool take(std::size_t n) {
    if (!ok_ || n > size_ - pos_) {
      ok_ = false;
      return false;
    }
    pos_ += n;
    return true;
  }
  const std::uint8_t* data_ = nullptr;
  std::size_t size_ = 0;
  std::size_t pos_ = 0;
  bool ok_ = true;
};

// A number worked out from a message type's name, so two ends can tell they mean the same message. FNV-1a, 32 bit.
inline std::uint32_t type_id(const std::string& name) {
  std::uint32_t h = 2166136261u;
  for (char c : name) {
    h ^= static_cast<std::uint8_t>(c);
    h *= 16777619u;
  }
  return h;
}

enum class FrameStatus { ok, too_short, wrong_type, bad_length };

// type id (4 bytes), payload length (4 bytes), payload.
inline std::vector<std::uint8_t> frame(std::uint32_t id, const std::vector<std::uint8_t>& payload) {
  ByteWriter w;
  w.put_u32(id);
  w.put_u32(static_cast<std::uint32_t>(payload.size()));
  std::vector<std::uint8_t> out = w.bytes();
  out.insert(out.end(), payload.begin(), payload.end());
  return out;
}

// Take a frame apart. Fewer than 8 bytes is too_short; a different type id is wrong_type; a length that does not equal what follows is bad_length.
// The payload is only filled in when the answer is ok.
inline FrameStatus unframe(const std::vector<std::uint8_t>& bytes, std::uint32_t expected_id, std::vector<std::uint8_t>& payload) {
  if (bytes.size() < 8) return FrameStatus::too_short;
  ByteReader r(bytes);
  const std::uint32_t id = r.get_u32();
  const std::uint32_t length = r.get_u32();
  if (id != expected_id) return FrameStatus::wrong_type;
  if (length != r.remaining()) return FrameStatus::bad_length;
  payload.assign(bytes.begin() + 8, bytes.end());
  return FrameStatus::ok;
}

}  // namespace bus
}  // namespace rc

#endif  // RC_BUS_MESSAGE_HPP
