#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
};

// A mass on a spring, stiffness 400 and mass 1, so that it swings at 20 rad/s. Position 1 at rest to begin with, and so its energy is 200.
struct Oscillator {
  double x = 1.0;
  double v = 0.0;
  void step(double dt) {
    v += -400.0 * x * dt;     // semi-implicit Euler: the speed first, then the position with the new speed
    x += v * dt;
  }
  double energy() const { return 0.5 * v * v + 200.0 * x * x; }
};

// Drive an oscillator for `seconds` of simulated time, with frames of the given real durations, one fixed step each time they have built up for.
Oscillator run_fixed(const std::vector<double>& frames, double dt, double seconds) {
  FixedStepper stepper(dt, 1000);
  Oscillator o;
  const int target = static_cast<int>(seconds / dt + 0.5);       // the same number of steps whatever the frames were
  int taken = 0;
  for (std::size_t i = 0; i < frames.size() && taken < target; ++i) {
    const int n = stepper.advance(frames[i]);
    for (int k = 0; k < n && taken < target; ++k, ++taken) o.step(dt);
  }
  return o;
}

// The same, stepping by however long the frame took.
Oscillator run_variable(const std::vector<double>& frames, double seconds) {
  Oscillator o;
  double t = 0.0;
  for (std::size_t i = 0; i < frames.size() && t < seconds; ++i) {
    o.step(frames[i]);
    t += frames[i];
  }
  return o;
}

std::vector<double> steady(double frame, double seconds) { return std::vector<double>(static_cast<std::size_t>(seconds / frame + 2), frame); }

std::vector<double> jittery(double seconds, std::uint32_t seed) {
  Rng rng{seed};
  std::vector<double> f;
  double t = 0.0;
  while (t < seconds + 1.0) {
    const double d = 0.008 + 0.040 * rng.uniform();        // 8 to 48 ms
    f.push_back(d);
    t += d;
  }
  return f;
}

}  // namespace

RC_TEST("real time that builds up is turned into whole steps, the remainder is kept for next time, and no time is lost") {
  FixedStepper s(0.01, 100);
  RC_CHECK(s.advance(0.016) == 1);
  RC_CHECK_NEAR(s.alpha(), 0.6, 1e-9);
  RC_CHECK(s.advance(0.016) == 2);                       // 0.032 in all: three steps' worth, two more than before, and 0.002 left
  RC_CHECK(s.advance(0.016) == 1);                       // 0.048 in all: four steps' worth
  int total = 4;
  double real = 0.048;
  for (int i = 0; i < 1000; ++i) {
    const int n = s.advance(0.003);
    total += n;
    real += 0.003;
  }
  std::cout << "  " << real << " s of real time in frames of 3 and 16 ms: " << total << " steps of 10 ms taken, sim time " << std::fixed << std::setprecision(2) << s.sim_time()
            << " s, " << std::setprecision(4) << s.alpha() << " of a step still owed\n\n";
  RC_CHECK(total == static_cast<int>(real / 0.01 + 1e-6));
  RC_CHECK_NEAR(s.sim_time(), total * 0.01, 1e-9);
  RC_CHECK(s.alpha() >= 0.0 && s.alpha() < 1.0);
  RC_CHECK(s.dropped() == 0.0);
}

RC_TEST("a frame shorter than a step does no work, and a negative or zero elapsed time adds nothing") {
  FixedStepper s(0.02, 10);
  RC_CHECK(s.advance(0.005) == 0);
  RC_CHECK(s.advance(0.005) == 0);
  RC_CHECK(s.advance(0.0) == 0);
  RC_CHECK(s.advance(-1.0) == 0);                        // a clock that went backwards owes nothing
  RC_CHECK_NEAR(s.alpha(), 0.5, 1e-9);
  RC_CHECK(s.advance(0.01) == 1);
  RC_CHECK_NEAR(s.sim_time(), 0.02, 1e-12);
  FixedStepper guarded(-1.0, 0);                          // nonsense settings become something usable
  RC_CHECK(guarded.dt() > 0.0);
  RC_CHECK(guarded.advance(1.0) >= 1);
}

