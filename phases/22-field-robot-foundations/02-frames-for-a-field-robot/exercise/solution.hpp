#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <map>
#include <set>
#include <string>

// A pose in the plane: where the child frame's origin is in its parent, and how much it is turned (radians).
struct Tf2 {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

// compose(a, b) is the pose of c in a's parent when b is the pose of c in a: b is applied first.
inline Tf2 compose(const Tf2& a, const Tf2& b) {
  const double c = std::cos(a.theta);
  const double s = std::sin(a.theta);
  Tf2 out;
  out.x = a.x + c * b.x - s * b.y;
  out.y = a.y + s * b.x + c * b.y;
  out.theta = std::atan2(std::sin(a.theta + b.theta), std::cos(a.theta + b.theta));
  return out;
}

inline Tf2 inverse(const Tf2& a) {
  const double c = std::cos(a.theta);
  const double s = std::sin(a.theta);
  Tf2 out;
  out.x = -(c * a.x + s * a.y);
  out.y = -(-s * a.x + c * a.y);
  out.theta = -a.theta;
  return out;
}

// A pose in space: a rotation matrix (row major) and a translation.
struct Tf3 {
  double r[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  double t[3] = {0, 0, 0};
};

struct Point3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

// The rotation of yaw about z, then pitch about y, then roll about x (the aircraft and vehicle order), with a translation. A positive pitch tips the nose down.
inline Tf3 from_rpy(double x, double y, double z, double roll, double pitch, double yaw) {
  const double cr = std::cos(roll), sr = std::sin(roll);
  const double cp = std::cos(pitch), sp = std::sin(pitch);
  const double cy = std::cos(yaw), sy = std::sin(yaw);
  Tf3 out;
  out.r[0] = cy * cp;
  out.r[1] = cy * sp * sr - sy * cr;
  out.r[2] = cy * sp * cr + sy * sr;
  out.r[3] = sy * cp;
  out.r[4] = sy * sp * sr + cy * cr;
  out.r[5] = sy * sp * cr - cy * sr;
  out.r[6] = -sp;
  out.r[7] = cp * sr;
  out.r[8] = cp * cr;
  out.t[0] = x;
  out.t[1] = y;
  out.t[2] = z;
  return out;
}

inline Point3 apply(const Tf3& a, const Point3& p) {
  Point3 q;
  q.x = a.r[0] * p.x + a.r[1] * p.y + a.r[2] * p.z + a.t[0];
  q.y = a.r[3] * p.x + a.r[4] * p.y + a.r[5] * p.z + a.t[1];
  q.z = a.r[6] * p.x + a.r[7] * p.y + a.r[8] * p.z + a.t[2];
  return q;
}

// compose(a, b) applies b first and then a.
inline Tf3 compose(const Tf3& a, const Tf3& b) {
  Tf3 out;
  (void)a;  // TODO: the rotation is a.r times b.r, and the translation is a.r times b.t plus a.t
  (void)b;
  return out;
}

inline Tf3 inverse(const Tf3& a) {
  Tf3 out;
  (void)a;  // TODO: the rotation is the transpose, and the translation is minus that times a.t
  return out;
}

// The frames of a robot, each given as its pose in its parent. between(from, to) is the transform that takes a point written in `from` and gives it in `to`.
class FrameTree {
 public:
  void add(const std::string& child, const std::string& parent, const Tf3& pose_in_parent) {
    Link link;
    link.parent = parent;
    link.pose = pose_in_parent;
    links_[child] = link;
    known_.insert(child);
    known_.insert(parent);
  }

  // Replaces the pose of one frame: the odometry frame moves under the map as the robot drives.
  bool update(const std::string& child, const Tf3& pose_in_parent) {
    auto it = links_.find(child);
    if (it == links_.end()) return false;
    it->second.pose = pose_in_parent;
    return true;
  }

  bool between(const std::string& from, const std::string& to, Tf3& out) const {
    if (known_.count(from) == 0 || known_.count(to) == 0) return false;
    Tf3 a;
    Tf3 b;
    std::string root_a;
    std::string root_b;
    if (!to_root(from, a, root_a) || !to_root(to, b, root_b)) return false;
    if (root_a != root_b) return false;
    out = compose(inverse(b), a);
    return true;
  }

 private:
  struct Link {
    std::string parent;
    Tf3 pose;
  };

  // The transform from `frame` to the top of its chain, and the name of the top. A chain that does not end within 64 steps has a loop in it.
  bool to_root(const std::string& frame, Tf3& out, std::string& root) const {
    Tf3 acc;
    std::string current = frame;
    for (int depth = 0; depth < 64; ++depth) {
      auto it = links_.find(current);
      if (it == links_.end()) {
        out = acc;
        root = current;
        return true;
      }
      acc = compose(it->second.pose, acc);
      current = it->second.parent;
    }
    return false;
  }

  std::map<std::string, Link> links_;
  std::set<std::string> known_;
};


#endif
