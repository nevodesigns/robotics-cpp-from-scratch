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

rc::perceive::GridSpec field() {
  rc::perceive::GridSpec s;
  s.origin_x = 0.0;
  s.origin_y = 0.0;
  s.resolution = 0.25;
  s.width = 80;
  s.height = 80;
  return s;
}

rc::field::Cell of(const rc::field::Surface& s, double gx = 0.0, double gy = 0.0) {
  rc::field::Cell c;
  c.surface = s;
  c.grade_x = gx;
  c.grade_y = gy;
  return c;
}

// 20 m by 20 m of tarmac, with a band of sand from x = 6 to x = 14 all the way across except a gravel ford at y = 14 to 16.
TerrainGrid sand_band() {
  TerrainGrid t(field());
  t.paint(0.0, 0.0, 20.0, 20.0, of(rc::field::tarmac()));
  t.paint(6.0, 0.0, 14.0, 20.0, of(rc::field::sand()));
  t.paint(6.0, 14.0, 14.0, 16.0, of(rc::field::gravel()));
  return t;
}

}  // namespace

RC_TEST("a metre costs more over sand than tarmac, and more up a slope than across it") {
  const double sand = energy_per_metre(of(rc::field::sand()), 0.0, 10.0);
  const double tarmac = energy_per_metre(of(rc::field::tarmac()), 0.0, 10.0);
  // A slope of 14 degrees rising towards +x: grade_x is the rise over the run.
  const double grade = std::tan(14.0 * kPi / 180.0);
  const double up = energy_per_metre(of(rc::field::tarmac(), grade, 0.0), 0.0, 10.0);
  const double across = energy_per_metre(of(rc::field::tarmac(), grade, 0.0), kPi / 2.0, 10.0);
  const double down = energy_per_metre(of(rc::field::tarmac(), grade, 0.0), kPi, 10.0);
  rec("e_sand", sand, 1);
  rec("e_tarmac", tarmac, 2);
  rec("e_up", up, 1);
  rec("e_across", across, 2);
  RC_CHECK_NEAR(sand / tarmac, 0.100 / 0.015, 1e-9);
  RC_CHECK(up > 10.0 * across);
  RC_CHECK(down >= 0.0);
  RC_CHECK(down < across);
}

RC_TEST("with no weight on energy the shortest path goes through the sand, with weight it takes the ford") {
  const TerrainGrid t = sand_band();
  OffroadWeights plain, cheap;
  cheap.energy = 1.0;
  const OffroadPath a = plan_offroad(t, nullptr, 2.0, 4.0, 18.0, 4.0, plain);
  const OffroadPath b = plan_offroad(t, nullptr, 2.0, 4.0, 18.0, 4.0, cheap);
  rec("len_short", a.length, 1);
  rec("len_cheap", b.length, 1);
  rec("en_short", path_energy(t, a.points, 10.0), 0);
  rec("en_cheap", path_energy(t, b.points, 10.0), 0);
  RC_CHECK(a.found && b.found);
  RC_CHECK(a.length < b.length);
  RC_CHECK(path_energy(t, b.points, 10.0) < path_energy(t, a.points, 10.0));
  bool through_ford = false;
  for (std::size_t i = 0; i < b.points.size(); ++i)
    if (b.points[i].x > 7.0 && b.points[i].x < 13.0 && b.points[i].y > 13.5 && b.points[i].y < 16.5) through_ford = true;
  RC_CHECK(through_ford);
}

RC_TEST("a patch of risk that is not blocked is crossed unless risk has a weight") {
  TerrainGrid t(field());
  t.paint(0.0, 0.0, 20.0, 20.0, of(rc::field::tarmac()));
  rc::perceive::CostGrid risk(field());
  // A patch of cost 200 (risky, not blocked) from x = 8 to 12, y = 2 to 18: leaving a way round at the top and bottom.
  for (int j = 0; j < 80; ++j)
    for (int i = 0; i < 80; ++i) {
      const double x = risk.centre_x(i), y = risk.centre_y(j);
      if (x >= 8.0 && x <= 12.0 && y >= 2.0 && y <= 18.0) risk.set(i, j, 200);
    }
  OffroadWeights blind, aware;
  aware.risk = 5.0;
  const OffroadPath a = plan_offroad(t, &risk, 2.0, 10.0, 18.0, 10.0, blind);
  const OffroadPath b = plan_offroad(t, &risk, 2.0, 10.0, 18.0, 10.0, aware);
  rec("risk_blind", path_max_risk(risk, a.points), 0);
  rec("risk_aware", path_max_risk(risk, b.points), 0);
  RC_CHECK(a.found && b.found);
  RC_CHECK(path_max_risk(risk, a.points) >= 200.0);
  RC_CHECK(path_max_risk(risk, b.points) < 200.0);
}

RC_TEST("blocked cells are never crossed, and a goal that cannot be reached is not found") {
  TerrainGrid t(field());
  rc::perceive::CostGrid risk(field());
  // A ring of blocked cells round the goal.
  for (int j = 0; j < 80; ++j)
    for (int i = 0; i < 80; ++i) {
      const double d = std::hypot(risk.centre_x(i) - 16.0, risk.centre_y(j) - 10.0);
      if (d >= 2.0 && d <= 2.6) risk.set(i, j, 254);
    }
  OffroadWeights w;
  const OffroadPath a = plan_offroad(t, &risk, 2.0, 10.0, 16.0, 10.0, w);
  rec("exp_none", a.expansions, 0);
  RC_CHECK(!a.found);
  RC_CHECK(a.points.empty());
  // The same grid with a gap in the ring: found, and no point of the path is in a blocked cell.
  for (int j = 0; j < 80; ++j)
    for (int i = 0; i < 80; ++i)
      if (std::fabs(risk.centre_y(j) - 10.0) < 0.6 && risk.centre_x(i) < 16.0) risk.set(i, j, 0);
  const OffroadPath b = plan_offroad(t, &risk, 2.0, 10.0, 16.0, 10.0, w);
  RC_CHECK(b.found);
  RC_CHECK(path_max_risk(risk, b.points) < 253.0);
}

RC_TEST("on flat tarmac with no weights the path is as long as the straight line, to the cell") {
  TerrainGrid t(field());
  OffroadWeights w;
  const OffroadPath a = plan_offroad(t, nullptr, 2.0, 2.0, 18.0, 2.0, w);
  RC_CHECK(a.found);
  RC_CHECK_NEAR(a.length, 16.0, 0.3);
}
