#include <rc/test/rc_test.hpp>
#include <rc/test/repeat.hpp>

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

// File scope, not inside a test. A constexpr local used in a lambda needs no
// capture on gcc and clang and is rejected by MSVC as C3493, which this
// repository has met four times, and audit rule L022 now refuses. E-CPP-0023
// is the entry.
constexpr std::uint32_t kAllOnes = 0xFFFFFFFFu;
constexpr std::uint32_t kAlternating = 0xAAAAAAAAu;

// These prove that an evaluation happened during compilation without saying
// anything about the answer, which is the distinction the lesson turns on.
//
// A constexpr variable may only be initialised by a constant expression, so
// each line below compiles if and only if the compiler could work the value
// out for itself. What the value is then checked at runtime, separately,
// because a static_assert on the value would stop the exercise compiling at
// all and a learner needs a suite that runs and reports.
constexpr std::size_t kTableSize = kBitsPerByte.size();
constexpr std::uint8_t kBitsIn255 = kBitsPerByte[255];
constexpr std::uint8_t kBitsIn0xAA = kBitsPerByte[0xAA];
constexpr int kOnesInAllOnes = count_set_by_table(kAllOnes);
constexpr int kOnesInAlternating = count_set_by_table(kAlternating);
constexpr std::uint8_t kBitsInByteDirect = bits_in_byte(0xAA);

RC_TEST("the table exists before the program runs") {
  // That the file compiled at all is the compile-time proof: the constants at
  // the top of this file are constexpr, so each one had to be computed by the
  // compiler. Nothing ran at startup to fill the table in.
  static_assert(kTableSize == 256, "the size is fixed by the type, not by the work");

  // The answers are a separate question, checked here.
  RC_CHECK(kTableSize == 256u);
  RC_CHECK(kBitsIn255 == 8);
  RC_CHECK(kBitsIn0xAA == 4);
  RC_CHECK(kOnesInAllOnes == 32);
  RC_CHECK(kOnesInAlternating == 16);
  RC_CHECK(kBitsPerByte[0] == 0);
  RC_CHECK(kBitsPerByte[1] == 1);
  RC_CHECK(kBitsPerByte[3] == 2);

  std::cout << "  256 entries, computed during compilation\n\n";
  std::cout << "    kBitsPerByte[255]              " << static_cast<int>(kBitsIn255) << "\n";
  std::cout << "    kBitsPerByte[0xAA]             " << static_cast<int>(kBitsIn0xAA) << "\n";
  std::cout << "    count_set_by_table(0xFFFFFFFF) " << kOnesInAllOnes << "\n";
  std::cout << "    count_set_by_table(0xAAAAAAAA) " << kOnesInAlternating << "\n\n";
  std::cout << "  each of those is a constexpr variable at file scope, so it\n";
  std::cout << "  compiles only if the compiler could work the value out. The\n";
  std::cout << "  value itself is checked above, at runtime, on purpose: a\n";
  std::cout << "  static_assert on the answer would mean an unfinished\n";
  std::cout << "  exercise could not be built, and a suite that cannot run\n";
  std::cout << "  tells a learner nothing about which function to write.\n\n";
}

RC_TEST("the table and the loop agree on every value a byte can hold") {
  // Anchored first, and the reason is worth keeping. Written as a pure
  // agreement check, this test passed with both functions stubbed to return
  // 0, because two implementations that are equally wrong agree perfectly.
  // Agreement is evidence that one of them is not uniquely broken, and it is
  // not evidence that either is right.
  RC_CHECK(count_set_by_loop(0u) == 0);
  RC_CHECK(count_set_by_loop(1u) == 1);
  RC_CHECK(count_set_by_loop(0xFFu) == 8);
  RC_CHECK(count_set_by_loop(kAllOnes) == 32);
  RC_CHECK(count_set_by_table(0xFFu) == 8);

  // Not a sample. All 256, because there are only 256 and a sampled test of
  // an exhaustive domain is a choice to know less for no saving.
  int mismatches = 0;
  for (std::uint32_t byte = 0; byte < 256; ++byte)
    if (count_set_by_table(byte) != count_set_by_loop(byte)) ++mismatches;
  RC_CHECK(mismatches == 0);

  // And a spread of wider values, where exhaustive is 4 billion and a
  // deliberate selection is the honest alternative.
  const std::vector<std::uint32_t> wide = {0u,          1u,          kAllOnes,
                                           kAlternating, 0x55555555u, 0x80000001u,
                                           0x0000FFFFu, 0xFFFF0000u, 0x12345678u};
  for (const std::uint32_t value : wide)
    RC_CHECK(count_set_by_table(value) == count_set_by_loop(value));

  std::cout << "  all 256 byte values agree, and " << wide.size()
            << " chosen 32 bit values agree\n\n";
}

