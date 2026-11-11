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
  // TODO: x = a.x + cos(a.theta) * b.x - sin(a.theta) * b.y; y = a.y + sin(a.theta) * b.x + cos(a.theta) * b.y; theta = wrap_angle(a.theta + b.theta)
  (void)a;
  (void)b;
  return Transform2();
}

// The transform that undoes a: compose(inverse(a), a) is the identity.
inline Transform2 inverse(const Transform2& a) {
  // TODO: x = -(cos(a.theta) * a.x + sin(a.theta) * a.y); y = -(-sin(a.theta) * a.x + cos(a.theta) * a.y); theta = wrap_angle(-a.theta)
  (void)a;
  return Transform2();
}

// A point of the transformed frame, in the frame it hangs from.
inline void apply(const Transform2& t, double px, double py, double& ox, double& oy) {
  ox = t.x + std::cos(t.theta) * px - std::sin(t.theta) * py;
  oy = t.y + std::sin(t.theta) * px + std::cos(t.theta) * py;
}

// Between two transforms, `f` of the way from a (0) to b (1). The heading goes the short way round.
inline Transform2 blend(const Transform2& a, const Transform2& b, double f) {
  // TODO: x and y: a + (b - a) * f. theta: a.theta + wrap_angle(b.theta - a.theta) * f, then wrap_angle: the short way round
  (void)a;
  (void)b;
  (void)f;
  return Transform2();
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
    // TODO: unknown_frame if either name is not a frame (known() says). Find the chain from each frame up to its root (chain()) and the nearest frame on both
    // (not_connected if there is none). Work out where source is as seen from that common frame, and where target is, with path_to() (pass on any status that is not ok).
    // The answer is compose(inverse(target_from_common), source_from_common) with the two as seen from the common frame.
    (void)target;
    (void)source;
    (void)t;
    (void)out;
    return LookupStatus::unknown_frame;
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
    // TODO: no samples: not_connected. A static edge: its one transform at any time. Otherwise t before the first sample or after the last is `extrapolation`
    // (never guess outside what was reported). Between two samples: blend() from the earlier to the later, by (t - earlier time) / (the span between them).
    // Exactly one sample and t is its time: that transform.
    (void)e;
    (void)t;
    (void)out;
    return LookupStatus::extrapolation;
  }

  std::map<std::string, Edge> edges_;
  std::map<std::string, bool> parents_;
};

#endif  // LESSON_SOLUTION_HPP
