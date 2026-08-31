#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <cstring>
#include <vector>

#include "solution.hpp"

namespace {

// The struct from the lesson, here so the padding can be measured rather than
// described.
struct Reading {
  std::uint8_t id;
  double x;
  std::uint16_t sequence;
};

constexpr std::size_t kSumOfReadingFields =
    sizeof(std::uint8_t) + sizeof(double) + sizeof(std::uint16_t);

std::vector<std::uint8_t> buffer_of(std::size_t bytes) {
  return std::vector<std::uint8_t>(bytes, 0xAA);   // a filler that is not zero
}

}  // namespace

RC_TEST("a struct is bigger than the data in it, which is why none is ever sent") {
  // Eleven bytes of fields. The compiler inserts padding so that the double
  // lands on an address it is allowed to live at, and the padding is a decision
  // your compiler made for your processor, not a fact the device agrees with.
  RC_CHECK_EQ(kSumOfReadingFields, static_cast<std::size_t>(11));
  RC_CHECK(sizeof(Reading) > kSumOfReadingFields);
}

RC_TEST("a single byte goes out as itself") {
  auto storage = buffer_of(1);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};

  RC_CHECK(writer.put_u8(0x7F));
  RC_CHECK_EQ(writer.written(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(static_cast<int>(storage[0]), 0x7F);
}

RC_TEST("a byte above 127 survives, because the buffer is not made of char") {
  auto storage = buffer_of(1);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};
  RC_REQUIRE(writer.put_u8(0xFF));

  // On a platform where char is signed this is -1, and a checksum accumulated
  // in the wrong type gets a different answer from the same bytes.
  RC_CHECK_EQ(static_cast<int>(storage[0]), 255);
}

RC_TEST("sixteen bits go out most significant byte first") {
  // The check that catches an encoder writing the memory of the value instead
  // of shifting it. A round trip alone passes with the order reversed at both
  // ends, which is exactly the bug that reaches the device.
  auto storage = buffer_of(2);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};
  RC_REQUIRE(writer.put_u16(0x0102));

  RC_CHECK_EQ(static_cast<int>(storage[0]), 0x01);
  RC_CHECK_EQ(static_cast<int>(storage[1]), 0x02);
}

RC_TEST("thirty two bits go out most significant byte first") {
  auto storage = buffer_of(4);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};
  RC_REQUIRE(writer.put_u32(0x01020304u));

  RC_CHECK_EQ(static_cast<int>(storage[0]), 0x01);
  RC_CHECK_EQ(static_cast<int>(storage[1]), 0x02);
  RC_CHECK_EQ(static_cast<int>(storage[2]), 0x03);
  RC_CHECK_EQ(static_cast<int>(storage[3]), 0x04);
}

RC_TEST("a negative number goes out as its two's complement bits") {
  auto storage = buffer_of(2);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};
  RC_REQUIRE(writer.put_i16(-2));

  RC_CHECK_EQ(static_cast<int>(storage[0]), 0xFF);
  RC_CHECK_EQ(static_cast<int>(storage[1]), 0xFE);
}

RC_TEST("a float goes out as the bits IEEE 754 says it is") {
  // 1.0f is 0x3F800000. Pinning a known value proves the bits travelled rather
  // than that some transformation was applied consistently in both directions.
  auto storage = buffer_of(4);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};
  RC_REQUIRE(writer.put_f32(1.0f));

  RC_CHECK_EQ(static_cast<int>(storage[0]), 0x3F);
  RC_CHECK_EQ(static_cast<int>(storage[1]), 0x80);
  RC_CHECK_EQ(static_cast<int>(storage[2]), 0x00);
  RC_CHECK_EQ(static_cast<int>(storage[3]), 0x00);
}

RC_TEST("writing more than fits fails, and writes nothing at all") {
  auto storage = buffer_of(3);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};

  RC_CHECK(!writer.put_u32(0xDEADBEEFu));
  RC_CHECK_EQ(writer.written(), static_cast<std::size_t>(0));
  RC_CHECK(!writer.ok());

  // The filler is untouched. A partial write would leave a message that looks
  // like a message and is not one.
  RC_CHECK_EQ(static_cast<int>(storage[0]), 0xAA);
  RC_CHECK_EQ(static_cast<int>(storage[1]), 0xAA);
  RC_CHECK_EQ(static_cast<int>(storage[2]), 0xAA);
}

RC_TEST("ok stays false once it has been false") {
  auto storage = buffer_of(4);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};

  RC_REQUIRE(writer.put_u16(1));
  RC_CHECK(!writer.put_u32(1));   // only two bytes left
  RC_CHECK(writer.put_u8(2));     // this one fits
  RC_CHECK(!writer.ok());         // and the failure is still recorded
}

RC_TEST("bytes read back most significant first") {
  const std::vector<std::uint8_t> wire{0x01, 0x02, 0x03, 0x04};
  ByteReader reader{rc::span<const std::uint8_t>(wire.data(), wire.size())};

  std::uint32_t value = 0;
  RC_REQUIRE(reader.get_u32(value));
  RC_CHECK_EQ(value, 0x01020304u);
}

