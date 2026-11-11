// rc/kin/dh.hpp
//
// Denavit-Hartenberg frames, from lesson 13-12, graduated.
//
// A planar two-link table gave the tool where the sum of the links puts it; a SCARA table, two revolute joints and a prismatic one with a twist of pi, gave x and y from the two angles and z as the height less the slide; the same table in the standard and the modified convention gave tools more than a tenth of a metre apart; and the order of the four elementary moves is what makes the difference.

#ifndef RC_KIN_DH_HPP
#define RC_KIN_DH_HPP

#include <cmath>
#include <cstddef>
#include <vector>
#include <rc/math/rotation.hpp>
#include <rc/math/vector.hpp>

namespace rc {
namespace kin {

// An arm in space has joints that turn and joints that slide, and links between them, and it is described to a program as a table with a row for each joint. Four numbers in a row say how
// one joint sits on the one before: an angle about its axis (theta), a distance along it (d), a distance along the common perpendicular (a), and a twist between the axes (alpha). That
// is the Denavit-Hartenberg table, and a program turns each row into a transform and multiplies them.
//
// There are two conventions for what a row means, the standard one and the modified one (Craig). They use the same four letters. The transform of a row is a different product of the same
// four elementary moves, and the numbers of a table for one convention are a different arm in the other. The table is only half of the description: the other half is the name of the
// convention it was written for.

// A frame: where something is (p) and which way it is turned (r), relative to the one it is described in.
struct Frame {
  rc::math::Mat3 r;
  rc::math::Vec3 p;
};

inline Frame compose(const Frame& a, const Frame& b) {
  Frame out;
  out.r = rc::math::multiply(a.r, b.r);
  const rc::math::Vec3 rotated = rc::math::apply(a.r, b.p);
  out.p = rc::math::Vec3{a.p.x + rotated.x, a.p.y + rotated.y, a.p.z + rotated.z};
  return out;
}

inline Frame rot_z(double angle) { return Frame{rc::math::rotation_z(angle), rc::math::Vec3{0.0, 0.0, 0.0}}; }
inline Frame rot_x(double angle) { return Frame{rc::math::rotation_x(angle), rc::math::Vec3{0.0, 0.0, 0.0}}; }
inline Frame trans_z(double d) { return Frame{rc::math::Mat3(), rc::math::Vec3{0.0, 0.0, d}}; }
inline Frame trans_x(double a) { return Frame{rc::math::Mat3(), rc::math::Vec3{a, 0.0, 0.0}}; }

struct DhRow {
  double theta = 0.0;       // the angle about z: a fixed offset for a prismatic joint, and the offset of the zero for a revolute one
  double d = 0.0;           // the distance along z
  double a = 0.0;           // the distance along x
  double alpha = 0.0;       // the twist about x
  bool prismatic = false;   // the joint variable is added to d and not to theta
};

enum class Convention { kStandard, kModified };

// The frame of one joint relative to the one before it, for the joint variable q (an angle, or a distance if the joint is prismatic).
//   standard:  Rz(theta) Tz(d) Tx(a) Rx(alpha)
//   modified:  Rx(alpha) Tx(a) Rz(theta) Tz(d)
inline Frame dh_frame(const DhRow& row, double q, Convention convention) {
  const double theta = row.theta + (row.prismatic ? 0.0 : q);
  const double d = row.d + (row.prismatic ? q : 0.0);
  if (convention == Convention::kStandard) return compose(compose(compose(rot_z(theta), trans_z(d)), trans_x(row.a)), rot_x(row.alpha));
  return compose(compose(compose(rot_x(row.alpha), trans_x(row.a)), rot_z(theta)), trans_z(d));
}

// The frame of the tool relative to the base: the frames of the rows multiplied in order. A joint variable that is missing from `q` is 0.
inline Frame dh_chain(const std::vector<DhRow>& rows, const std::vector<double>& q, Convention convention) {
  Frame tool;
  for (std::size_t i = 0; i < rows.size(); ++i) tool = compose(tool, dh_frame(rows[i], i < q.size() ? q[i] : 0.0, convention));
  return tool;
}
}  // namespace kin
}  // namespace rc

#endif  // RC_KIN_DH_HPP
