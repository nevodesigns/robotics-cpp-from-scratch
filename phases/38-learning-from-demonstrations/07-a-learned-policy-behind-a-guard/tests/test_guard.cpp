#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <limits>

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
    {"clone", 87.0},
    {"zmax", 3.0},
    {"guarded", 98.0},
    {"fb_share", 11.59755268524813},
    {"fallback_alone", 98.0},
    {"fb_narrow", 2.4299999999999997},
    {"z_tight", 1.0},
    {"fb_tight", 19.840244731475188},
    {"guard_tight", 98.0},
    {"z_loose", 1000.0},
    {"fb_loose", 0.0},
    {"guard_loose", 87.0},
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


static std::vector<Sample> narrow_demos() { return demonstrations(LaneTask(), 30, 100, 0.2, 0.1, 0.0, 0.0, 21); }

RC_TEST("the guard says why it acted: a bad input, an unseen state, a bad output, an output past the limit, or the network's own") {
  const LaneTask t;
  const std::vector<Sample> d = narrow_demos();
  Learner l = clone(d, 16, 200, 0.02, 4);
  Guard g;
  g.seen = fit_normaliser(d);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  RC_CHECK(guarded_act(t, g, l, 0.01, 0.0).reason == kLearned);
  const GuardedAction bad_in = guarded_act(t, g, l, nan, 0.1);
  RC_CHECK(bad_in.reason == kBadInput && std::isfinite(bad_in.turn));
  RC_CHECK(std::fabs(bad_in.turn - fallback_turn(t, g, 0.0, 0.1)) < 1e-15);
  const GuardedAction far = guarded_act(t, g, l, 0.9, 0.0);
  RC_CHECK(far.reason == kUnseen && far.turn == fallback_turn(t, g, 0.9, 0.0));
  Learner broken = l;
  broken.net.b2 = nan;
  const GuardedAction bad_out = guarded_act(t, g, broken, 0.01, 0.0);
  RC_CHECK(bad_out.reason == kBadOutput && std::isfinite(bad_out.turn));
  Learner wild = l;
  wild.net.b2 = 50.0;
  const GuardedAction big = guarded_act(t, g, wild, 0.01, 0.0);
  RC_CHECK(big.reason == kClipped && big.turn == t.max_turn);
}

RC_TEST("behind the guard the clone of 38-05 succeeds from wide starts, and from the narrow ones the guard leaves it alone") {
  const LaneTask t;
  const std::vector<Sample> d = narrow_demos();
  const Learner l = clone(d, 16, 200, 0.02, 4);
  Guard g;
  g.seen = fit_normaliser(d);
  double fb = 0.0, fb_narrow = 0.0;
  const double clone_rate = success_rate(t, &l, g.seen, 0.6, 0.8, 100, 77, 300);
  const double guarded = guarded_success(t, g, l, 0.6, 0.8, 100, 77, 300, &fb);
  // the fall-back alone: a guard that calls everything unseen
  Guard only = g;
  only.z_max = 0.0;
  const double alone = guarded_success(t, only, l, 0.6, 0.8, 100, 77, 300, nullptr);
  const double narrow = guarded_success(t, g, l, 0.2, 0.1, 100, 78, 300, &fb_narrow);
  rec("clone", clone_rate * 100.0, 0);
  rec("zmax", g.z_max, 0);
  rec("guarded", guarded * 100.0, 0);
  rec("fb_share", fb * 100.0, 0);
  rec("fallback_alone", alone * 100.0, 0);
  rec("fb_narrow", fb_narrow * 100.0, 1);
  RC_CHECK(guarded > clone_rate + 0.05);
  RC_CHECK(fb_narrow < 0.05);
  RC_CHECK(narrow >= 0.95);
  RC_CHECK(fb < 0.5);
}

RC_TEST("a tighter threshold hands more of the driving to the fall-back, and one so loose it never acts leaves the clone as it was") {
  const LaneTask t;
  const std::vector<Sample> d = narrow_demos();
  const Learner l = clone(d, 16, 200, 0.02, 4);
  Guard tight, loose;
  tight.seen = loose.seen = fit_normaliser(d);
  tight.z_max = 1.0;
  loose.z_max = 1000.0;
  double ft = 0.0, fl = 0.0;
  const double st = guarded_success(t, tight, l, 0.6, 0.8, 100, 77, 300, &ft), sl = guarded_success(t, loose, l, 0.6, 0.8, 100, 77, 300, &fl);
  rec("z_tight", tight.z_max, 0);
  rec("fb_tight", ft * 100.0, 0);
  rec("guard_tight", st * 100.0, 0);
  rec("z_loose", loose.z_max, 0);
  rec("fb_loose", fl * 100.0, 0);
  rec("guard_loose", sl * 100.0, 0);
  Guard usual = tight;
  usual.z_max = 3.0;
  double fu = 0.0;
  guarded_success(t, usual, l, 0.6, 0.8, 100, 77, 300, &fu);
  RC_CHECK(ft > fu && fu > fl);
  RC_CHECK(fl == 0.0);
  RC_CHECK(sl == success_rate(t, &l, tight.seen, 0.6, 0.8, 100, 77, 300));   // a guard that never acts is the clone
}
