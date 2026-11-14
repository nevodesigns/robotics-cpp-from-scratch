#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

struct FrictionModel {
  double coulomb = 0.05;        // N m
  double stribeck_extra = 0.02; // N m added to the Coulomb at zero speed
  double stribeck_speed = 0.02; // rad/s over which it falls away
  double viscous = 0.005;       // N m s/rad
  double smooth = 0.01;         // rad/s over which the sign is smoothed
};

// The torque with which the joint resists motion at a speed.
inline double friction_torque(const FrictionModel& f, double v) {
  const double x = v / f.stribeck_speed;
  return (f.coulomb + f.stribeck_extra * std::exp(-x * x)) * std::tanh(v / f.smooth) + f.viscous * v;
}

struct TwoMass {
  double jm = 0.005;       // kg m2, the motor and what is on its side of the gear
  double jl = 0.02;        // kg m2, the load as the motor sees it through the gear
  double half_gap = 0.005; // rad, half of the play in the gear
  double k = 300.0;        // N m/rad, the contact
  double c = 0.2;          // N m s/rad, damping in contact
  double bm = 0.0005;      // N m s/rad, viscous friction on the motor
  FrictionModel load_friction;
  double thm = 0.0, wm = 0.0, thl = 0.0, wl = 0.0;

  // Advances by dt under a motor torque.
  void step(double tau, double dt) {
    const double d = thm - thl;
    double tc = 0.0;
    if (std::fabs(d) > half_gap) {
      const double pen = d - (d > 0.0 ? half_gap : -half_gap);
      tc = k * pen + c * (wm - wl);
    }
    wm += (tau - tc - bm * wm) / jm * dt;
    wl += (tc - friction_torque(load_friction, wl)) / jl * dt;
    thm += wm * dt;
    thl += wl * dt;
  }
};

struct JointController {
  double kp_pos = 10.0;      // 1/s
  double ki_pos = 0.0;       // 1/s2: an integral on the position error, which a loop that has to hold a target to the last bit wants
  double kp_vel = 2.0;       // N m s/rad: the inertia (0.025) times a bandwidth of 80 rad/s
  double ki_vel = 40.0;      // N m/rad
  bool friction_ff = false;  // add the friction torque for the velocity of the reference
  bool feedback_load = true; // take the position from the load's encoder, else the motor's (the velocity is always the motor's)
  FrictionModel believed;    // what the feedforward thinks the friction is
  double integ = 0.0;
  double integ_pos = 0.0;

  double torque(const TwoMass& s, double q_ref, double v_ref, double dt) {
    const double q = feedback_load ? s.thl : s.thm;  // the position from the load's encoder or the motor's
    const double v = s.wm;                           // the velocity loop is on the motor, as a drive's is
    integ_pos += (q_ref - q) * dt;
    const double v_cmd = kp_pos * (q_ref - q) + ki_pos * integ_pos + v_ref;
    const double ev = v_cmd - v;
    integ += ki_vel * ev * dt;
    double tau = kp_vel * ev + integ;
    if (friction_ff) tau += friction_torque(believed, v_ref);
    return tau;
  }
};

struct RampResult {
  double mean_error = 0.0;  // rad, the mean absolute error of the load once the ramp is under way (after 2 s)
  double max_error = 0.0;
  double start_peak = 0.0;    // rad, the largest error in the first 2 s, while the joint breaks away
  double start_settle = 0.0;  // s, the last moment in the first 2 s at which the error was over 0.1 mrad
};

// Follows a ramp of the load's position at a speed for some seconds; the controller runs at 1 kHz, the plant at 10 kHz.
inline RampResult track_ramp(TwoMass s, JointController c, double speed, double seconds) {
  RampResult out;
  const double dt = 1e-4;
  const int sub = 10;
  const int ctrl_steps = static_cast<int>(seconds / 1e-3);
  double sum = 0.0;
  int count = 0;
  for (int k = 0; k < ctrl_steps; ++k) {
    const double t = k * 1e-3;
    const double tau = c.torque(s, speed * t, speed, 1e-3);
    for (int i = 0; i < sub; ++i) s.step(tau, dt);
    const double e = std::fabs(speed * (t + 1e-3) - s.thl);
    if (!(e < 1e6)) {
      out.mean_error = out.max_error = out.start_peak = 1e9;  // the run blew up
      return out;
    }
    if (t > 2.0) {
      sum += e;
      ++count;
      out.max_error = std::fmax(out.max_error, e);
    } else {
      out.start_peak = std::fmax(out.start_peak, e);
      if (e > 1e-4) out.start_settle = t + 1e-3;
    }
  }
  out.mean_error = count ? sum / count : 0.0;
  return out;
}

struct HoldResult {
  double load_pp = 0.0;    // rad, how far the load moved over the last second (peak to peak)
  double final_error = 0.0;  // rad, the load's distance from the target at the end
};

// Starts at the target less an offset and holds the target for some seconds.
inline HoldResult hold(TwoMass s, JointController c, double target, double offset, double seconds) {
  HoldResult out;
  s.thl = s.thm = target - offset;
  const double dt = 1e-4;
  const int sub = 10;
  const int ctrl_steps = static_cast<int>(seconds / 1e-3);
  double lo = 1e9, hi = -1e9;
  for (int k = 0; k < ctrl_steps; ++k) {
    const double tau = c.torque(s, target, 0.0, 1e-3);
    for (int i = 0; i < sub; ++i) s.step(tau, dt);
    if (k * 1e-3 > seconds - 1.0) {
      lo = std::fmin(lo, s.thl);
      hi = std::fmax(hi, s.thl);
    }
  }
  out.load_pp = hi - lo;
  out.final_error = std::fabs(target - s.thl);
  if (!(out.load_pp < 1e6) || !(out.final_error < 1e6)) out.load_pp = out.final_error = 1e9;  // the run blew up
  return out;
}

// How much play there is: push the motor with a torque above the friction and see how far it goes before the load starts to move.
inline double measure_play(TwoMass s) {
  const double dt = 1e-4;
  // Start in contact on the positive side, then push the other way.
  s.thm = s.thl + s.half_gap;
  const double start = s.thm;
  for (int i = 0; i < 20000; ++i) {
    s.step(-0.2, dt);
    if (std::fabs(s.wl) > 1e-3) return start - s.thm;
  }
  return 0.0;
}


#endif
