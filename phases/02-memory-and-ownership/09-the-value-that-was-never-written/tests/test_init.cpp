#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// Three numbers, the shape of every pose, reading and command in this
// curriculum.
struct Pose {
  double x;
  double y;
  double heading;
};

// The same three numbers, written the way lesson 01-01 said to write them.
// The defaults are not decoration: they change what a partial brace means to
// the compiler, and the suite below measures the difference.
struct Reading {
  double x = 0.0;
  double y = 0.0;
  double heading = 0.0;
};

// The C shape a device driver hands you: a function that fills an output
// parameter and reports whether it managed. One of these is wrong.

bool fill_pose(Pose* out) {
  if (out == nullptr) return false;
  out->x = 1.5;
  out->y = -0.25;
  out->heading = 0.75;
  return true;
}

bool fill_pose_forgetting_heading(Pose* out) {
  if (out == nullptr) return false;
  out->x = 1.5;
  out->y = -0.25;
  // heading is never written, and this still returns true.
  return true;
}

// Reads a buffer of unsigned char. Doing this to storage nobody wrote is
// defined: the value is unspecified rather than the read being undefined,
// which is what makes the two measurements below legal to take at all. The
// same read through an int or a double would not be.
std::size_t bytes_matching(const unsigned char* data, std::size_t count,
                           unsigned char value) {
  std::size_t matched = 0;
  for (std::size_t i = 0; i < count; ++i)
    if (data[i] == value) ++matched;
  return matched;
}

}  // namespace

RC_TEST("the heap hands back what the last owner left in it") {
  const std::size_t size = 32;

  // A block straight from the operating system, before anything in this
  // process has used it.
  unsigned char* first = new unsigned char[size];
  const std::size_t zero_on_arrival = bytes_matching(first, size, 0x00);

  // Write something recognisable, then give it back.
  for (std::size_t i = 0; i < size; ++i) first[i] = 0x7e;
  delete[] first;

  // Ask for the same size again. Nothing in the language says this is the
  // same block, and an ordinary allocator is very likely to reuse it.
  unsigned char* second = new unsigned char[size];
  const std::size_t recovered = bytes_matching(second, size, 0x7e);

  // Whatever was in there, poisoning it makes every byte known. This is the
  // baseline the rest of the lesson stands on: after the call there is no
  // leftover left, and the counter can see that.
  poison_bytes(second, size);
  RC_CHECK(bytes_still_poisoned(second, size) == size);
  delete[] second;

  std::cout << "  a block of " << size << " bytes, asked for twice\n\n";
  std::cout << "    bytes that were zero on arrival:     " << zero_on_arrival
            << " of " << size << "\n";
  std::cout << "    bytes still holding 0x7e afterwards: " << recovered
            << " of " << size << "\n\n";
  std::cout << "  the second number is the one that matters. Those bytes\n";
  std::cout << "  were written by this program, handed back, and returned\n";
  std::cout << "  verbatim. The rest were overwritten by the allocator's own\n";
  std::cout << "  bookkeeping for the freed block, which varies per run.\n\n";
  std::cout << "  the first number is the more instructive one. A program\n";
  std::cout << "  whose very first act is this allocation sees all 32 zero,\n";
  std::cout << "  because the operating system must not hand one process\n";
  std::cout << "  another's data. This suite has already allocated, so it\n";
  std::cout << "  does not. Nothing is the first allocation for long, and\n";
  std::cout << "  that is why new memory looking zeroed proves nothing.\n\n";

  // Nothing is asserted about either number. A fresh page is zero because the
  // operating system must not hand one process another's data, not because
  // new promises anything, and a quarantining allocator recycles nothing.
  RC_CHECK(size == 32u);
}

RC_TEST("a brace with one value in it writes all of them") {
  // The one rule here that is a rule. Aggregate initialisation value
  // initialises every member you did not mention, so this is three defined
  // doubles and not one defined and two inherited.
  const Reading braced = {1.5};
  RC_CHECK_NEAR(braced.x, 1.5, 1e-12);
  RC_CHECK_NEAR(braced.y, 0.0, 1e-12);
  RC_CHECK_NEAR(braced.heading, 0.0, 1e-12);

  // Empty braces do the same, with or without the defaults.
  const Pose empty = {};
  RC_CHECK_NEAR(empty.x, 0.0, 1e-12);
  RC_CHECK_NEAR(empty.y, 0.0, 1e-12);
  RC_CHECK_NEAR(empty.heading, 0.0, 1e-12);

  // And the same distinction on the heap, where the parentheses are the
  // whole difference between four defined ints and four indeterminate ones.
  std::vector<int> from_vector(4);
  int* from_new = new int[4]();
  RC_CHECK(from_vector[0] == 0 && from_vector[3] == 0);
  RC_CHECK(from_new[0] == 0 && from_new[3] == 0);
  delete[] from_new;

  // The same rule, watched from the byte level rather than asserted about
  // the members: a poisoned Pose has nothing written, and assigning a braced
  // one over it writes all of it, not just the member you were thinking of.
  Pose p;
  poison_bytes(&p, sizeof(p));
  const std::size_t before = bytes_still_poisoned(&p, sizeof(p));
  p = Pose{};
  const std::size_t after = bytes_still_poisoned(&p, sizeof(p));
  RC_CHECK(before == sizeof(Pose));
  RC_CHECK(after == 0u);

  std::cout << "  a poisoned Pose has " << before << " of " << sizeof(Pose)
            << " bytes unwritten; after = Pose{} it has " << after << "\n";
  std::cout << "  Reading{1.5} is {1.5, 0, 0}, not 1.5 and two leftovers\n";
  std::cout << "  Pose{} is three zeros even with no member defaults\n";
  std::cout << "  new int[4]() is four zeros; new int[4] is four unknowns\n\n";
  std::cout << "  the partial brace above is on Reading, whose members carry\n";
  std::cout << "  defaults, and it is silent. The identical brace on Pose,\n";
  std::cout << "  whose members do not, is just as well defined and warns on\n";
  std::cout << "  both gcc and clang as -Wmissing-field-initializers. That\n";
  std::cout << "  warning is the compiler asking for the defaults, not a\n";
  std::cout << "  claim that the members were left unwritten.\n\n";
}

