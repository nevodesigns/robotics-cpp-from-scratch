#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <vector>

struct JointSim {
  double inertia = 0.05;     // kg m2
  double kp = 10.0;          // N m per rad
  double kd = 1.5;           // N m s per rad
  double tau_max = 10.0;     // N m
  double friction = 0.05;    // N m, Coulomb
  double delay = 0.0;        // s between the command and the motor
  double lag = 0.0;          // s, the time constant with which the torque follows the command
};

struct StepResult {
  bool settled = false;
  double overshoot = 0.0;    // fraction of the step beyond the target
  double t_settle = 0.0;     // s, when it entered the 2 percent band for good (negative if it never did)
};

// A step from rest to `target` rad over `secs` seconds, in steps of dt.
inline StepResult simulate_step(const JointSim& j, double target, double secs, double dt) {
  StepResult r;
  r.t_settle = -1.0;
  const int n = static_cast<int>(secs / dt);
  const int d = static_cast<int>(j.delay / dt + 0.5);
  std::vector<double> ring(static_cast<std::size_t>(d) + 1, 0.0);
  double q = 0.0, w = 0.0, applied = 0.0, peak = 0.0;
  int in_band_since = -1;
  (void)target;  // TODO: each step: the command is kp (target - q) - kd w clipped to the torque limit; put it in a ring and take the one from `delay` ago (zero before that); the torque applied follows it with the lag (applied += (delayed - applied) dt / lag; the delayed command itself if the lag is zero); subtract Coulomb friction (the friction's sign against the speed; at rest, no motion if the drive is within the friction); w += (applied - friction) / inertia dt; q += w dt; track the peak; settled if it was within 2 percent of the target for the last second, t_settle when it entered the band for good; a runaway is not settled
  (void)secs;
  (void)dt;
  (void)d;
  (void)peak;
  (void)in_band_since;
  (void)q;
  (void)w;
  (void)applied;
  (void)ring;
  return r;
}

// The largest delay (s) for which the joint still settles, by halving an interval up to `limit`.
inline double max_stable_delay(JointSim j, double target, double secs, double dt, double limit) {
  double lo = 0.0, hi = limit;
  j.delay = hi;
  if (simulate_step(j, target, secs, dt).settled) return limit;
  for (int i = 0; i < 20; ++i) {
    j.delay = 0.5 * (lo + hi);
    if (simulate_step(j, target, secs, dt).settled) {
      lo = j.delay;
    } else {
      hi = j.delay;
    }
  }
  return lo;
}

// The share of n joints drawn at random (inertia within plus or minus `spread` of the nominal, delay from 0 to max_delay, lag from 0 to max_lag) for which the step settles.
inline double success_rate(const JointSim& nominal, double spread, double max_delay, double max_lag, int n, unsigned long long seed, double target, double secs, double dt) {
  unsigned long long st = seed;
  int ok = 0;
  for (int i = 0; i < n; ++i) {
    double u[3];
    for (int k = 0; k < 3; ++k) {
      st = st * 6364136223846793005ULL + 1442695040888963407ULL;
      u[k] = static_cast<double>(st >> 11) / 9007199254740992.0;
    }
    JointSim j = nominal;
    j.inertia = nominal.inertia * (1.0 + spread * (2.0 * u[0] - 1.0));
    j.delay = max_delay * u[1];
    j.lag = max_lag * u[2];
    if (simulate_step(j, target, secs, dt).settled) ++ok;
  }
  return static_cast<double>(ok) / n;
}


#endif
