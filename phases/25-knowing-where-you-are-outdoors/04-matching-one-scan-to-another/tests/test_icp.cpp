#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include <rc/core/random.hpp>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

// The points of an L shaped room with a post in it, as a scan would see them, every 5 cm along the walls.
std::vector<P2> room() {
  std::vector<P2> pts;
  for (double x = 0.0; x <= 8.0; x += 0.05) pts.push_back(P2(x, 0.0));
  for (double y = 0.0; y <= 6.0; y += 0.05) pts.push_back(P2(8.0, y));
  for (double x = 8.0; x >= 3.0; x -= 0.05) pts.push_back(P2(x, 6.0));
  for (double y = 6.0; y >= 2.0; y -= 0.05) pts.push_back(P2(3.0, y));
  for (double x = 3.0; x >= 0.0; x -= 0.05) pts.push_back(P2(x, 2.0));
  for (double y = 2.0; y >= 0.0; y -= 0.05) pts.push_back(P2(0.0, y));
  for (int k = 0; k < 24; ++k) pts.push_back(P2(5.0 + 0.2 * std::cos(k * 0.2618), 3.0 + 0.2 * std::sin(k * 0.2618)));
  return pts;
}

// The same points seen after the sensor moved by `t`: each point is where it was, in the sensor's new frame. A move of the source onto the target is then t.
std::vector<P2> seen_after(const std::vector<P2>& pts, const rc::field::Tf2& t, double sigma, unsigned int seed) {
  rc::core::Rng rng(seed);
  const rc::field::Tf2 inv = rc::field::inverse(t);
  std::vector<P2> out;
  for (std::size_t i = 0; i < pts.size(); ++i) {
    P2 q = move_point(inv, pts[i]);
    q.first += sigma * rng.gaussian();
    q.second += sigma * rng.gaussian();
    out.push_back(q);
  }
  return out;
}

rc::field::Tf2 pose(double x, double y, double th) {
  rc::field::Tf2 t;
  t.x = x;
  t.y = y;
  t.theta = th;
  return t;
}

}  // namespace

RC_TEST("a scan moved by a known amount is matched back to it, from a first guess of no move") {
  const std::vector<P2> target = room();
  const rc::field::Tf2 truth = pose(0.30, 0.10, 0.15);
  const std::vector<P2> source = seen_after(target, truth, 0.01, 3);
  const IcpResult r = icp(source, target, pose(0, 0, 0), 60, 1.0, 1e-6);
  const double err_xy = std::hypot(r.move.x - truth.x, r.move.y - truth.y);
  const double err_th = std::fabs(r.move.theta - truth.theta);
  rec("err_xy", err_xy, 4);
  rec("err_theta", err_th, 4);
  rec("iters", r.rounds, 0);
  std::cout << "  recovered (" << r.move.x << ", " << r.move.y << ", " << r.move.theta << ") for (0.30, 0.10, 0.15) in " << r.rounds << " rounds, rms " << r.rms << "\n";
  RC_CHECK(r.converged);
  RC_CHECK(err_xy < 0.02);
  RC_CHECK(err_th < 0.01);
  RC_CHECK(r.rms < 0.02);
}

RC_TEST("it converges only from a first guess not too far off, and the limit is a matter of the shape") {
  const std::vector<P2> target = room();
  const rc::field::Tf2 truth = pose(0.2, 0.1, 0.0);
  const std::vector<P2> source = seen_after(target, truth, 0.005, 5);
  double basin = 0.0, first_fail = -1.0;
  const double angles[] = {0.1, 0.3, 0.5, 0.8, 1.2, 2.0};
  for (int i = 0; i < 6; ++i) {
    const IcpResult r = icp(source, target, pose(0, 0, angles[i]), 80, 2.0, 1e-6);
    const bool ok = std::hypot(r.move.x - truth.x, r.move.y - truth.y) < 0.05 && std::fabs(r.move.theta) < 0.02;
    if (ok && first_fail < 0.0) basin = angles[i];
    if (!ok && first_fail < 0.0) first_fail = angles[i];
  }
  rec("basin", basin, 1);
  rec("fail_angle", first_fail, 1);
  RC_CHECK(basin >= 0.3);
  RC_CHECK(first_fail > 0.0);
}

