#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include <rc/core/ring.hpp>

#include "solution.hpp"

namespace {

// A second container, so the iterator is shown serving something it was not
// written against. Three members and the iterator works.
struct Samples {
  using value_type = double;

  std::vector<double> values;

  std::size_t size() const { return values.size(); }
  const double& operator[](std::size_t index) const { return values[index]; }

  // Member begin and end, which is what range-for looks for first.
  IndexIterator<Samples> begin() const { return ::index_begin(*this); }
  IndexIterator<Samples> end() const { return ::index_end(*this); }
};

using Window = rc::core::Ring<double, 4>;

// Written as ::index_begin rather than index_begin throughout.
//
// rc::core::Ring now carries begin() and end() of its own, built on the
// graduated rc::core::IndexIterator, so for a Ring argument argument dependent
// lookup finds the library's index_begin as well as this lesson's. Both are
// visible and neither is better, which is an ambiguity rather than a choice.
//
// The leading :: says this suite is exercising the implementation in
// exercise/solution.hpp, which is the one being graded.

// A wrapped ring: four slots, six readings pushed, so the oldest two were
// forgotten and the survivors are not in array order.
Window wrapped_window() {
  Window window;
  for (int value = 1; value <= 6; ++value) window.push(static_cast<double>(value));
  return window;
}

std::string sequence(const Window& window) {
  std::string text;
  for (std::size_t i = 0; i < window.size(); ++i) {
    if (i != 0) text += " ";
    text += std::to_string(static_cast<int>(window[i]));
  }
  return text;
}

}  // namespace

RC_TEST("the iterator walks in logical order, not in storage order") {
  const Window window = wrapped_window();

  // What the slots actually hold, found by asking which logical position maps
  // to each one. This is what an iterator built on the raw array would see.
  std::string storage_order;
  for (std::size_t slot = 0; slot < window.capacity(); ++slot) {
    for (std::size_t i = 0; i < window.size(); ++i) {
      if (window.slot(i) != slot) continue;
      if (!storage_order.empty()) storage_order += " ";
      storage_order += std::to_string(static_cast<int>(window[i]));
      break;
    }
  }

  std::string walked;
  for (auto it = ::index_begin(window); it != ::index_end(window); ++it) {
    if (!walked.empty()) walked += " ";
    walked += std::to_string(static_cast<int>(*it));
  }

  std::cout << "\n  a ring of 4 with 1 to 6 pushed into it\n\n";
  std::cout << "    operator[] order   " << sequence(window) << "\n";
  std::cout << "    the iterator walks " << walked << "\n";
  std::cout << "    the slots hold     " << storage_order << "\n\n";

  // The iterator agrees with operator[], because it goes through it.
  RC_CHECK(walked == sequence(window));
  RC_CHECK(walked == "3 4 5 6");

  // And the storage is in a different order entirely. An iterator that walked
  // the array would return the right four readings in the wrong sequence,
  // which is the bug this design cannot have.
  RC_CHECK(storage_order != walked);
  std::cout << "  an iterator over the raw array would give the right four readings\n";
  std::cout << "  in the wrong order, and only once the buffer had wrapped\n\n";
}

RC_TEST("range-for asks for exactly three operations") {
  const Samples samples{{1.5, 2.5, 3.5, 4.0}};

  // This loop is the whole test. It expands to a dereference, an increment and
  // a comparison, and nothing else.
  double total = 0.0;
  int seen = 0;
  for (const double value : samples) {
    total += value;
    ++seen;
  }

  RC_CHECK(seen == 4);
  RC_CHECK_NEAR(total, 11.5, 1e-12);

  std::cout << "  range-for over a struct with size(), operator[] and three\n";
  std::cout << "  iterator operations: " << seen << " readings, total " << total << "\n\n";

  // An empty container iterates zero times rather than once or forever, which
  // is where a begin that is not equal to end when it should be shows up.
  const Samples nothing{{}};
  int touched = 0;
  for (const double value : nothing) {
    (void)value;
    ++touched;
  }
  RC_CHECK(touched == 0);
}

