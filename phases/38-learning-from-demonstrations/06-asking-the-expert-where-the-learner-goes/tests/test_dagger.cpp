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
    {"n0", 3000.0},
    {"round0", 87.0},
    {"n1", 4570.0},
    {"round1", 98.0},
    {"n2", 6570.0},
    {"round2", 98.0},
    {"expert", 98.0},
    {"verr_before", 0.010821996475833194},
    {"verr_after", 8.636248762167333e-05},
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

RC_TEST("the learner drives and the expert labels every state it reaches, with the learner's own path and the expert's actions") {
  const LaneTask t;
  const Learner l = clone(narrow_demos(), 16, 200, 0.02, 4);
  const std::vector<Sample> c = collect_on_learner(t, l, 0.6, 0.8, 5, 100, 1000, 9);
  RC_CHECK(!c.empty());
  for (const Sample& x : c) {
    RC_CHECK(x.a == lane_expert(t, x.s[0], x.s[1]));
    RC_CHECK(x.episode >= 1000 && x.episode < 1005);
  }
  // the states follow the learner's actions, not the expert's
  double e = c[0].s[0], h = c[0].s[1];
  lane_step(t, e, h, learner_act(l, c[0].s[0], c[0].s[1]));
  RC_CHECK(c[1].episode == c[0].episode);
  RC_CHECK(std::fabs(c[1].s[0] - e) < 1e-15 && std::fabs(c[1].s[1] - h) < 1e-15);
  // a run that leaves the lane ends there
  for (const Sample& x : c) RC_CHECK(std::fabs(x.s[0]) <= t.edge);
}

RC_TEST("two rounds of asking the expert bring the learner from the clone's success to the expert's") {
  const LaneTask t;
  const std::vector<Sample> d = narrow_demos();
  Learner l = clone(d, 16, 200, 0.02, 4);
  const double before = success_rate(t, &l, l.scale, 0.6, 0.8, 100, 77, 300);
  const double expert = success_rate(t, nullptr, l.scale, 0.6, 0.8, 100, 77, 300);
  const double verr_before = visited_error(t, l, 0.6, 0.8, 50, 300, 55);
  const std::vector<DaggerRound> rounds = dagger(t, d, l, 2, 20, 100, 0.6, 0.8, 16, 100, 0.02, 500);
  rec("n0", static_cast<double>(d.size()), 0);
  rec("round0", before * 100.0, 0);
  rec("n1", rounds[0].samples, 0);
  rec("round1", rounds[0].success * 100.0, 0);
  rec("n2", rounds[1].samples, 0);
  rec("round2", rounds[1].success * 100.0, 0);
  rec("expert", expert * 100.0, 0);
  rec("verr_before", verr_before, 5);
  rec("verr_after", visited_error(t, l, 0.6, 0.8, 50, 300, 55), 5);
  RC_CHECK(rounds[1].success > before + 0.05);
  RC_CHECK(rounds[1].success >= expert - 0.02);
  RC_CHECK(visited_error(t, l, 0.6, 0.8, 50, 300, 55) < 0.5 * verr_before);
  RC_CHECK(rounds[1].samples > rounds[0].samples && rounds[0].samples > static_cast<int>(d.size()));
}
