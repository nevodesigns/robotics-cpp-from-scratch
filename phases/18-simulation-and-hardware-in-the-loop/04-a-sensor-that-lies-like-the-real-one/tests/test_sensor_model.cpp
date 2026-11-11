#include <rc/test/rc_test.hpp>
#include <rc/sim/plant.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

struct Stats {
  double mean = 0.0;
  double sd = 0.0;
  double rms_error = 0.0;
};

// Read a constant `truth` n times.
Stats measure(SensorOptions o, double truth, int n) {
  SensorModel s(o);
  std::vector<double> v;
  for (int i = 0; i < n; ++i) {
    const Reading r = s.read(truth);
    if (r.valid) v.push_back(r.value);
  }
  Stats st;
  if (v.empty()) return st;
  double sum = 0.0;
  for (double x : v) sum += x;
  st.mean = sum / static_cast<double>(v.size());
  double sq = 0.0;
  double err = 0.0;
  for (double x : v) {
    sq += (x - st.mean) * (x - st.mean);
    err += (x - truth) * (x - truth);
  }
  st.sd = std::sqrt(sq / static_cast<double>(v.size()));
  st.rms_error = std::sqrt(err / static_cast<double>(v.size()));
  return st;
}

}  // namespace

RC_TEST("a sensor with nothing wrong with it reads the truth, exactly and at once") {
  SensorModel s(SensorOptions{});
  for (double truth : {0.0, 1.5, -3.25, 1e6}) {
    const Reading r = s.read(truth);
    RC_CHECK(r.valid);
    RC_CHECK(r.value == truth);
  }
  RC_CHECK(s.dropped() == 0);
}

RC_TEST("a bias moves every reading by the same amount, and noise spreads them by the standard deviation it was given") {
  SensorOptions biased;
  biased.bias = 0.3;
  const Stats b = measure(biased, 2.0, 1000);
  SensorOptions noisy;
  noisy.noise = 0.05;
  const Stats n = measure(noisy, 2.0, 20000);
  std::cout << "  a bias of 0.3 on a truth of 2.0: mean " << std::fixed << std::setprecision(4) << b.mean << ", spread " << b.sd << "\n"
            << "  noise of 0.05: mean " << n.mean << ", spread " << n.sd << "\n\n";
  RC_CHECK_NEAR(b.mean, 2.3, 1e-12);
  RC_CHECK_NEAR(b.sd, 0.0, 1e-12);
  RC_CHECK_NEAR(n.mean, 2.0, 0.003);
  RC_CHECK_NEAR(n.sd, 0.05, 0.003);
}

RC_TEST("the range holds a reading, and a resolution rounds it to a multiple of the step, with an error of the step over root twelve") {
  SensorOptions ranged;
  ranged.low = -5.0;
  ranged.high = 5.0;
  SensorModel r(ranged);
  RC_CHECK(r.read(100.0).value == 5.0);
  RC_CHECK(r.read(-100.0).value == -5.0);
  RC_CHECK(r.read(1.25).value == 1.25);

  SensorOptions coarse;
  coarse.step = 0.1;
  SensorModel q(coarse);
  double sum_sq = 0.0;
  bool multiples = true;
  const int n = 20000;
  for (int i = 0; i < n; ++i) {
    const double truth = i * 0.000731;
    const double v = q.read(truth).value;
    if (std::fabs(v / 0.1 - std::round(v / 0.1)) > 1e-9) multiples = false;
    sum_sq += (v - truth) * (v - truth);
  }
  const double rms = std::sqrt(sum_sq / n);
  std::cout << "  a resolution of 0.1: every reading a multiple of it: " << (multiples ? "yes" : "no") << "; rms error " << std::setprecision(5) << rms << ", step over root twelve "
            << 0.1 / std::sqrt(12.0) << "\n\n";
  RC_CHECK(multiples);
  RC_CHECK_NEAR(rms, 0.1 / std::sqrt(12.0), 0.002);
}

RC_TEST("a delay makes the reading old by that many samples, and until it has history it gives the first reading") {
  SensorOptions o;
  o.delay = 3;
  SensorModel s(o);
  std::vector<double> got;
  for (int i = 0; i < 8; ++i) got.push_back(s.read(i * 1.0).value);
  std::cout << "  a delay of 3 samples on a ramp 0, 1, 2, ...: read";
  for (double g : got) std::cout << " " << g;
  std::cout << "\n\n";
  RC_CHECK(got.size() == 8);
  if (got.size() != 8) return;
  RC_CHECK(got[0] == 0.0 && got[1] == 0.0 && got[2] == 0.0 && got[3] == 0.0);        // no history yet: the first reading, until there is some
  RC_CHECK(got[4] == 1.0);
  RC_CHECK(got[7] == 4.0);
}

