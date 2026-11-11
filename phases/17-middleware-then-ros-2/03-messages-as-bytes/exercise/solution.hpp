#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Builds a message a byte at a time, little endian, with no padding. The layout is what is written here and nothing else:
// the compiler's idea of a struct has no part in it.
class ByteWriter {
 public:
  void put_u8(std::uint8_t v) { bytes_.push_back(v); }
  void put_u16(std::uint16_t v) {
    // TODO: low byte first, then the high byte (put_u8 twice)
    (void)v;
  }
  void put_u32(std::uint32_t v) {
    // TODO: four bytes, lowest first: for i in 0..3, put_u8((v >> (8 * i)) & 0xFF)
    (void)v;
  }
  void put_f64(double v) {
    // TODO: copy the eight bytes of the double into a std::uint64_t with std::memcpy, then write it lowest byte first
    (void)v;
  }
  // A length (two bytes), then the characters. Longer than 65535 is cut there.
  void put_string(const std::string& s) {
    // TODO: a u16 length (the size, cut at 65535) then that many characters as bytes
    (void)s;
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
    // TODO: take(2); low byte then high byte; 0 if there was not enough
    return 0;
  }
  std::uint32_t get_u32() {
    // TODO: take(4); the lowest byte first; 0 if there was not enough
    return 0;
  }
  double get_f64() {
    // TODO: take(8); assemble a std::uint64_t lowest byte first, memcpy it into a double; 0.0 if there was not enough
    return 0.0;
  }
  // A length and that many characters. A length longer than what is left fails the whole read and gives an empty string.
  std::string get_string() {
    // TODO: n = get_u16(); if take(n) fails return an empty string; otherwise the n characters just taken
    return std::string();
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
  // TODO: FNV-1a 32 bit: h = 2166136261; for each character: h ^= the byte, h *= 16777619 (unsigned 32 bit)
  (void)name;
  return 0;
}

enum class FrameStatus { ok, too_short, wrong_type, bad_length };

// type id (4 bytes), payload length (4 bytes), payload.
inline std::vector<std::uint8_t> frame(std::uint32_t id, const std::vector<std::uint8_t>& payload) {
  // TODO: a ByteWriter with put_u32(id) and put_u32(payload size), then the payload bytes appended to its bytes()
  (void)id;
  (void)payload;
  return std::vector<std::uint8_t>();
}

// Take a frame apart. Fewer than 8 bytes is too_short; a different type id is wrong_type; a length that does not equal what follows is bad_length.
// The payload is only filled in when the answer is ok.
inline FrameStatus unframe(const std::vector<std::uint8_t>& bytes, std::uint32_t expected_id, std::vector<std::uint8_t>& payload) {
  // TODO: fewer than 8 bytes: too_short. Read the id and the length with a ByteReader. A different id: wrong_type.
  // A length that is not what remains after the 8 bytes: bad_length. Otherwise copy the rest into payload and return ok.
  (void)bytes;
  (void)expected_id;
  (void)payload;
  return FrameStatus::too_short;
}

#endif  // LESSON_SOLUTION_HPP
