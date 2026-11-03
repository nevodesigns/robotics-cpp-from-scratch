#include <rc/test/rc_test.hpp>

#include <climits>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// Interesting values rather than a sweep: the two ends, the neighbours of the
// ends, the signs, and the square roots of the limits, which is where
// multiplication turns over.
const int kEdges[] = {INT_MIN, INT_MIN + 1, -65536, -46341, -46340, -3, -2, -1,
                      0,       1,           2,      3,      46340,  46341,
                      65536,   INT_MAX - 1, INT_MAX};

// The reference answer, computed in a type wide enough that it cannot overflow.
// Every assertion in this file is against this rather than against my opinion.
bool really_overflows(long long result) {
  return result > INT_MAX || result < INT_MIN;
}

}  // namespace

RC_TEST("the predictions are exactly right, everywhere it is swept") {
  // Four exhaustive sweeps, not a sample. One in the middle where nothing can
  // overflow, two against the ends where addition turns over, and one around
  // the square root of the limit where multiplication does.
  //
  // The middle window alone would be useless: every answer in it is no, and a
  // function that always answers no would pass it.
  struct Sweep {
    const char* name;
    int a_low;
    int a_high;
    int b_low;
    int b_high;
  };
  const Sweep sweeps[] = {
      {"around zero", -600, 600, -600, 600},
      {"against the top", INT_MAX - 1200, INT_MAX, -600, 600},
      {"against the bottom", INT_MIN, INT_MIN + 1200, -600, 600},
      {"where products turn over", 46041, 46641, 46041, 46641},
  };

  std::cout << "\n  " << std::left << std::setw(28) << "sweep" << std::right
            << std::setw(14) << "predictions" << std::setw(10) << "said yes"
            << std::setw(10) << "wrong" << "\n";

  long long total_made = 0;
  long long total_wrong = 0;
  long long total_yes = 0;
  for (const Sweep& sweep : sweeps) {
    long long made = 0;
    long long wrong = 0;
    long long yes = 0;

    for (int a = sweep.a_low;; ++a) {
      for (int b = sweep.b_low; b <= sweep.b_high; ++b) {
        const long long sum = static_cast<long long>(a) + b;
        const long long difference = static_cast<long long>(a) - b;
        const long long product = static_cast<long long>(a) * b;

        const bool said[] = {add_overflows(a, b), subtract_overflows(a, b),
                             multiply_overflows(a, b)};
        const bool truth[] = {really_overflows(sum), really_overflows(difference),
                              really_overflows(product)};
        for (int i = 0; i < 3; ++i) {
          ++made;
          if (said[i]) ++yes;
          if (said[i] != truth[i]) ++wrong;
        }
      }
      // Written as a break rather than a loop condition because a_high can be
      // INT_MAX, and ++a past it would be the undefined behaviour this lesson
      // is about, inside the test that checks for it.
      if (a == sweep.a_high) break;
    }

    std::cout << "  " << std::left << std::setw(28) << sweep.name << std::right
              << std::setw(14) << made << std::setw(10) << yes << std::setw(10)
              << wrong << "\n";
    total_made += made;
    total_wrong += wrong;
    total_yes += yes;
  }
  std::cout << "\n  " << total_made << " predictions, " << total_wrong << " wrong\n\n";

  RC_CHECK(total_made > 5000000);
  RC_CHECK(total_wrong == 0);

  // Both answers occur, so a function that always says no cannot pass.
  RC_CHECK(total_yes > 0);
  RC_CHECK(total_yes < total_made);
}

RC_TEST("and at the boundaries, which is the only place it matters") {
  long long made = 0;
  long long wrong = 0;

  for (int a : kEdges) {
    for (int b : kEdges) {
      const long long sum = static_cast<long long>(a) + b;
      const long long difference = static_cast<long long>(a) - b;
      const long long product = static_cast<long long>(a) * b;

      made += 3;
      if (add_overflows(a, b) != really_overflows(sum)) {
        ++wrong;
        std::cout << "    add " << a << " + " << b << "\n";
      }
      if (subtract_overflows(a, b) != really_overflows(difference)) {
        ++wrong;
        std::cout << "    subtract " << a << " - " << b << "\n";
      }
      if (multiply_overflows(a, b) != really_overflows(product)) {
        ++wrong;
        std::cout << "    multiply " << a << " * " << b << "\n";
      }
    }
  }

  std::cout << "  boundaries: " << made << " predictions, " << wrong << " wrong\n\n";
  RC_CHECK(wrong == 0);

  // The specific ones worth naming.
  RC_CHECK(add_overflows(INT_MAX, 1));
  RC_CHECK(!add_overflows(INT_MAX, 0));
  RC_CHECK(add_overflows(INT_MIN, -1));
  RC_CHECK(subtract_overflows(INT_MIN, 1));
  RC_CHECK(!subtract_overflows(INT_MIN, 0));

  // Subtracting INT_MIN is not the same question as adding its negation,
  // because its negation does not exist.
  RC_CHECK(subtract_overflows(1, INT_MIN));
  RC_CHECK(negate_overflows(INT_MIN));
  RC_CHECK(!negate_overflows(INT_MAX));
  RC_CHECK(!negate_overflows(0));

  // 46340 squared fits and 46341 squared does not, which is where a product of
  // two sensor counts turns over.
  RC_CHECK(!multiply_overflows(46340, 46340));
  RC_CHECK(multiply_overflows(46341, 46341));

  // The two cases that would be undefined behaviour inside the check itself.
  RC_CHECK(multiply_overflows(INT_MIN, -1));
  RC_CHECK(multiply_overflows(-1, INT_MIN));
  RC_CHECK(!multiply_overflows(INT_MIN, 0));
  RC_CHECK(!multiply_overflows(INT_MIN, 1));
}

