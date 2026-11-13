// rc/arm/fk.hpp
//
// Where the tool is, from lesson 28-02.
//
// row_pose reads one row of an ArmTable for a joint angle: the angle plus the row's offset, in the table's convention. link_frames gives the frame after each joint, from the base; forward gives the tool: the base, then every row, then a tool offset (the gripper's tip is not the flange). within_limits says whether every joint is inside the interval of its row. rpy_from_rotation reads a rotation as roll about x, pitch about y and yaw about z (applied in the order z, y, x), and where the pitch is a quarter turn, where roll and yaw are not separate, it sets the yaw to zero and gives all of it to the roll. rotation_from_rpy goes back.
//
// With every joint at zero the UR5's tool was at (-0.81725, -0.19145, -0.00549) m, the sums of its link lengths and offsets; the Panda's flange, with the 0.107 m of the flange as a tool offset, was at (0.088, -0.000, 0.926) m, and that zero pose is 0 (1 is inside the limits, 0 outside: joint 4 may not be zero). Turning the first joint turned the tool about the base axis to 0.000000000000 m. A tool offset of 0.15 m moved the tool 0.150 m. Two rotations 0.0001 rad of pitch apart, near a pitch of 90 degrees, differed by 57 degrees in roll.

#ifndef RC_RC_ARM_FK_HPP
#define RC_RC_ARM_FK_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/frames.hpp>
#include <vector>

namespace rc {
namespace arm {

// One row of the table for a joint angle: the angle plus the row's offset, in the table's convention.
inline Pose row_pose(const ArmTable& t, std::size_t i, double q) {
  const JointRow& r = t.rows[i];
  const double theta = q + r.theta_offset;
  return t.convention == kStandard ? dh_standard(r.a, r.alpha, r.d, theta) : dh_modified(r.a, r.alpha, r.d, theta);
}

// The frame after each joint, from the base.
inline std::vector<Pose> link_frames(const ArmTable& t, const std::vector<double>& q, const Pose& base) {
  std::vector<Pose> out;
  Pose acc = base;
  for (std::size_t i = 0; i < t.rows.size(); ++i) {
    acc = compose(acc, row_pose(t, i, q[i]));
    out.push_back(acc);
  }
  return out;
}

// The tool: the base, every row, then the tool offset.
inline Pose forward(const ArmTable& t, const std::vector<double>& q, const Pose& base, const Pose& tool) {
  Pose acc = base;
  for (std::size_t i = 0; i < t.rows.size(); ++i) acc = compose(acc, row_pose(t, i, q[i]));
  acc = compose(acc, tool);
  return acc;
}

// True if every joint is inside the interval of its row (a joint angle that is not a number is outside).
inline bool within_limits(const ArmTable& t, const std::vector<double>& q) {
  for (std::size_t i = 0; i < t.rows.size(); ++i)
    if (!(q[i] >= t.rows[i].lo && q[i] <= t.rows[i].hi)) return false;
  return true;
}

struct Rpy {
  double roll = 0.0;
  double pitch = 0.0;
  double yaw = 0.0;
};

// R = Rz(yaw) Ry(pitch) Rx(roll). At a pitch of a quarter turn, yaw is set to zero and the roll takes the rest.
inline Rpy rpy_from_rotation(const Pose& a) {
  Rpy out;
  out.pitch = -std::asin(std::fmax(-1.0, std::fmin(1.0, a.r[2][0])));
  if (std::fabs(a.r[2][0]) < 1.0 - 1e-12) {
    out.roll = std::atan2(a.r[2][1], a.r[2][2]);
    out.yaw = std::atan2(a.r[1][0], a.r[0][0]);
  } else {
    out.yaw = 0.0;
    const double s = a.r[2][0] < 0.0 ? 1.0 : -1.0;
    out.roll = std::atan2(s * a.r[0][1], s * a.r[0][2]);
  }
  return out;
}

inline Pose rotation_from_rpy(const Rpy& e) { return compose(rot_z(e.yaw), compose(rot_y(e.pitch), rot_x(e.roll))); }


}  // namespace arm
}  // namespace rc

#endif
