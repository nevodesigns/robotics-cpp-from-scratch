#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <queue>
#include <vector>

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

// Counts how often it is moved, so the cost of taking from the front can be seen.
struct Moves {
  static long& count() {
    static long c = 0;
    return c;
  }
  int value;
  explicit Moves(int v) : value(v) {}
  Moves(const Moves& o) : value(o.value) {}
  Moves(Moves&& o) noexcept : value(o.value) { ++count(); }
  Moves& operator=(const Moves& o) {
    value = o.value;
    return *this;
  }
  Moves& operator=(Moves&& o) noexcept {
    value = o.value;
    ++count();
    return *this;
  }
};

}  // namespace

RC_TEST("a plain priority_queue gives the largest first, and the event queue gives the earliest") {
  std::priority_queue<int> plain;
  for (int v : {5, 1, 9, 3}) plain.push(v);
  RC_CHECK_EQ(plain.top(), 9);        // the largest, which is the opposite of what a schedule wants

  EventQueue q;
  q.schedule(5.0, 5);
  q.schedule(1.0, 1);
  q.schedule(9.0, 9);
  q.schedule(3.0, 3);
  RC_CHECK_EQ(q.size(), static_cast<std::size_t>(4));
  RC_CHECK_EQ(q.pop().id, 1);
  RC_CHECK_EQ(q.pop().id, 3);
  RC_CHECK_EQ(q.pop().id, 5);
  RC_CHECK_EQ(q.pop().id, 9);
  RC_CHECK(q.empty());
}

RC_TEST("events at the same time come out in the order they were scheduled") {
  EventQueue q;
  for (int id = 0; id < 8; ++id) q.schedule(2.5, id);
  q.schedule(1.0, 100);

  RC_CHECK_EQ(q.pop().id, 100);
  bool in_order = true;
  std::vector<int> got;
  for (int id = 0; id < 8; ++id) {
    const int popped = q.pop().id;
    got.push_back(popped);
    if (popped != id) in_order = false;
  }
  // For comparison, a plain priority_queue ordered by time alone. The standard does not say what order ties
  // come out in, so this line is printed and not checked: it differs between libraries.
  struct ByTimeOnly {
    bool operator()(const Event& a, const Event& b) const { return a.time > b.time; }
  };
  std::priority_queue<Event, std::vector<Event>, ByTimeOnly> bare;
  for (int id = 0; id < 8; ++id) {
    Event e;
    e.time = 2.5;
    e.id = id;
    bare.push(e);
  }
  std::cout << "  eight events at one time, scheduled 0 to 7, came out of the event queue:";
  for (int g : got) std::cout << " " << g;
  std::cout << "\n  and out of a priority_queue that compares only the time (not promised, library dependent):";
  while (!bare.empty()) {
    std::cout << " " << bare.top().id;
    bare.pop();
  }
  std::cout << "\n\n";
  RC_CHECK(in_order);
}

RC_TEST("ten thousand events come out in exactly sorted order") {
  Rng rng{77};
  EventQueue q;
  std::vector<double> times;
  for (int i = 0; i < 10000; ++i) {
    // Only 500 different times, so there are plenty of ties.
    const double t = static_cast<double>(rng.next() % 500) * 0.01;
    times.push_back(t);
    q.schedule(t, i);
  }
  std::vector<double> expected = times;
  std::stable_sort(expected.begin(), expected.end());

  bool sorted = true;
  bool ties_fifo = true;
  double last_time = -1.0;
  int last_id = -1;
  std::size_t index = 0;
  while (!q.empty()) {
    const Event e = q.pop();
    if (e.time != expected[index++]) sorted = false;
    if (e.time == last_time && e.id < last_id) ties_fifo = false;
    last_time = e.time;
    last_id = e.id;
  }
  std::cout << "  10000 events over 500 different times: sorted " << (sorted ? "yes" : "no") << ", ties in order "
            << (ties_fifo ? "yes" : "no") << "\n\n";
  RC_CHECK_EQ(index, static_cast<std::size_t>(10000));
  RC_CHECK(sorted);
  RC_CHECK(ties_fifo);
}

RC_TEST("a window keeps only the latest readings and their mean") {
  Window w(4);
  RC_CHECK_EQ(w.size(), static_cast<std::size_t>(0));
  RC_CHECK_NEAR(w.mean(), 0.0, 1e-12);

  w.add(1.0);
  w.add(2.0);
  RC_CHECK_EQ(w.size(), static_cast<std::size_t>(2));
  RC_CHECK_NEAR(w.mean(), 1.5, 1e-12);

  w.add(3.0);
  w.add(4.0);
  RC_CHECK_NEAR(w.mean(), 2.5, 1e-12);
  RC_CHECK_NEAR(w.oldest(), 1.0, 1e-12);

  w.add(10.0);        // 1 leaves
  RC_CHECK_EQ(w.size(), static_cast<std::size_t>(4));
  RC_CHECK_NEAR(w.oldest(), 2.0, 1e-12);
  RC_CHECK_NEAR(w.mean(), (2.0 + 3.0 + 4.0 + 10.0) / 4.0, 1e-12);

  for (int i = 0; i < 1000; ++i) w.add(7.0);
  RC_CHECK_EQ(w.size(), static_cast<std::size_t>(4));
  RC_CHECK_NEAR(w.mean(), 7.0, 1e-9);        // the running sum did not drift away from the truth
}

RC_TEST("taking from the front of a vector moves everything behind it, and a deque moves nothing") {
  const int capacity = 1000;
  const int pushes = 5000;

  // The vector version of a window: erase the first element once it is full.
  Moves::count() = 0;
  std::vector<Moves> v;
  v.reserve(static_cast<std::size_t>(capacity) + 1);
  for (int i = 0; i < pushes; ++i) {
    v.emplace_back(i);
    if (v.size() > static_cast<std::size_t>(capacity)) {
      v.erase(v.begin());
    }
  }
  const long vector_moves = Moves::count();

  // The same with a deque, counting only what pop_front does.
  Moves::count() = 0;
  std::deque<Moves> d;
  long deque_pop_moves = 0;
  for (int i = 0; i < pushes; ++i) {
    d.emplace_back(i);
    if (d.size() > static_cast<std::size_t>(capacity)) {
      const long before = Moves::count();
      d.pop_front();
      deque_pop_moves += Moves::count() - before;
    }
  }

  std::cout << "  a window of " << capacity << " kept over " << pushes << " readings: erasing from the front of a vector moved "
            << vector_moves << " elements, pop_front on a deque moved " << deque_pop_moves << "\n\n";
  RC_CHECK_EQ(v.size(), static_cast<std::size_t>(capacity));
  RC_CHECK_EQ(d.size(), static_cast<std::size_t>(capacity));
  RC_CHECK(vector_moves > 1000000);
  RC_CHECK_EQ(deque_pop_moves, 0L);
  RC_CHECK_EQ(v.front().value, pushes - capacity);
  RC_CHECK_EQ(d.front().value, pushes - capacity);

  // And the Window of this lesson ends up with the same oldest reading.
  Window w(static_cast<std::size_t>(capacity));
  for (int i = 0; i < pushes; ++i) w.add(static_cast<double>(i));
  RC_CHECK_NEAR(w.oldest(), static_cast<double>(pushes - capacity), 1e-12);
}
