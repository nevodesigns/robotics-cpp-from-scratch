#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Where a second order system is, and how fast it is going.
//
// A mass on a spring, a joint with compliance, a robot with inertia. Anything
// whose acceleration is a function of its position.
struct Motion {
  double position = 0.0;
  double velocity = 0.0;
};

// TODO 1: one step of explicit Euler.
//
// Advance both halves from the state at the start of the step:
//
//     next.position = position + velocity * dt
//     next.velocity = velocity + acceleration(position) * dt
//
// This is the obvious reading of the definition and it is why almost everybody
// writes it first. The test in this lesson runs it for ten simulated minutes
// and finds it holding twenty six billion times the energy it started with.
template <class Acceleration>
Motion euler_step(const Motion& state, Acceleration acceleration, double dt) {
  (void)acceleration;
  (void)dt;
  return state;
}

// TODO 2: one step of semi-implicit Euler.
//
// The same two lines with one thing changed: advance the velocity first, and
// then advance the position using the velocity that came out rather than the
// one that went in.
//
// Same cost, same first order accuracy, and slightly worse over one second.
// Over ten minutes the energy stays inside one percent of where it began, for
// ever. That is why this is the method under most game physics and most robot
// simulators, and the reason is stability rather than accuracy.
template <class Acceleration>
Motion semi_implicit_step(const Motion& state, Acceleration acceleration, double dt) {
  (void)acceleration;
  (void)dt;
  return state;
}

// TODO 3: one step of classical Runge Kutta.
//
// Four slopes, each taken at a state the previous one predicted:
//
//     k1 at the start
//     k2 half a step along k1
//     k3 half a step along k2
//     k4 a whole step along k3
//
// A slope here is a Motion holding how position and velocity are each changing:
// position changes at the velocity, velocity changes at the acceleration.
//
// Then advance by the weighted average, with the middle two counted twice:
//
//     next = state + dt / 6 * (k1 + 2*k2 + 2*k3 + k4)
//
// The weights make the errors of the first three orders cancel, so halving the
// step divides the error by sixteen. The test measures that ratio and finds it
// between 16.00 and 16.11.
template <class Acceleration>
Motion rk4_step(const Motion& state, Acceleration acceleration, double dt) {
  (void)acceleration;
  (void)dt;
  return state;
}

#endif  // LESSON_SOLUTION_HPP
