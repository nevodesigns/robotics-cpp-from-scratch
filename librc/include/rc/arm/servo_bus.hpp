// rc/arm/servo_bus.hpp
//
// An arm made of smart servos, from lesson 29-07.
//
// quantise rounds an angle to the steps of the servo (4096 to a turn is a step of 0.0015 rad). bus_cycle_seconds is how long the bus takes to send one goal to every servo and read one position from every servo back: the bytes of the write and of the reads, ten bit times to a byte, at a baud rate, plus the servos' return delay for each read; its inverse is the fastest rate at which the arm can be updated. ServoJoint is a servo as the controller sees it: a goal that is set in steps, and an internal loop that pulls the joint to it like a spring (a stiffness) and a damper, with the torque and the speed limited; step advances it by a time under a load torque. droop_under_load is the load over the stiffness. goal_for_load shifts a goal by that, so that the servo holds a loaded pose where it was wanted. ramp_lag finds how far behind a commanded ramp the joint runs, in time, at a given update rate.
//
// A step of 4096 to a turn is 0.00153 rad, which at 0.7 m from the shaft is 1.07 mm. At 1 Mbit/s the bus took 2.98 ms to write six servos and read six back, at 115 200 bit/s 14.3 ms, which is 336 Hz against 70 Hz at most, and twelve servos at 1 Mbit/s took 5.68 ms. A load of 2 N m on a stiffness of 15 N m/rad drooped the joint 0.133 rad, and with the goal shifted by that it was 0.0002 rad off. A ramp of 0.5 rad/s commanded at 20 Hz was followed 97 ms late and at 100 Hz 77 ms late. A step of 1 rad was covered at the speed limit of 3 rad/s in 0.39 s.

#ifndef RC_RC_ARM_SERVO_BUS_HPP
#define RC_RC_ARM_SERVO_BUS_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace arm {

// Rounds an angle to the steps of the servo (steps to a turn).
inline double quantise(double angle, int steps_per_turn) {
  const double step = 6.283185307179586 / steps_per_turn;
  return std::round(angle / step) * step;
}

struct BusSpec {
  double baud = 1.0e6;
  int write_bytes_per_servo = 5;   // the id and four bytes of goal
  int write_overhead_bytes = 14;   // header, instruction, address, length and check
  int read_request_bytes = 14;     // the request for the positions
  int read_reply_bytes = 15;       // the status packet each servo sends back
  double return_delay_s = 250e-6;  // each servo's wait before it replies
};

// How long to write a goal to n servos and read a position back from each, in seconds: ten bit times to a byte, plus each servo's return delay.
inline double bus_cycle_seconds(const BusSpec& b, int n_servos) {
  const double byte_time = 10.0 / b.baud;
  const double write = (b.write_overhead_bytes + n_servos * b.write_bytes_per_servo) * byte_time;
  const double read = b.read_request_bytes * byte_time + n_servos * (b.read_reply_bytes * byte_time + b.return_delay_s);
  return write + read;
}

struct ServoSpec {
  int steps_per_turn = 4096;
  double stiffness = 15.0;  // N m per rad: the torque the servo's own loop gives per radian of error
  double damping = 1.1;     // N m s/rad
  double stall = 4.0;       // N m, the most torque it gives
  double max_speed = 3.0;   // rad/s
  double inertia = 0.02;    // kg m2 at the joint
};

class ServoJoint {
 public:
  explicit ServoJoint(const ServoSpec& s) : s_(s) {}

  void set_goal(double angle) { goal_ = quantise(angle, s_.steps_per_turn); }
  double goal() const { return goal_; }
  double position() const { return q_; }
  void reset(double q) {
    q_ = q;
    qd_ = 0.0;
    goal_ = quantise(q, s_.steps_per_turn);
  }

  // Advances by dt under a load torque (positive pulls the joint up).
  void step(double load, double dt) {
    double tau = s_.stiffness * (goal_ - q_) - s_.damping * qd_;
    tau = std::fmax(-s_.stall, std::fmin(s_.stall, tau));
    qd_ += (tau + load) / s_.inertia * dt;
    qd_ = std::fmax(-s_.max_speed, std::fmin(s_.max_speed, qd_));
    q_ += qd_ * dt;
  }

 private:
  ServoSpec s_;
  double goal_ = 0.0, q_ = 0.0, qd_ = 0.0;
};

// How far a joint drops under a constant load, held by the servo's spring alone.
inline double droop_under_load(const ServoSpec& s, double load) { return load / s.stiffness; }

// The goal that makes a loaded joint settle at the wanted angle: shifted against the load by the load over the stiffness.
inline double goal_for_load(const ServoSpec& s, double wanted, double load) { return wanted - load / s.stiffness; }

// How late a joint runs behind a ramp of goals at a speed, set at an update rate, in seconds, once it is going.
inline double ramp_lag(const ServoSpec& s, double speed, double update_hz, double seconds) {
  ServoJoint j(s);
  j.reset(0.0);
  const double dt = 1e-3;
  const int steps = static_cast<int>(seconds / dt);
  const int per_update = static_cast<int>(1.0 / (update_hz * dt));
  double goal_angle = 0.0;
  double lag_sum = 0.0;
  int count = 0;
  for (int k = 0; k < steps; ++k) {
    const double t = k * dt;
    if (k % per_update == 0) {
      goal_angle = speed * t;
      j.set_goal(goal_angle);
    }
    j.step(0.0, dt);
    if (t > seconds * 0.5) {
      lag_sum += (speed * t - j.position()) / speed;
      ++count;
    }
  }
  return count ? lag_sum / count : 0.0;
}


}  // namespace arm
}  // namespace rc

#endif
