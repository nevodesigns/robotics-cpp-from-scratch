#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
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

// A corridor that loops, 24 places, a door (1) or a wall (0) at each, irregular so that most stretches are unique.
const std::vector<int>& corridor() {
  static const std::vector<int> world = {1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0};
  return world;
}

int doors_in(const std::vector<int>& w) {
  int d = 0;
  for (int v : w) d += v;
  return d;
}

double total(const Belief& b) {
  double s = 0.0;
  for (double v : b.cells()) s += v;
  return s;
}

}  // namespace

RC_TEST("a belief always sums to one, and one with nothing left in it goes back to knowing nothing") {
  Belief b(24);
  RC_CHECK_NEAR(total(b), 1.0, 1e-12);
  RC_CHECK_NEAR(b.at(7), 1.0 / 24.0, 1e-12);
  RC_CHECK_NEAR(b.entropy(), std::log2(24.0), 1e-9);

  b.correct(corridor(), 1, 0.9, 0.1);
  RC_CHECK_NEAR(total(b), 1.0, 1e-12);

  // Every place ruled out at once.
  Belief dead(24);
  dead.correct(corridor(), 1, 0.0, 0.0);
  RC_CHECK_NEAR(total(dead), 1.0, 1e-12);
  RC_CHECK_NEAR(dead.at(3), 1.0 / 24.0, 1e-12);
  RC_CHECK_NEAR(dead.entropy(), std::log2(24.0), 1e-9);
  RC_CHECK(Belief(0).size() == 1);
}

RC_TEST("moving shifts the belief round the ring and smears it by the slip, and the ring wraps") {
  Belief exact(24);
  std::vector<int> only23(24, 0);
  only23[23] = 1;
  exact.correct(only23, 1, 1.0, 0.0);                  // all of it at place 23
  RC_CHECK(exact.most_likely() == 23);
  if (exact.most_likely() != 23) return;   // not RC_REQUIRE, which would skip the destructors
  RC_CHECK_NEAR(exact.at(23), 1.0, 1e-12);
  exact.predict(1, 1.0, 0.0, 0.0);
  RC_CHECK(exact.most_likely() == 0);                  // round the ring
  RC_CHECK_NEAR(exact.at(0), 1.0, 1e-12);

  Belief slip(24);
  std::vector<int> only5(24, 0);
  only5[5] = 1;
  slip.correct(only5, 1, 1.0, 0.0);
  slip.predict(1, 0.8, 0.1, 0.1);                      // meant one place on, from 5: 6 with 0.8, 5 with 0.1, 7 with 0.1
  std::cout << "  all of the belief at place 5, moved one place with slip: place 5 " << std::fixed << std::setprecision(2) << slip.at(5)
            << ", place 6 " << slip.at(6) << ", place 7 " << slip.at(7) << "; total " << total(slip) << "\n\n";
  RC_CHECK_NEAR(slip.at(6), 0.8, 1e-12);
  RC_CHECK_NEAR(slip.at(5), 0.1, 1e-12);
  RC_CHECK_NEAR(slip.at(7), 0.1, 1e-12);
  RC_CHECK_NEAR(total(slip), 1.0, 1e-12);

  Belief back(24);
  back.correct(only5, 1, 1.0, 0.0);
  back.predict(-6, 1.0, 0.0, 0.0);                     // backwards, past the start of the ring
  RC_CHECK(back.most_likely() == 23);
  RC_CHECK(slip.entropy() > 0.5);
}

RC_TEST("one reading of a door leaves a belief with a peak at every door, which is something one number cannot say") {
  const std::vector<int>& w = corridor();
  Belief b(24);
  b.correct(w, 1, 0.9, 0.1);
  const int doors = doors_in(w);
  const double expected_door = 0.9 / (0.9 * doors + 0.1 * (24 - doors));
  const double expected_wall = 0.1 / (0.9 * doors + 0.1 * (24 - doors));
  int peaks = 0;
  for (std::size_t i = 0; i < 24; ++i)
    if (b.at(i) > 0.5 * b.peak()) ++peaks;
  std::cout << "  one reading of a door, sensor right 90 percent of the time: " << doors << " doors in the corridor, " << peaks
            << " places with a share near the top, each door " << std::setprecision(4) << b.at(0) << " (predicted " << expected_door
            << "), each wall " << b.at(1) << " (predicted " << expected_wall << ")\n  entropy " << std::setprecision(2) << b.entropy()
            << " bits, from " << std::log2(24.0) << "\n\n";
  RC_CHECK(peaks == doors);
  RC_CHECK_NEAR(b.at(0), expected_door, 1e-12);
  RC_CHECK_NEAR(b.at(1), expected_wall, 1e-12);
  RC_CHECK(b.entropy() < std::log2(24.0));
}

