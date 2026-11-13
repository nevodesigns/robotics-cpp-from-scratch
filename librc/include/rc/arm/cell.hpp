// rc/arm/cell.hpp
//
// Choosing the robot for the job, from lesson 32-06. Every figure for a family is typical and not from one maker.
//
// TaskSpec is the job as numbers: the half widths of the rectangle the work lies in (x, y), the payload, whether the tool must tilt, the accuracy needed (millimetres), the cycle wanted (seconds), and the lengths of the short and the long moves, and the time the hand takes. FamilyData is what is known of a kind of robot: payload, speed, acceleration, repeatability, whether it tilts, the height it works at and how far in x it is mounted from the middle of the work (a SCARA cannot reach under itself, so it is mounted beside the work). Robots holds the reach test of each family (32-05). check_family says, for one family and one task, whether it fails the area (the share of a grid of the work that its reach test reaches), the payload, the tilt, the accuracy and the cycle (from the cycle time of 31-07 with the family's speed and acceleration), and gives the cycle it would take. choose_family gives the family with the shortest cycle among those that fail nothing, or minus one if none passes.
//
// The families are numbered 0 SCARA, 1 delta, 2 Stewart platform, 3 gantry and 4 six axis arm. Four jobs were given to the five families. Small light parts, fast, with a vacuum cup, went to family 1 with a cycle of 0.81 s; a heavier part turned over and inserted went to family 4 (2.71 s); optics held and tilted to a hundredth of a millimetre went to family 2 (2.0 s); and a wide table of boxes to family 3 (2.82 s). A job of 500 kg gave -1 (none). A SCARA mounted over the middle of the work reached 0.48 of it and beside it 1.00. On the wide table the delta reached 0.33 of the work, the Stewart platform 0.05 and the arm 0.89.

#ifndef RC_RC_ARM_CELL_HPP
#define RC_RC_ARM_CELL_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/compare.hpp>
#include <rc/arm/cycle_time.hpp>

namespace rc {
namespace arm {

struct TaskSpec {
  double half_x = 0.2;      // m, the work lies in a rectangle of half widths half_x and half_y
  double half_y = 0.15;
  double payload = 0.5;     // kg
  bool needs_tilt = false;
  double accuracy_mm = 0.2; // the repeatability needed
  double cycle_s = 2.0;     // the cycle time wanted
  double d_app = 0.05;      // m, the short moves
  double d_transit = 0.30;  // m, the long moves
  double dwell_s = 0.8;     // s, the hand closing and opening and settling (a vacuum cup takes much less than a pinching hand)
};

struct FamilyData {
  const char* name = "";
  int kind = 0;             // 0 SCARA, 1 delta, 2 Stewart, 3 gantry, 4 six axis arm
  double payload = 1.0;     // kg (typical)
  double v = 1.0;           // m/s (typical)
  double a = 5.0;           // m/s2 (typical)
  double repeat_mm = 0.1;   // typical
  bool tilt = false;
  double z_work = 0.0;      // m, the height the robot works at, in its own frame
  double mount_x = 0.0;     // m, how far in x the middle of the work is from the robot's axis
};

struct Verdict {
  bool area_ok = false;
  bool payload_ok = false;
  bool tilt_ok = false;
  bool accuracy_ok = false;
  bool cycle_ok = false;
  double cycle_s = 0.0;
  double coverage = 0.0;    // the share of the work's grid that the reach test reaches
  bool all_ok() const { return area_ok && payload_ok && tilt_ok && accuracy_ok && cycle_ok; }
};

// The reach tests of the five families.
class Robots {
 public:
  Robots() : scara_(Scara(), 0.0), delta_(Delta()), stewart_(Stewart(), 0.28, 0.44), gantry_(glo(), ghi()), arm_(elbow_arm(), tool()) {}
  const ReachTest& get(int kind) const {
    switch (kind) {
      case 0: return scara_;
      case 1: return delta_;
      case 2: return stewart_;
      case 3: return gantry_;
      default: return arm_;
    }
  }

 private:
  static const double* glo() {
    static const double v[3] = {-0.5, -0.4, 0.0};
    return v;
  }
  static const double* ghi() {
    static const double v[3] = {0.5, 0.4, 0.15};
    return v;
  }
  static Pose tool() {
    Pose p;
    p.p[2] = 0.10;
    return p;
  }
  ScaraReach scara_;
  DeltaReach delta_;
  StewartReach stewart_;
  GantryReach gantry_;
  ArmReach arm_;
};

// The five families with typical figures.
inline void typical_families(FamilyData out[5]) {
  const FamilyData f[5] = {
      {"SCARA", 0, 3.0, 4.0, 30.0, 0.02, false, 0.05, 0.28},
      {"delta", 1, 1.0, 6.0, 80.0, 0.10, false, -0.40, 0.0},
      {"Stewart", 2, 20.0, 0.3, 3.0, 0.01, true, 0.34, 0.0},
      {"gantry", 3, 10.0, 2.0, 10.0, 0.05, false, 0.05, 0.0},
      {"six axis arm", 4, 5.0, 1.5, 8.0, 0.05, true, 0.20, 0.30}};
  for (int i = 0; i < 5; ++i) out[i] = f[i];
}

// One family against one task: which requirements it fails, and the cycle it would take.
inline Verdict check_family(const Robots& r, const FamilyData& f, const TaskSpec& t) {
  Verdict v;
  // the share of a grid of the work (9 by 7 points) that the family's reach test reaches
  int hit = 0, n = 0;
  for (int i = 0; i < 9; ++i)
    for (int j = 0; j < 7; ++j) {
      const double x = f.mount_x + (-t.half_x + 2.0 * t.half_x * i / 8.0);
      const double y = -t.half_y + 2.0 * t.half_y * j / 6.0;
      ++n;
      if (r.get(f.kind).reaches(x, y, f.z_work)) ++hit;
    }
  v.coverage = static_cast<double>(hit) / static_cast<double>(n);
  v.area_ok = hit == n;
  v.payload_ok = t.payload <= f.payload;
  v.tilt_ok = !t.needs_tilt || f.tilt;
  v.accuracy_ok = f.repeat_mm <= t.accuracy_mm;
  CycleParams c;
  c.d_app = t.d_app;
  c.d_transit = t.d_transit;
  c.v = f.v;
  c.a = f.a;
  c.close_s = 0.375 * t.dwell_s;
  c.open_s = 0.375 * t.dwell_s;
  c.settle_s = 0.125 * t.dwell_s;
  v.cycle_s = cycle_time(c, 1.0);
  v.cycle_ok = v.cycle_s <= t.cycle_s;
  return v;
}

// The index of the family with the shortest cycle among those that fail nothing, or -1; the verdicts are filled in.
inline int choose_family(const Robots& r, const FamilyData* fam, int n, const TaskSpec& t, Verdict* out) {
  int best = -1;
  for (int i = 0; i < n; ++i) {
    out[i] = check_family(r, fam[i], t);
    if (out[i].all_ok() && (best < 0 || out[i].cycle_s < out[best].cycle_s)) best = i;
  }
  return best;
}


}  // namespace arm
}  // namespace rc

#endif