RC_TEST("lost readings are a share of the readings near the chance given, are marked, and carry the last good value and not a zero") {
  SensorOptions o;
  o.dropout = 0.1;
  SensorModel s(o);
  int lost = 0;
  bool marked_value_ok = true;
  double previous_good = 0.0;
  const int n = 20000;
  for (int i = 0; i < n; ++i) {
    const double truth = 1.0 + 0.001 * i;
    const Reading r = s.read(truth);
    if (!r.valid) {
      ++lost;
      if (r.value != previous_good) marked_value_ok = false;
    } else {
      previous_good = r.value;
    }
  }
  std::cout << "  a dropout chance of 0.1 over " << n << " readings: " << lost << " lost (" << std::fixed << std::setprecision(3) << static_cast<double>(lost) / n << "), counted by the model as "
            << s.dropped() << "\n\n";
  RC_CHECK_NEAR(static_cast<double>(lost) / n, 0.1, 0.01);
  RC_CHECK(s.dropped() == lost);
  RC_CHECK(marked_value_ok);
}

RC_TEST("the same seed gives the same sensor, a different seed a different one, and switching on one effect does not change the noise of another") {
  SensorOptions a;
  a.noise = 0.2;
  a.seed = 77;
  SensorOptions b = a;
  SensorOptions c = a;
  c.seed = 78;
  SensorOptions with_delay = a;
  with_delay.delay = 2;
  SensorModel sa(a);
  SensorModel sb(b);
  SensorModel sc(c);
  SensorModel sd(with_delay);
  bool same = true;
  bool different = false;
  bool delayed_noise_matches = true;
  std::vector<double> plain;
  std::vector<double> delayed;
  for (int i = 0; i < 100; ++i) {
    const double x = sa.read(1.0).value;
    if (x != sb.read(1.0).value) same = false;
    if (x != sc.read(1.0).value) different = true;
    plain.push_back(x);
    delayed.push_back(sd.read(1.0).value);
  }
  for (int i = 2; i < 100; ++i)
    if (delayed[static_cast<std::size_t>(i)] != plain[static_cast<std::size_t>(i - 2)]) delayed_noise_matches = false;
  std::cout << "  seeds: the same twice " << (same ? "agree" : "differ") << "; another seed " << (different ? "differs" : "agrees") << "; the delayed sensor is the plain one two samples late: "
            << (delayed_noise_matches ? "yes" : "no") << "\n\n";
  RC_CHECK(same);
  RC_CHECK(different);
  RC_CHECK(delayed_noise_matches);
}

RC_TEST("a controller that settles with a perfect sensor overshoots three times as far and is still swinging with a delayed one, which is what a sensor model is for") {
  struct Outcome {
    double overshoot = 0.0;
    double last_swing = 0.0;
  };
  const auto run = [](const SensorOptions& o) {
    rc::sim::SimMotor motor(2.0, 0.1, 12.0);
    SensorModel sensor(o);
    Outcome out;
    double measured = 0.0;
    for (int i = 0; i < 600; ++i) {
      const double command = 6.0 * (1.0 - measured);                 // go to position 1
      motor.apply(command, 0.005);
      measured = sensor.read(motor.position()).value;
      if (motor.position() - 1.0 > out.overshoot) out.overshoot = motor.position() - 1.0;
      if (i >= 500 && std::fabs(motor.position() - 1.0) > out.last_swing) out.last_swing = std::fabs(motor.position() - 1.0);
    }
    return out;
  };
  SensorOptions perfect;
  SensorOptions late;
  late.delay = 12;                                                       // 60 ms old
  SensorOptions realistic;
  realistic.delay = 12;
  realistic.noise = 0.005;
  realistic.step = 0.01;
  const Outcome p = run(perfect);
  const Outcome l = run(late);
  const Outcome r = run(realistic);
  std::cout << "  the same proportional controller to position 1\n  sensor                      overshoot   swing in the last half second\n  perfect                     " << std::fixed << std::setprecision(4) << std::setw(9)
            << p.overshoot << "   " << std::setw(10) << p.last_swing << "\n  60 ms old                   " << std::setw(9) << l.overshoot << "   " << std::setw(10) << l.last_swing
            << "\n  60 ms old, noisy, coarse    " << std::setw(9) << r.overshoot << "   " << std::setw(10) << r.last_swing << "\n\n";
  RC_CHECK(p.overshoot > 0.0 && p.overshoot < 0.25);
  RC_CHECK(p.last_swing < 0.001);
  RC_CHECK(l.overshoot > 2.5 * p.overshoot);
  RC_CHECK(l.last_swing > 0.02);
}
