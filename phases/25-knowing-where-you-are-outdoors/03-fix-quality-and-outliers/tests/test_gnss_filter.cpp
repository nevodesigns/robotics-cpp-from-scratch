#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <algorithm>
#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"worst_ungated", 6.035186508540445},
    {"worst_gated", 2.2866298412390096},
    {"false_pct", 0.22058823529411764},
    {"rms_weighed", 0.6658818415051981},
    {"rms_overconfident", 2.8086323945120233},
    {"recover_frames", 5.0},
    {"never_frames", 68.0},
    {"never_seconds", 13.600000000000001},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

struct Outcome {
  double worst = 0.0;
  double rms_single = 0.0;
  int refused_good = 0;
  int good = 0;
  int restarts = 0;
};

rc::field::Fix level_at(double t) {
  if (t < 40.0) return rc::field::Fix::fixed;
  if (t < 80.0) return rc::field::Fix::single;
  if (t < 100.0) return rc::field::Fix::floating;
  if (t < 110.0) return rc::field::Fix::none;
  return rc::field::Fix::fixed;
}

// A robot going at 0.5 m/s along x for two minutes, a fix every 0.2 s, 1 percent of them thrown 8 m by multipath.
Outcome run(const GnssFilterConfig& cfg, unsigned int seed) {
  rc::field::GnssSpec spec;
  rc::field::Gnss gnss(spec, seed);
  GnssFilter f(cfg);
  Outcome o;
  double sq = 0.0;
  int n_single = 0;
  for (int k = 1; k <= 600; ++k) {
    const double t = 0.2 * k;
    const double tx = 0.5 * t, ty = 0.0;
    const rc::field::GnssFix fix = gnss.measure(tx, ty, level_at(t));
    const bool jumped = fix.valid && std::hypot(fix.x - tx, fix.y - ty) > 5.0;
    const bool used = f.update(0.2, fix);
    if (fix.valid && !jumped) {
      ++o.good;
      if (!used) ++o.refused_good;
    }
    if (k > 10) {
      const double e = std::hypot(f.x() - tx, f.y() - ty);
      o.worst = std::max(o.worst, e);
      if (level_at(t) == rc::field::Fix::single) {
        sq += e * e;
        ++n_single;
      }
    }
  }
  o.rms_single = n_single > 0 ? std::sqrt(sq / n_single) : 0.0;
  o.restarts = f.restarts();
  return o;
}

}  // namespace

RC_TEST("a fix is believed as much as its level says: a centimetre for fixed, a metre and a half for single") {
  RC_CHECK_NEAR(fix_sigma(rc::field::Fix::fixed), 0.02, 1e-12);
  RC_CHECK_NEAR(fix_sigma(rc::field::Fix::floating), 0.25, 1e-12);
  RC_CHECK_NEAR(fix_sigma(rc::field::Fix::single), 1.5, 1e-12);
  RC_CHECK(fix_sigma(rc::field::Fix::none) > 1e8);
  // A fixed fix pulls the filter much further than a single one at the same distance from the prediction.
  GnssFilterConfig cfg;
  cfg.use_gate = false;
  GnssFilter a(cfg), b(cfg);
  rc::field::GnssFix start;
  start.valid = true;
  start.fix = rc::field::Fix::single;
  start.x = 0.0;
  start.y = 0.0;
  a.update(0.2, start);
  b.update(0.2, start);
  rc::field::GnssFix fixed = start, single = start;
  fixed.fix = rc::field::Fix::fixed;
  fixed.x = 1.0;
  single.x = 1.0;
  a.update(0.2, fixed);
  b.update(0.2, single);
  RC_CHECK(a.x() > b.x());
}

RC_TEST("a jump of 8 m drags a filter that takes every fix, and a gate refuses it") {
  GnssFilterConfig open;
  open.use_gate = false;
  GnssFilterConfig gated;
  double worst_open = 0.0, worst_gated = 0.0, false_pct = 0.0;
  int refused_good = 0, good = 0;
  for (unsigned int seed = 1; seed <= 5; ++seed) {
    const Outcome a = run(open, seed);
    const Outcome b = run(gated, seed);
    worst_open = std::max(worst_open, a.worst);
    worst_gated = std::max(worst_gated, b.worst);
    refused_good += b.refused_good;
    good += b.good;
  }
  false_pct = 100.0 * refused_good / good;
  rec("worst_ungated", worst_open, 2);
  rec("worst_gated", worst_gated, 2);
  rec("false_pct", false_pct, 1);
  std::cout << "  worst error over five runs: " << worst_open << " m ungated, " << worst_gated << " m gated; " << false_pct << " percent of the good fixes were refused\n";
  RC_CHECK(worst_open > 1.0);
  RC_CHECK(worst_gated < 0.6 * worst_open);
  RC_CHECK(false_pct < 5.0);
}

RC_TEST("a filter that takes every fix as fixed is overconfident on the single ones") {
  GnssFilterConfig weighed;
  GnssFilterConfig overconfident;
  overconfident.weigh_by_level = false;
  double w = 0.0, o = 0.0;
  for (unsigned int seed = 1; seed <= 5; ++seed) {
    w += run(weighed, seed).rms_single;
    o += run(overconfident, seed).rms_single;
  }
  rec("rms_weighed", w / 5.0, 2);
  rec("rms_overconfident", o / 5.0, 2);
  RC_CHECK(w < o);
}

RC_TEST("a robot carried 20 m is found again late by a gate that never gives up and at once by one that does") {
  auto carried = [](int max_refused) {
    GnssFilterConfig cfg;
    cfg.max_refused = max_refused;
    GnssFilter f(cfg);
    rc::field::Gnss gnss(rc::field::GnssSpec(), 9);
    int recovered_after = -1;
    for (int k = 1; k <= 200; ++k) {
      const double t = 0.2 * k;
      // At 20 s the robot is picked up and put down 20 m away.
      const double tx = k < 100 ? 0.5 * t : 0.5 * t + 20.0;
      const rc::field::GnssFix fix = gnss.measure(tx, 0.0, rc::field::Fix::fixed);
      f.update(0.2, fix);
      if (k >= 100 && recovered_after < 0 && std::hypot(f.x() - tx, f.y()) < 0.5) recovered_after = k - 99;
    }
    return recovered_after;
  };
  const int never = carried(1000000);
  const int gives_up = carried(5);
  rec("recover_frames", gives_up, 0);
  rec("never_frames", never, 0);
  rec("never_seconds", never * 0.2, 0);
  std::cout << "  the gate that never gives up found the robot after " << never << " fixes; the one that gives up after five after " << gives_up << "\n";
  RC_CHECK(gives_up >= 5 && gives_up <= 7);
  RC_CHECK(never > 5 * gives_up);
}
