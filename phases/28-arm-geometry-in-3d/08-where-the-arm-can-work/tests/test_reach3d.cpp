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
    {"vol_small", 1.0810000000000002},
    {"vol_mid", 2.5810000000000004},
    {"vol_big", 2.958},
    {"fine_60k", 2.032875},
    {"fine_200k", 2.5537500000000004},
    {"reach_ur5", 1.051284928076114},
    {"reach_panda", 1.1556816170554933},
    {"far_any", 0.8602325267042628},
    {"far_down", 0.7400000000000004},
    {"dex_share", 61.88094657233471},
    {"cov_free", 96.07843137254902},
    {"cov_120", 96.07843137254902},
    {"cov_lim", 78.43137254901961},
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


RC_TEST("a small sample underestimates the volume and a large one settles, if there are many samples to a cell") {
  const ArmTable t = elbow_arm();
  VoxelGrid small(0.10, 1.0), mid(0.10, 1.0), big(0.10, 1.0);
  sample_reach(t, Pose(), Pose(), 2000, 1, 0.0, small);
  sample_reach(t, Pose(), Pose(), 20000, 1, 0.0, mid);
  sample_reach(t, Pose(), Pose(), 200000, 1, 0.0, big);
  rec("vol_small", volume(small), 3);
  rec("vol_mid", volume(mid), 3);
  rec("vol_big", volume(big), 3);
  RC_CHECK(volume(small) < volume(mid) && volume(mid) < volume(big));
  RC_CHECK(volume(small) < 0.6 * volume(big));
  RC_CHECK(volume(mid) > 0.85 * volume(big));
  // With cells half the size there are eight times as many to fill, and the same sample is not enough.
  VoxelGrid f60(0.05, 1.0), f200(0.05, 1.0);
  sample_reach(t, Pose(), Pose(), 60000, 1, 0.0, f60);
  sample_reach(t, Pose(), Pose(), 200000, 1, 0.0, f200);
  rec("fine_60k", volume(f60), 3);
  rec("fine_200k", volume(f200), 3);
  RC_CHECK(volume(f200) > 1.1 * volume(f60));
}

RC_TEST("the farthest reach is within what the lengths of the table allow, and is most of it") {
  const ArmTable u = ur5(), p = panda();
  VoxelGrid gu(0.04, 1.2), gp(0.04, 1.2);
  sample_reach(u, Pose(), Pose(), 200000, 3, 0.0, gu);
  sample_reach(p, Pose(), Pose(), 200000, 3, 0.0, gp);
  rec("reach_ur5", farthest(gu), 2);
  rec("reach_panda", farthest(gp), 2);
  double bound_u = 0.0, bound_p = 0.0;
  for (std::size_t i = 0; i < u.rows.size(); ++i) bound_u += std::fabs(u.rows[i].a) + std::fabs(u.rows[i].d);
  for (std::size_t i = 0; i < p.rows.size(); ++i) bound_p += std::fabs(p.rows[i].a) + std::fabs(p.rows[i].d);
  RC_CHECK(farthest(gu) <= bound_u + 0.05);
  RC_CHECK(farthest(gp) <= bound_p + 0.05);
  RC_CHECK(farthest(gu) > 0.7 * (0.425 + 0.39225));
  RC_CHECK(farthest(gp) > 0.7 * (0.333 + 0.316 + 0.384));
}

RC_TEST("the reach with the tool pointing down is shorter than the reach in any orientation") {
  const ArmTable t = elbow_arm();
  VoxelGrid g(0.04, 1.2);
  sample_reach(t, Pose(), Pose(), 300000, 5, 0.0, g);
  const double any = farthest_horizontal(g, 0.10, 0.20);
  const double down = tool_down_radius(t, 0.15, Pose());
  rec("far_any", any, 2);
  rec("far_down", down, 2);
  RC_CHECK(down < any - 0.03);
  RC_CHECK(down > 0.5);
  RC_CHECK(tool_down_reachable(t, 0.0, 0.0, 0.15, Pose()));
}

RC_TEST("the poses where the arm can move well are a part of the poses it can reach") {
  const ArmTable t = elbow_arm();
  VoxelGrid all(0.05, 1.0), dex(0.05, 1.0);
  sample_reach(t, Pose(), Pose(), 60000, 7, 0.0, all);
  sample_reach(t, Pose(), Pose(), 60000, 7, 0.1, dex);
  const double share = 100.0 * volume(dex) / volume(all);
  rec("dex_share", share, 0);
  RC_CHECK(volume(dex) < volume(all));
  RC_CHECK(share > 10.0);
  RC_CHECK(share < 95.0);
}

RC_TEST("what fits on a table depends on the limits, once they are tight enough") {
  ArmTable wide = elbow_arm(), tight = elbow_arm();
  for (std::size_t i = 0; i < wide.rows.size(); ++i) {
    wide.rows[i].lo = -2.0944;
    wide.rows[i].hi = 2.0944;
    tight.rows[i].lo = -1.0472;
    tight.rows[i].hi = 1.0472;
  }
  const ArmTable free_arm = elbow_arm();
  const double a = table_coverage(free_arm, Pose(), 0.15, 0.70, -0.40, 0.40, 0.15, 0.05);
  const double w = table_coverage(wide, Pose(), 0.15, 0.70, -0.40, 0.40, 0.15, 0.05);
  const double b = table_coverage(tight, Pose(), 0.15, 0.70, -0.40, 0.40, 0.15, 0.05);
  rec("cov_free", 100.0 * a, 0);
  rec("cov_120", 100.0 * w, 0);
  rec("cov_lim", 100.0 * b, 0);
  RC_CHECK(a > 0.5);
  RC_CHECK(w <= a);
  RC_CHECK(b < 0.9 * a);
  RC_CHECK(tool_down_reachable(free_arm, 0.5, 0.0, 0.3, Pose()));
  RC_CHECK(!tool_down_reachable(free_arm, 2.0, 0.0, 0.3, Pose()));
}
