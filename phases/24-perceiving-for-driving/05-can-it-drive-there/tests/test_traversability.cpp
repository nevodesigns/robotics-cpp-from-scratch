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

const double kPi = 3.14159265358979323846;

GridSpec spec_grid() {
  GridSpec s;
  s.origin_x = 0.0;
  s.origin_y = 0.0;
  s.resolution = 0.1;
  s.width = 60;
  s.height = 30;
  return s;
}

// A flat floor with a ramp of 10 degrees from x = 1 to 2, a slope of 30 degrees from x = 3 to 4, a step of 5 cm at x = 4.5 and a rough patch (noise of 3 cm) from x = 5.
HeightGrid terrain(unsigned int seed) {
  HeightGrid g(spec_grid());
  rc::core::Rng rng(seed);
  for (int j = 0; j < 30; ++j)
    for (int i = 0; i < 60; ++i) {
      const double x = 0.1 * i + 0.05;
      double z = 0.0;
      if (x > 1.0 && x <= 2.0) z = std::tan(10.0 * kPi / 180.0) * (x - 1.0);
      else if (x > 2.0 && x <= 3.0) z = std::tan(10.0 * kPi / 180.0);
      else if (x > 3.0 && x <= 4.0) z = std::tan(10.0 * kPi / 180.0) + std::tan(30.0 * kPi / 180.0) * (x - 3.0);
      else if (x > 4.0) z = std::tan(10.0 * kPi / 180.0) + std::tan(30.0 * kPi / 180.0);
      if (x > 4.5) z += 0.05;
      if (x > 5.0) z += 0.03 * rng.gaussian();
      g.set(i, j, z);
    }
  return g;
}

}  // namespace

RC_TEST("the slope of a ramp is read from the plane of its neighbourhood, and does not depend on its direction") {
  const HeightGrid g = terrain(1);
  const SlopeRoughness on_ramp = slope_and_roughness(g, 15, 15);  // x = 1.55
  rec("ramp_read", on_ramp.slope * 180.0 / kPi, 1);
  RC_CHECK(on_ramp.known);
  RC_CHECK_NEAR(on_ramp.slope * 180.0 / kPi, 10.0, 0.3);
  RC_CHECK_NEAR(on_ramp.roughness, 0.0, 1e-9);
  RC_CHECK_NEAR(slope_and_roughness(g, 5, 15).slope, 0.0, 1e-9);
  RC_CHECK_NEAR(slope_and_roughness(g, 35, 15).slope * 180.0 / kPi, 30.0, 0.5);
  // The same ramp turned to run along y.
  HeightGrid turned(spec_grid());
  for (int j = 0; j < 30; ++j)
    for (int i = 0; i < 60; ++i) turned.set(i, j, std::tan(10.0 * kPi / 180.0) * 0.1 * j);
  RC_CHECK_NEAR(slope_and_roughness(turned, 20, 15).slope * 180.0 / kPi, 10.0, 0.05);
}

RC_TEST("noise on flat ground is roughness, and the step shows at its edge") {
  const HeightGrid g = terrain(2);
  double sum = 0.0;
  int n = 0;
  for (int j = 2; j < 28; ++j)
    for (int i = 53; i < 59; ++i) {
      sum += slope_and_roughness(g, i, j).roughness;
      ++n;
    }
  const double mean = sum / n;
  rec("rough_read", mean, 3);
  RC_CHECK_NEAR(mean, 0.03, 0.006);
  const SlopeRoughness edge = slope_and_roughness(g, 45, 15);   // across x = 4.5
  rec("step_slope", edge.slope * 180.0 / kPi, 1);
  rec("step_rough", edge.roughness, 4);
  rec("step_grad", edge.max_gradient * 180.0 / kPi, 1);
  RC_CHECK(edge.roughness > 0.01 && edge.roughness < 0.025);
  RC_CHECK_NEAR(edge.max_gradient * 180.0 / kPi, std::atan(0.5) * 180.0 / kPi, 0.01);
  RC_CHECK(slope_and_roughness(g, 47, 15).roughness < 1e-9);
  // Cells at the edge of the grid have no 3 by 3 neighbourhood.
  RC_CHECK(!slope_and_roughness(g, 0, 15).known);
}

