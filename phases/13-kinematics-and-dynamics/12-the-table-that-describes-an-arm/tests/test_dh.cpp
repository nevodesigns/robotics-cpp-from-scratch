#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {
const double kPi = 3.14159265358979323846;

double distance(const Frame& a, const Frame& b) { return std::sqrt((a.p.x - b.p.x) * (a.p.x - b.p.x) + (a.p.y - b.p.y) * (a.p.y - b.p.y) + (a.p.z - b.p.z) * (a.p.z - b.p.z)); }
}  // namespace

RC_TEST("a link along x turned by theta puts the tool where a link of that length turned by theta would, and composing is associative") {
  DhRow row;
  row.a = 2.0;
  const Frame f = dh_frame(row, 0.6, Convention::kStandard);
  RC_CHECK_NEAR(f.p.x, 2.0 * std::cos(0.6), 1e-12);
  RC_CHECK_NEAR(f.p.y, 2.0 * std::sin(0.6), 1e-12);
  RC_CHECK_NEAR(f.p.z, 0.0, 1e-12);

  // Two links in the plane: the tool of a table of alpha 0 and d 0 is the planar arm of lesson 13-01.
  std::vector<DhRow> planar(2);
  planar[0].a = 1.0;
  planar[1].a = 0.7;
  const double q1 = 0.4, q2 = -0.9;
  const Frame tool = dh_chain(planar, std::vector<double>{q1, q2}, Convention::kStandard);
  RC_CHECK_NEAR(tool.p.x, 1.0 * std::cos(q1) + 0.7 * std::cos(q1 + q2), 1e-12);
  RC_CHECK_NEAR(tool.p.y, 1.0 * std::sin(q1) + 0.7 * std::sin(q1 + q2), 1e-12);

  const Frame a = dh_frame(DhRow{0.3, 0.1, 0.5, 0.2, false}, 0.7, Convention::kStandard);
  const Frame b = dh_frame(DhRow{-0.2, 0.4, 0.3, -0.5, false}, -0.4, Convention::kStandard);
  const Frame c = dh_frame(DhRow{0.1, 0.0, 0.8, 1.1, false}, 1.3, Convention::kStandard);
  RC_CHECK(distance(compose(a, compose(b, c)), compose(compose(a, b), c)) < 1e-12);
}

RC_TEST("a row of the standard convention is Rz Tz Tx Rx, and its rotation and position are the closed form of that product") {
  DhRow row;
  row.theta = 0.0;
  row.d = 0.5;
  row.a = 0.4;
  row.alpha = kPi / 2.0;
  const double q = 0.3;
  const Frame f = dh_frame(row, q, Convention::kStandard);
  const double c = std::cos(q), s = std::sin(q);
  // R = Rz(q) Rx(alpha) with alpha = pi/2: [[c, 0, s], [s, 0, -c], [0, 1, 0]]; p = (a c, a s, d).
  RC_CHECK_NEAR(f.r.m[0][0], c, 1e-12);
  RC_CHECK_NEAR(f.r.m[0][1], 0.0, 1e-12);
  RC_CHECK_NEAR(f.r.m[0][2], s, 1e-12);
  RC_CHECK_NEAR(f.r.m[1][0], s, 1e-12);
  RC_CHECK_NEAR(f.r.m[1][2], -c, 1e-12);
  RC_CHECK_NEAR(f.r.m[2][1], 1.0, 1e-12);
  RC_CHECK_NEAR(f.r.m[2][2], 0.0, 1e-12);
  RC_CHECK_NEAR(f.p.x, 0.4 * c, 1e-12);
  RC_CHECK_NEAR(f.p.y, 0.4 * s, 1e-12);
  RC_CHECK_NEAR(f.p.z, 0.5, 1e-12);
  RC_CHECK(rc::math::is_orthonormal(f.r, 1e-12));
}

