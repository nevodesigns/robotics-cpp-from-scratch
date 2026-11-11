#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"

namespace {
const double kPi = 3.14159265358979323846;

// The rotation of `angle` about the unit axis (ax, ay, az): Rodrigues.
rc::math::Mat3 about(double ax, double ay, double az, double angle) {
  const double c = std::cos(angle), s = std::sin(angle), t = 1.0 - c;
  rc::math::Mat3 r;
  r.m[0][0] = c + ax * ax * t;      r.m[0][1] = ax * ay * t - az * s; r.m[0][2] = ax * az * t + ay * s;
  r.m[1][0] = ay * ax * t + az * s; r.m[1][1] = c + ay * ay * t;      r.m[1][2] = ay * az * t - ax * s;
  r.m[2][0] = az * ax * t - ay * s; r.m[2][1] = az * ay * t + ax * s; r.m[2][2] = c + az * az * t;
  return r;
}

double norm(const rc::math::Vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
}  // namespace

RC_TEST("the rotation vector of a turn about an axis is the axis times the angle, for small turns, large ones and ones near a half turn") {
  const double ax = 1.0 / 3.0, ay = 2.0 / 3.0, az = 2.0 / 3.0;
  const double angles[] = {0.0, 1e-9, 0.01, 0.3, 1.5, 3.0, 3.14, 3.1415};
  int off = 0;
  for (double angle : angles) {
    const rc::math::Vec3 v = rotation_vector(about(ax, ay, az, angle));
    const double e = std::fabs(v.x - ax * angle) + std::fabs(v.y - ay * angle) + std::fabs(v.z - az * angle);
    if (e > 1e-6) ++off;
  }
  std::cout << "  8 angles from 0 to 3.1415 about the axis (1, 2, 2) / 3: " << off << " recovered to worse than 1e-6\n\n";
  RC_CHECK_EQ(off, 0);
  const rc::math::Vec3 id = rotation_vector(rc::math::Mat3());
  RC_CHECK_NEAR(norm(id), 0.0, 1e-15);
}

RC_TEST("a yaw of 359 degrees against -1 degree is a difference of 360 as angles, and no difference as rotations") {
  const double target_yaw = -1.0 * kPi / 180.0;
  const double current_yaw = 359.0 * kPi / 180.0;
  const double euler_difference = (target_yaw - current_yaw) * 180.0 / kPi;
  Frame current, target;
  current.r = rc::math::rotation_z(current_yaw);
  target.r = rc::math::rotation_z(target_yaw);
  const PoseError e = pose_error(current, target);
  std::cout << "  yaw -1 against 359 degrees: the difference of the Euler angles is " << euler_difference << " degrees; the rotation error is " << e.rotation_angle() << " rad\n";
  RC_CHECK_NEAR(euler_difference, -360.0, 1e-9);
  RC_CHECK(e.rotation_angle() < 1e-9);
  // And a real difference: 10 degrees of yaw is 10 degrees, whichever way the angles are written.
  Frame ahead;
  ahead.r = rc::math::rotation_z(current_yaw + 10.0 * kPi / 180.0);
  RC_CHECK_NEAR(pose_error(current, ahead).rotation_angle(), 10.0 * kPi / 180.0, 1e-9);
  RC_CHECK_NEAR(pose_error(current, ahead).rotation.z, 10.0 * kPi / 180.0, 1e-9);        // about z, in the positive direction
  RC_CHECK_NEAR(pose_error(ahead, current).rotation.z, -10.0 * kPi / 180.0, 1e-9);        // and the other way, the other sign
}

RC_TEST("a very small turn is read from the antisymmetric part, where the arc cosine of the trace has lost it") {
  const double tiny = 1e-10;
  const rc::math::Mat3 r = about(0.0, 0.0, 1.0, tiny);
  const double trace = r.m[0][0] + r.m[1][1] + r.m[2][2];
  const double naive = std::acos(std::min(1.0, (trace - 1.0) / 2.0));
  const rc::math::Vec3 v = rotation_vector(r);
  std::cout << "  a turn of 1e-10 about z: acos of the trace gives " << naive << ", the rotation vector " << v.z << "\n";
  RC_CHECK(naive < 1e-7);                                   // the arc cosine of a number that rounds to 1 is 0: it has lost it
  RC_CHECK_NEAR(v.z, tiny, 1e-18);
  RC_CHECK_NEAR(v.x, 0.0, 1e-18);
  RC_CHECK_NEAR(v.y, 0.0, 1e-18);
}

RC_TEST("a half turn is its angle along its axis, and nearly a half turn is too, with the sign that the turn has") {
  const rc::math::Vec3 half_x = rotation_vector(about(1.0, 0.0, 0.0, kPi));
  RC_CHECK_NEAR(norm(half_x), kPi, 1e-9);
  RC_CHECK_NEAR(std::fabs(half_x.x), kPi, 1e-9);
  RC_CHECK_NEAR(half_x.y, 0.0, 1e-9);
  RC_CHECK_NEAR(half_x.z, 0.0, 1e-9);

  const rc::math::Vec3 below = rotation_vector(about(0.0, 1.0, 0.0, kPi - 1e-6));
  const rc::math::Vec3 below_back = rotation_vector(about(0.0, -1.0, 0.0, kPi - 1e-6));
  std::cout << "  pi - 1e-6 about +y: (" << below.x << ", " << below.y << ", " << below.z << "); about -y: y = " << below_back.y << "\n";
  RC_CHECK_NEAR(below.y, kPi - 1e-6, 1e-6);
  RC_CHECK_NEAR(below_back.y, -(kPi - 1e-6), 1e-6);          // the same angle the other way round is the other sign
  RC_CHECK_NEAR(below.x, 0.0, 1e-6);
  RC_CHECK_NEAR(below.z, 0.0, 1e-6);
  const rc::math::Vec3 diagonal = rotation_vector(about(0.6, 0.0, 0.8, kPi - 1e-7));
  RC_CHECK_NEAR(diagonal.x, 0.6 * (kPi - 1e-7), 1e-6);
  RC_CHECK_NEAR(diagonal.z, 0.8 * (kPi - 1e-7), 1e-6);
}

RC_TEST("a pose that is right in position and out in orientation is not converged, and neither is the other way round") {
  Frame current;
  current.p = rc::math::Vec3{0.5, 0.2, 0.1};
  current.r = rc::math::rotation_z(0.3);
  Frame target = current;
  RC_CHECK(converged(pose_error(current, target), 1e-6, 1e-6));          // the same pose
  RC_CHECK_NEAR(pose_error(current, target).position_norm(), 0.0, 1e-15);

  Frame tilted = current;
  tilted.r = rc::math::multiply(rc::math::rotation_x(0.1), current.r);    // the same place, a tenth of a radian out
  const PoseError tilt = pose_error(current, tilted);
  std::cout << "  the right place, a tenth of a radian out: position error " << tilt.position_norm() << " m, rotation error " << tilt.rotation_angle() << " rad, converged to 1 mm and 1 mrad: "
            << converged(tilt, 1e-3, 1e-3) << "\n";
  RC_CHECK_NEAR(tilt.position_norm(), 0.0, 1e-15);
  RC_CHECK_NEAR(tilt.rotation_angle(), 0.1, 1e-9);
  RC_CHECK(!converged(tilt, 1e-3, 1e-3));

  Frame moved = current;
  moved.p.x += 0.02;
  const PoseError off = pose_error(current, moved);
  RC_CHECK_NEAR(off.position_norm(), 0.02, 1e-12);
  RC_CHECK_NEAR(off.position.x, 0.02, 1e-12);
  RC_CHECK_NEAR(off.rotation_angle(), 0.0, 1e-9);
  RC_CHECK(!converged(off, 1e-3, 1e-3));
  RC_CHECK(converged(off, 0.05, 1e-3));                                   // within a looser position tolerance
}
