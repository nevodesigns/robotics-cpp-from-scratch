#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

rc::field::World2 room() {
  rc::field::World2 w;
  w.walls.push_back(rc::field::Segment{0, 0, 10, 0});
  w.walls.push_back(rc::field::Segment{10, 0, 10, 6});
  w.walls.push_back(rc::field::Segment{10, 6, 0, 6});
  w.walls.push_back(rc::field::Segment{0, 6, 0, 0});
  w.walls.push_back(rc::field::Segment{4.0, 0.0, 4.0, 2.0});
  w.posts.push_back(rc::field::Circle{7.5, 4.5, 0.3});
  return w;
}

std::vector<double> beam_angles() {
  std::vector<double> a;
  for (int i = 0; i < 24; ++i) a.push_back(-kPi + (i + 0.5) * 2.0 * kPi / 24.0);
  return a;
}

struct Outcome {
  double err_before = 0.0;
  double err_after = 0.0;
  int recover_steps = -1;
  int flag_steps = -1;
  double injected = 0.0;
};

// The robot starts at (2, 2) and drives for 40 steps; the cloud is started on it, a little spread. At step 41 it is picked up and put down at (7.6, 3.0) facing 2 rad, and drives on for 40 steps more.
Outcome run(bool recover, unsigned int seed) {
  rc::core::Rng noise(seed * 17u + 3u);
  rc::core::Rng scatter_rng(seed * 19u + 5u);
  const rc::field::World2 map = room();
  MonteCarlo mc(map, 1500, seed);
  {
    std::vector<Particle>& ps = mc.mutable_particles();
    for (std::size_t i = 0; i < ps.size(); ++i) {
      ps[i].x = 2.0 + 0.3 * noise.gaussian();
      ps[i].y = 2.0 + 0.3 * noise.gaussian();
      ps[i].theta = 0.5 + 0.2 * noise.gaussian();
      ps[i].w = 1.0 / static_cast<double>(ps.size());
    }
  }
  RecoveryMonitor monitor((RecoverySettings()));
  const std::vector<double> angles = beam_angles();
  double tx = 2.0, ty = 2.0, tth = 0.5;
  Outcome o;
  double injected = 0.0;
  int inject_steps = 0;
  for (int step = 1; step <= 80; ++step) {
    if (step == 41) {
      tx = 7.6;
      ty = 3.0;
      tth = 2.0;
    }
    const double turn = (step % 7 == 0) ? 0.35 : 0.0;
    double extra = 0.0;
    const double nx = tx + 0.2 * std::cos(tth), ny = ty + 0.2 * std::sin(tth);
    if (nx < 0.6 || nx > 9.4 || ny < 0.6 || ny > 5.4) extra = 0.5;
    const double t = turn + extra;
    // The robot moves; at the kidnap the odometry shows nothing of the carry.
    tx += 0.2 * std::cos(tth + 0.5 * t);
    ty += 0.2 * std::sin(tth + 0.5 * t);
    tth += t;
    mc.predict(0.2, t, 0.02, 0.02);
    std::vector<double> ranges;
    for (std::size_t b = 0; b < angles.size(); ++b) ranges.push_back(rc::field::cast_ray(map, tx, ty, tth + angles[b], 12.0) + 0.05 * noise.gaussian());
    const double avg = mc.correct(angles, ranges, McMeasure());
    const double share = monitor.update(avg);
    mc.resample_if_needed();
    if (recover && share > 0.0) scatter(mc, share, 0.0, 10.0, 0.0, 6.0, scatter_rng);
    if (recover && step <= 40) {
      injected += share;
      ++inject_steps;
    }
    const Particle e = mc.mean_pose();
    const double err = std::hypot(e.x - tx, e.y - ty);
    if (step == 40) o.err_before = err;
    if (step > 41 && o.flag_steps < 0 && verdict(monitor.fast(), monitor.slow(), mc.spread()) == Health::lost) o.flag_steps = step - 41;
    if (step > 41 && o.recover_steps < 0 && err < 0.3 && mc.spread() < 0.5) o.recover_steps = step - 41;
    if (step == 80) o.err_after = err;
  }
  o.injected = inject_steps > 0 ? injected / inject_steps : 0.0;
  return o;
}

}  // namespace