// Where the measured work is written so the compiler cannot delete it.
//
// The first version of the suite below ended each timed loop with
// `if (total < 0) std::cout << "";`, which reads like it keeps the work and
// does not. At -O2 both loops measured 0.0 us and the test failed, because
// the accumulation was dead and the optimiser removed it. A volatile store is
// the cheap fix: the compiler is not allowed to elide it, so everything the
// total depends on has to happen.
volatile int sink = 0;

RC_TEST("four lookups against thirty two iterations") {
  // A million values, the same ones for both, so the comparison is of the two
  // functions and not of two different workloads.
  std::vector<std::uint32_t> values;
  values.reserve(100000);
  std::uint32_t state = 0x12345678u;
  for (std::size_t i = 0; i < 100000; ++i) {
    state = state * 1664525u + 1013904223u;  // unsigned, so this wraps and is defined
    values.push_back(state);
  }

  const auto time_table = [&values]() {
    const auto start = std::chrono::steady_clock::now();
    int total = 0;
    for (const std::uint32_t value : values) total += count_set_by_table(value);
    sink = total;
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
  };

  const auto time_loop = [&values]() {
    const auto start = std::chrono::steady_clock::now();
    int total = 0;
    for (const std::uint32_t value : values) total += count_set_by_loop(value);
    sink = total;
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
  };

  // Best of several, interleaved, because the machine is shared and noise
  // only ever adds time. Lesson 05-05 measured why the minimum is the
  // statistic and the mean is not.
  double table_best = -1.0;
  double loop_best = -1.0;
  for (int round = 0; round < 7; ++round) {
    const double t = time_table();
    const double l = time_loop();
    if (table_best < 0.0 || t < table_best) table_best = t;
    if (loop_best < 0.0 || l < loop_best) loop_best = l;
  }

  std::cout << "  100000 values, best of 7 interleaved rounds\n\n";
  std::cout << "    four table lookups   " << std::fixed << std::setprecision(1)
            << table_best << " us\n";
  std::cout << "    thirty two iterations " << loop_best << " us\n";
  std::cout << "    ratio                 " << std::setprecision(2)
            << (table_best > 0.0 ? loop_best / table_best : 0.0) << "x\n\n";

  // The direction is asserted and the ratio is not. A number here would be a
  // claim about somebody else's machine, and the point of the table is not
  // that it is a particular multiple faster.
  RC_CHECK(table_best > 0.0);
  RC_CHECK(loop_best > 0.0);
  RC_CHECK(loop_best > table_best);

  std::cout << "  the speed is the smaller half of the argument. The table\n";
  std::cout << "  costs 256 bytes of read only data and no startup work at\n";
  std::cout << "  all, which on a controller that has to be ready when power\n";
  std::cout << "  comes up is worth more than the microseconds.\n\n";
}

RC_TEST("constexpr is permission, not instruction") {
  // The same function, used two ways. kBitsInByteDirect at the top of this
  // file is constexpr, so that call was made by the compiler.

  // This one cannot be: the value came out of a container filled at runtime.
  std::vector<std::size_t> input = {0xAA};
  const std::uint8_t at_runtime = bits_in_byte(input[0]);
  RC_CHECK(at_runtime == 4);

  // Same function, same answer, two different machines did the work.
  RC_CHECK(kBitsInByteDirect == 4);
  RC_CHECK(kBitsInByteDirect == at_runtime);

  // A constexpr variable is how you insist. Written with input[0] in place of
  // the literal it would not compile, and checks/ holds that refusal.

  std::cout << "  bits_in_byte(0xAA) is 4 whether the compiler worked it out\n";
  std::cout << "  or the processor did, and the answer does not say which.\n\n";
  std::cout << "  this is the honest limit of the lesson: a static_assert\n";
  std::cout << "  proves an evaluation happened at compile time, and nothing\n";
  std::cout << "  here can prove one did not. So when it matters, do not\n";
  std::cout << "  hope. Demand it, by requiring the result where only a\n";
  std::cout << "  constant will do.\n\n";
}
