#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/arm/collision.hpp>
#include <vector>

using namespace rc::arm;

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
    (void)cap;  // TODO: the growth is the speed times the time since it was last seen, never less than zero and never more than the cap
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


#endif
