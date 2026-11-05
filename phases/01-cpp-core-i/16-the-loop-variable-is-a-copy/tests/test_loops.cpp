#include <rc/test/rc_test.hpp>

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// Readings 0, 1, 2, ... n-1, built before the counter is reset, so the number
// reported afterwards is the loop's and not the construction's.
std::vector<Reading> make_readings(int n) {
  std::vector<Reading> readings;
  readings.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) readings.emplace_back(static_cast<double>(i));
  copies_made() = 0;
  return readings;
}

// The four spellings of "visit each one", written the obvious way. These are
// the subject of the lesson and not the learner's work, so they live here.
double by_value(const std::vector<Reading>& readings) {
  double sum = 0.0;
  for (Reading r : readings) sum += r.value;
  return sum;
}
double by_const_reference(const std::vector<Reading>& readings) {
  double sum = 0.0;
  for (const Reading& r : readings) sum += r.value;
  return sum;
}
double by_auto(const std::vector<Reading>& readings) {
  double sum = 0.0;
  for (auto r : readings) sum += r.value;
  return sum;
}
double by_const_auto_reference(const std::vector<Reading>& readings) {
  double sum = 0.0;
  for (const auto& r : readings) sum += r.value;
  return sum;
}

// The bug, as it is usually written: a loop that scales every reading and
// scales none of them.
void scale_by_copy(std::vector<double>& values, double factor) {
  // clang 14 reports -Wunused-but-set-variable on this loop with nothing after
  // the multiplication, because the copy is set and never read. The cast reads
  // it, which keeps the suite warning free and keeps the bug exactly as it is
  // written everywhere else.
  for (double v : values) {
    v *= factor;
    (void)v;
  }
}
void scale_by_reference(std::vector<double>& values, double factor) {
  for (double& v : values) v *= factor;
}

double sum_of(const std::vector<double>& values) {
  double sum = 0.0;
  for (double v : values) sum += v;
  return sum;
}

// Whether the loop variable is the element it stands for, asked from inside
// the loop where the answer is still meaningful.
bool copy_is_the_element(const std::vector<double>& values, std::size_t index) {
  bool same = false;
  std::size_t position = 0;
  for (double v : values) {
    if (position == index) same = (&v == &values[index]);
    ++position;
  }
  return same;
}
bool reference_is_the_element(const std::vector<double>& values, std::size_t index) {
  bool same = false;
  std::size_t position = 0;
  for (const double& v : values) {
    if (position == index) same = (&v == &values[index]);
    ++position;
  }
  return same;
}

// Counts how many Holders have been destroyed, so that a test can ask the
// question without touching one that is gone.
int g_holders_destroyed = 0;

struct Holder {
  std::vector<Reading> readings;
  explicit Holder(int n) : readings(make_readings(n)) {}
  ~Holder() { ++g_holders_destroyed; }
  Holder(const Holder&) = delete;
  Holder& operator=(const Holder&) = delete;

  // A copy, handed back by value. The range expression keeps a returned value
  // alive for the whole loop.
  std::vector<Reading> readings_by_value() const { return readings; }
};

}  // namespace

RC_TEST("a loop variable declared by value is a copy, so changing it changes nothing") {
  std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};
  const double before = sum_of(values);

  scale_by_copy(values, 2.0);
  const double after_copy = sum_of(values);

  scale_by_reference(values, 2.0);
  const double after_reference = sum_of(values);

  std::cout << "  five readings summing to " << before << ", doubled two ways:\n";
  std::cout << "    for (double v : values) v *= 2       sum is now " << after_copy << "\n";
  std::cout << "    for (double& v : values) v *= 2      sum is now " << after_reference << "\n\n";

  RC_CHECK_NEAR(after_copy, 15.0, 1e-12);
  RC_CHECK_NEAR(after_reference, 30.0, 1e-12);

  // And the learner's version, on readings that count their copies.
  std::vector<Reading> readings = make_readings(5);
  scale_all(readings, 3.0);
  RC_REQUIRE_EQ(readings.size(), 5u);
  for (std::size_t i = 0; i < readings.size(); ++i)
    RC_CHECK_NEAR(readings[i].value, 3.0 * static_cast<double>(i), 1e-12);
  RC_CHECK_EQ(copies_made(), 0);
}

RC_TEST("by value copies every element and by reference copies none") {
  const std::vector<Reading> readings = make_readings(1000);

  struct Spelling {
    const char* text;
    double (*loop)(const std::vector<Reading>&);
    int expected;
  };
  const Spelling spellings[] = {
      {"for (Reading r : readings)          ", by_value, 1000},
      {"for (const Reading& r : readings)   ", by_const_reference, 0},
      {"for (auto r : readings)             ", by_auto, 1000},
      {"for (const auto& r : readings)      ", by_const_auto_reference, 0},
  };

  std::cout << "  1000 readings, each loop run once, copies made:\n";
  for (const Spelling& spelling : spellings) {
    copies_made() = 0;
    const double sum = spelling.loop(readings);
    std::cout << "    " << spelling.text << std::setw(6) << copies_made() << "\n";
    RC_CHECK_NEAR(sum, 499500.0, 1e-9);
    RC_CHECK_EQ(copies_made(), spelling.expected);
  }
  std::cout << "\n";

  // The learner's loops copy nothing, and still get the right answers.
  copies_made() = 0;
  RC_CHECK_NEAR(total(readings), 499500.0, 1e-9);
  RC_CHECK_EQ(count_above(readings, 989.5), 10);
  RC_CHECK_EQ(copies_made(), 0);
}

