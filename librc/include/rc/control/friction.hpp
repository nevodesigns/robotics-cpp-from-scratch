// rc/control/friction.hpp
//
// A plant that sticks, and the test for a loop that hunts, from lesson 14-08, graduated.
//
// Static friction holds a mass still with a force up to a limit and then lets go at once,
// and the force that keeps it moving is smaller than the force that started it. A
// proportional controller sticks short of its target inside a band, the breakaway over
// the gain. An integral keeps pushing, winds up past the breakaway, and the mass overshoots,
// sticks on the far side, and the integral winds the other way: a limit cycle.
//
// Numbers from that lesson, a 1 kg mass with a breakaway of 1.5 N and Coulomb friction of
// 1.0 N held at one metre with a gain of 20: no integral stuck at 0.0623, an integral gain
// of 5 settled, 20 hunted by 0.0397 peak to peak, and 50 by 0.0499. Freezing the integral
// inside a band of 0.01 stopped it with a final error of 0.0051, and a band of 0.08 was quiet
// and 0.0367 off, further than the hunting had gone.
//
// Friction written as sign(velocity) times a force chatters across zero for ever. It has to
// be a state, stuck or sliding.

#ifndef RC_CONTROL_FRICTION_HPP
#define RC_CONTROL_FRICTION_HPP

#include <cmath>
#include <cstddef>

#include <rc/core/compat.hpp>

namespace rc {
namespace control {

// A mass on a surface that grips it.
//
// Three kinds of friction, and the middle one is the trouble. Viscous friction pushes
// back in proportion to speed and is gentle. Coulomb friction pushes back with a fixed
// force whenever the mass is moving. Static friction, stiction, holds a mass that is
// standing still with a force up to a limit that is bigger than the Coulomb force, and
// then lets go all at once. A gearbox, a seal, a linear guide, a joint with a brake
// pad: all of them stick.
struct FrictionPlant {
  double mass = 1.0;
  double viscous = 0.5;     // newtons per metre a second
  double coulomb = 1.0;     // newtons, while it is moving
  double breakaway = 1.5;   // newtons it takes to start it moving, more than the one that keeps it moving

  double position = 0.0;
  double velocity = 0.0;

  // Advances by dt under a force u.
  //
  // The awkward case is zero velocity. Written as sign(velocity) times the Coulomb force,
  // friction is zero at exactly zero and a mass crossing zero in discrete time chatters
  // from one side to the other without ever stopping. So it is a state: stuck or sliding.
  //
  // Stuck: the mass stays put as long as the applied force is no more than the breakaway
  // force. Past it, it slips, and from then friction is the smaller Coulomb force.
  // Sliding: friction opposes the motion, and if one step would carry the velocity
  // through zero it stops there, and is stuck again.
  void step(double u, double dt) {
    const double sign_u = u > 0.0 ? 1.0 : (u < 0.0 ? -1.0 : 0.0);

    if (std::fabs(velocity) < kStopped) {
      if (std::fabs(u) <= breakaway) {
        velocity = 0.0;
        return;
      }
      velocity = (u - sign_u * coulomb) / mass * dt;
    } else {
      const double sign_v = velocity > 0.0 ? 1.0 : -1.0;
      const double acceleration = (u - viscous * velocity - sign_v * coulomb) / mass;
      double next = velocity + acceleration * dt;
      if (velocity * next < 0.0) next = 0.0;   // it would have gone through zero: it stops
      velocity = next;
    }
    position += velocity * dt;
  }

 private:
  // Slower than this is stopped. A speed of a millionth of a metre a second is not
  // motion, and comparing a double with exactly zero would never be true.
  static constexpr double kStopped = 1e-6;
};

// What a signal did over a stretch of time, for telling a loop that has settled from one
// that is hunting.
struct HuntingReport {
  double peak_to_peak = 0.0;   // how far it swung
  int crossings = 0;           // how many times it passed through its own mean
  bool hunting = false;
};

// Whether a signal, usually the error of a loop that should have settled, is still
// swinging: it must swing by at least `min_swing` and cross its mean at least
// `min_crossings` times. A constant offset is not hunting, however large, and neither is
// a swing too small to matter.
inline HuntingReport analyze_hunting(rc::span<const double> signal, double min_swing, int min_crossings) {
  HuntingReport report;
  if (signal.size() < 2) return report;

  double lo = signal[0];
  double hi = signal[0];
  double sum = 0.0;
  for (const double x : signal) {
    if (x < lo) lo = x;
    if (x > hi) hi = x;
    sum += x;
  }
  report.peak_to_peak = hi - lo;

  // Crossings of the mean, not of zero: a loop that hunts about an offset is still
  // hunting, and one that sits steadily off zero is not.
  const double mean = sum / static_cast<double>(signal.size());
  for (std::size_t i = 1; i < signal.size(); ++i) {
    if ((signal[i - 1] - mean) * (signal[i] - mean) < 0.0) ++report.crossings;
  }

  report.hunting = report.peak_to_peak >= min_swing && report.crossings >= min_crossings;
  return report;
}

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_FRICTION_HPP
