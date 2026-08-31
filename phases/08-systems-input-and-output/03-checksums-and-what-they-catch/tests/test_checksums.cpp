#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/framing.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

constexpr std::uint8_t kStart = 0xAA;

rc::span<const std::uint8_t> view(const std::vector<std::uint8_t>& v) {
  return rc::span<const std::uint8_t>(v.data(), v.size());
}

// The nine bytes every published CRC quotes its check value for.
std::vector<std::uint8_t> check_vector() {
  const std::string digits = "123456789";
  return std::vector<std::uint8_t>(digits.begin(), digits.end());
}

}  // namespace

RC_TEST("the published check values, which is how you know you agree with anyone else") {
  // Round tripping a CRC against itself succeeds for every wrong variant of it.
  // These two numbers are the only thing that distinguishes a correct CRC-8
  // from a self consistent one that no device on earth computes.
  const std::vector<std::uint8_t> bytes = check_vector();

  RC_CHECK_EQ(static_cast<int>(crc8(view(bytes))), 0xF4);
  RC_CHECK_EQ(static_cast<int>(crc16_ccitt(view(bytes))), 0x29B1);
}

RC_TEST("an empty message has the starting value and nothing else") {
  const std::vector<std::uint8_t> nothing;
  RC_CHECK_EQ(static_cast<int>(crc8(view(nothing))), 0x00);
  RC_CHECK_EQ(static_cast<int>(crc16_ccitt(view(nothing))), 0xFFFF);
  RC_CHECK_EQ(static_cast<int>(sum8(view(nothing))), 0);
  RC_CHECK_EQ(static_cast<int>(xor8(view(nothing))), 0);
}

RC_TEST("the cheap schemes cannot see a reordering, and that is arithmetic rather than luck") {
  // Addition and exclusive or are commutative, so this is a certainty and not
  // a statistic. It is also the exact failure a parser locked onto a false
  // start byte produces: correct bytes in the wrong places.
  const std::vector<std::uint8_t> original{0x11, 0x22, 0x33, 0x44};
  const std::vector<std::uint8_t> swapped{0x44, 0x22, 0x33, 0x11};

  RC_CHECK_EQ(sum8(view(original)), sum8(view(swapped)));
  RC_CHECK_EQ(xor8(view(original)), xor8(view(swapped)));

  // The CRC is not commutative, which is the entire reason to pay for it.
  RC_CHECK(crc8(view(original)) != crc8(view(swapped)));
  RC_CHECK(crc16_ccitt(view(original)) != crc16_ccitt(view(swapped)));
}

RC_TEST("a sum misses two changes that cancel") {
  // 0x00 0x00 and 0x80 0x80 have the same sum: 128 plus 128 is 256, which is
  // zero in eight bits. One in sixteen two bit errors looks like this.
  const std::vector<std::uint8_t> original{0x00, 0x00};
  const std::vector<std::uint8_t> corrupted{0x80, 0x80};

  RC_CHECK_EQ(sum8(view(original)), sum8(view(corrupted)));
  RC_CHECK(crc8(view(original)) != crc8(view(corrupted)));
}

RC_TEST("an exclusive or misses two flips in the same bit position") {
  const std::vector<std::uint8_t> original{0x00, 0x00};
  const std::vector<std::uint8_t> corrupted{0x01, 0x01};

  RC_CHECK_EQ(xor8(view(original)), xor8(view(corrupted)));
  RC_CHECK(crc8(view(original)) != crc8(view(corrupted)));
}

RC_TEST("every single bit flip is caught, exhaustively") {
  const std::vector<std::uint8_t> original{0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x7F};
  const std::uint8_t good8 = crc8(view(original));
  const std::uint16_t good16 = crc16_ccitt(view(original));

  int checked = 0;
  for (std::size_t byte = 0; byte < original.size(); ++byte) {
    for (int bit = 0; bit < 8; ++bit) {
      std::vector<std::uint8_t> corrupted = original;
      corrupted[byte] = static_cast<std::uint8_t>(corrupted[byte] ^ (1u << bit));

      RC_CHECK(crc8(view(corrupted)) != good8);
      RC_CHECK(crc16_ccitt(view(corrupted)) != good16);
      ++checked;
    }
  }
  RC_CHECK_EQ(checked, 48);
}

RC_TEST("every single byte burst is caught, which is the guarantee a CRC makes") {
  // A CRC of width n detects every burst of n bits or fewer by construction.
  // For CRC-8 that is one byte, and this is exhaustive over all 255 wrong
  // values of every position.
  const std::vector<std::uint8_t> original{0x01, 0x02, 0x03, 0x04, 0x05};
  const std::uint8_t good = crc8(view(original));

  long checked = 0;
  for (std::size_t at = 0; at < original.size(); ++at) {
    for (int value = 0; value < 256; ++value) {
      if (static_cast<std::uint8_t>(value) == original[at]) continue;
      std::vector<std::uint8_t> corrupted = original;
      corrupted[at] = static_cast<std::uint8_t>(value);

      RC_REQUIRE(crc8(view(corrupted)) != good);
      ++checked;
    }
  }
  RC_CHECK_EQ(checked, static_cast<long>(5 * 255));
}