RC_TEST("walking and sensing, the belief narrows to the true place, and moving widens it a little each time") {
  const std::vector<int>& w = corridor();
  Belief b(24);
  std::size_t truth = 3;
  std::cout << "  starting nowhere, at place 3, a perfect sensor read as 90 percent right, steps of one place\n"
            << "  step   reads   most likely   its share   entropy (bits)\n";
  double first_entropy = 0.0;
  double last_entropy = 0.0;
  bool widened_by_moving = true;
  for (int step = 0; step <= 14; ++step) {
    if (step > 0) {
      const double before = b.entropy();
      b.predict(1, 0.8, 0.1, 0.1);
      truth = (truth + 1) % 24;
      if (before > 0.0 && b.entropy() < before - 1e-9) widened_by_moving = false;
    }
    b.correct(w, w[truth], 0.9, 0.1);
    if (step == 0) first_entropy = b.entropy();
    last_entropy = b.entropy();
    std::cout << "  " << std::setw(4) << step << "   " << std::setw(5) << (w[truth] ? "door" : "wall") << "   " << std::setw(11) << b.most_likely()
              << "   " << std::fixed << std::setprecision(3) << std::setw(9) << b.peak() << "   " << std::setw(14) << std::setprecision(2) << b.entropy() << "\n";
  }
  std::cout << "  the true place is " << truth << "\n\n";
  RC_CHECK(first_entropy > 2.0);
  RC_CHECK(last_entropy < 0.5 * first_entropy);
  RC_CHECK(b.most_likely() == truth);
  RC_CHECK(widened_by_moving);
}

RC_TEST("a sensor that says nothing is no better than a guess, and one that allows for its errors finds the place most of the time") {
  const std::vector<int>& w = corridor();
  std::cout << "  200 trips of 60 steps, a sensor wrong one reading in twenty, slipping one place in five\n"
            << "  belief says a wrong reading has probability   found the right place at the end\n";
  double success_zero = 0.0;
  double success_ten = 0.0;
  double success_none = 1.0;
  for (double p_mismatch : {0.0, 0.02, 0.05, 0.1, 0.3, 0.5}) {
    int right = 0;
    const int trials = 200;
    Rng rng{99};
    for (int t = 0; t < trials; ++t) {
      Belief b(24);
      std::size_t truth = static_cast<std::size_t>(rng.next() % 24);
      for (int step = 0; step < 60; ++step) {
        b.predict(1, 0.8, 0.1, 0.1);
        const double r = rng.uniform();
        const std::size_t move = r < 0.8 ? 1 : (r < 0.9 ? 0 : 2);
        truth = (truth + move) % 24;
        int reading = w[truth];
        if (rng.uniform() < 0.05) reading = 1 - reading;
        b.correct(w, reading, 1.0 - p_mismatch, p_mismatch);
      }
      if (b.most_likely() == truth) ++right;
    }
    const double fraction = static_cast<double>(right) / trials;
    std::cout << "  " << std::setw(44) << std::fixed << std::setprecision(2) << p_mismatch << "   " << std::setw(24) << std::setprecision(3)
              << fraction << "\n";
    if (p_mismatch == 0.0) success_zero = fraction;
    if (p_mismatch == 0.05) success_ten = fraction;
    if (p_mismatch == 0.5) success_none = fraction;
  }
  std::cout << "\n";
  RC_CHECK(success_ten > 0.6);
  RC_CHECK(success_zero > 0.0);
  RC_CHECK(success_none < 0.15);
}

RC_TEST("one wrong reading wipes out a belief that was told the sensor never errs, and one that allows for it keeps the true place") {
  const std::vector<int>& w = corridor();
  std::cout << "  localised after 25 steps, then one wrong reading, then right ones; moves are exact\n"
            << "  belief says a wrong reading has probability   share of the true place just after   steps to get back above one half\n";
  double after_zero = 1.0;
  double after_ten = 0.0;
  int back_zero = 0;
  int back_ten = 0;
  for (double p_mismatch : {0.0, 0.01, 0.1}) {
    Belief b(24);
    std::size_t truth = 3;
    for (int step = 0; step < 25; ++step) {
      b.predict(1, 1.0, 0.0, 0.0);
      truth = (truth + 1) % 24;
      b.correct(w, w[truth], 1.0 - p_mismatch, p_mismatch);
    }
    b.predict(1, 1.0, 0.0, 0.0);
    truth = (truth + 1) % 24;
    b.correct(w, 1 - w[truth], 1.0 - p_mismatch, p_mismatch);               // the one wrong reading
    const double just_after = b.at(truth);
    int back = -1;
    for (int step = 1; step <= 40 && back < 0; ++step) {
      b.predict(1, 1.0, 0.0, 0.0);
      truth = (truth + 1) % 24;
      b.correct(w, w[truth], 1.0 - p_mismatch, p_mismatch);
      if (b.at(truth) > 0.5) back = step;
    }
    std::cout << "  " << std::setw(44) << std::fixed << std::setprecision(2) << p_mismatch << "   " << std::setw(31) << std::setprecision(3)
              << just_after << "   " << std::setw(32) << back << "\n";
    if (p_mismatch == 0.0) {
      after_zero = just_after;
      back_zero = back;
    }
    if (p_mismatch == 0.1) {
      after_ten = just_after;
      back_ten = back;
    }
  }
  std::cout << "\n";
  RC_CHECK(after_zero < 0.1);
  RC_CHECK(after_ten > 0.2);
  RC_CHECK(back_ten >= 1);
  RC_CHECK(back_ten < back_zero || back_zero < 0);
}
