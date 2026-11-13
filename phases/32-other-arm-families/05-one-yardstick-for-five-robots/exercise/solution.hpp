#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/delta.hpp>
#include <rc/arm/gantry.hpp>
#include <rc/arm/ik_closed.hpp>
#include <rc/arm/reach3d.hpp>
#include <rc/arm/scara.hpp>
#include <rc/arm/stewart.hpp>

using namespace rc::arm;

struct ReachBox {
  double lo[3];
  double hi[3];
  double volume() const { return (hi[0] - lo[0]) * (hi[1] - lo[1]) * (hi[2] - lo[2]); }
};

// The one question put to every robot.
class ReachTest {
 public:
  virtual ~ReachTest() {}
  virtual bool reaches(double x, double y, double z) const = 0;
};

// The volume of the points that answer yes, by random sampling of the box; `se` gets the standard error.
inline double workspace_volume(const ReachTest& r, const ReachBox& b, int n, unsigned long long seed, double* se) {
  (void)r;  // TODO: draw n random points in the box (a generator of your own from the seed), count the ones for which reaches is true, and return the box's volume times that share; the standard error is the box's volume times sqrt(p (1 - p) / n)
  (void)b;
  (void)n;
  (void)seed;
  if (se) *se = 0.0;
  return 0.0;
}

class ScaraReach : public ReachTest {
 public:
  ScaraReach(const Scara& s, double z0) : s_(s), z0_(z0) {}
  bool reaches(double x, double y, double z) const override {
    if (z < z0_ || z > z0_ + s_.z_stroke) return false;
    double q1 = 0.0, q2 = 0.0;
    return scara_ik(s_, x, y, 1, q1, q2) || scara_ik(s_, x, y, -1, q1, q2);
  }

 private:
  Scara s_;
  double z0_;
};

class DeltaReach : public ReachTest {
 public:
  explicit DeltaReach(const Delta& d) : d_(d) {}
  bool reaches(double x, double y, double z) const override {
    double th[3];
    return delta_ik(d_, x, y, z, th);
  }

 private:
  Delta d_;
};

class StewartReach : public ReachTest {
 public:
  StewartReach(const Stewart& s, double lmin, double lmax) : s_(s), lmin_(lmin), lmax_(lmax) {}
  bool reaches(double x, double y, double z) const override {
    StewartPose p;
    p.v[0] = x;
    p.v[1] = y;
    p.v[2] = z;
    double l[6];
    stewart_lengths(s_, p, l);
    for (int i = 0; i < 6; ++i)
      if (l[i] < lmin_ || l[i] > lmax_) return false;
    return true;
  }

 private:
  Stewart s_;
  double lmin_, lmax_;
};

class GantryReach : public ReachTest {
 public:
  GantryReach(const double lo[3], const double hi[3]) {
    for (int k = 0; k < 3; ++k) {
      lo_[k] = lo[k];
      hi_[k] = hi[k];
    }
  }
  bool reaches(double x, double y, double z) const override {
    const double p[3] = {x, y, z};
    for (int k = 0; k < 3; ++k)
      if (p[k] < lo_[k] || p[k] > hi_[k]) return false;
    return true;
  }

 private:
  double lo_[3], hi_[3];
};

class ArmReach : public ReachTest {
 public:
  ArmReach(const ArmTable& t, const Pose& tool) : t_(t), tool_(tool) {}
  bool reaches(double x, double y, double z) const override { return tool_down_reachable(t_, x, y, z, tool_); }

 private:
  ArmTable t_;
  Pose tool_;
};

// The share of the room that a machine takes which is its reach.
inline double room_ratio(double reach_volume, double room_volume) { return room_volume > 0.0 ? reach_volume / room_volume : 0.0; }


#endif
