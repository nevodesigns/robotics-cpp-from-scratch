#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>

#include <rc/core/compat.hpp>

// A mass on a surface that grips it: viscous friction in proportion to speed, Coulomb
// friction with a fixed force while it moves, and static friction, which holds a mass
// that is standing still with a force up to a limit bigger than the Coulomb force and
// then lets go at once.
struct FrictionPlant {
  double mass = 1.0;
  double viscous = 0.5;     // newtons per metre a second
  double coulomb = 1.0;     // newtons, while it is moving
  double breakaway = 1.5;   // newtons it takes to start it moving, more than the one that keeps it moving

  double position = 0.0;
  double velocity = 0.0;

  // Advances by dt under a force u.
  void step(double u, double dt) {
    // TODO: the mass is either stuck or sliding, and it has to be a state.
    //
    // Written as sign(velocity) times the Coulomb force, friction is zero at exactly
    // zero velocity, and a mass crossing zero in discrete time chatters from one side
    // to the other and never stops.
    //
    // Stuck, when |velocity| is below kStopped: if |u| is no more than `breakaway` the
    // mass does not move at all, so set the velocity to zero and return. Otherwise it
    // slips: the velocity after this step is (u - sign(u) * coulomb) / mass * dt.
    //
    // Sliding: acceleration = (u - viscous * velocity - sign(velocity) * coulomb) / mass.
    // Compute the next velocity, and if it has the opposite sign to the present one,
    // the mass would have gone through zero in this step. It stops there: next = 0.
    //
    // Then, unless it returned stuck, position += velocity * dt.
    (void)u;
    (void)dt;
    (void)kStopped;
  }

 private:
  // Slower than this is stopped. Comparing a double with exactly zero is never true.
  static constexpr double kStopped = 1e-6;
};

// What a signal did over a stretch of time, for telling a loop that has settled from one
// that is hunting.
struct HuntingReport {
  double peak_to_peak = 0.0;   // how far it swung
  int crossings = 0;           // how many times it passed through its own mean
  bool hunting = false;
};

// Whether a signal, usually the error of a loop that should have settled, is still
// swinging.
inline HuntingReport analyze_hunting(rc::span<const double> signal, double min_swing, int min_crossings) {
  // TODO: peak to peak is the largest minus the smallest. Count the crossings of the
  // MEAN, not of zero: a loop that hunts about an offset is still hunting, and one that
  // sits steadily off zero is not. A crossing is two neighbouring samples on opposite
  // sides of the mean, so their deviations multiply to a negative number.
  //
  // It is hunting only if it swung by at least min_swing AND crossed its mean at least
  // min_crossings times. A signal of fewer than two samples is not hunting.
  (void)signal;
  (void)min_swing;
  (void)min_crossings;
  return HuntingReport{};
}

#endif  // LESSON_SOLUTION_HPP