RC_TEST("a byte with its high bit set does not pollute the bytes above it") {
  // The check that catches a decoder built on char. It has to be the low byte
  // that has the high bit set, and that detail is the whole trap: read as a
  // signed char, 0xE8 is -24, whose upper bits are all ones, and the or fills
  // every bit above it. Put the high bit in the top byte instead and the final
  // truncation to sixteen bits hides the damage, so the obvious test passes
  // while the decoder is broken.
  const std::vector<std::uint8_t> wire{0x03, 0xE8};   // 1000
  ByteReader reader{rc::span<const std::uint8_t>(wire.data(), wire.size())};

  std::uint16_t value = 0;
  RC_REQUIRE(reader.get_u16(value));
  RC_CHECK_EQ(value, static_cast<std::uint16_t>(1000));

  const std::vector<std::uint8_t> high_wire{0xFF, 0x01};
  ByteReader high_reader{rc::span<const std::uint8_t>(high_wire.data(), high_wire.size())};
  std::uint16_t high_value = 0;
  RC_REQUIRE(high_reader.get_u16(high_value));
  RC_CHECK_EQ(high_value, 0xFF01u);
}

RC_TEST("a byte too many is refused rather than written past the end") {
  // The buffer is exactly full, so this call has nowhere to put anything. A
  // bounds check placed after the write instead of before it passes every
  // other test in this file and corrupts whatever follows the buffer, which
  // the address sanitizer reports and an ordinary build does not.
  auto storage = buffer_of(2);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};
  RC_REQUIRE(writer.put_u8(1));
  RC_REQUIRE(writer.put_u8(2));

  RC_CHECK(!writer.put_u8(3));
  RC_CHECK_EQ(writer.written(), static_cast<std::size_t>(2));
  RC_CHECK(!writer.ok());
}

RC_TEST("reading past the end fails and does not invent a value") {
  const std::vector<std::uint8_t> wire{0x01, 0x02};
  ByteReader reader{rc::span<const std::uint8_t>(wire.data(), wire.size())};

  std::uint32_t value = 0xCCCCCCCCu;
  RC_CHECK(!reader.get_u32(value));
  RC_CHECK_EQ(value, 0xCCCCCCCCu);   // untouched
  RC_CHECK(!reader.ok());
  RC_CHECK_EQ(reader.read(), static_cast<std::size_t>(0));
}

RC_TEST("every signed value survives the trip, including the extremes") {
  const std::int32_t values[] = {0, 1, -1, 42, -42, 2147483647, -2147483647 - 1};

  for (const std::int32_t original : values) {
    auto storage = buffer_of(4);
    ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};
    RC_REQUIRE(writer.put_i32(original));

    ByteReader reader{rc::span<const std::uint8_t>(storage.data(), storage.size())};
    std::int32_t returned = 0;
    RC_REQUIRE(reader.get_i32(returned));
    RC_CHECK_EQ(returned, original);
  }
}

RC_TEST("a float survives the trip exactly, not approximately") {
  const float values[] = {0.0f, 1.0f, -1.0f, 3.14159f, -0.0f, 1e-8f, 1e8f};

  for (const float original : values) {
    auto storage = buffer_of(4);
    ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};
    RC_REQUIRE(writer.put_f32(original));

    ByteReader reader{rc::span<const std::uint8_t>(storage.data(), storage.size())};
    float returned = 1234.5f;
    RC_REQUIRE(reader.get_f32(returned));

    // Compared as bits, not as numbers. The encoder must not lose or change
    // anything, and comparing with a tolerance would hide exactly that.
    std::uint32_t original_bits = 0;
    std::uint32_t returned_bits = 0;
    std::memcpy(&original_bits, &original, sizeof(original_bits));
    std::memcpy(&returned_bits, &returned, sizeof(returned_bits));
    RC_CHECK_EQ(returned_bits, original_bits);
  }
}

RC_TEST("a whole message goes out and comes back") {
  auto storage = buffer_of(16);
  ByteWriter writer{rc::span<std::uint8_t>(storage.data(), storage.size())};

  writer.put_u8(0x2A);
  writer.put_u16(1000);
  writer.put_i32(-12345);
  writer.put_f32(2.5f);
  RC_REQUIRE(writer.ok());
  RC_CHECK_EQ(writer.written(), static_cast<std::size_t>(11));

  ByteReader reader{rc::span<const std::uint8_t>(storage.data(), writer.written())};
  std::uint8_t id = 0;
  std::uint16_t sequence = 0;
  std::int32_t offset = 0;
  float value = 0.0f;

  reader.get_u8(id);
  reader.get_u16(sequence);
  reader.get_i32(offset);
  reader.get_f32(value);

  RC_REQUIRE(reader.ok());
  RC_CHECK_EQ(static_cast<int>(id), 0x2A);
  RC_CHECK_EQ(sequence, static_cast<std::uint16_t>(1000));
  RC_CHECK_EQ(offset, -12345);
  RC_CHECK_NEAR(static_cast<double>(value), 2.5, 1e-12);
  RC_CHECK_EQ(reader.remaining(), static_cast<std::size_t>(0));
}
