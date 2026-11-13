#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

class Ball : public ReachTest {
 public:
  explicit Ball(double r) : r_(r) {}
  bool reaches(double x, double y, double z) const override { return x * x + y * y + z * z <= r_ * r_; }

 private:
  double r_;
};


}  // namespace

RC_TEST("the sampling gives the volume of a ball and of a box, with an error that shrinks with the points") {
  ReachBox b = {{-1, -1, -1}, {1, 1, 1}};
  double se1 = 0.0, se2 = 0.0;
  const double v1 = workspace_volume(Ball(1.0), b, 4000, 1, &se1);
  const double v2 = workspace_volume(Ball(1.0), b, 64000, 1, &se2);
  const double truth = 4.0 / 3.0 * kPi;
  RC_CHECK(std::fabs(v1 - truth) < 5.0 * se1);
  RC_CHECK(std::fabs(v2 - truth) < 5.0 * se2);
  RC_CHECK(se2 < se1 / 3.0);
  double lo[3] = {0, 0, 0}, hi[3] = {0.5, 0.5, 0.5};
  const double vbox = workspace_volume(GantryReach(lo, hi), b, 64000, 3, nullptr);
  RC_CHECK_NEAR(vbox, 0.125, 0.01);
}

RC_TEST("each kind of robot is asked the same question, and the volumes and the room they take are compared") {
  const int n = 40000;
  double se = 0.0;
  Scara scara;
  Delta delta;
  Stewart stewart;
  const double glo[3] = {-0.3, -0.2, 0.0}, ghi[3] = {0.3, 0.2, 0.15};
  ReachBox scara_box = {{-0.5, -0.5, 0.0}, {0.5, 0.5, 0.15}};
  ReachBox delta_box = {{-0.4, -0.4, -0.7}, {0.4, 0.4, -0.2}};
  ReachBox stewart_box = {{-0.4, -0.4, 0.0}, {0.4, 0.4, 0.6}};
  ReachBox gantry_box = {{-0.5, -0.5, -0.1}, {0.5, 0.5, 0.3}};
  const double v_scara = workspace_volume(ScaraReach(scara, 0.0), scara_box, n, 1, &se);
  const double se_scara = se;
  const double v_delta = workspace_volume(DeltaReach(delta), delta_box, n, 2, &se);
  const double v_stewart = workspace_volume(StewartReach(stewart, 0.28, 0.44), stewart_box, n, 3, &se);
  const double v_gantry = workspace_volume(GantryReach(glo, ghi), gantry_box, n, 4, &se);
  ArmTable arm = elbow_arm();
  Pose tool;
  tool.p[2] = 0.10;
  ReachBox arm_box = {{-1.0, -1.0, -0.6}, {1.0, 1.0, 1.2}};
  const double v_arm = workspace_volume(ArmReach(arm, tool), arm_box, n / 4, 5, &se);
  // the room each takes (typical): a cylinder or a box round the working machine, in m3
  const double room_scara = kPi * 0.45 * 0.45 * 0.40;
  const double room_delta = kPi * 0.30 * 0.30 * 0.55;
  const double room_stewart = kPi * 0.25 * 0.25 * 0.45;
  const double room_gantry = 0.7 * 0.5 * 0.35;
  const double room_arm = 4.0 / 3.0 * kPi * 0.75 * 0.75 * 0.75;  // a sphere of its reach, 0.40 + 0.35 m
  rec("n", n, 0);
  rec("v_scara", v_scara, 3);
  rec("v_delta", v_delta, 3);
  rec("v_stewart", v_stewart, 3);
  rec("v_gantry", v_gantry, 3);
  rec("v_arm", v_arm, 3);
  rec("se_pct", 100.0 * se_scara / v_scara, 1);
  rec("r_scara", room_ratio(v_scara, room_scara), 2);
  rec("r_delta", room_ratio(v_delta, room_delta), 2);
  rec("r_stewart", room_ratio(v_stewart, room_stewart), 2);
  rec("r_gantry", room_ratio(v_gantry, room_gantry), 2);
  rec("r_arm", room_ratio(v_arm, room_arm), 2);
  RC_CHECK(v_scara > 0.0 && v_delta > 0.0 && v_stewart > 0.0 && v_gantry > 0.0 && v_arm > 0.0);
  RC_CHECK_NEAR(v_gantry, 0.6 * 0.4 * 0.15, 0.1 * 0.6 * 0.4 * 0.15);
  const double r_s = room_ratio(v_scara, room_scara), r_d = room_ratio(v_delta, room_delta), r_w = room_ratio(v_stewart, room_stewart), r_g = room_ratio(v_gantry, room_gantry), r_a = room_ratio(v_arm, room_arm);
  RC_CHECK(r_a > r_d && r_a > r_s && r_a > r_g && r_a > r_w);
  RC_CHECK(r_w < r_s && r_w < r_d && r_w < r_g);
  RC_CHECK(r_a <= 1.01);
}
