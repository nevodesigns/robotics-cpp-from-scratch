// rc/math/integrate.hpp
//
// Three ways to advance a second order system, from lesson 06-06, and the
// reason the least accurate of them is usually the one to use.
//
// A mass on a spring at 2 rad/s, stepped at 0.01 s for ten simulated minutes,
// energy as a fraction of where it started:
//
//   seconds        euler   semi-implicit      rk4
//   100          54.5545        1.008696      1.0
//   300      162364.7522        1.000684      1.0
//   600   26362312744.2642      1.001365      1.0
//
// Explicit Euler does not merely have error, it adds energy, every step,
// without bound. Semi-implicit Euler is one line different, costs the same, is
// slightly less accurate over one second, and stays inside one percent for
// ever. Accuracy and stability are different properties, and a short test only
// measures the first.
//
// Runge Kutta is fourth order, measured at 16.00 to 16.11 per halving, and at
// equal work rather than equal step it is five orders of magnitude better:
// 3.937e-08 against 4.121e-03 for the same four calls to the acceleration.

#ifndef RC_MATH_INTEGRATE
#define RC_MATH_INTEGRATE

namespace rc {
namespace math {

// Where a second order system is, and how fast it is going.
//
// A mass on a spring, a joint with compliance, a robot with inertia. Anything
// whose acceleration is a function of its position.
struct Motion {
  double position = 0.0;
  double velocity = 0.0;
};

// One step of explicit Euler.
//
// Both halves are advanced from the state at the start of the step, which is
// the obvious reading of the definition and is why almost everybody writes this
// one first.
//
// It does not merely have error. It adds energy, every step, without bound. A
// mass on a spring left running for ten simulated minutes at a hundredth of a
// second per step finished with twenty six billion times the energy it started
// with, and the same arithmetic inside a robot simulator is a machine that
// slowly shakes itself apart for no reason in the model.
template <class Acceleration>
Motion euler_step(const Motion& state, Acceleration acceleration, double dt) {
  Motion next;
  next.position = state.position + state.velocity * dt;
  next.velocity = state.velocity + acceleration(state.position) * dt;
  return next;
}

// One step of semi-implicit Euler.
//
// The velocity is advanced first, and the position is then advanced using the
// velocity that came out. One line moved, the same two multiplications, the same
// first order accuracy, and the energy stays where it was put: over the same ten
// minutes it stayed between 0.991 and 1.009 of where it started, for ever.
//
// This is the method under most game physics and most robot simulators, and the
// reason is not accuracy. Measured after one second it is slightly less accurate
// than explicit Euler. It is stable, and those are different properties.
template <class Acceleration>
Motion semi_implicit_step(const Motion& state, Acceleration acceleration, double dt) {
  Motion next;
  next.velocity = state.velocity + acceleration(state.position) * dt;
  next.position = state.position + next.velocity * dt;
  return next;
}

// One step of classical Runge Kutta.
//
// Four evaluations of the acceleration, weighted so that the errors of the first
// three orders cancel. Halving the step divides the error by sixteen, measured
// at 16.00 to 16.11 across five halvings.
//
// Four times the work of Euler, so compare them at equal work rather than at
// equal step: RK4 at a fiftieth of a second against Euler at two hundredths,
// which is the same four calls, is 3.937e-08 against 4.121e-03. Five orders of
// magnitude for the same money.
template <class Acceleration>
Motion rk4_step(const Motion& state, Acceleration acceleration, double dt) {
  // Each stage is a slope: how position and velocity are changing there.
  const Motion k1{state.velocity, acceleration(state.position)};

  const Motion at2{state.position + 0.5 * dt * k1.position,
                   state.velocity + 0.5 * dt * k1.velocity};
  const Motion k2{at2.velocity, acceleration(at2.position)};

  const Motion at3{state.position + 0.5 * dt * k2.position,
                   state.velocity + 0.5 * dt * k2.velocity};
  const Motion k3{at3.velocity, acceleration(at3.position)};

  const Motion at4{state.position + dt * k3.position,
                   state.velocity + dt * k3.velocity};
  const Motion k4{at4.velocity, acceleration(at4.position)};

  Motion next;
  next.position = state.position + dt / 6.0 *
                                       (k1.position + 2.0 * k2.position +
                                        2.0 * k3.position + k4.position);
  next.velocity = state.velocity + dt / 6.0 *
                                       (k1.velocity + 2.0 * k2.velocity +
                                        2.0 * k3.velocity + k4.velocity);
  return next;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_INTEGRATE