RC_TEST("a frame that took far too long is not caught up with: the backlog is dropped, counted, and the next frame owes nothing for it") {
  FixedStepper s(0.01, 10);
  const int n = s.advance(5.0);                          // the window was dragged for five seconds
  const double dropped = s.dropped();
  const double sim_time = s.sim_time();
  const int next = s.advance(0.01);
  std::cout << "  a frame of 5 s with at most 10 steps allowed: " << n << " steps run, " << std::fixed << std::setprecision(2) << dropped << " s of real time given up; the frame after: "
            << next << " step\n\n";
  RC_CHECK(n == 10);
  RC_CHECK_NEAR(dropped, 4.9, 1e-9);
  RC_CHECK_NEAR(sim_time, 0.1, 1e-9);
  RC_CHECK(next == 1);
  RC_CHECK(s.advance(0.01) == 1);
}

RC_TEST("with a fixed step the answer is the same whatever the frame rate, and stepping by the frame time is not") {
  const double dt = 0.005;
  const Oscillator at60 = run_fixed(steady(1.0 / 60.0, 6.0), dt, 5.0);
  const Oscillator at30 = run_fixed(steady(1.0 / 30.0, 6.0), dt, 5.0);
  const Oscillator jitter = run_fixed(jittery(6.0, 7), dt, 5.0);
  const Oscillator v60 = run_variable(steady(1.0 / 60.0, 6.0), 5.0);
  const Oscillator v30 = run_variable(steady(1.0 / 30.0, 6.0), 5.0);
  std::cout << "  an oscillator, 5 s, position at the end\n  frames                   fixed step 5 ms   step = frame time\n"
            << "  steady 60 per second     " << std::fixed << std::setprecision(6) << std::setw(15) << at60.x << "   " << std::setw(15) << v60.x << "\n"
            << "  steady 30 per second     " << std::setw(15) << at30.x << "   " << std::setw(15) << v30.x << "\n"
            << "  jittery, 8 to 48 ms      " << std::setw(15) << jitter.x << "   (not meaningful)\n\n";
  // The three runs take a different number of whole steps only through where the last frame ended, so compare at the same number of steps.
  RC_CHECK(at60.x != 1.0);                                // the oscillator moved: the stepper gave it steps to take
  RC_CHECK_NEAR(at60.x, at30.x, 0.02);
  RC_CHECK_NEAR(at60.x, jitter.x, 0.02);
  RC_CHECK(std::fabs(v60.x - v30.x) > 10.0 * std::fabs(at60.x - at30.x) || std::fabs(v60.x - v30.x) > 0.01);
}

RC_TEST("two runs given exactly the same frames give exactly the same state, to the last bit") {
  const std::vector<double> frames = jittery(6.0, 99);
  const Oscillator a = run_fixed(frames, 0.005, 5.0);
  const Oscillator b = run_fixed(frames, 0.005, 5.0);
  std::cout << "  the same 6 s of jittery frames twice: x " << std::setprecision(17) << a.x << " and " << b.x << "\n\n";
  RC_CHECK(a.x == b.x);
  RC_CHECK(a.v == b.v);
  RC_CHECK(a.x != 1.0);
}

RC_TEST("a smaller step is a more accurate simulation, and the energy the spring should keep is what shows it") {
  std::cout << "  an oscillator of energy 200 for 5 s, largest error in its energy\n  dt (s)     largest energy error\n";
  double coarse = 0.0;
  double fine = 0.0;
  double previous = 1e18;
  bool falls = true;
  for (double dt : {0.05, 0.02, 0.01, 0.005, 0.001}) {
    FixedStepper s(dt, 1000);
    Oscillator o;
    double worst = 0.0;
    const int target = static_cast<int>(5.0 / dt + 0.5);
    int taken = 0;
    for (int frame = 0; frame < 2 * target + 10 && taken < target; ++frame) {
      const int n = s.advance(dt);
      for (int k = 0; k < n; ++k, ++taken) {
        o.step(dt);
        const double e = std::fabs(o.energy() - 200.0);
        if (e > worst) worst = e;
      }
    }
    std::cout << "  " << std::setw(7) << std::fixed << std::setprecision(3) << dt << "    " << std::setw(18) << std::setprecision(3) << worst << "\n";
    if (worst > previous) falls = false;
    previous = worst;
    if (dt == 0.05) coarse = worst;
    if (dt == 0.001) fine = worst;
  }
  std::cout << "\n";
  RC_CHECK(falls);
  RC_CHECK(coarse > 0.0);
  RC_CHECK(fine < 0.05 * coarse);
}
