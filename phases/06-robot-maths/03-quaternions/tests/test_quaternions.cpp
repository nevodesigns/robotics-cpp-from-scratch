#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kQuarter = kPi / 2.0;

const Vec3 kX{1.0, 0.0, 0.0};
const Vec3 kY{0.0, 1.0, 0.0};
const Vec3 kZ{0.0, 0.0, 1.0};

bool near_vec(const Vec3& a, const Vec3& b, double tolerance) {
  return std::fabs(a.x - b.x) <= tolerance && std::fabs(a.y - b.y) <= tolerance &&
         std::fabs(a.z - b.z) <= tolerance;
}

}  // namespace

RC_TEST("the identity rotates nothing") {
  const Vec3 v{1.0, 2.0, 3.0};
  RC_CHECK(near_vec(rotate(Quat{}, v), v, 1e-12));
}

RC_TEST("a quarter turn stores the half angle, not the angle") {
  // cos(45) rather than cos(90). Every quaternion bug that is not a sign error
  // is somebody forgetting this, and it gives a rotation of twice what was
  // asked for.
  const Quat q = from_axis_angle(kZ, kQuarter);
  RC_CHECK_NEAR(q.w, std::cos(kQuarter / 2.0), 1e-12);
  RC_CHECK_NEAR(q.w, 0.70710678, 1e-8);
  RC_CHECK_NEAR(q.z, std::sin(kQuarter / 2.0), 1e-12);
}

RC_TEST("a quarter turn about z sends x to y") {
  // The same check lesson 06-02 made of the rotation matrix. The two
  // representations must agree, and this is where a halved angle shows up as a
  // rotation of the wrong size.
  RC_CHECK(near_vec(rotate(from_axis_angle(kZ, kQuarter), kX), kY, 1e-12));
}

RC_TEST("a quarter turn about x sends y to z") {
  RC_CHECK(near_vec(rotate(from_axis_angle(kX, kQuarter), kY), kZ, 1e-12));
}

RC_TEST("a rotation leaves its own axis alone") {
  RC_CHECK(near_vec(rotate(from_axis_angle(kZ, 1.234), kZ), kZ, 1e-12));
}

RC_TEST("rotating keeps the length of a vector") {
  const Vec3 v{1.0, 2.0, 3.0};
  const Quat q = from_axis_angle(Vec3{1.0, 1.0, 1.0}, 0.9);
  RC_CHECK_NEAR(length(rotate(q, v)), length(v), 1e-12);
}

RC_TEST("the axis does not have to arrive normalised") {
  const Quat unit = from_axis_angle(kZ, kQuarter);
  const Quat scaled = from_axis_angle(Vec3{0.0, 0.0, 17.0}, kQuarter);
  RC_CHECK(same_rotation(unit, scaled, 1e-12));
}

RC_TEST("an axis of no length gives the identity, not NaN") {
  const Quat q = from_axis_angle(Vec3{0.0, 0.0, 0.0}, 1.0);
  RC_CHECK(!std::isnan(q.w));
  RC_CHECK(!std::isnan(q.x));
  RC_CHECK(same_rotation(q, Quat{}, 1e-12));
}

RC_TEST("every quaternion from an axis and angle has unit norm") {
  RC_CHECK_NEAR(norm(from_axis_angle(kZ, 0.4)), 1.0, 1e-12);
  RC_CHECK_NEAR(norm(from_axis_angle(Vec3{1.0, 2.0, 3.0}, -2.7)), 1.0, 1e-12);
}

RC_TEST("composing is applying one rotation then the other") {
  const Quat first = from_axis_angle(kZ, kQuarter);
  const Quat second = from_axis_angle(kY, kQuarter);

  const Vec3 stepwise = rotate(second, rotate(first, kX));
  const Vec3 composed = rotate(multiply(second, first), kX);
  RC_CHECK(near_vec(stepwise, composed, 1e-12));
}

RC_TEST("order matters here too") {
  const Quat x = from_axis_angle(kX, kQuarter);
  const Quat y = from_axis_angle(kY, kQuarter);
  RC_CHECK(!same_rotation(multiply(x, y), multiply(y, x), 1e-9));
}

RC_TEST("the conjugate undoes the rotation") {
  const Quat q = from_axis_angle(Vec3{1.0, -2.0, 0.5}, 1.1);
  const Vec3 v{3.0, 1.0, -2.0};
  RC_CHECK(near_vec(rotate(conjugate(q), rotate(q, v)), v, 1e-12));
}

RC_TEST("a quaternion times its conjugate is the identity") {
  const Quat q = from_axis_angle(Vec3{0.3, 0.4, 0.5}, 2.2);
  RC_CHECK(same_rotation(multiply(q, conjugate(q)), Quat{}, 1e-12));
}

RC_TEST("negating all four numbers describes the same rotation") {
  // The surprise. Code that compares quaternions directly works perfectly until
  // an orientation happens to cross the boundary, and then it does not.
  const Quat q = from_axis_angle(Vec3{1.0, 2.0, 3.0}, 1.3);
  const Quat negated{-q.w, -q.x, -q.y, -q.z};

  RC_CHECK(same_rotation(q, negated, 1e-12));

  const Vec3 v{0.5, -1.5, 2.0};
  RC_CHECK(near_vec(rotate(q, v), rotate(negated, v), 1e-12));
}

RC_TEST("same_rotation still tells genuinely different orientations apart") {
  const Quat a = from_axis_angle(kZ, 0.5);
  const Quat b = from_axis_angle(kZ, 0.9);
  RC_CHECK(!same_rotation(a, b, 1e-6));
}

RC_TEST("normalising repairs a quaternion that has drifted") {
  const Quat stretched{2.0, 0.0, 0.0, 0.0};
  RC_CHECK_NEAR(norm(normalized(stretched)), 1.0, 1e-12);
  RC_CHECK(same_rotation(normalized(stretched), Quat{}, 1e-12));
}

RC_TEST("normalising nothing gives the identity rather than NaN") {
  const Quat q = normalized(Quat{0.0, 0.0, 0.0, 0.0});
  RC_CHECK(!std::isnan(q.w));
  RC_CHECK_NEAR(norm(q), 1.0, 1e-12);
}

RC_TEST("drift after a hundred thousand compositions, and the cost of repair") {
  // The same measurement lesson 06-02 made of a rotation matrix, on the same
  // number of compositions, so the two are comparable.
  const Quat small = from_axis_angle(Vec3{0.001, 0.002, 0.003}, 0.001);
  Quat accumulated;
  for (int i = 0; i < 100000; ++i) accumulated = multiply(accumulated, small);

  const double drift = std::fabs(norm(accumulated) - 1.0);
  std::cout << "\n  after 100000 compositions the norm is off one by " << drift << "\n";
  std::cout << "  repairing it costs one square root and four divisions\n";

  RC_CHECK(drift > 0.0);
  RC_CHECK_NEAR(norm(normalized(accumulated)), 1.0, 1e-15);
}

RC_TEST("a repaired quaternion still describes the orientation it drifted to") {
  // Renormalising must not move the rotation, only restore the length. If it
  // did, an estimator repairing its state every update would wander.
  const Quat q = from_axis_angle(Vec3{1.0, 1.0, 0.0}, 0.7);
  const Quat stretched{q.w * 1.0001, q.x * 1.0001, q.y * 1.0001, q.z * 1.0001};
  const Vec3 v{1.0, 2.0, 3.0};
  RC_CHECK(near_vec(rotate(normalized(stretched), v), rotate(q, v), 1e-9));
}
