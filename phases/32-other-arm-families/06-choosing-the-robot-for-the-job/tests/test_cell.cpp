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
    {"pick_a", 1.0},
    {"pick_b", 4.0},
    {"pick_c", 2.0},
    {"pick_d", 3.0},
    {"t_a", 0.8060664646741275},
    {"t_b", 2.707894432683505},
    {"huge", 500.0},
    {"pick_none", -1.0},
    {"t_c", 1.9932652990377573},
    {"t_d", 2.82284271247462},
    {"cov_delta_d", 0.3333333333333333},
    {"cov_arm_d", 0.8888888888888888},
    {"cov_stew_d", 0.047619047619047616},
    {"cov_scara_c", 0.47619047619047616},
    {"cov_scara_m", 1.0},
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

struct Picked {
  int index;
  double cycle;
};

Picked pick(const Robots& r, const FamilyData* f, const TaskSpec& t, Verdict* v) {
  const int i = choose_family(r, f, 5, t, v);
  Picked p = {i, i >= 0 ? v[i].cycle_s : 0.0};
  return p;
}

}  // namespace

RC_TEST("the typical families fail the requirements that they should") {
  Robots r;
  FamilyData f[5];
  typical_families(f);
  TaskSpec t;
  t.payload = 30.0;
  Verdict v[5];
  for (int i = 0; i < 5; ++i) v[i] = check_family(r, f[i], t);
  for (int i = 0; i < 5; ++i) RC_CHECK(!v[i].payload_ok);
  TaskSpec tilt;
  tilt.needs_tilt = true;
  RC_CHECK(!check_family(r, f[0], tilt).tilt_ok);
  RC_CHECK(!check_family(r, f[1], tilt).tilt_ok);
  RC_CHECK(check_family(r, f[2], tilt).tilt_ok);
  RC_CHECK(check_family(r, f[4], tilt).tilt_ok);
  TaskSpec fine;
  fine.accuracy_mm = 0.03;
  RC_CHECK(check_family(r, f[0], fine).accuracy_ok);
  RC_CHECK(!check_family(r, f[1], fine).accuracy_ok);
  TaskSpec big;
  big.half_x = 0.45;
  big.half_y = 0.35;
  RC_CHECK(!check_family(r, f[1], big).area_ok);   // the delta's dome is too small
  RC_CHECK(!check_family(r, f[2], big).area_ok);
}

RC_TEST("four different jobs go to four different robots, and a job nothing can do is refused") {
  Robots r;
  FamilyData f[5];
  typical_families(f);
  Verdict v[5];
  // a: small light parts on a conveyor, fast
  TaskSpec a;
  a.half_x = 0.15;
  a.half_y = 0.12;
  a.payload = 0.3;
  a.accuracy_mm = 0.3;
  a.cycle_s = 1.0;
  a.d_app = 0.03;
  a.d_transit = 0.25;
  a.dwell_s = 0.2;  // a vacuum cup
  const Picked pa = pick(r, f, a, v);
  // b: a heavier part turned over and inserted, with a tilt
  TaskSpec b;
  b.half_x = 0.20;
  b.half_y = 0.15;
  b.payload = 4.0;
  b.needs_tilt = true;
  b.accuracy_mm = 0.1;
  b.cycle_s = 6.0;
  b.d_transit = 0.50;
  const Picked pb = pick(r, f, b, v);
  // c: optics held and tilted to a hundredth of a millimetre, slowly
  TaskSpec c;
  c.half_x = 0.03;
  c.half_y = 0.03;
  c.payload = 15.0;
  c.needs_tilt = true;
  c.accuracy_mm = 0.02;
  c.cycle_s = 30.0;
  c.d_app = 0.02;
  c.d_transit = 0.05;
  const Picked pc = pick(r, f, c, v);
  // d: a wide table of boxes, level, a few kilograms
  TaskSpec d;
  d.half_x = 0.45;
  d.half_y = 0.35;
  d.payload = 8.0;
  d.accuracy_mm = 0.5;
  d.cycle_s = 8.0;
  d.d_transit = 0.8;
  const Picked pd = pick(r, f, d, v);
  const double cov_delta_d = v[1].coverage, cov_arm_d = v[4].coverage, cov_stew_d = v[2].coverage;
  const double t_c = pc.cycle, t_d = pd.cycle;
  FamilyData scara_centred = f[0];
  scara_centred.mount_x = 0.0;
  const double cov_scara_c = check_family(r, scara_centred, a).coverage;
  const double cov_scara_m = check_family(r, f[0], a).coverage;
  TaskSpec huge;
  huge.payload = 500.0;
  const Picked pn = pick(r, f, huge, v);
  rec("pick_a", pa.index, 0);
  rec("pick_b", pb.index, 0);
  rec("pick_c", pc.index, 0);
  rec("pick_d", pd.index, 0);
  rec("t_a", pa.cycle, 2);
  rec("t_b", pb.cycle, 2);
  rec("huge", huge.payload, 0);
  rec("pick_none", pn.index, 0);
  rec("t_c", t_c, 1);
  rec("t_d", t_d, 2);
  rec("cov_delta_d", cov_delta_d, 2);
  rec("cov_arm_d", cov_arm_d, 2);
  rec("cov_stew_d", cov_stew_d, 2);
  rec("cov_scara_c", cov_scara_c, 2);
  rec("cov_scara_m", cov_scara_m, 2);
  RC_CHECK(cov_scara_c < 1.0 && cov_scara_m >= 1.0);
  RC_CHECK(cov_delta_d < 1.0 && cov_stew_d < 1.0);
  RC_CHECK(pa.index >= 0 && pb.index >= 0 && pc.index >= 0 && pd.index >= 0);
  RC_CHECK(pn.index == -1);
  RC_CHECK(f[pb.index].tilt && f[pc.index].tilt);
  RC_CHECK(pc.index == 2);
  RC_CHECK(pb.index == 4);
  RC_CHECK(pa.index != pb.index && pa.index != pc.index);
}
