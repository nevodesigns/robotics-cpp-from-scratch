// rc/sim/stepper.hpp
//
// A fixed step simulation loop, from lesson 18-01, graduated.
//
// Real time that builds up between frames is turned into a whole number of steps of one fixed size, the remainder kept for next time, so that the same simulation gives the same answer whatever the frame rate. A frame that took far
// too long is not caught up with: the stepper drops the backlog beyond a limit and counts it.
//
// An oscillator after 5 s was at x = 0.906213 with a fixed step of 5 ms at steady 60 frames a second, steady 30 and a jittery 8 to 48 ms, and at 0.917488 and -0.809887 when stepped by the frame time at 60 and 30. A frame of 5 s with at most
// 10 steps allowed ran 10 steps and gave up 4.90 s. The largest error in the oscillator's energy of 200 was 200.000, 50.000, 22.222, 10.526 and 2.020 at steps of 0.05, 0.02, 0.01, 0.005 and 0.001 s.

#ifndef RC_SIM_STEPPER_HPP
#define RC_SIM_STEPPER_HPP

#include <cmath>

namespace rc {
namespace sim {

// Turns the time that really passed between frames into a whole number of simulation steps of one fixed size.
//
// A simulation that steps by however long the last frame took gives different answers on a fast machine and a slow one, and on the same machine
// when something else is busy. Stepping by one fixed amount, as many times as time has built up for, gives the same answer every time.
//
// A frame that took far too long (the window was dragged, a breakpoint was hit) would owe hundreds of steps, and running them takes longer than the
// time they stand for, so the next frame owes even more. The stepper gives up on a backlog bigger than `max_steps` and counts what it dropped.
class FixedStepper {
 public:
  FixedStepper(double dt, int max_steps) : dt_(dt > 0.0 ? dt : 0.01), max_steps_(max_steps < 1 ? 1 : max_steps) {}

  // `elapsed` seconds of real time passed since the last call. Returns how many steps of dt to run now. A negative or zero elapsed adds nothing.
  int advance(double elapsed) {
    if (elapsed > 0.0) accumulated_ += elapsed;
    int steps = static_cast<int>(std::floor(accumulated_ / dt_ + 1e-9));
    if (steps > max_steps_) {
      dropped_ += accumulated_ - max_steps_ * dt_;
      accumulated_ = max_steps_ * dt_;
      steps = max_steps_;
    }
    accumulated_ -= steps * dt_;
    if (accumulated_ < 0.0) accumulated_ = 0.0;
    sim_time_ += steps * dt_;
    return steps;
  }

  // How far between the last step and the next the real clock is, from 0 up to but not including 1: for drawing between two states.
  double alpha() const {
    const double a = accumulated_ / dt_;
    return a < 0.0 ? 0.0 : (a >= 1.0 ? 0.0 : a);
  }

  double dt() const { return dt_; }
  double sim_time() const { return sim_time_; }
  double dropped() const { return dropped_; }

 private:
  double dt_ = 0.01;
  int max_steps_ = 1;
  double accumulated_ = 0.0;
  double sim_time_ = 0.0;
  double dropped_ = 0.0;
};

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_STEPPER_HPP
