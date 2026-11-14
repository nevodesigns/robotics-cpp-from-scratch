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
    {"wall_cells", 81.0},
    {"post_cells", 32.0},
    {"c_inscribed", 253.0},
    {"c_03", 216.0},
    {"c_05", 119.0},
    {"c_10", 30.0},
    {"free_wide", 2.0},
    {"free_narrow", 0.0},
    {"ghost_marking", 7.0},
    {"ghost_clearing", 0.0},
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

GridSpec spec_10m() {
  GridSpec s;
  s.origin_x = -1.0;
  s.origin_y = -5.0;
  s.resolution = 0.05;
  s.width = 240;
  s.height = 200;
  return s;
}

// The cost at a world point of a master with this world inflated.
unsigned char cost_at(const CostGrid& g, double x, double y) { return g.at(g.cell_i(x), g.cell_j(y)); }

}  // namespace

RC_TEST("the grid maps a world position to a cell and back, and is unknown outside itself") {
  CostGrid g(spec_10m());
  RC_CHECK(g.cell_i(-1.0) == 0 && g.cell_i(-0.96) == 0 && g.cell_i(-0.94) == 1);
  RC_CHECK(g.cell_j(0.0) == 100);
  RC_CHECK_NEAR(g.centre_x(0), -0.975, 1e-12);
  RC_CHECK_NEAR(g.centre_x(g.cell_i(2.37)), 2.375, 1e-9);
  RC_CHECK(g.at(-1, 0) == kCostUnknown && g.at(0, 200) == kCostUnknown);
  g.set(5, 5, kCostLethal);
  RC_CHECK(g.at(5, 5) == kCostLethal && g.count(kCostLethal) == 1);
  g.set(-5, 0, kCostLethal);
  RC_CHECK(g.count(kCostLethal) == 1);
}

RC_TEST("a wall and a post become lethal cells, as many as their size says") {
  rc::field::World2 w;
  w.walls.push_back(rc::field::Segment{2.0, -2.0, 6.0, -2.0});
  w.posts.push_back(rc::field::Circle{4.0, 2.0, 0.15});
  CostGrid wall_only(spec_10m());
  rc::field::World2 only_wall;
  only_wall.walls = w.walls;
  rasterise_world(wall_only, only_wall);
  CostGrid post_only(spec_10m());
  rc::field::World2 only_post;
  only_post.posts = w.posts;
  rasterise_world(post_only, only_post);
  const double wall_cells = static_cast<double>(wall_only.count(kCostLethal));
  const double post_cells = static_cast<double>(post_only.count(kCostLethal));
  rec("wall_cells", wall_cells, 0);
  rec("post_cells", post_cells, 0);
  RC_CHECK_NEAR(wall_cells, 4.0 / 0.05 + 1.0, 3.0);
  RC_CHECK_NEAR(post_cells, 3.14159 * 0.15 * 0.15 / (0.05 * 0.05), 6.0);
}

RC_TEST("the inflation is the footprint of the robot: inscribed inside its radius and a decay beyond it") {
  CostGrid g(spec_10m());
  rc::field::World2 w;
  w.posts.push_back(rc::field::Circle{4.0, 0.0, 0.04});
  rasterise_world(g, w);
  inflate(g, 0.25, 1.0, 3.0);
  const unsigned char ins = cost_at(g, 4.0 + 0.20, 0.0);
  const unsigned char c03 = cost_at(g, 4.325, 0.025);  // the centre of a cell 0.30 m from the nearest lethal one
  const unsigned char c05 = cost_at(g, 4.0 + 0.50, 0.0);
  const unsigned char c10 = cost_at(g, 4.0 + 0.975, 0.0);
  rec("c_inscribed", ins, 0);
  rec("c_03", c03, 0);
  rec("c_05", c05, 0);
  rec("c_10", c10, 0);
  RC_CHECK(ins == kCostInscribed);
  RC_CHECK_NEAR(static_cast<double>(c03), 252.0 * std::exp(-3.0 * (0.30 - 0.25)), 18.0);
  RC_CHECK_NEAR(static_cast<double>(c05), 252.0 * std::exp(-3.0 * (0.50 - 0.25)), 12.0);
  RC_CHECK(c10 < c05 && c05 < c03 && c03 < ins);
  RC_CHECK(cost_at(g, 4.0 + 1.3, 0.0) == kCostFree);
  // The same on the other side and above: the inflation is round.
  RC_CHECK(cost_at(g, 4.0, 0.5) == c05 || std::abs(static_cast<int>(cost_at(g, 4.0, 0.5)) - static_cast<int>(c05)) <= 6);
}

RC_TEST("a gap between two posts is open to the centre of the robot only if it is wider than twice the radius, and the inflation shows it") {
  auto free_across = [](double gap) {
    CostGrid g(spec_10m());
    rc::field::World2 w;
    w.posts.push_back(rc::field::Circle{4.0, -gap / 2.0 - 0.04, 0.04});
    w.posts.push_back(rc::field::Circle{4.0, gap / 2.0 + 0.04, 0.04});
    rasterise_world(g, w);
    inflate(g, 0.25, 0.75, 3.0);
    int free_cells = 0;
    for (int j = g.cell_j(-gap); j <= g.cell_j(gap); ++j)
      if (g.at(g.cell_i(4.0), j) < kCostInscribed) ++free_cells;
    return free_cells;
  };
  const int wide = free_across(0.60);
  const int narrow = free_across(0.40);
  rec("free_wide", wide, 0);
  rec("free_narrow", narrow, 0);
  RC_CHECK(wide >= 2);
  RC_CHECK(narrow == 0);
}

RC_TEST("an obstacle that has gone stays in a layer that only marks, and is forgotten by one that clears") {
  auto ghost = [](bool clear) {
    CostGrid layer(spec_10m());
    std::vector<rc::field::Point3> seen;
    for (int k = -3; k <= 3; ++k) seen.push_back(rc::field::Point3{5.025, 0.05 * k + 0.025, 0.0});
    mark_and_clear(layer, 0.0, 0.0, seen, clear);
    const std::size_t before = layer.count(kCostLethal);
    // Now the obstacle has gone and the sensor sees a wall beyond it.
    std::vector<rc::field::Point3> beyond;
    for (int k = -3; k <= 3; ++k) beyond.push_back(rc::field::Point3{8.025, 0.08 * k + 0.025, 0.0});
    mark_and_clear(layer, 0.0, 0.0, beyond, clear);
    const std::size_t ghost_cells = layer.count(kCostLethal) - 7;  // the seven of the wall beyond
    return std::pair<std::size_t, std::size_t>(before, ghost_cells);
  };
  const std::pair<std::size_t, std::size_t> marking = ghost(false);
  const std::pair<std::size_t, std::size_t> clearing = ghost(true);
  rec("ghost_marking", static_cast<double>(marking.second), 0);
  rec("ghost_clearing", static_cast<double>(clearing.second), 0);
  RC_CHECK(marking.first == 7);
  RC_CHECK(marking.second == 7);
  RC_CHECK(clearing.second == 0);
}
