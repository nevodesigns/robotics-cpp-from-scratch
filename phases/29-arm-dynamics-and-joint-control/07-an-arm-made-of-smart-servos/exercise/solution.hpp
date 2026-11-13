#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

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
  return 0.0;  // TODO: the write is its overhead plus the bytes for each servo; the read is the request plus, for each servo, its reply and its return delay; each byte takes ten bit times
  (void)byte_time;
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


#endif
