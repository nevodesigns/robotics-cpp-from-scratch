#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <vector>

#include <rc/core/compat.hpp>
#include <rc/math/quaternion.hpp>
#include <rc/math/transform.hpp>
#include <rc/math/vector.hpp>

// One joint of a serial arm.
//
// Two separate things, and keeping them separate is what makes the whole thing
// work. The geometry is fixed: where this joint sits relative to the previous
// one, decided by how the arm was built and never changing. The axis is what
// the joint turns about, expressed in the joint's own frame, and the angle is
// the only thing that varies.
struct Joint {
  rc::math::Transform parent_to_joint;   // fixed, from the drawing
  rc::math::Vec3 axis;                   // unit vector, in this joint's frame
};

// The transform a joint contributes when it has turned by `angle`.
//
// A rotation about the axis and no translation: a revolute joint turns, it does
// not move. The axis is in the joint's own frame, which is why this is applied
// after the fixed geometry rather than before it.
inline rc::math::Transform joint_motion(const Joint& joint, double angle) {
  rc::math::Transform motion;
  motion.rotation = rc::math::from_axis_angle(joint.axis, angle);
  motion.translation = rc::math::Vec3{0.0, 0.0, 0.0};
  return motion;
}

// A serial chain: each joint carried by the one before it.
class Chain {
 public:
  void add(const Joint& joint) { joints_.push_back(joint); }

  std::size_t size() const { return joints_.size(); }
  const Joint& at(std::size_t index) const { return joints_[index]; }

  // Where the frame of joint `index` is, in the base frame, after every joint
  // up to and including it has turned.
  //
  // The composition reads left to right down the arm, which is what the naming
  // rule from lesson 06-04 is for: base_1 composed with 1_2 is base_2, and the
  // subscripts cancel. Composing the other way round produces a transform that
  // is a perfectly good transform between two frames nobody wanted.
  rc::math::Transform frame_at(std::size_t index, rc::span<const double> angles) const {
    // TODO: walk the chain from the base out to this joint, composing as you go.
    //
    // Start at the identity, which is the base frame relative to itself.
    //
    // For each joint up to and including this one, two transforms: the fixed
    // geometry, then the joint's own motion. That order is not interchangeable.
    // The axis is written in the joint's own frame, and the joint's own frame
    // only exists once the fixed transform has been applied, so turning first
    // turns about an axis in the parent's frame instead.
    //
    // Compose in the direction the naming rule from lesson 06-04 describes:
    // base_1 composed with 1_2 is base_2, and the subscripts cancel. Composing
    // the other way round gives a perfectly good transform between two frames
    // nobody asked about.
    //
    // An angle that was not supplied is zero. Reading past the end of the array
    // is the difference between a wrong answer and undefined behaviour, and
    // only one of those can be debugged.
    (void)index;
    (void)angles;
    return identity();
  }

  // Where the tool is: every joint, composed.
  rc::math::Transform tool(rc::span<const double> angles) const {
    return frame_at(joints_.size(), angles);
  }

  // Whether the caller supplied one angle per joint. Worth asking out loud,
  // because too few angles is a chain that quietly holds the missing joints at
  // zero and returns a pose that looks reasonable.
  bool accepts(rc::span<const double> angles) const {
    // TODO: one angle per joint.
    (void)angles;
    return false;
  }

  static rc::math::Transform identity() {
    rc::math::Transform t;
    t.rotation = rc::math::Quat{1.0, 0.0, 0.0, 0.0};
    t.translation = rc::math::Vec3{0.0, 0.0, 0.0};
    return t;
  }

 private:
  std::vector<Joint> joints_;
};

// A joint whose geometry is a straight offset along x with no turn, which is
// what most textbook arms are made of and is enough to build one by hand.
inline Joint revolute_along_x(double offset, const rc::math::Vec3& axis) {
  Joint joint;
  joint.parent_to_joint = Chain::identity();
  joint.parent_to_joint.translation = rc::math::Vec3{offset, 0.0, 0.0};
  joint.axis = axis;
  return joint;
}

#endif  // LESSON_SOLUTION_HPP