RC_TEST("the loop variable is not the element, so its address is no use") {
  const std::vector<double> values = {1.0, 2.0, 3.0};

  const bool copy_same = copy_is_the_element(values, 1);
  const bool reference_same = reference_is_the_element(values, 1);
  std::cout << "  the address of the loop variable, compared with the element's:\n";
  std::cout << "    for (double v : values)         " << (copy_same ? "same" : "different") << "\n";
  std::cout << "    for (const double& v : values)  " << (reference_same ? "same" : "different") << "\n\n";
  RC_CHECK(!copy_same);
  RC_CHECK(reference_same);

  // A function that returns a pointer to the thing it found has to be written
  // with the second, or it returns the address of a copy that is gone.
  const std::vector<Reading> readings = make_readings(10);
  const Reading* found = first_above(readings, 6.5);
  RC_REQUIRE(found != nullptr);
  RC_CHECK(found == &readings[7]);
  RC_CHECK_NEAR(found->value, 7.0, 1e-12);

  RC_CHECK(first_above(readings, 100.0) == nullptr);
}

RC_TEST("a temporary in the range expression is gone before the body runs") {
  g_holders_destroyed = 0;
  int destroyed_when_the_body_began = -1;
  double sum = 0.0;

  // Holder(...) is a temporary. The vector it hands back is kept alive for the
  // loop, and the Holder is not.
  for (const Reading& r : Holder(4).readings_by_value()) {
    if (destroyed_when_the_body_began < 0) destroyed_when_the_body_began = g_holders_destroyed;
    sum += r.value;
  }

  g_holders_destroyed = 0;
  int destroyed_when_named_began = -1;
  {
    const Holder named(4);
    for (const Reading& r : named.readings) {
      if (destroyed_when_named_began < 0) destroyed_when_named_began = g_holders_destroyed;
      sum += r.value;
    }
  }

  std::cout << "  holders already destroyed when the loop body first ran:\n";
  std::cout << "    a temporary, a copy returned by value   " << destroyed_when_the_body_began << "\n";
  std::cout << "    a named holder                          " << destroyed_when_named_began << "\n\n";
  std::cout << "  Had the temporary handed back a reference into itself, the body would\n";
  std::cout << "  have been reading something already gone. Run once outside this suite,\n";
  std::cout << "  that read 2882482050 with no error, and the address sanitizer reported\n";
  std::cout << "  stack-use-after-scope.\n\n";

  RC_CHECK_EQ(destroyed_when_the_body_began, 1);
  RC_CHECK_EQ(destroyed_when_named_began, 0);
  RC_CHECK_NEAR(sum, 12.0, 1e-12);

  // The safe shape, through the learner's function.
  RC_CHECK_NEAR(total(Holder(5).readings_by_value()), 10.0, 1e-12);
}

RC_TEST("a vector of bool hands out stand-ins for bools") {
  std::vector<bool> flags = {true, false, true, true, false};
  RC_CHECK_EQ(count_true(flags), 3);

  mark_all(flags, true);
  RC_CHECK_EQ(count_true(flags), 5);
  mark_all(flags, false);
  RC_CHECK_EQ(count_true(flags), 0);

  // What indexing gives back is not a bool and not a reference to one.
  std::cout << "  sizeof(bool)             " << sizeof(bool) << "\n";
  std::cout << "  sizeof(flags[0])         " << sizeof(flags[0]) << "\n\n";
  RC_CHECK(sizeof(flags[0]) != sizeof(bool));

  const std::vector<bool> none;
  RC_CHECK_EQ(count_true(none), 0);
}

RC_TEST("nothing to visit is a normal answer, and one thing to visit is an answer too") {
  std::vector<Reading> empty;
  scale_all(empty, 2.0);
  RC_CHECK_NEAR(total(empty), 0.0, 1e-12);
  RC_CHECK_EQ(count_above(empty, 0.0), 0);
  RC_CHECK(first_above(empty, 0.0) == nullptr);

  // An all zero stub gives exactly the answers above, so the same functions
  // are asked about one reading too.
  std::vector<Reading> one = make_readings(0);
  one.emplace_back(5.0);
  copies_made() = 0;
  scale_all(one, 2.0);
  RC_CHECK_NEAR(one[0].value, 10.0, 1e-12);
  RC_CHECK_NEAR(total(one), 10.0, 1e-12);
  RC_CHECK_EQ(count_above(one, 9.0), 1);
  RC_CHECK_EQ(count_above(one, 10.0), 0);
  RC_CHECK(first_above(one, 9.0) == &one[0]);
  RC_CHECK_EQ(copies_made(), 0);
}
