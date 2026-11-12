#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <rc/control/limits.hpp>
#include <rc/safety/supervisor.hpp>
#include <rc/safety/tilt.hpp>
#include <rc/sensor/complementary.hpp>

// A robot on two wheels that stands like a broom on a hand: the body is a pendulum of length L above the axle, leaning by an angle theta (radians, positive forward, 0 upright), and the only way to move the top is to move the axle.
// With the axle's acceleration a (m/s^2, positive forward) the body's angular acceleration is (g sin(theta) - a cos(theta)) / L.
inline double pendulum_accel(double theta, double a, double g, double length) {
  // TODO: (g sin(theta) - a cos(theta)) / length
  (void)theta;
  (void)a;
  (void)g;
  (void)length;
  return 0.0;
}

// The steepest lean it can be caught from: the wheels must be able to accelerate at g times the tangent of the lean to hold it there, so with a motor limit of a_max that is atan(a_max / g). Zero for a motor limit of zero or less.
inline double max_recoverable_tilt(double a_max, double g) {
  // TODO: atan(a_max / g) for a_max above zero, else zero
  (void)a_max;
  (void)g;
  return 0.0;
}

// The wheel acceleration asked for, from the estimated lean and its rate and the speed of the robot: the lean to aim for is minus kv times the speed (lean back to slow down), and the command is kp times (lean minus that aim) plus kd times the lean rate, kp above g so that it
// pushes harder than gravity does. The aim is limited to 0.1 radians either way.
inline double balance_command(double kp, double kd, double kv, double theta_hat, double omega_hat, double speed) {
  // TODO: aim = -kv * speed limited to +-0.1; return kp * (theta_hat - aim) + kd * omega_hat
  (void)kp;
  (void)kd;
  (void)kv;
  (void)theta_hat;
  (void)omega_hat;
  (void)speed;
  return 0.0;
}

struct BalanceConfig {
  double kp = 25.0;
  double kd = 3.0;
  double kv = 0.05;
  double filter_tau = 0.5;          // the complementary filter's time constant, seconds
  double gyro_bias = 0.0;           // radians a second
  double accel_noise = 0.0;         // the standard deviation of the angle the accelerometer gives, radians
  int delay_frames = 0;             // control frames between the command and the wheels
  double initial_tilt = 0.05;
  double a_max = 4.0;               // the most the motors can accelerate the axle
  double v_max = 1.5;               // the fastest the axle can go
  bool use_supervisor = true;
  double imu_dead_at = -1.0;        // the IMU reads NaN from this time
};

struct BalanceResult {
  bool fell = false;                // the true lean passed 60 degrees
  double fell_at = 0.0;
  double rms_tilt = 0.0;            // over the part before it fell, radians
  double distance = 0.0;            // how far the axle went from where it started, metres
  double max_speed = 0.0;
  bool stopped = false;             // the supervisor stopped it
  double stopped_at = 0.0;
  unsigned causes = 0;
  double distance_after_fall = 0.0; // how far the axle went between the lean passing 45 degrees and the end
  double time = 0.0;
};

// A small noise source that is the same on every machine: a 64 bit generator and the sum of twelve uniform numbers.
struct BalanceNoise {
  std::uint64_t s = 88172645463325252ULL;
  double uniform() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return static_cast<double>(s >> 11) / 9007199254740992.0;
  }
  double gauss() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};

