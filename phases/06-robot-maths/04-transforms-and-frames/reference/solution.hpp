#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <rc/math/quaternion.hpp>
#include <rc/math/vector.hpp>

using rc::math::Quat;
using rc::math::Vec3;

// Where one frame sits relative to another: turn by the rotation, then move by
// the translation. Name every instance for the two frames it relates, with the
// destination first, so that adjacent names cancel when composing.
struct Transform {
  Quat rotation;
  Vec3 translation;
};

// A point is a place, so moving the frame moves it: rotate, then translate.
inline Vec3 apply_to_point(const Transform& t, const Vec3& p) {
  return rc::math::add(rc::math::rotate(t.rotation, p), t.translation);
}

// A direction is an arrow with no location: a velocity, a surface normal, the
// way a sensor points. Only the rotation applies. Adding the translation would
// be meaningless, and gives an answer wrong by exactly that offset, which is
// small and plausible and survives review for months.
inline Vec3 apply_to_direction(const Transform& t, const Vec3& d) {
  return rc::math::rotate(t.rotation, d);
}

// compose(T_a_b, T_b_c) is T_a_c. The inner names match and cancel, which is
// the check you can perform by reading rather than by reasoning.
inline Transform compose(const Transform& a_b, const Transform& b_c) {
  Transform a_c;
  a_c.rotation = rc::math::multiply(a_b.rotation, b_c.rotation);

  // The inner offset is expressed in the inner frame, so it has to be brought
  // into the outer frame before the two translations can be added.
  a_c.translation =
      rc::math::add(a_b.translation, rc::math::rotate(a_b.rotation, b_c.translation));
  return a_c;
}

inline Transform inverse(const Transform& t) {
  const Quat undo = rc::math::conjugate(t.rotation);

  // Negating the translation is not enough, and the version that only negates
  // is exactly right whenever the rotation is the identity, which is why it
  // survives a test suite that never rotates anything.
  //
  // The original translation is expressed in the original frame. The inverse
  // needs it in the rotated one, so it is rotated by the undone rotation.
  Transform back;
  back.rotation = undo;
  back.translation = rc::math::rotate(undo, rc::math::scale(t.translation, -1.0));
  return back;
}

#endif  // LESSON_SOLUTION_HPP
