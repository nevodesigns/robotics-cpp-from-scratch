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
  (void)ask;  // TODO: clip the asked command (scale vx and vy together by the ellipse's measure sqrt((vx / vx_max)^2 + (vy / vy_max)^2) if it is over 1; clamp wz), then move each component of `now` toward it by at most its acceleration times dt
  (void)now;
  (void)l;
  (void)dt;
  out = now;
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
