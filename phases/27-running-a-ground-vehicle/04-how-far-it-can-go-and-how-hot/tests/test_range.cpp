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

rc::field::Cell of(const rc::field::Surface& s) {
  rc::field::Cell c;
  c.surface = s;
  return c;
}

TerrainGrid sand_band() {
  TerrainGrid t(field());
  t.paint(0.0, 0.0, 20.0, 20.0, of(rc::field::tarmac()));
  t.paint(6.0, 0.0, 14.0, 20.0, of(rc::field::sand()));
  return t;
}

std::vector<CarPose> along_y(double x0, double x1, double y) {
  std::vector<CarPose> v;
  const int n = static_cast<int>(std::lround(std::fabs(x1 - x0) / 0.25));
  const double step = x1 >= x0 ? 0.25 : -0.25;
  for (int i = 0; i <= n; ++i) {
    CarPose p;
    p.x = x0 + i * step;
    p.y = y;
    v.push_back(p);
  }
  return v;
}

}  // namespace

RC_TEST("a metre costs the work, over the efficiency, plus the computer that never sleeps") {
  const TerrainGrid t = sand_band();
  DrivePower d;
  const double tarmac = route_wh(t, along_y(1.0, 5.0, 10.0), d) * 3600.0 / 4.0;
  const double sand = route_wh(t, along_y(7.0, 11.0, 10.0), d) * 3600.0 / 4.0;
  rec("j_tarmac", tarmac, 1);
  rec("j_sand", sand, 1);
  rec("idle_share", 100.0 * d.idle_w / tarmac, 0);
  RC_CHECK_NEAR(tarmac, 1.47 / 0.7 + 8.0, 0.1);
  RC_CHECK(sand > 2.0 * (tarmac - 8.0) + 8.0);
  RC_CHECK(d.idle_w / tarmac > 0.7);
}

RC_TEST("the way home estimated as flat strands a vehicle that the way home planned over the ground does not") {
  const TerrainGrid t = sand_band();
  DrivePower d;
  const double flat_wh_per_m = (1.47 / 0.7 + 8.0) / 3600.0;
  const double start_wh = 0.14, reserve_wh = 75.0 / 3600.0;
  const double charger = 2.0;
  OffroadWeights w;
  w.mass = d.mass;
  w.energy = 1.0;
  double x_flat = 0.0, short_flat = 0.0, x_terrain = 0.0, left_terrain = 0.0;
  for (int method = 0; method < 2; ++method) {
    double x = charger, spent = 0.0;
    std::vector<CarPose> out;
    CarPose p;
    p.x = charger;
    p.y = 10.0;
    out.push_back(p);
    for (; x < 19.5; x += 0.25) {
      CarPose q;
      q.x = x + 0.25;
      q.y = 10.0;
      std::vector<CarPose> step(2);
      step[0] = out.back();
      step[1] = q;
      spent += route_wh(t, step, d);
      out.push_back(q);
      double home_wh;
      if (method == 0) {
        home_wh = (q.x - charger) * flat_wh_per_m;
      } else {
        const OffroadPath back = plan_offroad(t, nullptr, q.x, 10.0, charger, 10.0, w);
        home_wh = route_wh(t, back.points, d);
      }
      if (must_turn_back(start_wh - spent, home_wh, reserve_wh)) {
        x = q.x;
        break;
      }
    }
    const OffroadPath back = plan_offroad(t, nullptr, x, 10.0, charger, 10.0, w);
    const double true_home = route_wh(t, back.points, d);
    const double left = (start_wh - spent) - true_home;
    if (method == 0) {
      x_flat = x - charger;
      short_flat = -left * 3600.0;
    } else {
      x_terrain = x - charger;
      left_terrain = left * 3600.0;
    }
  }
  rec("x_flat", x_flat, 1);
  rec("short_flat", short_flat, 0);
  rec("x_terrain", x_terrain, 1);
  rec("left_terrain", left_terrain, 0);
  RC_CHECK(x_flat > x_terrain);
  RC_CHECK(short_flat > 0.0);
  RC_CHECK(left_terrain > 0.0);
}

RC_TEST("a slope that a motor can hold for ever, and one that heats it to its limit in a minute") {
  MotorThermal m;
  const double r = 0.05, mass = 10.0, crr = 0.04, eff = 0.8;
  const double f14 = slope_force(mass, 14.0 * kPi / 180.0, crr);
  const double f20 = slope_force(mass, 20.0 * kPi / 180.0, crr);
  const double i14 = motor_current(f14, r, 4, eff, m.kt_out);
  const double i20 = motor_current(f20, r, 4, eff, m.kt_out);
  rec("rise14", steady_rise(i14, m), 0);
  const double t20 = time_to_limit(i20, m);
  rec("t20", t20, 0);
  rec("d20", 0.5 * t20, 0);
  RC_CHECK(steady_rise(i14, m) < m.rise_limit);
  RC_CHECK(time_to_limit(i14, m) > 1e17);
  RC_CHECK(t20 > 30.0 && t20 < 120.0);
  // Step the lump: dT/dt = (P - T / Rth) / Cth, and see where it reaches the limit.
  const double P = i20 * i20 * m.resistance;
  double T = 0.0, t = 0.0;
  const double dt = 0.001;
  while (T < m.rise_limit && t < 1000.0) {
    T += (P - T / m.r_th) / m.c_th * dt;
    t += dt;
  }
  RC_CHECK_NEAR(t, t20, 0.5);
}

RC_TEST("flat ground never heats a motor to its limit, and a slope is more current than flat") {
  MotorThermal m;
  const double flat = motor_current(slope_force(10.0, 0.0, 0.04), 0.05, 4, 0.8, m.kt_out);
  const double up = motor_current(slope_force(10.0, 0.3, 0.04), 0.05, 4, 0.8, m.kt_out);
  RC_CHECK(time_to_limit(flat, m) > 1e17);
  RC_CHECK(up > 5.0 * flat);
}

RC_TEST("a vehicle with just enough turns back, and one with a little less already must") {
  RC_CHECK(!must_turn_back(1.0, 0.5, 0.2));
  RC_CHECK(must_turn_back(0.6, 0.5, 0.2));
}
