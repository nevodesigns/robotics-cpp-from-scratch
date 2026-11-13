#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct PegHole {
  double clearance = 0.0001;   // m, the hole's diameter less the peg's
  double chamfer_w = 0.003;    // m, how far the chamfer reaches out from the hole
  double chamfer_angle = 0.7853981633974483;  // rad, the slope of the chamfer from the horizontal (45 degrees)
  double mu = 0.3;             // friction at the contact (typical, steel on steel with a little oil)
};

enum InsertResult { kInserted, kMissed, kJammed };

// Whether a peg on the chamfer can slide at all: the slope must be steeper than the friction.
inline bool can_slide(const PegHole& h) { return std::tan(h.chamfer_angle) > h.mu; }

// The push down that brings a peg offset by e (metres) in over the lip, for a hand of sideways stiffness k (N/m); 0 if it is already in, infinite if it cannot slide.
inline double push_to_centre(const PegHole& h, double e, double k) {
  (void)h;  // TODO: the offset beyond half the clearance, times k, over (tan of the angle less mu); 0 if it is within, a huge number if it cannot slide
  (void)e;
  (void)k;
  return 0.0;
}

// The outcome of pushing down with at most fmax on a hand of stiffness k with the peg offset by e.
inline InsertResult insert(const PegHole& h, double e, double k, double fmax) {
  if (std::fabs(e) > 0.5 * h.clearance + h.chamfer_w) return kMissed;
  return push_to_centre(h, e, k) <= fmax ? kInserted : kJammed;
}


#endif
