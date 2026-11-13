// rc/legged/velocity_cmd.hpp
//
// What the operator asks for, from lesson 36-01.
//
// Cmd is a forward speed, a sideways speed (both m/s) and a turn rate (rad/s). CmdLimits holds the greatest of each, and the accelerations the command may change at. limit_command takes the command asked for and the command in use and gives the next one: the asked command is first clipped (its forward and sideways speeds scaled together until they are inside the ellipse of the two limits, and the turn rate to its cap), and each component of the command in use then moves toward it by at most its acceleration times the time step. max_speed is the fastest the legs allow: the longest step the leg's reach allows over the stance time of the quickest swing the servos allow. gait_for_speed gives the stance time and the step length a speed needs, the step length being the speed times the stance time, and says whether the legs can do it.
//
// With limits of 1.0 m/s forward and 0.4 sideways, a command of 1.0 forward and 0.4 sideways was scaled to 0.71 and 0.28; a jump from rest to 0.8 m/s with an acceleration limit of 1.0 m/s2 took 0.80 s. The legs allowed 1.60 m/s (0.20 m step over a stance of 0.125 s); a speed of 0.5 m/s needed a step of 0.062 m, 1 (1 is within the leg's reach), and one of 2.0 m/s 0.

#ifndef RC_RC_LEGGED_VELOCITY_CMD_HPP
#define RC_RC_LEGGED_VELOCITY_CMD_HPP

#include <cmath>

namespace rc {
namespace legged {

struct Cmd {
  double vx = 0.0;   // m/s forward
  double vy = 0.0;   // m/s to the left
  double wz = 0.0;   // rad/s turning left
};

struct CmdLimits {
  double vx_max = 1.0;
  double vy_max = 0.4;
  double wz_max = 1.5;
  double ax = 1.0;     // m/s2
  double ay = 1.0;
  double az = 3.0;     // rad/s2
};

// The next command in use: the asked one clipped to the limits, then eased toward by the bounded accelerations.
inline Cmd limit_command(const Cmd& ask, const Cmd& now, const CmdLimits& l, double dt) {
  Cmd out;
  Cmd t = ask;
  const double e = std::sqrt((t.vx / l.vx_max) * (t.vx / l.vx_max) + (t.vy / l.vy_max) * (t.vy / l.vy_max));
  if (e > 1.0) {
    t.vx /= e;
    t.vy /= e;
  }
  if (t.wz > l.wz_max) t.wz = l.wz_max;
  if (t.wz < -l.wz_max) t.wz = -l.wz_max;
  double d = t.vx - now.vx;
  if (d > l.ax * dt) d = l.ax * dt;
  if (d < -l.ax * dt) d = -l.ax * dt;
  out.vx = now.vx + d;
  d = t.vy - now.vy;
  if (d > l.ay * dt) d = l.ay * dt;
  if (d < -l.ay * dt) d = -l.ay * dt;
  out.vy = now.vy + d;
  d = t.wz - now.wz;
  if (d > l.az * dt) d = l.az * dt;
  if (d < -l.az * dt) d = -l.az * dt;
  out.wz = now.wz + d;
  return out;
}

// The fastest the legs allow: the longest step over the stance time of the quickest swing.
inline double max_speed(double step_max, double duty, double period_min) { return step_max / (duty * period_min); }

struct Stride {
  double stance = 0.0;   // s
  double step = 0.0;     // m
  bool ok = false;       // the step is within the leg's reach
};

// The stance time and step length a speed needs: the stance is held at `stance` seconds (the duty times the period) and the step is the speed times it.
inline Stride gait_for_speed(double speed, double stance, double step_max) {
  Stride s;
  s.stance = stance;
  s.step = speed * stance;
  s.ok = s.step <= step_max;
  return s;
}


}  // namespace legged
}  // namespace rc

#endif