RC_TEST("a SCARA arm, two joints that turn and one that slides, is x and y from the angles and z as the height less the slide") {
  std::vector<DhRow> scara(3);
  scara[0].d = 0.4;
  scara[0].a = 0.5;
  scara[1].a = 0.3;
  scara[1].alpha = kPi;                                    // flips z, so that the slide goes down
  scara[2].prismatic = true;
  int checked = 0;
  int off = 0;
  for (double q1 = -2.0; q1 <= 2.0; q1 += 0.5) {
    for (double q2 = -2.0; q2 <= 2.0; q2 += 0.5) {
      for (double q3 = 0.0; q3 <= 0.3; q3 += 0.1) {
        const Frame tool = dh_chain(scara, std::vector<double>{q1, q2, q3}, Convention::kStandard);
        const double x = 0.5 * std::cos(q1) + 0.3 * std::cos(q1 + q2);
        const double y = 0.5 * std::sin(q1) + 0.3 * std::sin(q1 + q2);
        const double z = 0.4 - q3;
        ++checked;
        if (std::fabs(tool.p.x - x) > 1e-12 || std::fabs(tool.p.y - y) > 1e-12 || std::fabs(tool.p.z - z) > 1e-12) ++off;
      }
    }
  }
  std::cout << "  " << checked << " configurations of a SCARA table: " << off << " off from the closed form\n\n";
  RC_CHECK(checked > 100);
  RC_CHECK_EQ(off, 0);
}

RC_TEST("the same table in the two conventions is two different arms") {
  std::vector<DhRow> table(3);
  table[0].a = 0.5;
  table[0].alpha = kPi / 2.0;
  table[1].a = 0.4;
  table[1].d = 0.1;
  table[2].a = 0.3;
  table[2].alpha = -kPi / 2.0;
  const std::vector<double> q = {0.5, -0.4, 0.8};
  const Frame standard = dh_chain(table, q, Convention::kStandard);
  const Frame modified = dh_chain(table, q, Convention::kModified);
  std::cout << "  one table, one set of angles: the standard tool is at (" << standard.p.x << ", " << standard.p.y << ", " << standard.p.z << "), the modified at (" << modified.p.x << ", "
            << modified.p.y << ", " << modified.p.z << "): " << distance(standard, modified) << " m apart\n";
  RC_CHECK(distance(standard, modified) > 0.1);

  // Even a flat table, with no twists and no offsets, is two arms: the modified convention puts the length of a row before its angle.
  std::vector<DhRow> flat(2);
  flat[0].a = 1.0;
  flat[1].a = 0.7;
  const Frame fs = dh_chain(flat, std::vector<double>{0.4, -0.9}, Convention::kStandard);
  const Frame fm = dh_chain(flat, std::vector<double>{0.4, -0.9}, Convention::kModified);
  std::cout << "  a flat table of two rows: " << distance(fs, fm) << " m apart\n\n";
  RC_CHECK(distance(fs, fm) > 0.1);
  // With all joint variables at zero and no twist or offset the conventions agree: the links are laid end to end along x.
  RC_CHECK(distance(dh_chain(flat, std::vector<double>{0.0, 0.0}, Convention::kStandard), dh_chain(flat, std::vector<double>{0.0, 0.0}, Convention::kModified)) < 1e-12);
}

RC_TEST("a prismatic joint slides along z and does not turn, a missing variable is zero, and an offset of the zero shifts a revolute joint") {
  DhRow slide;
  slide.prismatic = true;
  slide.d = 0.2;
  const Frame f = dh_frame(slide, 0.5, Convention::kStandard);
  RC_CHECK_NEAR(f.p.z, 0.7, 1e-12);
  RC_CHECK_NEAR(f.r.m[0][0], 1.0, 1e-12);
  RC_CHECK_NEAR(f.r.m[1][0], 0.0, 1e-12);

  std::vector<DhRow> rows(2);
  rows[0].a = 1.0;
  rows[1].a = 1.0;
  const Frame only_first = dh_chain(rows, std::vector<double>{kPi / 2.0}, Convention::kStandard);       // the second variable is missing: 0
  RC_CHECK_NEAR(only_first.p.x, 0.0, 1e-12);
  RC_CHECK_NEAR(only_first.p.y, 2.0, 1e-12);

  DhRow offset;
  offset.a = 1.0;
  offset.theta = kPi / 2.0;                                                           // the zero of the joint points along y
  const Frame shifted = dh_frame(offset, 0.0, Convention::kStandard);
  RC_CHECK_NEAR(shifted.p.x, 0.0, 1e-12);
  RC_CHECK_NEAR(shifted.p.y, 1.0, 1e-12);
  RC_CHECK_NEAR(dh_chain(std::vector<DhRow>(), std::vector<double>(), Convention::kStandard).p.x, 0.0, 1e-12);          // no joints: the tool is the base
}
