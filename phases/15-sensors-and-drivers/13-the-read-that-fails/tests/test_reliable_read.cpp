#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
};

const std::size_t kFrame = 5;   // four payload bytes and the CRC

// A bus that times out with probability p_timeout, and otherwise returns the frame with one bit flipped with
// probability p_flip. The frame it holds has a correct CRC. The CRC is worked out here, bit by bit, on the bus's side.
struct FlakyBus {
  Rng rng;
  double p_timeout;
  double p_flip;
  std::uint8_t frame[kFrame];
  int reads = 0;

  static std::uint8_t reference_crc(const std::uint8_t* d, std::size_t n) {
    unsigned crc = 0;
    for (std::size_t i = 0; i < n; ++i) {
      crc ^= d[i];
      for (int b = 0; b < 8; ++b) crc = (crc & 0x80u) ? ((crc << 1) ^ 0x07u) & 0xFFu : (crc << 1) & 0xFFu;
    }
    return static_cast<std::uint8_t>(crc);
  }

  FlakyBus(std::uint32_t seed, double timeout, double flip) : rng{seed}, p_timeout(timeout), p_flip(flip) {
    const std::uint8_t payload[4] = {0x12, 0x34, 0x56, 0x78};
    std::memcpy(frame, payload, 4);
    frame[4] = reference_crc(payload, 4);
  }

  bool operator()(std::uint8_t* out, std::size_t n) {
    ++reads;
    if (rng.uniform() < p_timeout) return false;
    std::memcpy(out, frame, n);
    if (rng.uniform() < p_flip) {
      const std::uint32_t bit = rng.next() % (static_cast<std::uint32_t>(n) * 8u);
      out[bit / 8] = static_cast<std::uint8_t>(out[bit / 8] ^ (1u << (bit % 8)));
    }
    return true;
  }
};

}  // namespace

RC_TEST("the CRC-8 of the standard test string is 0xF4, and a frame checks only if its last byte is the CRC") {
  const char* text = "123456789";
  const std::uint8_t crc = crc8(reinterpret_cast<const std::uint8_t*>(text), 9);
  std::cout << "  crc8 of 123456789: 0x" << std::hex << std::uppercase << static_cast<int>(crc) << std::dec << "\n\n";
  RC_CHECK(crc == 0xF4);
  RC_CHECK(crc8(reinterpret_cast<const std::uint8_t*>(text), 0) == 0);

  FlakyBus bus(1, 0.0, 0.0);
  RC_CHECK(frame_ok(bus.frame, kFrame));
  RC_CHECK(!frame_ok(bus.frame, 1));
  RC_CHECK(!frame_ok(nullptr, kFrame));
  std::uint8_t bad[kFrame];
  std::memcpy(bad, bus.frame, kFrame);
  bad[4] = static_cast<std::uint8_t>(bad[4] ^ 1);
  RC_CHECK(!frame_ok(bad, kFrame));
}

RC_TEST("a CRC catches every single bit flip and every swap of two bytes, which a plain sum does not") {
  FlakyBus bus(1, 0.0, 0.0);
  int missed_flips = 0;
  for (std::size_t bit = 0; bit < kFrame * 8; ++bit) {
    std::uint8_t f[kFrame];
    std::memcpy(f, bus.frame, kFrame);
    f[bit / 8] = static_cast<std::uint8_t>(f[bit / 8] ^ (1u << (bit % 8)));
    if (frame_ok(f, kFrame)) ++missed_flips;
  }
  int missed_swaps = 0;
  int sum_missed_swaps = 0;
  int swaps = 0;
  for (std::size_t i = 0; i < 4; ++i)
    for (std::size_t j = i + 1; j < 4; ++j) {
      if (bus.frame[i] == bus.frame[j]) continue;
      ++swaps;
      std::uint8_t f[kFrame];
      std::memcpy(f, bus.frame, kFrame);
      const std::uint8_t t = f[i];
      f[i] = f[j];
      f[j] = t;
      if (frame_ok(f, kFrame)) ++missed_swaps;
      unsigned sum_before = 0;
      unsigned sum_after = 0;
      for (std::size_t k = 0; k < 4; ++k) {
        sum_before += bus.frame[k];
        sum_after += f[k];
      }
      if ((sum_before & 0xFF) == (sum_after & 0xFF)) ++sum_missed_swaps;
    }
  std::cout << "  of 40 single bit flips the CRC missed " << missed_flips << "; of " << swaps
            << " swaps of two bytes the CRC missed " << missed_swaps << " and a plain sum missed " << sum_missed_swaps << "\n\n";
  RC_CHECK(frame_ok(bus.frame, kFrame));
  RC_CHECK(missed_flips == 0);
  RC_CHECK(swaps > 0);
  RC_CHECK(missed_swaps == 0);
  RC_CHECK(sum_missed_swaps == swaps);
}