RC_TEST("every two byte burst is caught by the sixteen bit one, and not by the eight bit one") {
  // The rule the measurements produced: below the width of the checksum a CRC
  // is a guarantee, and above it everything is chance. Two bytes is sixteen
  // bits, so CRC-16 must be perfect here and CRC-8 must not be.
  const std::vector<std::uint8_t> original{0x10, 0x20, 0x30, 0x40};
  const std::uint8_t good8 = crc8(view(original));
  const std::uint16_t good16 = crc16_ccitt(view(original));

  long crc8_missed = 0;
  long crc16_missed = 0;

  for (std::size_t at = 0; at + 2 <= original.size(); ++at) {
    for (int first = 0; first < 256; ++first) {
      for (int second = 0; second < 256; ++second) {
        std::vector<std::uint8_t> corrupted = original;
        corrupted[at] = static_cast<std::uint8_t>(first);
        corrupted[at + 1] = static_cast<std::uint8_t>(second);
        if (corrupted == original) continue;

        if (crc8(view(corrupted)) == good8) ++crc8_missed;
        if (crc16_ccitt(view(corrupted)) == good16) ++crc16_missed;
      }
    }
  }

  RC_CHECK_EQ(crc16_missed, 0L);
  RC_CHECK(crc8_missed > 0);   // eight bits cannot cover a sixteen bit burst
}

RC_TEST("a payload with its checksum is accepted, and the body comes back") {
  std::vector<std::uint8_t> payload{1, 2, 3};
  payload.push_back(crc8(view(payload)));

  rc::span<const std::uint8_t> body;
  RC_REQUIRE(verify_and_strip(view(payload), body));
  RC_CHECK_EQ(body.size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(static_cast<int>(body[0]), 1);
  RC_CHECK_EQ(static_cast<int>(body[2]), 3);
}

RC_TEST("a corrupted payload is refused, at every position") {
  std::vector<std::uint8_t> good{1, 2, 3, 4};
  good.push_back(crc8(rc::span<const std::uint8_t>(good.data(), good.size())));

  for (std::size_t at = 0; at < good.size(); ++at) {
    std::vector<std::uint8_t> corrupted = good;
    corrupted[at] = static_cast<std::uint8_t>(corrupted[at] ^ 0x01u);

    rc::span<const std::uint8_t> body;
    RC_CHECK(!verify_and_strip(view(corrupted), body));
  }
}

RC_TEST("an empty payload is refused rather than trusted") {
  const std::vector<std::uint8_t> nothing;
  rc::span<const std::uint8_t> body;
  RC_CHECK(!verify_and_strip(view(nothing), body));
}

RC_TEST("a message survives the encoder, the framing and the checksum together") {
  // The three lessons of this phase so far, composed. The first two come from
  // the library they were graduated into, which is the point of graduating
  // them.
  std::vector<std::uint8_t> body(11, 0);
  rc::io::ByteWriter encoder{rc::span<std::uint8_t>(body.data(), body.size())};
  encoder.put_u8(0x2A);
  encoder.put_u16(1000);
  encoder.put_i32(-12345);
  encoder.put_f32(2.5f);
  RC_REQUIRE(encoder.ok());

  // The checksum travels inside the frame's payload, as its last byte, so the
  // framing needs no knowledge of it.
  std::vector<std::uint8_t> payload = body;
  payload.push_back(crc8(view(body)));

  std::vector<std::uint8_t> wire{kStart, static_cast<std::uint8_t>(payload.size())};
  wire.insert(wire.end(), payload.begin(), payload.end());

  std::vector<std::uint8_t> storage(32, 0);
  rc::io::FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  bool complete = false;
  for (const std::uint8_t byte : wire) complete = parser.push(byte) || complete;
  RC_REQUIRE(complete);

  rc::span<const std::uint8_t> checked;
  RC_REQUIRE(verify_and_strip(parser.payload(), checked));

  rc::io::ByteReader decoder{checked};
  std::uint8_t id = 0;
  std::uint16_t sequence = 0;
  std::int32_t offset = 0;
  float value = 0.0f;
  decoder.get_u8(id);
  decoder.get_u16(sequence);
  decoder.get_i32(offset);
  decoder.get_f32(value);

  RC_REQUIRE(decoder.ok());
  RC_CHECK_EQ(static_cast<int>(id), 0x2A);
  RC_CHECK_EQ(sequence, static_cast<std::uint16_t>(1000));
  RC_CHECK_EQ(offset, -12345);
  RC_CHECK_NEAR(static_cast<double>(value), 2.5, 1e-12);
}

RC_TEST("a corrupted frame is refused after it has been framed correctly") {
  std::vector<std::uint8_t> body{9, 8, 7};
  std::vector<std::uint8_t> payload = body;
  payload.push_back(crc8(view(body)));
  payload[1] = static_cast<std::uint8_t>(payload[1] ^ 0x20u);   // the line was noisy

  std::vector<std::uint8_t> wire{kStart, static_cast<std::uint8_t>(payload.size())};
  wire.insert(wire.end(), payload.begin(), payload.end());

  std::vector<std::uint8_t> storage(16, 0);
  rc::io::FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));
  bool complete = false;
  for (const std::uint8_t byte : wire) complete = parser.push(byte) || complete;

  // The framing is perfectly happy. It has no way not to be: the bytes arrived
  // in the right shape. Only the checksum knows.
  RC_REQUIRE(complete);
  rc::span<const std::uint8_t> checked;
  RC_CHECK(!verify_and_strip(parser.payload(), checked));
}
