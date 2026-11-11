#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

std::string hex(const std::vector<std::uint8_t>& v) {
  std::string s;
  const char* digits = "0123456789ABCDEF";
  for (std::uint8_t b : v) {
    s += digits[b >> 4];
    s += digits[b & 15];
    s += ' ';
  }
  return s;
}

// What a compiler makes of the obvious struct, for comparison. Its size and padding are the compiler's business and differ between them.
struct Naive {
  std::uint8_t kind;
  double value;
  std::uint16_t count;
};

}  // namespace

RC_TEST("integers are written low byte first, one after the other with nothing between them") {
  ByteWriter w;
  w.put_u8(0xAB);
  w.put_u16(0x1234);
  w.put_u32(0xDEADBEEFu);
  std::cout << "  wire: " << hex(w.bytes()) << "\n\n";
  RC_CHECK(w.bytes().size() == 7);
  RC_CHECK(w.bytes().size() == 7 && w.bytes()[0] == 0xAB);
  RC_CHECK(w.bytes().size() == 7 && w.bytes()[1] == 0x34 && w.bytes()[2] == 0x12);
  RC_CHECK(w.bytes().size() == 7 && w.bytes()[3] == 0xEF && w.bytes()[4] == 0xBE && w.bytes()[5] == 0xAD && w.bytes()[6] == 0xDE);
}

RC_TEST("a message round trips exactly, a double to the last bit, a string with its length") {
  ByteWriter w;
  w.put_u32(7);
  w.put_f64(-0.1234567890123456);
  w.put_f64(1e300);
  w.put_string("base_link");
  w.put_string("");
  ByteReader r(w.bytes());
  RC_CHECK(r.get_u32() == 7);
  const double a = r.get_f64();
  const double b = r.get_f64();
  RC_CHECK(a == -0.1234567890123456);               // equal, not near
  RC_CHECK(b == 1e300);
  RC_CHECK(r.get_string() == "base_link");
  RC_CHECK(r.get_string().empty());
  RC_CHECK(r.ok());
  RC_CHECK(r.remaining() == 0);
  std::cout << "  " << w.bytes().size() << " bytes: 4 + 8 + 8 + (2 + 9) + 2\n\n";
  RC_CHECK(w.bytes().size() == 4 + 8 + 8 + 2 + 9 + 2);
}

RC_TEST("the wire size is the fields added up, and a struct written as it sits in memory is not") {
  ByteWriter w;
  w.put_u8(1);
  w.put_f64(2.0);
  w.put_u16(3);
  std::cout << "  the fields: 1 + 8 + 2 = " << w.bytes().size() << " bytes on the wire; the same struct in memory is " << sizeof(Naive)
            << " bytes on this compiler\n\n";
  RC_CHECK(w.bytes().size() == 11);
  RC_CHECK(sizeof(Naive) > 11);                       // padding after the first byte and at the end
}

RC_TEST("a read past the end sets a flag that stays set, and gives zero, and never reads beyond the bytes") {
  ByteWriter w;
  w.put_u16(5);
  w.put_u8(9);
  ByteReader r(w.bytes());
  RC_CHECK(r.get_u16() == 5);
  RC_CHECK(r.ok());
  RC_CHECK(r.get_u32() == 0);                          // only one byte left
  RC_CHECK(!r.ok());
  RC_CHECK(r.get_u8() == 0);                           // the flag stays set, and so a byte that is there is not handed out
  RC_CHECK(!r.ok());
  RC_CHECK(r.get_f64() == 0.0);
}

RC_TEST("a string whose length claims more than there is fails the read, and does not return what lies beyond") {
  ByteWriter w;
  w.put_u16(500);                                       // says 500 characters
  w.put_u8('h');
  w.put_u8('i');                                        // and has two
  ByteReader r(w.bytes());
  const std::string s = r.get_string();
  std::cout << "  a length of 500 with 2 bytes behind it: ok " << (r.ok() ? "yes" : "no") << ", string of " << s.size() << " characters\n\n";
  RC_CHECK(!r.ok());
  RC_CHECK(s.empty());
  ByteReader empty(nullptr, 0);
  RC_CHECK(empty.get_string().empty());
  RC_CHECK(!empty.ok());
}

RC_TEST("the type id is worked out from the name, so two names that differ by a letter or a version differ") {
  const std::uint32_t a = type_id("rc/msg/Pose2D");
  const std::uint32_t b = type_id("rc/msg/Pose2d");
  const std::uint32_t c = type_id("rc/msg/Pose2D/v2");
  std::cout << "  ids: Pose2D " << std::hex << a << ", Pose2d " << b << ", Pose2D/v2 " << c << std::dec << "\n\n";
  RC_CHECK(a != b);
  RC_CHECK(a != c);
  RC_CHECK(type_id("") == 2166136261u);
  RC_CHECK(type_id("a") == 0xE40C292Cu);                 // the published FNV-1a value for "a"
  RC_CHECK(type_id("rc/msg/Pose2D") == a);
}

RC_TEST("a frame says what it carries and how long it is, and a receiver that expected something else says so") {
  ByteWriter body;
  body.put_f64(1.5);
  body.put_f64(-2.5);
  const std::uint32_t pose = type_id("rc/msg/Pose2D");
  const std::vector<std::uint8_t> bytes = frame(pose, body.bytes());
  RC_CHECK(bytes.size() == 8 + 16);

  std::vector<std::uint8_t> payload;
  RC_CHECK(unframe(bytes, pose, payload) == FrameStatus::ok);
  RC_CHECK(payload == body.bytes());

  std::vector<std::uint8_t> untouched = {42};
  RC_CHECK(unframe(bytes, type_id("rc/msg/Twist"), untouched) == FrameStatus::wrong_type);
  RC_CHECK(untouched.size() == 1 && untouched[0] == 42);       // not filled in unless ok

  std::vector<std::uint8_t> cut(bytes.begin(), bytes.end() - 3);
  RC_CHECK(unframe(cut, pose, untouched) == FrameStatus::bad_length);
  std::vector<std::uint8_t> extra = bytes;
  extra.push_back(0);
  RC_CHECK(unframe(extra, pose, untouched) == FrameStatus::bad_length);
  std::vector<std::uint8_t> tiny(bytes.begin(), bytes.begin() + 5);
  RC_CHECK(unframe(tiny, pose, untouched) == FrameStatus::too_short);
  std::cout << "  a good frame, a frame for another type, one cut short, one with an extra byte, and one cut to 5 bytes: "
            << "ok, wrong_type, bad_length, bad_length, too_short\n\n";
  RC_CHECK(unframe(std::vector<std::uint8_t>(), pose, untouched) == FrameStatus::too_short);
}
