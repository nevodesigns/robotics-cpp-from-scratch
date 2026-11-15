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
    {"n_ep", 40.0},
    {"steps", 100.0},
    {"n", 4000.0},
    {"worst_offset", 0.7094852073565325},
    {"sd_offset", 0.17390962829776183},
    {"sd_heading", 0.10791660187549669},
    {"shared_sample", 40.0},
    {"shared_episode", 0.0},
    {"mse_sample", 0.0011833905097225275},
    {"mse_episode", 0.003171750325630311},
    {"ratio", 2.6802228846452376},
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


RC_TEST("the expert keeps the car in the lane from every start of the demonstrations, and each sample is tagged with its episode") {
  LaneTask t;
  const std::vector<Sample> d = demonstrations(t, 40, 100, 0.6, 0.4, 0.1, 0.3, 1);
  rec("n_ep", 40, 0);
  rec("steps", 100, 0);
  rec("n", static_cast<double>(d.size()), 0);
  RC_CHECK(d.size() == 4000u);
  double worst = 0.0;
  for (const Sample& x : d) worst = std::fabs(x.s[0]) > worst ? std::fabs(x.s[0]) : worst;
  rec("worst_offset", worst, 2);
  RC_CHECK(worst < t.edge);
  RC_CHECK(d.front().episode == 0 && d.back().episode == 39);
  // the label is the expert's own action, within the car's limit
  for (const Sample& x : d) RC_CHECK(std::fabs(x.a) <= t.max_turn);
  // the same seed gives the same data, another seed other data
  const std::vector<Sample> again = demonstrations(t, 40, 100, 0.6, 0.4, 0.1, 0.3, 1), other = demonstrations(t, 40, 100, 0.6, 0.4, 0.1, 0.3, 2);
  RC_CHECK(again[1234].s[0] == d[1234].s[0]);
  RC_CHECK(other[1234].s[0] != d[1234].s[0]);
}

RC_TEST("a normaliser fitted on a set gives that set zero mean and a spread of one, and a set with no spread does not divide by zero") {
  LaneTask t;
  const std::vector<Sample> d = demonstrations(t, 10, 50, 0.6, 0.4, 0.1, 0.3, 3);
  const Normaliser n = fit_normaliser(d);
  double m[2] = {0.0, 0.0}, q[2] = {0.0, 0.0};
  for (const Sample& x : d) {
    double z[2];
    normalise(n, x.s, z);
    for (int j = 0; j < 2; ++j) {
      m[j] += z[j];
      q[j] += z[j] * z[j];
    }
  }
  for (int j = 0; j < 2; ++j) {
    RC_CHECK(std::fabs(m[j] / d.size()) < 1e-9);
    RC_CHECK(std::fabs(q[j] / d.size() - 1.0) < 1e-9);
  }
  rec("sd_offset", n.sd[0], 3);
  rec("sd_heading", n.sd[1], 3);
  std::vector<Sample> flat(5);
  const Normaliser f = fit_normaliser(flat);
  RC_CHECK(f.sd[0] > 0.0 && std::isfinite(1.0 / f.sd[0]));
  const Normaliser none = fit_normaliser(std::vector<Sample>());
  RC_CHECK(none.sd[0] == 1.0 && none.mean[0] == 0.0);
}

RC_TEST("a split by episode keeps every episode on one side, and a split by sample puts every episode on both") {
  LaneTask t;
  const std::vector<Sample> d = demonstrations(t, 40, 100, 0.6, 0.4, 0.1, 0.3, 1);
  std::vector<Sample> tr_s, te_s, tr_e, te_e;
  split_by_sample(d, 5, tr_s, te_s);
  split_by_episode(d, 5, tr_e, te_e);
  rec("shared_sample", shared_episodes(tr_s, te_s), 0);
  rec("shared_episode", shared_episodes(tr_e, te_e), 0);
  RC_CHECK(shared_episodes(tr_s, te_s) == 40);
  RC_CHECK(shared_episodes(tr_e, te_e) == 0);
  RC_CHECK(te_e.size() == 800u && tr_e.size() == 3200u);
  RC_CHECK(te_s.size() == 800u);
  for (const Sample& x : te_e) RC_CHECK(x.episode % 5 == 0);
}

RC_TEST("splitting by sample makes a nearest neighbour look better than it is, because its test samples sit beside training samples of their own episode") {
  LaneTask t;
  const std::vector<Sample> d = demonstrations(t, 40, 100, 0.6, 0.4, 0.1, 0.3, 1);
  std::vector<Sample> tr_s, te_s, tr_e, te_e;
  split_by_sample(d, 5, tr_s, te_s);
  split_by_episode(d, 5, tr_e, te_e);
  const double ms = nearest_mse(tr_s, te_s), me = nearest_mse(tr_e, te_e);
  rec("mse_sample", ms, 5);
  rec("mse_episode", me, 5);
  rec("ratio", me / ms, 1);
  RC_CHECK(me > 1.5 * ms);
}