RC_TEST("what the vehicle can climb is the smallest of its motors, its tyres and its balance") {
  rc::vehicle::DriveTrain d;
  const VehicleLimits gravel = vehicle_limits(d, rc::field::gravel(), 0.25, 0.10);
  const VehicleLimits tarmac = vehicle_limits(d, rc::field::tarmac(), 0.25, 0.10);
  const double motors = rc::vehicle::max_climb(d, 0.04) * 180.0 / kPi;
  const double grip = rc::field::max_climb_angle(rc::field::gravel()) * 180.0 / kPi;
  rec("motor_limit", motors, 1);
  rec("grip_limit", grip, 1);
  rec("slope_limit", gravel.max_slope * 180.0 / kPi, 1);
  rec("rough_limit", gravel.max_roughness, 3);
  RC_CHECK_NEAR(gravel.max_slope * 180.0 / kPi, grip, 0.01);
  RC_CHECK(tarmac.max_slope > gravel.max_slope);
  RC_CHECK_NEAR(tarmac.max_slope * 180.0 / kPi, rc::vehicle::max_climb(d, 0.015) * 180.0 / kPi, 0.01);
  RC_CHECK_NEAR(gravel.max_roughness, 0.025, 1e-12);
  // A tall vehicle tips before it slips.
  const VehicleLimits tall = vehicle_limits(d, rc::field::tarmac(), 0.25, 0.40);
  RC_CHECK_NEAR(tall.max_slope, std::atan(0.125 / 0.40), 1e-12);
}

RC_TEST("the layer is free on smooth ground, rises with the slope, and is lethal on the steep slope and at the step") {
  rc::vehicle::DriveTrain d;
  const VehicleLimits lim = vehicle_limits(d, rc::field::gravel(), 0.25, 0.10);
  const HeightGrid g = terrain(3);
  const CostGrid layer = traversability_layer(g, lim);
  RC_CHECK(layer.at(5, 15) == 0);                         // flat
  RC_CHECK(layer.at(15, 15) > 0 && layer.at(15, 15) < 100);  // the ramp of 10 degrees
  RC_CHECK(layer.at(35, 15) == kCostLethal);              // 30 degrees
  RC_CHECK(layer.at(45, 15) == kCostLethal && layer.at(44, 15) == kCostLethal);  // the step, on both sides of the edge
  RC_CHECK(layer.at(0, 0) == kCostUnknown);
  std::size_t lethal_step = 0;
  for (int i = 40; i < 50; ++i)
    for (int j = 2; j < 28; ++j)
      if (layer.at(i, j) == kCostLethal) ++lethal_step;
  rec("step_cells", static_cast<double>(lethal_step) / 26.0, 0);
  RC_CHECK(lethal_step > 0);
}

RC_TEST("the same slope is passable on tarmac and not on gravel, because the grip is not the same") {
  rc::vehicle::DriveTrain d;
  HeightGrid ramp(spec_grid());
  for (int j = 0; j < 30; ++j)
    for (int i = 0; i < 60; ++i) ramp.set(i, j, std::tan(24.0 * kPi / 180.0) * 0.1 * i);
  const CostGrid on_tarmac = traversability_layer(ramp, vehicle_limits(d, rc::field::tarmac(), 0.25, 0.10));
  const CostGrid on_gravel = traversability_layer(ramp, vehicle_limits(d, rc::field::gravel(), 0.25, 0.10));
  rec("cost_motors", on_tarmac.at(30, 15), 0);
  rec("cost_gravel", on_gravel.at(30, 15), 0);
  std::cout << "  a slope of 24 degrees: cost " << static_cast<int>(on_tarmac.at(30, 15)) << " on tarmac and " << static_cast<int>(on_gravel.at(30, 15)) << " on gravel\n";
  RC_CHECK(on_tarmac.at(30, 15) < kCostInscribed);
  RC_CHECK(on_gravel.at(30, 15) == kCostLethal);
}
