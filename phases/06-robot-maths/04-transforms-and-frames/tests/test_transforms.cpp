#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kQuarter = kPi / 2.0;

const Vec3 kZ{0.0, 0.0, 1.0};

bool near_vec(const Vec3& a, const Vec3& b, double tolerance) {
  return std::fabs(a.x - b.x) <= tolerance && std::fabs(a.y - b.y) <= tolerance &&
         std::fabs(a.z - b.z) <= tolerance;
}

Transform moved(const Vec3& by) { return Transform{Quat{}, by}; }

Transform turned(double angle) {
  return Transform{rc::math::from_axis_angle(kZ, angle), Vec3{}};
}

}  // namespace

RC_TEST("the identity transform changes nothing") {
  const Transform identity;
  const Vec3 p{1.0, 2.0, 3.0};
  RC_CHECK(near_vec(apply_to_point(identity, p), p, 1e-12));
  RC_CHECK(near_vec(apply_to_direction(identity, p), p, 1e-12));
}

RC_TEST("a translation moves a point") {
  RC_CHECK(near_vec(apply_to_point(moved(Vec3{1.0, 2.0, 3.0}), Vec3{0.0, 0.0, 0.0}),
                    Vec3{1.0, 2.0, 3.0}, 1e-12));
}

RC_TEST("a translation does not move a direction") {
  // The distinction that produces an error exactly equal to a fixed offset:
  // small, plausible, and able to survive review for months.
  const Vec3 north{0.0, 1.0, 0.0};
  RC_CHECK(near_vec(apply_to_direction(moved(Vec3{5.0, 5.0, 5.0}), north), north, 1e-12));
}

RC_TEST("a rotation turns both points and directions") {
  const Transform t = turned(kQuarter);
  RC_CHECK(near_vec(apply_to_point(t, Vec3{1.0, 0.0, 0.0}), Vec3{0.0, 1.0, 0.0}, 1e-12));
  RC_CHECK(near_vec(apply_to_direction(t, Vec3{1.0, 0.0, 0.0}), Vec3{0.0, 1.0, 0.0}, 1e-12));
}

RC_TEST("a point is rotated first and translated second") {
  // Order matters. Translating then rotating would move the offset as well and
  // put the point somewhere else entirely.
  Transform t;
  t.rotation = rc::math::from_axis_angle(kZ, kQuarter);
  t.translation = Vec3{10.0, 0.0, 0.0};

  RC_CHECK(near_vec(apply_to_point(t, Vec3{1.0, 0.0, 0.0}), Vec3{10.0, 1.0, 0.0}, 1e-12));
}

RC_TEST("composing two transforms is applying them in turn") {
  Transform a_b;
  a_b.rotation = rc::math::from_axis_angle(kZ, kQuarter);
  a_b.translation = Vec3{1.0, 0.0, 0.0};

  Transform b_c;
  b_c.rotation = rc::math::from_axis_angle(kZ, kQuarter);
  b_c.translation = Vec3{2.0, 0.0, 0.0};

  const Vec3 p{0.5, -0.25, 3.0};
  const Vec3 stepwise = apply_to_point(a_b, apply_to_point(b_c, p));
  const Vec3 composed = apply_to_point(compose(a_b, b_c), p);
  RC_CHECK(near_vec(stepwise, composed, 1e-12));
}

RC_TEST("composing translations adds them") {
  const Transform total = compose(moved(Vec3{1.0, 0.0, 0.0}), moved(Vec3{0.0, 2.0, 0.0}));
  RC_CHECK(near_vec(total.translation, Vec3{1.0, 2.0, 0.0}, 1e-12));
}

RC_TEST("the inner offset is expressed in the outer frame before being added") {
  // A quarter turn, then two metres along the inner x axis. In the outer frame
  // that is two metres along y, not along x.
  const Transform total = compose(turned(kQuarter), moved(Vec3{2.0, 0.0, 0.0}));
  RC_CHECK(near_vec(total.translation, Vec3{0.0, 2.0, 0.0}, 1e-12));
}

