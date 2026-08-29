#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"

namespace {
constexpr double kPi = 3.14159265358979323846;
}

RC_TEST("vectors add, subtract and scale") {
  const Vec2 a{3.0, 4.0};
  const Vec2 b{1.0, 2.0};
  RC_CHECK_NEAR(add(a, b).x, 4.0, 1e-12);
  RC_CHECK_NEAR(add(a, b).y, 6.0, 1e-12);
  RC_CHECK_NEAR(subtract(a, b).x, 2.0, 1e-12);
  RC_CHECK_NEAR(scale(a, 2.0).y, 8.0, 1e-12);
}

RC_TEST("length is the distance from the origin") {
  RC_CHECK_NEAR(length(Vec2{3.0, 4.0}), 5.0, 1e-12);
  RC_CHECK_NEAR(length(Vec2{0.0, 0.0}), 0.0, 1e-12);
  RC_CHECK_NEAR(length(Vec2{-3.0, -4.0}), 5.0, 1e-12);
}

RC_TEST("length survives very large components") {
  // sqrt(x*x + y*y) overflows here and answers infinity. hypot does not.
  const double huge = 1e200;
  RC_CHECK(std::isfinite(length(Vec2{huge, huge})));
}

RC_TEST("the dot product is largest when the vectors agree") {
  const Vec2 east{1.0, 0.0};
  RC_CHECK_NEAR(dot(east, Vec2{1.0, 0.0}), 1.0, 1e-12);
  RC_CHECK_NEAR(dot(east, Vec2{0.0, 1.0}), 0.0, 1e-12);
  RC_CHECK_NEAR(dot(east, Vec2{-1.0, 0.0}), -1.0, 1e-12);
}

RC_TEST("the sign of the dot product answers in front or behind") {
  const Vec2 heading{1.0, 0.0};
  RC_CHECK(dot(heading, Vec2{5.0, 2.0}) > 0.0);    // ahead
  RC_CHECK(dot(heading, Vec2{-5.0, 2.0}) < 0.0);   // behind
}

RC_TEST("the cross product is zero for parallel vectors") {
  RC_CHECK_NEAR(cross(Vec2{1.0, 0.0}, Vec2{3.0, 0.0}), 0.0, 1e-12);
  RC_CHECK_NEAR(cross(Vec2{1.0, 2.0}, Vec2{2.0, 4.0}), 0.0, 1e-12);
}

RC_TEST("the sign of the cross product answers which way it is turned") {
  const Vec2 east{1.0, 0.0};
  RC_CHECK(cross(east, Vec2{0.0, 1.0}) > 0.0);    // north is anticlockwise
  RC_CHECK(cross(east, Vec2{0.0, -1.0}) < 0.0);   // south is clockwise
}

RC_TEST("swapping the arguments negates the cross product") {
  // The check that catches the terms written the wrong way round, which is easy
  // to do and reverses every steering decision downstream.
  const Vec2 a{2.0, 1.0};
  const Vec2 b{1.0, 3.0};
  RC_CHECK_NEAR(cross(a, b), -cross(b, a), 1e-12);
  RC_CHECK(cross(a, b) > 0.0);
}

RC_TEST("normalising gives length one in the same direction") {
  const Vec2 unit = normalized(Vec2{3.0, 4.0});
  RC_CHECK_NEAR(length(unit), 1.0, 1e-12);
  RC_CHECK_NEAR(unit.x, 0.6, 1e-12);
  RC_CHECK_NEAR(unit.y, 0.8, 1e-12);
}

RC_TEST("normalising a zero vector does not produce NaN") {
  // Dividing by a length of zero fills the result with NaN, which then spreads
  // through every later calculation without a word.
  const Vec2 nothing = normalized(Vec2{0.0, 0.0});
  RC_CHECK(!std::isnan(nothing.x));
  RC_CHECK(!std::isnan(nothing.y));
  RC_CHECK_NEAR(length(nothing), 0.0, 1e-12);
}

