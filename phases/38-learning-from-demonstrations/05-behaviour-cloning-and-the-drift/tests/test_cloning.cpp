#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

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
    {"hidden", 16.0},
    {"n_demo", 3000.0},
    {"e_demo", 0.2},
    {"h_demo", 0.1},
    {"mse_same", 1.419184682126539e-05},
    {"expert_narrow", 100.0},
    {"clone_narrow", 100.0},
    {"e_wide", 0.6},
    {"h_wide", 0.8},
    {"expert_wide", 98.0},
    {"clone_wide", 87.0},
    {"unseen_fail", 100.0},
    {"unseen_ok", 12.363984674329503},
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


// demonstrations from near the centre line, by a consistent expert with clean execution
static std::vector<Sample> narrow_demos() { return demonstrations(LaneTask(), 30, 100, 0.2, 0.1, 0.0, 0.0, 21); }

RC_TEST("the clone copies the expert's actions closely on demonstrations of the kind it was trained on") {
  const std::vector<Sample> d = narrow_demos();
  const Learner l = clone(d, 16, 200, 0.02, 4);
  const std::vector<Sample> fresh = demonstrations(LaneTask(), 10, 100, 0.2, 0.1, 0.0, 0.0, 99);
  rec("hidden", 16, 0);
  rec("n_demo", static_cast<double>(d.size()), 0);
  rec("e_demo", 0.2, 1);
  rec("h_demo", 0.1, 1);
  rec("mse_same", mlp_mse(l.net, l.scale, fresh), 6);
  RC_CHECK(mlp_mse(l.net, l.scale, fresh) < 1e-3);
}

RC_TEST("driving from starts like the demonstrations', the clone does as well as the expert") {
  const LaneTask t;
  const std::vector<Sample> d = narrow_demos();
  const Learner l = clone(d, 16, 200, 0.02, 4);
  const Normaliser seen = fit_normaliser(d);
  const double ex = success_rate(t, nullptr, seen, 0.2, 0.1, 100, 78, 300), cl = success_rate(t, &l, seen, 0.2, 0.1, 100, 78, 300);
  rec("expert_narrow", ex * 100.0, 0);
  rec("clone_narrow", cl * 100.0, 0);
  RC_CHECK(cl >= 0.95 && ex >= 0.95);
  // a start the expert recovers from and the rollout reports it
  const Rollout r = rollout(t, nullptr, seen, 0.1, 0.0, 300);
  RC_CHECK(r.stayed && r.settled && r.steps == 300);
}

RC_TEST("from wider starts the clone fails where the expert succeeds, and its failures are spent in states nobody demonstrated") {
  const LaneTask t;
  const std::vector<Sample> d = narrow_demos();
  const Learner l = clone(d, 16, 200, 0.02, 4);
  const Normaliser seen = fit_normaliser(d);
  const double ex = success_rate(t, nullptr, seen, 0.6, 0.8, 100, 77, 300), cl = success_rate(t, &l, seen, 0.6, 0.8, 100, 77, 300);
  rec("e_wide", 0.6, 1);
  rec("h_wide", 0.8, 1);
  rec("expert_wide", ex * 100.0, 0);
  rec("clone_wide", cl * 100.0, 0);
  RC_CHECK(ex - cl >= 0.1);
  // where the clone's runs spent their time
  Rng r = make_rng(77);
  int fail_steps = 0, fail_unseen = 0, ok_steps = 0, ok_unseen = 0;
  for (int k = 0; k < 100; ++k) {
    const double e = 0.6 * (2.0 * uniform(r) - 1.0), h = 0.8 * (2.0 * uniform(r) - 1.0);
    const Rollout ro = rollout(t, &l, seen, e, h, 300);
    if (ro.stayed && ro.settled) {
      ok_steps += ro.steps;
      ok_unseen += ro.unseen;
    } else {
      fail_steps += ro.steps;
      fail_unseen += ro.unseen;
    }
  }
  const double uf = 100.0 * fail_unseen / fail_steps, uo = 100.0 * ok_unseen / ok_steps;
  rec("unseen_fail", uf, 0);
  rec("unseen_ok", uo, 0);
  RC_CHECK(uf > 2.0 * uo);
}