RC_TEST("a transform composed with its inverse is the identity") {
  Transform t;
  t.rotation = rc::math::from_axis_angle(Vec3{1.0, 2.0, 3.0}, 0.9);
  t.translation = Vec3{4.0, -1.0, 2.5};

  const Transform back = compose(t, inverse(t));
  RC_CHECK(near_vec(back.translation, Vec3{}, 1e-12));
  RC_CHECK(rc::math::same_rotation(back.rotation, Quat{}, 1e-12));
}

RC_TEST("the inverse takes a transformed point back where it came from") {
  Transform t;
  t.rotation = rc::math::from_axis_angle(kZ, 0.7);
  t.translation = Vec3{3.0, -2.0, 1.0};

  const Vec3 p{1.5, 0.5, -2.0};
  RC_CHECK(near_vec(apply_to_point(inverse(t), apply_to_point(t, p)), p, 1e-12));
}

RC_TEST("inverting a pure translation only negates it") {
  // This is the case the wrong implementation gets right, which is why a suite
  // that stops here would pass while being broken.
  const Transform back = inverse(moved(Vec3{1.0, 2.0, 3.0}));
  RC_CHECK(near_vec(back.translation, Vec3{-1.0, -2.0, -3.0}, 1e-12));
}

RC_TEST("inverting a transform that rotates does more than negate") {
  // And this is the case it gets wrong. A quarter turn with an offset along x:
  // the inverse translation is not simply the negation, because the offset has
  // to be expressed in the rotated frame.
  Transform t;
  t.rotation = rc::math::from_axis_angle(kZ, kQuarter);
  t.translation = Vec3{2.0, 0.0, 0.0};

  const Transform back = inverse(t);
  RC_CHECK(!near_vec(back.translation, Vec3{-2.0, 0.0, 0.0}, 1e-6));
  RC_CHECK(near_vec(back.translation, Vec3{0.0, 2.0, 0.0}, 1e-12));
}

RC_TEST("where is the gripper, in the world frame") {
  // The question the whole of phase 13 is about, on a three link arm.
  //
  //   the base sits at (2, 0, 0) in the world, turned a quarter turn
  //   the shoulder sits 0.5 up from the base
  //   the gripper reaches 1.0 along the shoulder's x axis
  Transform world_base;
  world_base.rotation = rc::math::from_axis_angle(kZ, kQuarter);
  world_base.translation = Vec3{2.0, 0.0, 0.0};

  const Transform base_shoulder = moved(Vec3{0.0, 0.0, 0.5});
  const Transform shoulder_gripper = moved(Vec3{1.0, 0.0, 0.0});

  // Adjacent names match and cancel: world_base * base_shoulder * shoulder_gripper.
  const Transform world_gripper =
      compose(compose(world_base, base_shoulder), shoulder_gripper);

  const Vec3 tip = apply_to_point(world_gripper, Vec3{});

  // The base is turned a quarter turn, so the arm's reach along its own x axis
  // points along the world y axis.
  RC_CHECK(near_vec(tip, Vec3{2.0, 1.0, 0.5}, 1e-12));
}

RC_TEST("going back down the chain returns the gripper to its own origin") {
  Transform world_base;
  world_base.rotation = rc::math::from_axis_angle(kZ, 0.4);
  world_base.translation = Vec3{2.0, 1.0, 0.0};

  Transform base_gripper;
  base_gripper.rotation = rc::math::from_axis_angle(Vec3{0.0, 1.0, 0.0}, -0.8);
  base_gripper.translation = Vec3{0.3, 0.0, 0.7};

  const Transform world_gripper = compose(world_base, base_gripper);
  const Vec3 tip_in_world = apply_to_point(world_gripper, Vec3{});
  const Vec3 tip_in_gripper = apply_to_point(inverse(world_gripper), tip_in_world);

  RC_CHECK(near_vec(tip_in_gripper, Vec3{}, 1e-12));
}
