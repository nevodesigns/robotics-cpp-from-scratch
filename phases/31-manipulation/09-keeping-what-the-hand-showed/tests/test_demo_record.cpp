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
    {"dur_raw", 5.002035700913063},
    {"n_raw", 151.0},
    {"rate", 30.0},
    {"gap_ok", 0.03860679976363268},
    {"n_res", 151.0},
    {"idle_s", 1.0},
    {"dur_trim", 2.833333333333329},
    {"lag_ms", 100.0},
    {"lag_true_ms", 100.0},
    {"lost", 20.0},
    {"gap_bad", 0.7009350872293214},
    {"ok_bad", 0.0},
    {"ok_good", 1.0},
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

unsigned long long g_state = 7;
double uni() {
  g_state = g_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>(g_state >> 11) / 9007199254740992.0;
}

// The leader stands still for 1 s, reaches for 3 s (a smooth rise of 0.8 rad on joint 0, 0.3 on joint 1), and stands still for 1 s.
double reach(double t) {
  if (t <= 1.0) return 0.0;
  if (t >= 4.0) return 1.0;
  const double u = (t - 1.0) / 3.0;
  return 10 * u * u * u - 15 * u * u * u * u + 6 * u * u * u * u * u;
}

// A recorded episode at about 30 Hz with timing jitter; `lost` frames in a row are dropped from frame 60.
Episode record(int lost, double follower_lag) {
  Episode e;
  const double dt = 1.0 / 30.0;
  const int n = static_cast<int>(5.0 / dt);
  for (int i = 0; i <= n; ++i) {
    if (lost > 0 && i >= 60 && i < 60 + lost) continue;
    Sample s;
    s.t = i * dt + (uni() - 0.5) * 0.006;
    if (s.t < 0.0) s.t = 0.0;
    s.q[0] = 0.8 * reach(s.t);
    s.q[1] = 0.3 * reach(s.t);
    s.qf[0] = 0.8 * reach(s.t - follower_lag);
    s.qf[1] = 0.3 * reach(s.t - follower_lag);
    s.grip = 0.085;
    e.push_back(s);
  }
  return e;
}

}  // namespace

RC_TEST("the longest gap is found, and resampling gives a steady grid that follows the original") {
  g_state = 7;
  const Episode raw = record(0, 0.1);
  const double gap = max_gap(raw);
  const Episode res = resample(raw, 30.0);
  rec("dur_raw", raw.back().t - raw.front().t, 1);
  rec("n_raw", raw.size(), 0);
  rec("rate", 30, 0);
  rec("gap_ok", gap, 3);
  rec("n_res", res.size(), 0);
  RC_CHECK(gap < 0.045);
  RC_CHECK(res.size() > 140 && res.size() < 160);
  for (std::size_t i = 1; i < res.size(); ++i) RC_CHECK_NEAR(res[i].t - res[i - 1].t, 1.0 / 30.0, 1e-9);
  for (std::size_t i = 0; i < res.size(); ++i) RC_CHECK_NEAR(res[i].q[0], 0.8 * reach(res[i].t), 0.01);
}

RC_TEST("the idle ends are trimmed to the motion with a little padding") {
  g_state = 7;
  const Episode res = resample(record(0, 0.1), 30.0);
  const Episode trimmed = trim_idle(res, 0.02, 3);
  rec("idle_s", 1.0, 0);
  rec("dur_trim", trimmed.back().t - trimmed.front().t, 1);
  RC_CHECK(trimmed.size() < res.size());
  RC_CHECK(trimmed.back().t - trimmed.front().t > 2.6);
  RC_CHECK(trimmed.back().t - trimmed.front().t < 3.6);
  Episode still;
  for (int i = 0; i < 10; ++i) {
    Sample s;
    s.t = i * 0.033;
    still.push_back(s);
  }
  RC_CHECK(trim_idle(still, 0.02, 3).empty());
}

RC_TEST("the delay of the follower behind the leader is found") {
  g_state = 7;
  const Episode res = resample(record(0, 0.1), 30.0);
  const double lag = estimate_lag(res, 30.0, 0.5);
  rec("lag_ms", lag * 1000.0, 0);
  rec("lag_true_ms", 100, 0);
  RC_CHECK_NEAR(lag, 0.1, 0.034);
}

RC_TEST("an episode that lost frames is refused, and a good one kept") {
  g_state = 7;
  const Episode good = record(0, 0.1);
  g_state = 7;
  const Episode bad = record(20, 0.1);
  const EpisodeCheck cg = check_episode(good, 0.02, 0.2, 2.0, 0.7);
  const EpisodeCheck cb = check_episode(bad, 0.02, 0.2, 2.0, 0.7);
  rec("lost", 20, 0);
  rec("gap_bad", cb.longest_gap, 2);
  rec("ok_bad", cb.ok ? 1 : 0, 0);
  rec("ok_good", cg.ok ? 1 : 0, 0);
  RC_CHECK(cg.ok);
  RC_CHECK(!cb.ok);
  RC_CHECK(cb.longest_gap > 0.6);
  const EpisodeCheck idle = check_episode(trim_idle(resample(good, 30.0), 0.02, 3), 0.02, 0.2, 2.0, 0.3);
  RC_CHECK(idle.ok);
  RC_CHECK(cg.idle_fraction > idle.idle_fraction);
}
