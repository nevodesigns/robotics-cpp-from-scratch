#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

enum PickState { kPickIdle, kPickApproach, kPickDescend, kPickClose, kPickVerify, kPickLift, kPickTransit, kPickLower, kPickRelease, kPickRetreat, kPickDone, kPickFault };

constexpr int kAbovePick = 1;
constexpr int kAtPick = 2;
constexpr int kAbovePlace = 3;
constexpr int kAtPlace = 4;

struct PickIn {
  int reached = 0;       // the target the arm has arrived at, 0 while it moves
  double width = 0.085;  // the gap between the fingers, m
  bool settled = false;  // the hand has stopped moving
};

struct PickCmd {
  int target = 0;        // the target to go to, 0 for none (hold where it is)
  bool close = false;    // true to close the hand, false to open it
};

struct PickMachine {
  PickState s = kPickIdle;
  int retries = 0;
  int max_retries = 2;
  double obj_width = 0.04;  // the part's width, m
  double tol = 0.006;       // how far the gap may be from it, m
  int ticks = 0;            // ticks in this state
  int timeout = 60;         // ticks allowed in a state
  int fault = 0;            // 1 no part, 2 the part fell, 3 a step took too long
  bool closed = false;      // what the hand was last told
};

// Whether the gap is that of the part.
inline bool holding(double width, double obj_width, double tol) {
  (void)width;  // TODO: true if the gap is within tol of the part's width (a closed hand on nothing has a gap near 0)
  (void)obj_width;
  (void)tol;
  return true;
}

inline void enter(PickMachine& m, PickState s) {
  m.s = s;
  m.ticks = 0;
}

// One tick: what was sensed in, what to do next out.
inline PickCmd pick_step(PickMachine& m, const PickIn& in) {
  PickCmd c;
  ++m.ticks;
  if (m.s != kPickDone && m.s != kPickFault && m.ticks > m.timeout) {
    m.fault = 3;
    enter(m, kPickFault);
  }
  const bool carrying = m.s == kPickLift || m.s == kPickTransit || m.s == kPickLower;
  if (carrying && !holding(in.width, m.obj_width, m.tol)) {
    m.fault = 2;
    enter(m, kPickFault);
  }
  switch (m.s) {
    case kPickIdle:
      enter(m, kPickApproach);
      break;
    case kPickApproach:
      c.target = kAbovePick;
      c.close = false;
      if (in.reached == kAbovePick) enter(m, kPickDescend);
      break;
    case kPickDescend:
      c.target = kAtPick;
      c.close = false;
      if (in.reached == kAtPick) enter(m, kPickClose);
      break;
    case kPickClose:
      c.target = kAtPick;
      c.close = true;
      if (in.settled && in.width < 0.5 * (m.obj_width + 1.0)) enter(m, kPickVerify);
      break;
    case kPickVerify:
      c.target = kAtPick;
      c.close = true;
      if (holding(in.width, m.obj_width, m.tol)) {
        enter(m, kPickLift);
      } else {
        ++m.retries;
        if (m.retries > m.max_retries) {
          m.fault = 1;
          enter(m, kPickFault);
        } else {
          enter(m, kPickApproach);
        }
      }
      break;
    case kPickLift:
      c.target = kAbovePick;
      c.close = true;
      if (in.reached == kAbovePick) enter(m, kPickTransit);
      break;
    case kPickTransit:
      c.target = kAbovePlace;
      c.close = true;
      if (in.reached == kAbovePlace) enter(m, kPickLower);
      break;
    case kPickLower:
      c.target = kAtPlace;
      c.close = true;
      if (in.reached == kAtPlace) enter(m, kPickRelease);
      break;
    case kPickRelease:
      c.target = kAtPlace;
      c.close = false;
      if (in.settled && in.width > m.obj_width + m.tol) enter(m, kPickRetreat);
      break;
    case kPickRetreat:
      c.target = kAbovePlace;
      c.close = false;
      if (in.reached == kAbovePlace) enter(m, kPickDone);
      break;
    case kPickDone:
      break;
    case kPickFault:
      c.target = 0;
      c.close = m.closed;
      break;
  }
  m.closed = c.close;
  return c;
}


#endif
