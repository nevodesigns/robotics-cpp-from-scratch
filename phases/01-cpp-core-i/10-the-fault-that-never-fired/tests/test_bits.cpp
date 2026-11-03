#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// A motor driver status register, laid out the way a data sheet lays one out.
constexpr Bits kOverCurrent = Bits{1u} << 0;
constexpr Bits kOverTemp = Bits{1u} << 1;
constexpr Bits kStalled = Bits{1u} << 2;
constexpr Bits kEncoderLost = Bits{1u} << 3;

// Every fault the driver knows about, which is what has_all is asked about.
constexpr Bits kAllFaults = kOverCurrent | kOverTemp | kStalled | kEncoderLost;

std::string as_hex(Bits value) {
  static const char* digits = "0123456789ABCDEF";
  std::string text = "0x";
  for (int shift = 28; shift >= 0; shift -= 4)
    text += digits[(value >> shift) & 0xFu];
  return text;
}

}  // namespace

RC_TEST("the precedence mistake reports the wrong fault, quietly") {
  // Over temperature and stalled. Over current is not set.
  const Bits status = kOverTemp | kStalled;

  // What the check is meant to ask.
  const bool meant = has_any(status, kOverTemp);

  // What `status & kOverTemp != 0` actually parses as. Written with the
  // parentheses the language supplies, because writing it without them is a
  // warning on gcc and clang and this suite builds clean.
  const Bits misparsed = status & static_cast<Bits>(kOverTemp != 0u);

  std::cout << "\n  status " << as_hex(status)
            << ": over temperature and stalled are set\n\n";
  std::cout << "    has_any(status, kOverTemp)        " << (meant ? "true" : "false")
            << "\n";
  std::cout << "    status & kOverTemp != 0  parses as status & (kOverTemp != 0)\n";
  std::cout << "                                      = status & 1 = " << misparsed
            << ", so " << (misparsed != 0u ? "true" : "false") << "\n\n";

  // The fault is there and the mistaken check says it is not.
  RC_CHECK(meant);
  RC_CHECK(misparsed == 0u);

  // And the reason it is so hard to spot: the mistaken expression is not
  // always wrong. It answers a question about bit zero, so whenever the fault
  // being looked for happens to be bit zero, it agrees.
  const Bits with_over_current = status | kOverCurrent;
  RC_CHECK(has_any(with_over_current, kOverCurrent));
  RC_CHECK((with_over_current & static_cast<Bits>(kOverCurrent != 0u)) != 0u);

  std::cout << "    with over current also set, the mistaken form agrees, because\n";
  std::cout << "    the question it really asks is about bit zero\n\n";
}

RC_TEST("any is not all, and the difference is a whole mask") {
  const Bits status = kOverTemp;

  // One of the four faults is live.
  RC_CHECK(has_any(status, kAllFaults));
  RC_CHECK(!has_all(status, kAllFaults));

  // A mask of one bit cannot tell the two apart, which is why the mistake
  // survives testing on single flags.
  RC_CHECK(has_any(status, kOverTemp));
  RC_CHECK(has_all(status, kOverTemp));

  // Two bits, one set.
  const Bits two = kOverTemp | kStalled;
  RC_CHECK(has_any(status, two));
  RC_CHECK(!has_all(status, two));

  // Both set.
  RC_CHECK(has_all(kOverTemp | kStalled, two));

  // An empty mask. Every one of no bits is set, and no bit of it is set.
  RC_CHECK(has_all(status, 0u));
  RC_CHECK(!has_any(status, 0u));

  std::cout << "  a one bit mask cannot distinguish any from all, which is how\n";
  std::cout << "  the wrong one of the two reaches a release\n\n";
}

RC_TEST("a bit index that does not exist gives no bits, rather than undefined behaviour") {
  std::cout << "  " << std::left << std::setw(10) << "index" << std::setw(14) << "valid"
            << "bit(index)\n" << std::right;

  const int indices[] = {0, 1, 7, 31, 32, 33, 99, -1};
  for (int index : indices) {
    const bool valid = is_valid_bit_index(index);
    std::cout << "  " << std::left << std::setw(10) << index << std::setw(14)
              << (valid ? "yes" : "NO") << as_hex(bit(index)) << "\n" << std::right;
  }
  std::cout << "\n";

  // Every legal index gives exactly one bit, and they are all different.
  Bits seen = 0u;
  for (int index = 0; index < kBitCount; ++index) {
    const Bits one = bit(index);
    RC_CHECK(is_single_bit(one));
    RC_CHECK(!has_any(seen, one));
    seen = with_set(seen, one);
  }
  // All thirty two of them, which is every bit in the word.
  RC_CHECK(seen == 0xFFFFFFFFu);
  RC_CHECK(count_set(seen) == kBitCount);

  // Out of range gives nothing, on both sides.
  RC_CHECK(bit(kBitCount) == 0u);
  RC_CHECK(bit(99) == 0u);
  RC_CHECK(bit(-1) == 0u);
  RC_CHECK(!is_valid_bit_index(kBitCount));
  RC_CHECK(!is_valid_bit_index(-1));

  // And zero is chosen so that a bad index cannot corrupt anything: it is the
  // identity for setting and it matches nothing when tested.
  const Bits status = kOverTemp | kStalled;
  RC_CHECK(with_set(status, bit(99)) == status);
  RC_CHECK(with_cleared(status, bit(99)) == status);
  RC_CHECK(!has_any(status, bit(99)));
}

