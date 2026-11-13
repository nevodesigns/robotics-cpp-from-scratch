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


RC_TEST("a straight line across axes of different strength is slower than letting each go on its own") {
  Gantry g;
  const double d[3] = {0.4, 0.0, 0.4};
  const double t_axes = axes_time(g, d);
  const double t_line = straight_line_time(g, d);
  rec("dx", d[0], 1);
  rec("dz", d[2], 1);
  rec("t_axes", t_axes, 3);
  rec("t_line", t_line, 3);
  RC_CHECK(t_line > t_axes);
  // when one axis sets both the speed and the acceleration the line costs nothing extra
  const double lopsided[3] = {0.6, 0.0, 0.1};
  RC_CHECK_NEAR(straight_line_time(g, lopsided), axes_time(g, lopsided), 1e-9);
  const double along_x[3] = {0.5, 0.0, 0.0};
  RC_CHECK_NEAR(straight_line_time(g, along_x), axes_time(g, along_x), 1e-12);
  const double zero[3] = {0.0, 0.0, 0.0};
  RC_CHECK_NEAR(straight_line_time(g, zero), 0.0, 1e-12);
}

RC_TEST("with equal axes a diagonal takes the time of one axis") {
  Gantry g;
  g.v[2] = g.v[0];
  g.a[2] = g.a[0];
  const double d[3] = {0.5, 0.5, 0.0};
  RC_CHECK_NEAR(straight_line_time(g, d), axes_time(g, d), 1e-9);
}

RC_TEST("a beam sags with the load and the cube of its length, and its resonance falls with the mass") {
  const double e = 69e9, i = 3.0e-7;   // aluminium and an extruded 40 by 80 section (typical)
  const double span = 1.0, load = 50.0, mass = 5.0;
  const double sag = beam_sag(load, span, e * i);
  const double k = beam_stiffness(span, e * i);
  const double f = first_mode_hz(k, mass);
  rec("span", span, 1);
  rec("load", load, 0);
  rec("sag_um", sag * 1e6, 0);
  rec("k_mn", k / 1e6, 1);
  rec("mass", mass, 0);
  rec("f_hz", f, 0);
  rec("sag_ratio", beam_sag(load, 2.0 * span, e * i) / sag, 0);
  RC_CHECK_NEAR(sag * k, load, 1e-9);
  RC_CHECK_NEAR(beam_sag(load, 2.0 * span, e * i) / sag, 8.0, 1e-9);
  RC_CHECK(first_mode_hz(k, 2.0 * mass) < f);
  RC_CHECK(f > 30.0 && f < 300.0);
}

RC_TEST("a beam driven from two ends turns when the ends disagree, and the error grows with the lever") {
  const double skew = 0.0002, span = 1.0, lever = 0.3;
  rec("skew_mm", skew * 1000.0, 1);
  rec("lever", lever, 1);
  rec("rack_um", rack_error(skew, span, lever) * 1e6, 0);
  RC_CHECK_NEAR(rack_error(skew, span, lever), 6e-5, 1e-12);
  RC_CHECK(rack_error(skew, span, 0.6) > rack_error(skew, span, lever));
}