RC_TEST("poisoning turns an undefined read into a loud defined one") {
  Pose p;
  poison_bytes(&p, sizeof(p));

  // Reading p.heading now is ordinary defined behaviour, because every byte
  // of it has been written. Before the poison it was undefined and the
  // compiler was entitled to assume it could not happen.
  RC_CHECK(std::isnan(p.heading));

  // The property that makes it useful: it does not stay in one field.
  p.x = 1.0;
  p.y = 2.0;
  const double travelled = std::sqrt(p.x * p.x + p.y * p.y) * std::cos(p.heading);
  RC_CHECK(std::isnan(travelled));

  // And every comparison against it is false, including the one that looks
  // like it must be true.
  RC_CHECK(!(p.heading < 0.0));
  RC_CHECK(!(p.heading >= 0.0));
  RC_CHECK(!(p.heading == p.heading));

  std::cout << "  a poisoned double is NaN, and NaN survives arithmetic:\n";
  std::cout << "    heading            " << p.heading << "\n";
  std::cout << "    distance * cos     " << travelled << "\n";
  std::cout << "    heading == heading " << (p.heading == p.heading ? "true" : "false")
            << "\n\n";
  std::cout << "  a leftover value that happens to be 0.0 is a heading of\n";
  std::cout << "  due north, and it steers the robot. This one cannot.\n\n";
}

RC_TEST("poison and count proves whether a function filled its output") {
  Pose good;
  poison_bytes(&good, sizeof(good));
  const bool good_said_yes = fill_pose(&good);

  Pose bad;
  poison_bytes(&bad, sizeof(bad));
  const bool bad_said_yes = fill_pose_forgetting_heading(&bad);

  // Both report success. Only one of them is telling the truth.
  RC_CHECK(good_said_yes);
  RC_CHECK(bad_said_yes);

  const std::size_t good_left = bytes_still_poisoned(&good, sizeof(good));
  const std::size_t bad_left = bytes_still_poisoned(&bad, sizeof(bad));

  RC_CHECK(good_left == 0u);
  RC_CHECK(bad_left == sizeof(double));
  RC_CHECK(all_bytes_written(&good, sizeof(good)));
  RC_CHECK(!all_bytes_written(&bad, sizeof(bad)));

  std::cout << "  both functions returned true\n\n";
  std::cout << "    filler                  bytes of " << sizeof(Pose)
            << " left unwritten\n";
  std::cout << "    fill_pose               " << good_left << "\n";
  std::cout << "    forgetting the heading  " << bad_left << "\n\n";
  std::cout << "  " << bad_left << " is sizeof(double), and it names the field:\n";
  std::cout << "  the one the return value said nothing about.\n\n";
}

RC_TEST("the pattern cannot tell a written 0xff from an unwritten byte") {
  // The honest limit, measured rather than mentioned.
  unsigned char frame[8];
  poison_bytes(frame, sizeof(frame));

  // A protocol that legitimately writes 0xff into two of its eight bytes,
  // which is exactly what a fill byte or an all-ones mask looks like.
  frame[0] = 0x02;
  frame[1] = 0xff;
  frame[2] = 0xff;
  for (std::size_t i = 3; i < sizeof(frame); ++i) frame[i] = static_cast<unsigned char>(i);

  const std::size_t reported = bytes_still_poisoned(frame, sizeof(frame));
  RC_CHECK(reported == 2u);

  std::cout << "  every byte of this frame was written, and the counter\n";
  std::cout << "  reports " << reported << " unwritten, because two of them were\n";
  std::cout << "  written to the pattern itself.\n\n";
  std::cout << "  so it is an aid and not a proof. For a struct of doubles\n";
  std::cout << "  the answer is exact, because 0xff repeated is NaN and no\n";
  std::cout << "  correct code writes that. For raw bytes, read it as a\n";
  std::cout << "  lower bound on what was written, not an upper bound.\n\n";
}
