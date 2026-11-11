#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

// A position and a heading in a plane: where a frame is, as seen from the frame it hangs from.
struct Transform2 {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

inline double wrap_angle(double a) {
  const double pi = 3.14159265358979323846;
  while (a > pi) a -= 2.0 * pi;
  while (a <= -pi) a += 2.0 * pi;
  return a;
}

// First b, then a: the transform that does what b does, seen from the frame that a is in.
inline Transform2 compose(const Transform2& a, const Transform2& b) {
  Transform2 r;
  r.x = a.x + std::cos(a.theta) * b.x - std::sin(a.theta) * b.y;
  r.y = a.y + std::sin(a.theta) * b.x + std::cos(a.theta) * b.y;
  r.theta = wrap_angle(a.theta + b.theta);
  return r;
}

// The transform that undoes a: compose(inverse(a), a) is the identity.
inline Transform2 inverse(const Transform2& a) {
  Transform2 r;
  r.x = -(std::cos(a.theta) * a.x + std::sin(a.theta) * a.y);
  r.y = -(-std::sin(a.theta) * a.x + std::cos(a.theta) * a.y);
  r.theta = wrap_angle(-a.theta);
  return r;
}

// A point of the transformed frame, in the frame it hangs from.
inline void apply(const Transform2& t, double px, double py, double& ox, double& oy) {
  ox = t.x + std::cos(t.theta) * px - std::sin(t.theta) * py;
  oy = t.y + std::sin(t.theta) * px + std::cos(t.theta) * py;
}

// Between two transforms, `f` of the way from a (0) to b (1). The heading goes the short way round.
inline Transform2 blend(const Transform2& a, const Transform2& b, double f) {
  Transform2 r;
  r.x = a.x + (b.x - a.x) * f;
  r.y = a.y + (b.y - a.y) * f;
  r.theta = wrap_angle(a.theta + wrap_angle(b.theta - a.theta) * f);
  return r;
}

enum class LookupStatus { ok, unknown_frame, not_connected, extrapolation };

// A tree of frames. Each frame has one parent, and where it is relative to that parent at the times it was reported.
class FrameTree {
 public:
  // The frame `child` is at `where` relative to `parent` at time `t`. A frame may be given several times and is then known between the first and the last.
  void set(const std::string& child, const std::string& parent, double t, const Transform2& where) {
    Edge& e = edges_[child];
    e.parent = parent;
    e.is_static = false;
    e.samples.push_back({t, where});
    parents_[parent];                                     // so that the parent is known as a frame, even if it hangs from nothing
  }

  // A frame that does not move relative to its parent, such as a sensor bolted to the robot: known at every time.
  void set_static(const std::string& child, const std::string& parent, const Transform2& where) {
    Edge& e = edges_[child];
    e.parent = parent;
    e.is_static = true;
    e.samples.clear();
    e.samples.push_back({0.0, where});
    parents_[parent];
  }

  // The transform that takes a point written in `source` to the same point written in `target`, at time `t`.
  LookupStatus lookup(const std::string& target, const std::string& source, double t, Transform2& out) const {
    if (!known(target) || !known(source)) return LookupStatus::unknown_frame;
    std::vector<std::string> up_target = chain(target);
    std::vector<std::string> up_source = chain(source);
    // The nearest frame that both hang from.
    std::string common;
    bool found = false;
    for (const std::string& a : up_source) {
      for (const std::string& b : up_target)
        if (a == b) {
          common = a;
          found = true;
          break;
        }
      if (found) break;
    }
    if (!found) return LookupStatus::not_connected;

    Transform2 common_from_source;
    Transform2 common_from_target;
    LookupStatus s = path_to(source, common, t, common_from_source);
    if (s != LookupStatus::ok) return s;
    s = path_to(target, common, t, common_from_target);
    if (s != LookupStatus::ok) return s;
    out = compose(inverse(common_from_target), common_from_source);
    return LookupStatus::ok;
  }

 private:
  struct Sample {
    double t;
    Transform2 where;
  };
  struct Edge {
    std::string parent;
    bool is_static = false;
    std::vector<Sample> samples;
  };

  bool known(const std::string& name) const { return edges_.count(name) != 0 || parents_.count(name) != 0; }

  // The frame, its parent, its parent's parent, up to the root.
  std::vector<std::string> chain(const std::string& name) const {
    std::vector<std::string> out;
    std::string current = name;
    for (int guard = 0; guard < 1000; ++guard) {
      out.push_back(current);
      const auto found = edges_.find(current);
      if (found == edges_.end()) break;
      current = found->second.parent;
    }
    return out;
  }

  // Where `from` is as seen from `ancestor`, at time t, by composing each link on the way up.
  LookupStatus path_to(const std::string& from, const std::string& ancestor, double t, Transform2& out) const {
    Transform2 total;
    std::string current = from;
    for (int guard = 0; guard < 1000 && current != ancestor; ++guard) {
      const auto found = edges_.find(current);
      if (found == edges_.end()) return LookupStatus::not_connected;
      Transform2 link;
      const LookupStatus s = at_time(found->second, t, link);
      if (s != LookupStatus::ok) return s;
      total = compose(link, total);
      current = found->second.parent;
    }
    out = total;
    return LookupStatus::ok;
  }

  static LookupStatus at_time(const Edge& e, double t, Transform2& out) {
    if (e.samples.empty()) return LookupStatus::not_connected;
    if (e.is_static) {
      out = e.samples.front().where;
      return LookupStatus::ok;
    }
    if (t < e.samples.front().t || t > e.samples.back().t) return LookupStatus::extrapolation;
    for (std::size_t i = 1; i < e.samples.size(); ++i) {
      if (t <= e.samples[i].t) {
        const Sample& a = e.samples[i - 1];
        const Sample& b = e.samples[i];
        const double span = b.t - a.t;
        out = span > 0.0 ? blend(a.where, b.where, (t - a.t) / span) : b.where;
        return LookupStatus::ok;
      }
    }
    out = e.samples.back().where;                         // exactly one sample, and t is its time
    return LookupStatus::ok;
  }

  std::map<std::string, Edge> edges_;
  std::map<std::string, bool> parents_;
};

#endif  // LESSON_SOLUTION_HPP
