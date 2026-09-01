// rc/math/transform.hpp
//
// The transform from lesson 06-04, graduated, and what the kinematics phase is
// built on.
//
// A rotation and a translation together: where a frame sits relative to another
// one. The rotation is a quaternion rather than a matrix, for the reasons
// lesson 06-02 measured and lesson 06-03 answered.
//
// The naming rule that came with it matters more than the code. Name every
// transform for the two frames it relates, destination first, so that adjacent
// names cancel when composing: base_tool composed with tool_grip is base_grip,
// and the subscripts do the checking. A transform named for one frame, or for
// neither, is waiting for somebody to compose it the wrong way round.

#ifndef RC_MATH_TRANSFORM_HPP
#define RC_MATH_TRANSFORM_HPP

#include <rc/math/quaternion.hpp>
#include <rc/math/vector.hpp>

namespace rc {
namespace math {

// Where one frame sits relative to another: turn by the rotation, then move by
// the translation. Name every instance for the two frames it relates, with the
// destination first, so that adjacent names cancel when composing.
struct Transform {
  Quat rotation;
  Vec3 translation;
};

// A point is a place, so moving the frame moves it: rotate, then translate.
inline Vec3 apply_to_point(const Transform& t, const Vec3& p) {
  return add(rotate(t.rotation, p), t.translation);
}

// A direction is an arrow with no location: a velocity, a surface normal, the
// way a sensor points. Only the rotation applies. Adding the translation would
// be meaningless, and gives an answer wrong by exactly that offset, which is
// small and plausible and survives review for months.
inline Vec3 apply_to_direction(const Transform& t, const Vec3& d) {
  return rotate(t.rotation, d);
}

// compose(T_a_b, T_b_c) is T_a_c. The inner names match and cancel, which is
// the check you can perform by reading rather than by reasoning.
inline Transform compose(const Transform& a_b, const Transform& b_c) {
  Transform a_c;
  a_c.rotation = multiply(a_b.rotation, b_c.rotation);

  // The inner offset is expressed in the inner frame, so it has to be brought
  // into the outer frame before the two translations can be added.
  a_c.translation =
      add(a_b.translation, rotate(a_b.rotation, b_c.translation));
  return a_c;
}

inline Transform inverse(const Transform& t) {
  const Quat undo = conjugate(t.rotation);

  // Negating the translation is not enough, and the version that only negates
  // is exactly right whenever the rotation is the identity, which is why it
  // survives a test suite that never rotates anything.
  //
  // The original translation is expressed in the original frame. The inverse
  // needs it in the rotated one, so it is rotated by the undone rotation.
  Transform back;
  back.rotation = undo;
  back.translation = rotate(undo, scale(t.translation, -1.0));
  return back;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_TRANSFORM_HPP
