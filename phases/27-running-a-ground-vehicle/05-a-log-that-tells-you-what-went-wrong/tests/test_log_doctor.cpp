#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

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
    {"found", 5.0},
    {"max_err", 1.799999999999983},
    {"fp_runs", 20.0},
    {"fp", 0.0},
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

unsigned long long g_s = 1;
double unit() {
  g_s = g_s * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>((g_s >> 33) & 0xFFFFFFULL) / 16777216.0;
}
double noise(double sigma) {
  double a = 0.0;
  for (int i = 0; i < 12; ++i) a += unit();
  return (a - 6.0) * sigma;
}

struct Faults {
  bool stuck = false, slip = false, off = false, drain = false, gap = false;
  double slip_ground = 0.3;  // the ground speed in the slip patch, m/s (the wheels say 1.0)
};

// 300 s at 10 Hz, driving along x at 1 m/s; faults put in at known times.
std::vector<LogRow> make_run(unsigned seed, const Faults& f) {
  g_s = seed * 7919ULL + 13ULL;
  std::vector<LogRow> rows;
  double x = 0.0, soc = 100.0;
  for (int i = 0; i < 3000; ++i) {
    const double t = i * 0.1;
    double v = 1.0, odom = 1.0 + noise(0.03), rate = 0.01, cte = noise(0.05);
    if (f.stuck && t >= 100.0 && t < 108.0) {
      v = 0.0;
      odom = 0.0;
    }
    if (f.slip && t >= 150.0 && t < 160.0) v = f.slip_ground;
    if (f.off && t >= 200.0 && t < 206.0) cte = 1.2;
    if (f.drain && t >= 250.0 && t < 262.0) rate = 0.04;
    x += v * 0.1;
    soc -= rate * 0.1;
    if (f.gap && t >= 270.0 && t < 290.0) continue;
    LogRow r;
    r.t = t;
    r.x = x + noise(0.02);
    r.y = noise(0.02);
    r.cmd_v = 1.0;
    r.odom_v = odom;
    r.soc_wh = soc + noise(0.0005);
    r.cte = cte;
    rows.push_back(r);
  }
  return rows;
}

const Finding* find(const std::vector<Finding>& v, FindingKind k) {
  for (std::size_t i = 0; i < v.size(); ++i)
    if (v[i].kind == k) return &v[i];
  return nullptr;
}

}  // namespace

RC_TEST("spans join near marks, split far ones and drop short ones") {
  std::vector<double> t;
  std::vector<bool> flag;
  for (int i = 0; i < 100; ++i) {
    t.push_back(i * 0.5);
    flag.push_back((i >= 10 && i <= 20) || (i >= 22 && i <= 30) || (i >= 60 && i <= 61) || (i >= 80 && i <= 90));
  }
  const std::vector<std::pair<double, double> > s = spans_from_flags(t, flag, 2.0, 1.0);
  RC_CHECK(s.size() == 2);
  RC_CHECK_NEAR(s[0].first, 5.0, 1e-9);
  RC_CHECK_NEAR(s[0].second, 15.0, 1e-9);
  RC_CHECK_NEAR(s[1].first, 40.0, 1e-9);
  RC_CHECK_NEAR(s[1].second, 45.0, 1e-9);
}

RC_TEST("a run with five faults put in gives five findings, each where it began") {
  Faults f;
  f.stuck = f.slip = f.off = f.drain = f.gap = true;
  const std::vector<LogRow> rows = make_run(1, f);
  const std::vector<Finding> found = analyse_log(rows, DoctorConfig());
  const FindingKind kinds[5] = {kFindStuck, kFindSlip, kFindOffPath, kFindDrain, kFindGap};
  const double t0[5] = {100.0, 150.0, 200.0, 250.0, 269.9};
  const double t1[5] = {108.0, 160.0, 206.0, 262.0, 290.0};
  int got = 0;
  double worst = 0.0;
  for (int k = 0; k < 5; ++k) {
    const Finding* g = find(found, kinds[k]);
    if (g == nullptr) continue;
    ++got;
    worst = std::max(worst, std::max(std::fabs(g->t0 - t0[k]), std::fabs(g->t1 - t1[k])));
  }
  rec("found", got, 0);
  rec("max_err", worst, 1);
  RC_CHECK(got == 5);
  RC_CHECK(worst < 2.6);
  RC_CHECK(found.size() == 5);
  for (std::size_t i = 1; i < found.size(); ++i) RC_CHECK(found[i - 1].t0 <= found[i].t0);
}

RC_TEST("clean runs with noise give no findings") {
  int fp = 0;
  for (unsigned seed = 1; seed <= 20; ++seed) fp += static_cast<int>(analyse_log(make_run(seed, Faults()), DoctorConfig()).size());
  rec("fp_runs", 20, 0);
  rec("fp", fp, 0);
  RC_CHECK(fp == 0);
}

RC_TEST("a slip that leaves the ground at 80 percent of the wheels is not reported and one at 50 percent is") {
  Faults mild, bad;
  mild.slip = bad.slip = true;
  mild.slip_ground = 0.8;
  bad.slip_ground = 0.5;
  RC_CHECK(find(analyse_log(make_run(3, mild), DoctorConfig()), kFindSlip) == nullptr);
  RC_CHECK(find(analyse_log(make_run(3, bad), DoctorConfig()), kFindSlip) != nullptr);
}

RC_TEST("a vehicle that was told to stand still is not called stuck") {
  std::vector<LogRow> rows;
  for (int i = 0; i < 600; ++i) {
    LogRow r;
    r.t = i * 0.1;
    r.soc_wh = 100.0 - 0.001 * i;
    rows.push_back(r);
  }
  RC_CHECK(analyse_log(rows, DoctorConfig()).empty());
}