RC_TEST("clearing a bit leaves the others exactly as they were") {
  const Bits status = kOverTemp | kStalled | kEncoderLost;

  const Bits cleared = with_cleared(status, kStalled);
  RC_CHECK(has_any(cleared, kOverTemp));
  RC_CHECK(!has_any(cleared, kStalled));
  RC_CHECK(has_any(cleared, kEncoderLost));
  RC_CHECK(count_set(cleared) == 2);

  // Clearing a bit that was not set changes nothing.
  RC_CHECK(with_cleared(cleared, kOverCurrent) == cleared);

  // Clearing everything, and setting everything.
  RC_CHECK(with_cleared(status, kAllFaults) == 0u);
  RC_CHECK(with_set(0u, kAllFaults) == kAllFaults);

  // The complement is a full width operation, so nothing above the mask is
  // disturbed. A register with its top bit set keeps it.
  const Bits top = bit(31);
  RC_CHECK(with_cleared(with_set(status, top), kStalled) == (cleared | top));
  RC_CHECK(has_any(with_cleared(top, kStalled), top));

  std::cout << "  status " << as_hex(status) << " with the stall bit cleared is "
            << as_hex(cleared) << "\n";
  std::cout << "  the same with bit 31 also set:  "
            << as_hex(with_cleared(with_set(status, top), kStalled)) << "\n\n";
}

RC_TEST("a narrow register promotes before it shifts, and the top bit goes nowhere") {
  // This is the trap the fixed width avoids, demonstrated so the reason is
  // visible rather than asserted.
  const std::uint8_t narrow = 0x80u;

  // The expression is an int, so nothing is lost.
  const int promoted = narrow << 1;

  // Stored back into the narrow type, the bit is gone.
  const std::uint8_t stored = static_cast<std::uint8_t>(narrow << 1);

  std::cout << "  a uint8_t of 0x80\n\n";
  std::cout << "    narrow << 1 as an expression   " << promoted
            << "   (0x100, an int)\n";
  std::cout << "    stored back into a uint8_t     " << static_cast<int>(stored)
            << "     (the bit is gone)\n";
  std::cout << "    sizeof(narrow)                 " << sizeof(narrow) << "\n";
  std::cout << "    sizeof(narrow << 1)            " << sizeof(narrow << 1) << "\n\n";

  RC_CHECK(promoted == 0x100);
  RC_CHECK(stored == 0u);
  RC_CHECK(sizeof(narrow) == 1u);
  RC_CHECK(sizeof(narrow << 1) == sizeof(int));

  // The same value widened to the register type keeps the bit, because there
  // is somewhere for it to go.
  const Bits wide = narrow;
  RC_CHECK((wide << 1) == 0x100u);
  RC_CHECK(count_set(wide << 1) == 1);

  // And the complement of a narrow mask is a signed int with the whole top of
  // the word set, which is the other half of the same problem.
  const std::uint8_t narrow_mask = 0x02u;
  RC_CHECK(~narrow_mask == -3);
  RC_CHECK(sizeof(~narrow_mask) == sizeof(int));

  // At the register width it is what it looks like.
  RC_CHECK(with_cleared(0xFFFFFFFFu, kOverTemp) == 0xFFFFFFFDu);
}

RC_TEST("a flag constant that is not a single bit is a transcription error") {
  // Every flag from the data sheet, checked once. This is a startup assertion
  // worth having in a real driver.
  const std::vector<Bits> flags = {kOverCurrent, kOverTemp, kStalled, kEncoderLost};
  for (Bits flag : flags) RC_CHECK(is_single_bit(flag));

  // The typo it catches: 0x03 where 0x02 was meant, which reports two faults
  // for one and clears a neighbour when acknowledged.
  RC_CHECK(!is_single_bit(0x03u));
  RC_CHECK(count_set(0x03u) == 2);

  RC_CHECK(!is_single_bit(0u));
  RC_CHECK(is_single_bit(1u));
  RC_CHECK(is_single_bit(bit(31)));
  RC_CHECK(!is_single_bit(0xFFFFFFFFu));

  // The flags do not overlap each other, which a data sheet guarantees and a
  // transcription does not.
  Bits combined = 0u;
  for (Bits flag : flags) {
    RC_CHECK(!has_any(combined, flag));
    combined = with_set(combined, flag);
  }
  RC_CHECK(combined == kAllFaults);
  RC_CHECK(count_set(kAllFaults) == 4);
}

RC_TEST("decoding a register the way a driver has to") {
  struct Named {
    const char* name;
    Bits mask;
  };
  const Named named[] = {{"over current", kOverCurrent},
                         {"over temperature", kOverTemp},
                         {"stalled", kStalled},
                         {"encoder lost", kEncoderLost}};

  const Bits status = kOverTemp | kEncoderLost;

  std::cout << "  register " << as_hex(status) << ", " << count_set(status)
            << " faults live\n\n";
  int reported = 0;
  for (const Named& entry : named) {
    const bool live = has_any(status, entry.mask);
    if (live) ++reported;
    std::cout << "    " << std::left << std::setw(20) << entry.name
              << (live ? "set" : "clear") << "\n" << std::right;
  }
  std::cout << "\n";

  RC_CHECK(reported == 2);
  RC_CHECK(reported == count_set(status));

  // Acknowledging one fault leaves the other, which is the whole reason the
  // operations are per bit rather than per register.
  const Bits after = with_cleared(status, kOverTemp);
  RC_CHECK(count_set(after) == 1);
  RC_CHECK(has_any(after, kEncoderLost));

  // Nothing outside the known faults is set, which is how a driver notices a
  // bit the data sheet did not mention.
  RC_CHECK(!has_any(status, ~kAllFaults));
  RC_CHECK(has_all(kAllFaults, status));
}