RC_TEST("the checks never perform the operation they are asking about") {
  // This test exists to be run under the sanitizers. If any of these reached
  // its answer by overflowing first, the undefined behaviour sanitizer lane
  // would report it here and the suite would fail. Running clean is the
  // assertion.
  int said_yes = 0;
  int said_no = 0;
  for (int a : kEdges) {
    for (int b : kEdges) {
      const bool answers[] = {add_overflows(a, b), subtract_overflows(a, b),
                              multiply_overflows(a, b)};
      for (bool answer : answers) {
        if (answer) ++said_yes;
        else ++said_no;
      }
    }
  }
  RC_CHECK(negate_overflows(INT_MIN));

  std::cout << "  " << (said_yes + said_no)
            << " boundary checks performed with nothing to report\n";
  std::cout << "  " << said_yes << " said it would overflow, " << said_no
            << " said it would not\n\n";

  // A predicate that answers the same thing every time is not a predicate, and
  // would make the paragraph above true for the wrong reason.
  RC_CHECK(said_yes > 0);
  RC_CHECK(said_no > 0);
}

RC_TEST("unsigned wrapping is defined, so it has an answer to assert") {
  // The same arithmetic the checks/ directory refuses for a signed type. Here
  // it is legal, and the result is specified exactly: modulo two to the width.
  const unsigned int biggest = UINT_MAX;
  const unsigned int wrapped = biggest + 1u;
  RC_CHECK(wrapped == 0u);

  unsigned int zero = 0u;
  zero -= 1u;
  RC_CHECK(zero == UINT_MAX);

  // And the shift the checks/ directory refuses, one short of the width, which
  // is the largest legal one.
  const unsigned int top_bit = 1u << 31;
  RC_CHECK(top_bit == 2147483648u);

  // The same two operations on a signed type, which is where the difference
  // lives: these are the ones with no answer to assert, and the artifact is
  // how you find that out without performing them.
  RC_CHECK(add_overflows(INT_MAX, 1));
  RC_CHECK(subtract_overflows(INT_MIN, 1));
  RC_CHECK(!add_overflows(INT_MAX - 1, 1));

  std::cout << "  UINT_MAX + 1 is " << wrapped << ", exactly, on every machine\n";
  std::cout << "  0 - 1 is " << zero << ", exactly, on every machine\n";
  std::cout << "  the same two operations on a signed int are undefined, and the\n";
  std::cout << "  checks directory holds three constants the compiler must refuse\n\n";
}

RC_TEST("a running total that is checked before each step, rather than after") {
  // What the artifact is for. An encoder count accumulating towards the limit
  // of its type, which on a real robot takes days rather than never.
  int total = INT_MAX - 10;
  int applied = 0;
  bool refused = false;

  const std::vector<int> increments = {3, 3, 3, 3, 3};
  bool escaped = false;
  for (int increment : increments) {
    if (add_overflows(total, increment)) {
      refused = true;
      break;
    }
    // A second opinion, in a type wide enough to hold the answer. The
    // predicate above is the thing being tested, and a broken one would walk
    // this loop into the exact undefined behaviour the lesson is about, which
    // under the sanitizer lane would end the run rather than report a failure.
    if (really_overflows(static_cast<long long>(total) + increment)) {
      escaped = true;
      break;
    }
    total += increment;
    ++applied;
  }

  std::cout << "  starting 10 below the limit and adding 3 at a time:\n";
  std::cout << "    applied " << applied << " of " << increments.size()
            << ", then refused\n";
  std::cout << "    total " << total << ", limit " << INT_MAX << "\n\n";

  // Nothing got past the predicate to the wider check behind it.
  RC_CHECK(!escaped);
  RC_CHECK(applied == 3);
  RC_CHECK(refused);
  RC_CHECK(total == INT_MAX - 1);

  // The point: the loop stopped while the total was still a number. There was
  // no moment at which it held something to be inspected and recovered from.
  RC_CHECK(total <= INT_MAX);
}
