#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kQuarter = kPi / 2.0;

bool same(const Mat3& a, const Mat3& b, double tolerance) {
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      if (std::fabs(a.m[row][col] - b.m[row][col]) > tolerance) return false;
    }
  }
  return true;
}

double largest_difference(const Mat3& a, const Mat3& b) {
  double worst = 0.0;
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      worst = std::max(worst, std::fabs(a.m[row][col] - b.m[row][col]));
    }
  }
  return worst;
}

}  // namespace

RC_TEST("a quarter turn about z sends x to y") {
  const Vec3 turned = apply(rotation_z(kQuarter), Vec3{1.0, 0.0, 0.0});
  RC_CHECK_NEAR(turned.x, 0.0, 1e-12);
  RC_CHECK_NEAR(turned.y, 1.0, 1e-12);
  RC_CHECK_NEAR(turned.z, 0.0, 1e-12);
}

RC_TEST("a quarter turn about x sends y to z") {
  const Vec3 turned = apply(rotation_x(kQuarter), Vec3{0.0, 1.0, 0.0});
  RC_CHECK_NEAR(turned.y, 0.0, 1e-12);
  RC_CHECK_NEAR(turned.z, 1.0, 1e-12);
}

RC_TEST("a quarter turn about y sends z to x") {
  // The axis whose matrix has its minus sign in a different place. Getting the
  // signs wrong here rotates the world the other way and nothing else complains.
  const Vec3 turned = apply(rotation_y(kQuarter), Vec3{0.0, 0.0, 1.0});
  RC_CHECK_NEAR(turned.x, 1.0, 1e-12);
  RC_CHECK_NEAR(turned.z, 0.0, 1e-12);
}

RC_TEST("a rotation about an axis leaves that axis alone") {
  const Vec3 axis = apply(rotation_z(1.234), Vec3{0.0, 0.0, 1.0});
  RC_CHECK_NEAR(axis.z, 1.0, 1e-12);
  RC_CHECK_NEAR(axis.x, 0.0, 1e-12);
}

RC_TEST("rotating keeps the length of a vector") {
  const Vec3 v{1.0, 2.0, 3.0};
  const Vec3 turned = apply(from_rpy(0.3, -0.7, 1.9), v);
  RC_CHECK_NEAR(length(turned), length(v), 1e-12);
}

RC_TEST("every rotation matrix is orthonormal") {
  RC_CHECK(is_orthonormal(rotation_x(0.4), 1e-12));
  RC_CHECK(is_orthonormal(rotation_y(-1.1), 1e-12));
  RC_CHECK(is_orthonormal(from_rpy(0.3, 0.5, -0.9), 1e-12));
}

RC_TEST("a matrix that is not a rotation is rejected") {
  Mat3 stretched;
  stretched.m[0][0] = 2.0;   // a column no longer of unit length
  RC_CHECK(!is_orthonormal(stretched, 1e-12));

  Mat3 sheared;
  sheared.m[0][1] = 0.5;     // columns no longer perpendicular
  RC_CHECK(!is_orthonormal(sheared, 1e-12));
}

RC_TEST("the transpose undoes the rotation") {
  const Mat3 r = from_rpy(0.3, 0.5, -0.9);
  const Mat3 identity = multiply(r, transposed(r));
  RC_CHECK(same(identity, Mat3{}, 1e-12));
}

RC_TEST("composing is applying one then the other") {
  const Vec3 start{1.0, 0.0, 0.0};
  const Mat3 first = rotation_z(kQuarter);
  const Mat3 second = rotation_y(kQuarter);

  const Vec3 stepwise = apply(second, apply(first, start));
  const Vec3 composed = apply(multiply(second, first), start);

  RC_CHECK_NEAR(stepwise.x, composed.x, 1e-12);
  RC_CHECK_NEAR(stepwise.y, composed.y, 1e-12);
  RC_CHECK_NEAR(stepwise.z, composed.z, 1e-12);
}

RC_TEST("order matters, and this is the surprise") {
  // Rotate a book about the axis pointing at you, then about the vertical, and
  // note where the cover faces. Do the two the other way round and it is
  // somewhere else. Rotation does not commute, which is why every convention
  // has to state its order.
  const Mat3 x_then_y = multiply(rotation_y(kQuarter), rotation_x(kQuarter));
  const Mat3 y_then_x = multiply(rotation_x(kQuarter), rotation_y(kQuarter));
  RC_CHECK(!same(x_then_y, y_then_x, 1e-9));
}

RC_TEST("gimbal lock: two different descriptions, one orientation") {
  // Pitch straight up, and the roll axis has been turned onto the yaw axis. Now
  // rolling and yawing do the same thing, and only their difference matters.
  //
  // This is not an error in the arithmetic. Three numbers cannot cover every
  // orientation smoothly, and no choice of axis order avoids it.
  const double pitch = kQuarter;
  const double d = 0.37;

  const Mat3 a = from_rpy(0.2, pitch, 0.9);
  const Mat3 b = from_rpy(0.2 + d, pitch, 0.9 + d);

  RC_CHECK(same(a, b, 1e-9));
  std::cout << "\n  at pitch = 90 degrees, adding " << d
            << " to both roll and yaw changes the matrix by "
            << largest_difference(a, b) << "\n";
}

RC_TEST("away from the singularity the same change does move the orientation") {
  // The same test at a harmless pitch, to show the effect above belongs to that
  // one orientation rather than to the arithmetic in general.
  const double pitch = 0.3;
  const double d = 0.37;

  const Mat3 a = from_rpy(0.2, pitch, 0.9);
  const Mat3 b = from_rpy(0.2 + d, pitch, 0.9 + d);

  RC_CHECK(!same(a, b, 1e-3));
}

RC_TEST("repeated composition drifts away from being a rotation") {
  // What a dead reckoning loop does for hours. Rounding accumulates until the
  // matrix stops keeping lengths and starts quietly scaling and shearing.
  const Mat3 small = from_rpy(0.001, 0.002, 0.003);
  Mat3 accumulated;
  for (int i = 0; i < 100000; ++i) accumulated = multiply(accumulated, small);

  double worst_column = 0.0;
  for (int i = 0; i < 3; ++i) {
    worst_column = std::max(worst_column, std::fabs(length(column(accumulated, i)) - 1.0));
  }
  std::cout << "  after 100000 compositions the columns are off unit length by "
            << worst_column << "\n";

  // Still a rotation to a loose tolerance, and measurably not one to a tight
  // tolerance. That gap is the whole reason long running code renormalises.
  RC_CHECK(is_orthonormal(accumulated, 1e-6));
  RC_CHECK(worst_column > 0.0);
}