RC_TEST("in a corridor the match cannot tell how far along it the sensor moved, and a partition across it tells it") {
  std::vector<P2> corridor;
  for (double x = -6.0; x <= 6.0; x += 0.1) {
    corridor.push_back(P2(x, 1.0));
    corridor.push_back(P2(x, -1.0));
  }
  const rc::field::Tf2 truth = pose(0.5, 0.05, 0.02);
  const std::vector<P2> source = seen_after(corridor, truth, 0.005, 7);
  // The odometry says 0.4 m along the corridor; the truth is 0.5.
  const IcpResult r = icp(source, corridor, pose(0.4, 0, 0), 60, 0.4, 1e-7);
  rec("corridor_x_err", std::fabs(r.move.x - truth.x), 2);
  RC_CHECK(std::fabs(r.move.x - truth.x) > 0.08);
  RC_CHECK_NEAR(r.move.y, truth.y, 0.02);
  RC_CHECK_NEAR(r.move.theta, truth.theta, 0.01);
  // A partition across half the corridor.
  std::vector<P2> with_post = corridor;
  for (int k = 0; k < 400; ++k) with_post.push_back(P2(1.0, -1.0 + 0.0025 * k));  // a partition across half the corridor, scanned closely
  const std::vector<P2> source2 = seen_after(with_post, truth, 0.005, 8);
  const IcpResult r2 = icp(source2, with_post, pose(0.4, 0, 0), 300, 0.4, 1e-8);
  rec("post_x_err", std::fabs(r2.move.x - truth.x), 3);
  RC_CHECK(std::fabs(r2.move.x - truth.x) < 0.03);
}

RC_TEST("a person in one scan and not the other pulls the match unless pairs that are too far apart are ignored") {
  const std::vector<P2> target = room();
  const rc::field::Tf2 truth = pose(0.20, 0.0, 0.0);
  std::vector<P2> source = seen_after(target, truth, 0.005, 9);
  for (int k = 0; k < 40; ++k) source.push_back(P2(3.0 + 0.01 * k, 1.0 + 0.3 * std::sin(k * 0.3)));  // a person who is in the source only
  const IcpResult ungated = icp(source, target, pose(0, 0, 0), 80, 20.0, 1e-7);
  const IcpResult gated = icp(source, target, pose(0, 0, 0), 80, 0.3, 1e-7);
  const double pull_ungated = std::hypot(ungated.move.x - truth.x, ungated.move.y - truth.y);
  const double pull_gated = std::hypot(gated.move.x - truth.x, gated.move.y - truth.y);
  rec("pull_ungated", pull_ungated, 3);
  rec("pull_gated", pull_gated, 3);
  RC_CHECK(pull_ungated > 2.0 * pull_gated);
  RC_CHECK(pull_gated < 0.05);
}

RC_TEST("a first guess from the odometry saves rounds") {
  const std::vector<P2> target = room();
  const rc::field::Tf2 truth = pose(0.30, 0.10, 0.15);
  const std::vector<P2> source = seen_after(target, truth, 0.005, 11);
  const IcpResult blind = icp(source, target, pose(0, 0, 0), 80, 1.0, 1e-6);
  const IcpResult odom = icp(source, target, pose(0.28, 0.12, 0.14), 80, 1.0, 1e-6);
  rec("iters_blind", blind.rounds, 0);
  rec("iters_odom", odom.rounds, 0);
  RC_CHECK(odom.rounds < blind.rounds);
  RC_CHECK(icp(std::vector<P2>(), target, pose(0, 0, 0), 10, 1.0, 1e-6).rounds == 0);
}
