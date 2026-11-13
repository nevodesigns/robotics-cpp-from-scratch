#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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


#endif