RC_TEST("the angle between equal directions is zero") {
  RC_CHECK_NEAR(angle_between(Vec2{1.0, 0.0}, Vec2{2.0, 0.0}), 0.0, 1e-12);
}

RC_TEST("the angle is signed, so it tells left from right") {
  // acos cannot do this at all: it only answers zero to pi.
  const Vec2 east{1.0, 0.0};
  RC_CHECK_NEAR(angle_between(east, Vec2{0.0, 1.0}), kPi / 2.0, 1e-12);
  RC_CHECK_NEAR(angle_between(east, Vec2{0.0, -1.0}), -kPi / 2.0, 1e-12);
}

RC_TEST("the angle between opposed directions is half a turn") {
  RC_CHECK_NEAR(std::fabs(angle_between(Vec2{1.0, 0.0}, Vec2{-1.0, 0.0})), kPi, 1e-12);
}

RC_TEST("a tiny angle is resolved accurately") {
  // Where acos falls apart. Cosine is flat near zero, so a small error in the
  // dot product becomes a large error in the angle, and this is exactly the
  // region a path follower spends its life in.
  const Vec2 a{1.0, 0.0};
  const Vec2 b{1.0, 1e-8};
  RC_CHECK_NEAR(angle_between(a, b), 1e-8, 1e-12);
}

RC_TEST("nearly parallel vectors do not produce NaN") {
  // Rounding can push the acos argument past one, and acos of anything above
  // one is not a number. atan2 has no domain to leave.
  const Vec2 a{1.0, 0.0};
  const Vec2 b = normalized(Vec2{1.0, 0.0});
  RC_CHECK(!std::isnan(angle_between(a, b)));
  RC_CHECK(!std::isnan(angle_between(b, b)));
}

RC_TEST("a robot on the path has no cross track error") {
  const Vec2 from{0.0, 0.0};
  const Vec2 to{10.0, 0.0};
  RC_CHECK_NEAR(cross_track_error(from, to, Vec2{5.0, 0.0}), 0.0, 1e-12);
  RC_CHECK_NEAR(cross_track_error(from, to, Vec2{0.0, 0.0}), 0.0, 1e-12);
}

RC_TEST("cross track error is positive to the left of the path") {
  // The convention, checked. Getting this backwards makes a path follower steer
  // away from the line instead of towards it.
  const Vec2 from{0.0, 0.0};
  const Vec2 to{10.0, 0.0};
  RC_CHECK(cross_track_error(from, to, Vec2{5.0, 2.0}) > 0.0);
  RC_CHECK(cross_track_error(from, to, Vec2{5.0, -2.0}) < 0.0);
}

RC_TEST("cross track error is the perpendicular distance") {
  const Vec2 from{0.0, 0.0};
  const Vec2 to{10.0, 0.0};
  RC_CHECK_NEAR(cross_track_error(from, to, Vec2{5.0, 3.0}), 3.0, 1e-12);
  RC_CHECK_NEAR(cross_track_error(from, to, Vec2{99.0, 3.0}), 3.0, 1e-12);
}

RC_TEST("cross track error works on a path that is not along an axis") {
  // A diagonal path, with the robot one unit off it perpendicular.
  const Vec2 from{0.0, 0.0};
  const Vec2 to{1.0, 1.0};
  const double offset = std::sqrt(2.0) / 2.0;
  RC_CHECK_NEAR(cross_track_error(from, to, Vec2{-offset, offset}), 1.0, 1e-9);
}

RC_TEST("a path segment with no length has no side") {
  const Vec2 same{4.0, 4.0};
  RC_CHECK_NEAR(cross_track_error(same, same, Vec2{9.0, 9.0}), 0.0, 1e-12);
}

RC_TEST("reversing the path reverses the sign") {
  const Vec2 a{0.0, 0.0};
  const Vec2 b{10.0, 0.0};
  const Vec2 robot{5.0, 2.0};
  RC_CHECK_NEAR(cross_track_error(a, b, robot), -cross_track_error(b, a, robot), 1e-12);
}