RC_TEST("the standard algorithms work once the typedefs are there") {
  const Window window = wrapped_window();          // holds 3 4 5 6

  const double total = std::accumulate(::index_begin(window), ::index_end(window), 0.0);
  const auto largest = std::max_element(::index_begin(window), ::index_end(window));
  const auto smallest = std::min_element(::index_begin(window), ::index_end(window));
  const auto above_four =
      std::count_if(::index_begin(window), ::index_end(window),
                    [](double value) { return value > 4.0; });
  const auto found = std::find(::index_begin(window), ::index_end(window), 5.0);

  std::cout << "  " << std::left << std::setw(16) << "accumulate" << total << "\n";
  std::cout << "  " << std::left << std::setw(16) << "max_element" << *largest << "\n";
  std::cout << "  " << std::left << std::setw(16) << "min_element" << *smallest << "\n";
  std::cout << "  " << std::left << std::setw(16) << "count_if > 4" << above_four << "\n";
  std::cout << "  " << std::left << std::setw(16) << "distance"
            << std::distance(::index_begin(window), ::index_end(window)) << "\n\n"
            << std::right;

  RC_CHECK_NEAR(total, 18.0, 1e-12);
  RC_CHECK_NEAR(*largest, 6.0, 1e-12);
  RC_CHECK_NEAR(*smallest, 3.0, 1e-12);
  RC_CHECK(above_four == 2);

  // find returns an iterator, and comparing it against end is how you ask
  // whether it found anything. Lesson 03-04 established that shape.
  RC_CHECK(found != ::index_end(window));
  RC_CHECK_NEAR(*found, 5.0, 1e-12);

  const auto missing = std::find(::index_begin(window), ::index_end(window), 99.0);
  RC_CHECK(!(missing != ::index_end(window)));
}

RC_TEST("declaring random access buys constant time distance and arithmetic") {
  const Samples samples{{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}};

  const auto first = ::index_begin(samples);
  const auto last = ::index_end(samples);

  // std::distance uses the subtraction rather than counting steps, because the
  // iterator_category said it could.
  RC_CHECK(std::distance(first, last) == 8);
  RC_CHECK(last - first == 8);

  // Jumping rather than stepping.
  RC_CHECK_NEAR(*(first + 5), 5.0, 1e-12);
  RC_CHECK_NEAR(first[3], 3.0, 1e-12);
  RC_CHECK_NEAR(*(last - 1), 7.0, 1e-12);

  // And the ordering that std::sort and the binary searches need.
  RC_CHECK(first < last);
  RC_CHECK(!(last < first));
  RC_CHECK(first == ::index_begin(samples));

  // std::lower_bound on sorted data, which needs the jumping to be cheap or it
  // is no better than a scan.
  const auto at_least_four =
      std::lower_bound(::index_begin(samples), ::index_end(samples), 4.0);
  RC_CHECK(at_least_four != ::index_end(samples));
  RC_CHECK_NEAR(*at_least_four, 4.0, 1e-12);
  RC_CHECK(at_least_four - ::index_begin(samples) == 4);

  std::cout << "  distance over 8 readings is " << std::distance(first, last)
            << ", computed by subtraction rather than by walking\n";
  std::cout << "  lower_bound found 4.0 at offset "
            << (at_least_four - ::index_begin(samples)) << "\n\n";
}

RC_TEST("one iterator, two containers it was not written against") {
  const Window window = wrapped_window();
  const Samples samples{{10.0, 20.0, 30.0}};

  // The same template, the same algorithm, two unrelated containers. Neither
  // knows about the other and the iterator knows about neither: it asks only
  // for operator[] and size().
  RC_CHECK_NEAR(std::accumulate(::index_begin(window), ::index_end(window), 0.0), 18.0, 1e-12);
  RC_CHECK_NEAR(std::accumulate(::index_begin(samples), ::index_end(samples), 0.0), 60.0, 1e-12);

  // And a std::vector, which has perfectly good iterators of its own, works
  // too. Not useful, and it proves the requirement really is just those two
  // members.
  const std::vector<double> plain = {1.0, 2.0, 3.0, 4.0};
  RC_CHECK_NEAR(std::accumulate(::index_begin(plain), ::index_end(plain), 0.0), 10.0, 1e-12);

  std::cout << "  the same iterator served a Ring, a struct of this file's own,\n";
  std::cout << "  and a std::vector, without any of them knowing about it\n\n";
}