RC_TEST("a bounded read gives up after its attempts and says whether it timed out or got a bad frame") {
  // A bus that always times out.
  FlakyBus dead(1, 1.0, 0.0);
  std::uint8_t frame[kFrame];
  const ReadOutcome a = read_reliably(dead, 4, frame, kFrame);
  RC_CHECK(!a.ok);
  RC_CHECK(a.attempts == 4);
  RC_CHECK(a.timeouts == 4);
  RC_CHECK(a.corrupt == 0);

  // A bus that always flips a bit.
  FlakyBus noisy(1, 0.0, 1.0);
  const ReadOutcome b = read_reliably(noisy, 3, frame, kFrame);
  RC_CHECK(!b.ok);
  RC_CHECK(b.attempts == 3);
  RC_CHECK(b.corrupt == 3);
  RC_CHECK(b.timeouts == 0);

  // A good bus succeeds on the first attempt and stops.
  FlakyBus fine(1, 0.0, 0.0);
  const ReadOutcome c = read_reliably(fine, 5, frame, kFrame);
  RC_CHECK(c.ok);
  RC_CHECK(c.attempts == 1);
  RC_CHECK(fine.reads == 1);
  RC_CHECK(frame_ok(frame, kFrame));
  std::cout << "  dead bus: " << a.attempts << " attempts, " << a.timeouts << " timeouts; noisy bus: " << b.attempts
            << " attempts, " << b.corrupt << " bad frames; good bus: " << c.attempts << " attempt\n\n";
}

RC_TEST("with failures at random, the fraction that never gets through is the failure rate to the power of the attempts") {
  // Each attempt fails with probability 1 - (1 - 0.2) * (1 - 0.1) = 0.28 (a timeout, or a flipped bit).
  const double p_fail = 1.0 - (1.0 - 0.2) * (1.0 - 0.1);
  std::cout << "  each attempt fails with probability " << std::fixed << std::setprecision(2) << p_fail
            << " (20 percent timeouts, 10 percent flipped bits)\n  attempts   gave up   predicted   mean attempts used\n";
  bool close = true;
  for (int attempts = 1; attempts <= 5; ++attempts) {
    FlakyBus bus(7, 0.2, 0.1);
    int gave_up = 0;
    long used = 0;
    const int reads = 20000;
    for (int i = 0; i < reads; ++i) {
      std::uint8_t frame[kFrame];
      const ReadOutcome o = read_reliably(bus, attempts, frame, kFrame);
      if (!o.ok) ++gave_up;
      used += o.attempts;
    }
    const double measured = static_cast<double>(gave_up) / reads;
    const double predicted = 1.0 - success_after(p_fail, attempts);
    std::cout << "  " << std::setw(8) << attempts << "   " << std::setw(7) << std::setprecision(4) << measured << "   "
              << std::setw(9) << predicted << "   " << std::setw(18) << std::setprecision(3)
              << static_cast<double>(used) / reads << "\n";
    if (std::fabs(measured - predicted) > 0.01) close = false;
  }
  std::cout << "\n";
  RC_CHECK(close);
  RC_CHECK_NEAR(success_after(0.5, 3), 0.875, 1e-12);
  RC_CHECK_NEAR(success_after(0.0, 3), 1.0, 1e-12);
  RC_CHECK_NEAR(success_after(1.0, 3), 0.0, 1e-12);
  RC_CHECK_NEAR(success_after(0.5, 0), 0.0, 1e-12);
}

RC_TEST("how many attempts are enough comes from the failure rate and the target, and some targets cannot be met") {
  std::cout << "  failure rate   for 99 percent   for 99.99 percent\n";
  const double rates[] = {0.05, 0.1, 0.28, 0.5, 0.9};
  for (double p : rates)
    std::cout << "  " << std::setw(12) << std::fixed << std::setprecision(2) << p << "   " << std::setw(14)
              << attempts_needed(p, 0.99) << "   " << std::setw(17) << attempts_needed(p, 0.9999) << "\n";
  std::cout << "\n";
  RC_CHECK(attempts_needed(0.5, 0.99) == 7);
  RC_CHECK(attempts_needed(0.1, 0.99) == 2);
  RC_CHECK(attempts_needed(0.0, 0.99) == 1);
  RC_CHECK(attempts_needed(1.0, 0.5) == 0);
  RC_CHECK(attempts_needed(0.5, 1.0) == 0);
  RC_CHECK(attempts_needed(0.5, 0.0) == 1);
  RC_CHECK(attempts_needed(0.28, 0.99) > attempts_needed(0.1, 0.99));
}
