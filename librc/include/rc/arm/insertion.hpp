// rc/arm/insertion.hpp
//
// Putting a peg in a hole, from lesson 31-05.
//
// A peg is a cylinder going into a hole with a clearance (the hole's diameter less the peg's, in metres) and a chamfer of a given width and angle. The sideways offset of the peg from the hole's axis is e. If e is within half the clearance the peg goes in; if it is beyond that and the chamfer's width it misses; in between it lands on the chamfer. There the push down presses on a slope and gives a sideways force that is the push times (tan of the angle less the friction coefficient), which a hand with sideways stiffness k gives way to by that force over k. can_slide is the friction cone test (tan of the angle over the friction). push_to_centre is the push down needed to bring the peg in over the lip, which is k times the offset beyond the clearance over that factor; it is infinite if the peg cannot slide. insert gives the outcome (in, missed or jammed) at a push limit.
//
// A peg with 100 micrometres of clearance and a chamfer 3 mm wide was 2 mm off. A hand with a sideways stiffness of 300 N/m needed 0.8 N to bring it in, one of 2000 N/m needed 5.6 N, and a stiff one of 100000 N/m needed 279 N, more than the 50 N the hand could give, so it jammed. At a friction of 1.2 the peg could not slide (0). Over offsets of 0 to 4 mm the soft hand got the peg in 13 times of 17, the stiff one 2.

#ifndef RC_RC_ARM_INSERTION_HPP
#define RC_RC_ARM_INSERTION_HPP

#include <cmath>

namespace rc {
namespace arm {

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
  const double over = std::fabs(e) - 0.5 * h.clearance;
  if (over <= 0.0) return 0.0;
  if (!can_slide(h)) return 1e18;
  return k * over / (std::tan(h.chamfer_angle) - h.mu);
}

// The outcome of pushing down with at most fmax on a hand of stiffness k with the peg offset by e.
inline InsertResult insert(const PegHole& h, double e, double k, double fmax) {
  if (std::fabs(e) > 0.5 * h.clearance + h.chamfer_w) return kMissed;
  return push_to_centre(h, e, k) <= fmax ? kInserted : kJammed;
}


}  // namespace arm
}  // namespace rc

#endif