// Run it for `seconds`: the control loop at 200 Hz and the physics in steps of a millisecond. Each control frame, in this order:
//  - the sensors: the gyro reads the true lean rate plus the bias; the accelerometer reads the true lean plus the axle's acceleration over g (the sensor cannot tell the two apart) plus noise; a dead IMU reads NaN for both;
//  - the filter: ComplementaryFilter with `filter_tau`, from the gyro and the accelerometer angle, giving the estimated lean; the rate is the gyro's;
//  - the supervisor (if used): the tilt monitor of 19-09 on the estimated lean (warn at 15 degrees, stop at 25 with a debounce of 3), the IMU faulted after 3 NaN frames; when it stops, the motors are off from then on;
//  - the command from balance_command, limited to a_max, taken through the delay of `delay_frames` frames, and zero if the axle is already at v_max and the command would push it further (the motors have no more speed to give), and zero when the motors are off;
//  - the physics: with the motors off the axle slows at 0.5 m/s^2 (rolling), otherwise it accelerates as commanded.
// The run ends at `seconds`, or 2 s after the lean passes 60 degrees.
inline BalanceResult run_balance(const BalanceConfig& cfg, double seconds) {
  BalanceResult r;
  const double g = 9.81;
  const double length = 0.15;
  const double dt = 0.005;
  const int sub = 5;
  double theta = cfg.initial_tilt;
  double omega = 0.0;
  double x = 0.0;
  double v = 0.0;
  double accel = 0.0;
  rc::sensor::ComplementaryFilter filter(cfg.filter_tau);
  filter.reset(0.0);
  rc::safety::TiltMonitor monitor(0.26, 0.44, 3);
  rc::safety::SupervisorConfig sc;
  sc.top_speed = cfg.v_max;
  sc.slow_speed = 0.5 * cfg.v_max;
  rc::safety::Supervisor sup(sc);
  rc::safety::Inputs healthy;
  healthy.estop_closed = true;
  healthy.heartbeat_fresh = true;
  healthy.fence = rc::safety::FenceState::inside;
  healthy.tilt = rc::safety::TiltState::ok;
  healthy.sensors = rc::safety::CheckState::ok;
  healthy.power.state = rc::safety::PowerState::ok;
  healthy.power.factor = 1.0;
  healthy.range_cap = cfg.v_max + 1.0;
  sup.reset(healthy);
  BalanceNoise noise;
  std::deque<double> pipe(static_cast<std::size_t>(cfg.delay_frames), 0.0);
  double squares = 0.0;
  int counted = 0;
  bool motors_off = false;
  int dead_frames = 0;
  double x_at_45 = 0.0;
  bool past_45 = false;
  double over_time = -1.0;
  const int frames = static_cast<int>(seconds / dt);
  for (int i = 0; i < frames; ++i) {
    const double t = i * dt;
    const bool dead = cfg.imu_dead_at >= 0.0 && t >= cfg.imu_dead_at;
    double gyro = omega + cfg.gyro_bias;
    double accel_angle = theta + accel / g + cfg.accel_noise * noise.gauss();
    if (dead) {
      gyro = std::nan("");
      accel_angle = std::nan("");
    }
    double theta_hat = filter.angle();
    double omega_hat = 0.0;
    if (!dead) {
      theta_hat = filter.update(gyro, accel_angle, dt);
      omega_hat = gyro;
    }
    if (cfg.use_supervisor) {
      dead_frames = dead ? dead_frames + 1 : 0;
      rc::safety::Inputs in = healthy;
      const rc::safety::TiltState ts = dead ? rc::safety::TiltState::ok : monitor.update(theta_hat);
      in.tilt = ts;
      in.sensors = dead_frames >= 3 ? rc::safety::CheckState::faulted : (dead_frames > 0 ? rc::safety::CheckState::suspect : rc::safety::CheckState::ok);
      const rc::safety::Decision d = sup.step(in);
      if (d.mode == rc::safety::Mode::stopped && !motors_off) {
        motors_off = true;
        r.stopped = true;
        r.stopped_at = t;
        r.causes = d.stop_causes;
      }
    }
    double command = dead ? 0.0 : balance_command(cfg.kp, cfg.kd, cfg.kv, theta_hat, omega_hat, v);
    command = std::max(-cfg.a_max, std::min(cfg.a_max, command));
    pipe.push_back(command);
    double applied = command;
    if (cfg.delay_frames > 0) {
      applied = pipe.front();
      pipe.pop_front();
    }
    if (motors_off) applied = 0.0;
    if ((v >= cfg.v_max && applied > 0.0) || (v <= -cfg.v_max && applied < 0.0)) applied = 0.0;
    for (int k = 0; k < sub; ++k) {
      const double h = dt / sub;
      if (motors_off) {
        double a = 0.0;
        if (v > 1e-12) a = -0.5;
        else if (v < -1e-12) a = 0.5;
        if (std::fabs(v) <= 0.5 * h) a = -v / h;
        accel = a;
      } else {
        accel = applied;
      }
      omega += pendulum_accel(theta, accel, g, length) * h;
      theta += omega * h;
      v += accel * h;
      x += v * h;
    }
    r.max_speed = std::max(r.max_speed, std::fabs(v));
    if (!r.fell) {
      squares += theta * theta;
      ++counted;
    }
    if (!past_45 && std::fabs(theta) > 0.785398) {
      past_45 = true;
      x_at_45 = x;
    }
    if (!r.fell && std::fabs(theta) > 1.047198) {
      r.fell = true;
      r.fell_at = t;
      over_time = t;
    }
    r.time = t + dt;
    if (over_time >= 0.0 && t - over_time >= 2.0) break;
  }
  r.rms_tilt = std::sqrt(squares / static_cast<double>(counted > 0 ? counted : 1));
  r.distance = x;
  r.distance_after_fall = past_45 ? x - x_at_45 : 0.0;
  return r;
}

#endif  // LESSON_SOLUTION_HPP
