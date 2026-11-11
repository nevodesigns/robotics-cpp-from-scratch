#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// Pack a frame by hand from a count for each axis (-2048 to 2047), a range code and the two flags. This is the chip's
// side of the wire and does not use any of the code under test.
void pack(std::uint8_t* out, int x, int y, int z, int code, bool ready, bool overrun) {
  out[0] = static_cast<std::uint8_t>((ready ? 1 : 0) | (overrun ? 2 : 0) | (code << 4));
  const int counts[3] = {x, y, z};
  for (int i = 0; i < 3; ++i) {
    const unsigned twelve = static_cast<unsigned>(counts[i]) & 0xFFFu;
    const unsigned word = twelve << 4;
    out[1 + 2 * i] = static_cast<std::uint8_t>(word >> 8);
    out[2 + 2 * i] = static_cast<std::uint8_t>(word & 0xFF);
  }
}

}  // namespace

RC_TEST("bits are taken from the middle of a register by shifting and masking") {
  RC_CHECK(extract_bits(0xABCD, 0, 4) == 0xD);
  RC_CHECK(extract_bits(0xABCD, 4, 4) == 0xC);
  RC_CHECK(extract_bits(0xABCD, 8, 8) == 0xAB);
  RC_CHECK(extract_bits(0x30, 4, 2) == 3);
  RC_CHECK(extract_bits(0xFFFFFFFFu, 0, 32) == 0xFFFFFFFFu);
  RC_CHECK(extract_bits(0xFF, 0, 0) == 0);
  RC_CHECK(extract_bits(0xFF, -1, 4) == 0);
  RC_CHECK(extract_bits(0xFF, 32, 4) == 0);
}

RC_TEST("a twelve bit number with its top bit set is negative, and sign extension is what says so") {
  std::cout << "  raw 12 bit    unsigned   signed\n";
  const unsigned raws[] = {0x000, 0x001, 0x7FF, 0x800, 0xFFF};
  const int expected[] = {0, 1, 2047, -2048, -1};
  for (int i = 0; i < 5; ++i) {
    std::cout << "  0x" << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << raws[i] << std::dec
              << std::setfill(' ') << "     " << std::setw(8) << raws[i] << "   " << std::setw(6) << sign_extend(raws[i], 12) << "\n";
    RC_CHECK(sign_extend(raws[i], 12) == expected[i]);
  }
  std::cout << "\n";
  RC_CHECK(sign_extend(0x80, 8) == -128);
  RC_CHECK(sign_extend(0xFF, 8) == -1);
  RC_CHECK(sign_extend(0x1FF5, 12) == sign_extend(0xFF5, 12));   // bits above the width are ignored
}

RC_TEST("the range code selects a full scale of 2, 4, 8 or 16 g, and anything else selects none") {
  RC_CHECK_NEAR(range_in_g(0), 2.0, 1e-12);
  RC_CHECK_NEAR(range_in_g(1), 4.0, 1e-12);
  RC_CHECK_NEAR(range_in_g(2), 8.0, 1e-12);
  RC_CHECK_NEAR(range_in_g(3), 16.0, 1e-12);
  RC_CHECK_NEAR(range_in_g(4), 0.0, 1e-12);
  RC_CHECK_NEAR(range_in_g(-1), 0.0, 1e-12);
}

RC_TEST("a frame is decoded to g, with negative readings negative") {
  std::uint8_t frame[7];
  pack(frame, 1024, -512, -2048, 0, true, false);   // plus or minus 2 g: 1 g per 1024 counts
  AccelSample s;
  const DecodeStatus status = decode_frame(frame, 7, s);
  std::cout << "  frame:";
  for (int i = 0; i < 7; ++i)
    std::cout << " " << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(frame[i]);
  std::cout << std::dec << std::setfill(' ') << "\n  decoded at plus or minus 2 g: x " << std::fixed << std::setprecision(3) << s.x
            << ", y " << s.y << ", z " << s.z << " g\n\n";
  RC_CHECK(status == DecodeStatus::Ok);
  RC_CHECK_NEAR(s.x, 1.0, 1e-12);
  RC_CHECK_NEAR(s.y, -0.5, 1e-12);
  RC_CHECK_NEAR(s.z, -2.0, 1e-12);
  RC_CHECK(s.range_code == 0);
  RC_CHECK(!s.overrun);
}

RC_TEST("every one of the 4096 counts on every range decodes to the count times the size of a count") {
  long wrong = 0;
  long checked = 0;
  std::uint8_t frame[7];
  for (int code = 0; code < 4; ++code) {
    const double per_count = range_in_g(code) / 2048.0;
    for (int count = -2048; count <= 2047; ++count) {
      pack(frame, count, -count - 1, count, code, true, false);
      AccelSample s;
      if (decode_frame(frame, 7, s) != DecodeStatus::Ok) { ++wrong; continue; }
      ++checked;
      if (std::fabs(s.x - count * per_count) > 1e-12) ++wrong;
      if (std::fabs(s.y - (-count - 1) * per_count) > 1e-12) ++wrong;
      if (std::fabs(s.z - count * per_count) > 1e-12) ++wrong;
    }
  }
  std::cout << "  frames checked: " << checked << ", axes wrong: " << wrong << "\n  one count is "
            << std::fixed << std::setprecision(6) << range_in_g(0) / 2048.0 << " g at 2 g and " << range_in_g(3) / 2048.0
            << " g at 16 g\n\n";
  RC_CHECK(checked == 4 * 4096);
  RC_CHECK(wrong == 0);
}

RC_TEST("a frame that is short, not ready or overrun says so, and is not read as data") {
  std::uint8_t frame[7];
  pack(frame, 100, 200, 300, 1, true, false);
  AccelSample before;
  before.x = 7.0;

  AccelSample s = before;
  RC_CHECK(decode_frame(frame, 6, s) == DecodeStatus::TooShort);
  RC_CHECK(decode_frame(nullptr, 7, s) == DecodeStatus::TooShort);
  RC_CHECK_NEAR(s.x, 7.0, 1e-12);

  std::uint8_t waiting[7];
  pack(waiting, 100, 200, 300, 1, false, false);
  RC_CHECK(decode_frame(waiting, 7, s) == DecodeStatus::NotReady);
  RC_CHECK_NEAR(s.x, 7.0, 1e-12);

  std::uint8_t lost[7];
  pack(lost, 100, 200, 300, 1, true, true);
  RC_CHECK(decode_frame(lost, 7, s) == DecodeStatus::Overrun);
  RC_CHECK(s.overrun);
  RC_CHECK_NEAR(s.x, 100.0 * 4.0 / 2048.0, 1e-12);    // the data is good
  RC_CHECK(decode_frame(frame, 7, s) == DecodeStatus::Ok);
  std::cout << "  too short, not ready and overrun are three different answers, and an overrun frame still carries data: x "
            << std::fixed << std::setprecision(4) << s.x << " g\n\n";
}