RC_TEST("the monitor asks for nothing while the scans are explained and for more as the fast average falls below the slow") {
  RecoverySettings open;
  open.max_share = 1.0;
  RecoveryMonitor m(open);
  for (int i = 0; i < 100; ++i) RC_CHECK(m.update(0.5) == 0.0);
  const double a = m.update(0.1);
  const double b = m.update(0.1);
  RC_CHECK(a > 0.0 && b > a);
  // The default caps the share.
  RecoveryMonitor capped((RecoverySettings()));
  for (int i = 0; i < 100; ++i) capped.update(0.5);
  RC_CHECK_NEAR(capped.update(0.0), RecoverySettings().max_share, 1e-12);
  RecoveryMonitor n((RecoverySettings()));
  n.update(0.4);
  RC_CHECK(n.update(0.8) == 0.0);  // an improvement is not a loss
}

RC_TEST("the verdict is good when the scans are explained as usual, doubtful when less so, and lost when hardly at all") {
  RC_CHECK(verdict(0.5, 0.5, 0.2) == Health::good);
  RC_CHECK(verdict(0.35, 0.5, 0.2) == Health::doubtful);
  RC_CHECK(verdict(0.5, 0.5, 2.0) == Health::doubtful);
  RC_CHECK(verdict(0.1, 0.5, 0.2) == Health::lost);
  RC_CHECK(verdict(0.1, 0.0, 0.2) == Health::lost);
}

RC_TEST("scattering replaces about the fraction asked for, with equal weights that sum to one") {
  const rc::field::World2 map = room();
  MonteCarlo mc(map, 2000, 1);
  std::vector<Particle>& ps = mc.mutable_particles();
  for (std::size_t i = 0; i < ps.size(); ++i) {
    ps[i].x = 2.0;
    ps[i].y = 2.0;
    ps[i].theta = 0.0;
    ps[i].w = 1.0 / 2000.0;
  }
  rc::core::Rng rng(4);
  scatter(mc, 0.2, 0.0, 10.0, 0.0, 6.0, rng);
  int moved = 0;
  double total = 0.0;
  for (std::size_t i = 0; i < mc.particles().size(); ++i) {
    if (mc.particles()[i].x != 2.0) ++moved;
    total += mc.particles()[i].w;
  }
  RC_CHECK(moved > 330 && moved < 470);
  RC_CHECK_NEAR(total, 1.0, 1e-9);
}

RC_TEST("a robot put down elsewhere is lost for good by a plain cloud and found again by one with a monitor") {
  Outcome plain_sum, with_sum;
  double flag = 0.0, rec_steps = 0.0;
  int flagged = 0, recovered = 0;
  const int seeds = 6;
  double plain_err = 0.0, false_inject = 0.0, err_before = 0.0, err_before_plain = 0.0;
  for (unsigned int seed = 1; seed <= static_cast<unsigned int>(seeds); ++seed) {
    const Outcome p = run(false, seed);
    const Outcome w = run(true, seed);
    plain_err += p.err_after;
    false_inject += w.injected;
    err_before += w.err_before;
    err_before_plain += p.err_before;
    if (w.flag_steps >= 0) {
      flag += w.flag_steps;
      ++flagged;
    }
    if (w.recover_steps >= 0) {
      rec_steps += w.recover_steps;
      ++recovered;
    }
    (void)plain_sum;
    (void)with_sum;
  }
  rec("plain_err", plain_err / seeds, 1);
  rec("recover_steps", recovered > 0 ? rec_steps / recovered : -1.0, 0);
  rec("flag_steps", flagged > 0 ? flag / flagged : -1.0, 0);
  rec("false_inject", 100.0 * false_inject / seeds, 1);
  rec("err_before", err_before / seeds, 3);
  rec("err_before_plain", err_before_plain / seeds, 3);
  rec("max_share", 100.0 * RecoverySettings().max_share, 0);
  std::cout << "  after the carry: plain cloud error " << plain_err / seeds << " m; with the monitor, recovered in " << recovered << " of " << seeds << " runs, " << (recovered > 0 ? rec_steps / recovered : -1.0) << " steps on average, flagged lost after " << (flagged > 0 ? flag / flagged : -1.0) << "\n";
  RC_CHECK(plain_err / seeds > 2.0);
  RC_CHECK(recovered >= seeds - 2);
  RC_CHECK(flagged >= seeds - 2);
  RC_CHECK(false_inject / seeds < 0.03);
  RC_CHECK(err_before / seeds < 0.3);
}
