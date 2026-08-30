#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <rc/math/quaternion.hpp>
#include <rc/math/vector.hpp>

using rc::math::Quat;
using rc::math::Vec3;

struct Transform {
  Quat rotation;
  Vec3 translation;
};

// A point is a place, so moving the frame moves it.
inline Vec3 apply_to_point(const Transform& t, const Vec3& p) {
  // TODO: rotate, then translate.
  (void)t;
  return p;
}

// A direction is an arrow with no location. Moving a frame does not change
// which way north is.
inline Vec3 apply_to_direction(const Transform& t, const Vec3& d) {
  // TODO: one of the two operations above, and only one.
  (void)t;
  return d;
}

// compose(T_a_b, T_b_c) is T_a_c.
inline Transform compose(const Transform& a_b, const Transform& b_c) {
  // TODO
  //
  // The rotations multiply in the order quaternions do. For the translation,
  // remember that the inner offset is expressed in the inner frame, so it has
  // to be brought into the outer frame before the two can be added.
  (void)b_c;
  return a_b;
}

inline Transform inverse(const Transform& t) {
  // TODO
  //
  // The rotation part is the conjugate. The translation is not simply negated:
  // the original is expressed in the original frame, and the inverse needs it
  // in the rotated one.
  //
  // The version that only negates is exactly right whenever the rotation is the
  // identity, which is why it passes a test suite that never rotates anything.
  // Two of the tests here do rotate.
  return t;
}

#endif  // LESSON_SOLUTION_HPP
