// rc/arm/scene_monitor.hpp
//
// A world that changes, from lesson 30-07.
//
// SceneObject is a sphere or an axis aligned box with an identifier, the time it was last seen and the speed it might move at (zero for the furniture). PlanningScene holds a list of them and a version that goes up with every addition, move or removal. scene_at gives the collision model of 30-01 at a time, with every object that is not fixed made bigger by the distance it could have moved since it was last seen, up to a cap, so that what has not been seen lately takes up more room. plan_is_stale says that a plan made at one version is out of date at the present one. held_arm makes the radii and the tool length that the planner should use when the gripper holds a part: the tool is longer by the part's length and the last link at least as fat.
//
// An object seen 2.0 s ago that might move at 0.5 m/s was 0.50 m bigger in the radius, and a pose that was 0.31 m clear of it was 0.11 m clear after that growth. Moving, adding and removing took the version of the scene from 0 to 4; a plan made at version 0 was stale after the first move, and when the object had been moved 0.5 m away the path was still free and when it had been moved onto the path it was not. A part 0.25 m long held in the gripper took a pose from 0.090 m clear to -0.090 m.

#ifndef RC_RC_ARM_SCENE_MONITOR_HPP
#define RC_RC_ARM_SCENE_MONITOR_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/arm/collision.hpp>
#include <vector>

namespace rc {
namespace arm {

struct SceneObject {
  int id = 0;
  int kind = 0;           // 0 a sphere, 1 an axis aligned box
  double c[3] = {0.0, 0.0, 0.0};
  double r = 0.0;         // the radius of a sphere
  double lo[3] = {0.0, 0.0, 0.0};
  double hi[3] = {0.0, 0.0, 0.0};
  double stamp = 0.0;     // when it was last seen, seconds
  double speed = 0.0;     // the speed it might move at, metres per second (zero for the furniture)
};

class PlanningScene {
 public:
  unsigned version() const { return version_; }
  const std::vector<SceneObject>& objects() const { return objects_; }

  void add(const SceneObject& o) {
    objects_.push_back(o);
    ++version_;
  }
  bool remove(int id) {
    for (std::size_t i = 0; i < objects_.size(); ++i)
      if (objects_[i].id == id) {
        objects_.erase(objects_.begin() + static_cast<long>(i));
        ++version_;
        return true;
      }
    return false;
  }
  // Moves an object by a vector and notes when it was seen.
  bool move(int id, const double by[3], double stamp) {
    for (std::size_t i = 0; i < objects_.size(); ++i)
      if (objects_[i].id == id) {
        for (int k = 0; k < 3; ++k) {
          objects_[i].c[k] += by[k];
          objects_[i].lo[k] += by[k];
          objects_[i].hi[k] += by[k];
        }
        objects_[i].stamp = stamp;
        ++version_;
        return true;
      }
    return false;
  }
  // Notes that an object was seen again, where it was.
  void seen(int id, double stamp) {
    for (std::size_t i = 0; i < objects_.size(); ++i)
      if (objects_[i].id == id) objects_[i].stamp = stamp;
  }

 private:
  std::vector<SceneObject> objects_;
  unsigned version_ = 0;
};

// The collision model at a time: an object that might have moved since it was last seen is bigger by the distance it could have gone, up to a cap.
inline Scene scene_at(const PlanningScene& ps, double now, double cap) {
  Scene out;
  for (std::size_t i = 0; i < ps.objects().size(); ++i) {
    const SceneObject& o = ps.objects()[i];
    double grow = 0.0;
    grow = o.speed * (now - o.stamp);
    if (grow < 0.0) grow = 0.0;
    if (grow > cap) grow = cap;
    if (o.kind == 0) {
      Sphere s;
      for (int k = 0; k < 3; ++k) s.c[k] = o.c[k];
      s.r = o.r + grow;
      out.spheres.push_back(s);
    } else {
      Box b;
      for (int k = 0; k < 3; ++k) {
        b.lo[k] = o.lo[k] - grow;
        b.hi[k] = o.hi[k] + grow;
      }
      out.boxes.push_back(b);
    }
  }
  return out;
}

inline bool plan_is_stale(unsigned plan_version, const PlanningScene& ps) { return ps.version() != plan_version; }

struct HeldArm {
  std::vector<double> radii;
  double tool_length = 0.0;
};

// What the planner should use when the gripper holds a part: the tool longer by the part's length, the last link at least as fat as the part.
inline HeldArm held_arm(const std::vector<double>& radii, double tool_length, double part_length, double part_radius) {
  HeldArm h;
  h.radii = radii;
  h.tool_length = tool_length + part_length;
  if (!h.radii.empty() && part_radius > h.radii.back()) h.radii.back() = part_radius;
  return h;
}


}  // namespace arm
}  // namespace rc

#endif
