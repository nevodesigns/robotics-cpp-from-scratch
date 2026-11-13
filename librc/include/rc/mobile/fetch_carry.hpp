// rc/mobile/fetch_carry.hpp
//
// Fetch and carry, from lesson 33-08 (it extends 21-07 and 31-02).
//
// Mission is a state machine for one fetch and one carry. It is given what was sensed (MissionIn: the station the base is at and whether it has stopped, where the arm is (1 tucked, 2 out above the work, 3 down at it, 0 moving), the gap between the fingers, whether the hand has settled, the energy left) and gives what to do next (MissionCmd: a station to drive to, a place for the arm to go to and whether the hand is to close). The steps are check, drive to the pick station, deploy, descend, close, verify, lift, stow, drive to the place station, deploy, lower, release, retreat, stow and done. The grasp is checked as in 31-02 (the gap is the part's width). guard is the interlock applied to every command: the base may be told to drive only when the arm is tucked, and the arm to go anywhere but tucked only when the base has stopped. A fault says why the machine stopped: 1 no part after the retries, 2 the part fell on the way, 3 a step took too long, 4 the battery would not last.
//
// A mission that went to plan took 97 ticks. A hand that missed once and tried again took 113, and one that always missed stopped in fault 1 after 3 tries. A part dropped while driving was seen at once (fault 2), a battery of 4.5 Wh for a mission needing 4.0 stopped it at the start (fault 4), and a faulty plan that tried to drive with the arm out was held back by the interlock and ended in fault 3 (a timeout) with 0 unsafe ticks, against 15 without it.

#ifndef RC_RC_MOBILE_FETCH_CARRY_HPP
#define RC_RC_MOBILE_FETCH_CARRY_HPP

#include <cmath>
#include <rc/arm/pick_place.hpp>

namespace rc {
namespace mobile {

using rc::arm::holding;

enum MState { kMIdle, kMCheck, kMDrivePick, kMDeploy, kMDescend, kMClose, kMVerify, kMLift, kMStow, kMDrivePlace, kMDeploy2, kMLower, kMRelease, kMRetreat, kMStow2, kMDone, kMFault };

struct MissionIn {
  int at_station = 0;       // the station the base has arrived at, 0 while driving or away
  bool base_stopped = true;
  int arm_at = 1;           // 1 tucked, 2 out above the work, 3 down at it, 0 moving
  double gap = 0.085;       // the gap between the fingers, m
  bool settled = false;     // the hand has stopped moving
  double energy_wh = 100.0;
};

struct MissionCmd {
  int drive_to = 0;         // a station (1 pick, 2 place), 0 for none
  int arm_goal = 0;         // 1 tucked, 2 out above, 3 down, 0 hold
  bool close = false;
};

struct Mission {
  MState s = kMIdle;
  int retries = 0;
  int max_retries = 2;
  double obj_width = 0.065;
  double tol = 0.006;
  int ticks = 0;
  int timeout = 120;
  int fault = 0;            // 1 no part, 2 the part fell, 3 a step took too long, 4 the battery
  bool closed = false;
  double energy_needed = 4.0;  // Wh for the mission
  bool skip_stow = false;      // a faulty plan, for testing: drive with the arm out
  bool use_guard = true;
};

// The interlock: the base drives only with the arm tucked; the arm goes out or down only with the base stopped. `blocked` says whether anything was held back.
inline MissionCmd guard(const MissionCmd& want, const MissionIn& in, bool* blocked) {
  MissionCmd c = want;
  bool b = false;
  if (c.drive_to != 0 && in.arm_at != 1) {
    c.drive_to = 0;
    b = true;
  }
  if (c.arm_goal > 1 && !in.base_stopped) {
    c.arm_goal = 0;
    b = true;
  }
  if (blocked) *blocked = b;
  return c;
}

inline void enter(Mission& m, MState s) {
  m.s = s;
  m.ticks = 0;
}

// One tick: what was sensed in, what to do next out.
inline MissionCmd mission_step(Mission& m, const MissionIn& in) {
  MissionCmd c;
  ++m.ticks;
  if (m.s != kMDone && m.s != kMFault && m.ticks > m.timeout) {
    m.fault = 3;
    enter(m, kMFault);
  }
  const bool carrying = m.s == kMStow || m.s == kMDrivePlace || m.s == kMDeploy2 || m.s == kMLower;
  if (carrying && !holding(in.gap, m.obj_width, m.tol)) {
    m.fault = 2;
    enter(m, kMFault);
  }
  switch (m.s) {
    case kMIdle:
      enter(m, kMCheck);
      break;
    case kMCheck:
      if (in.energy_wh < 1.2 * m.energy_needed) {
        m.fault = 4;
        enter(m, kMFault);
      } else {
        enter(m, kMDrivePick);
      }
      break;
    case kMDrivePick:
      c.drive_to = 1;
      c.arm_goal = 1;
      if (in.at_station == 1 && in.base_stopped && in.arm_at == 1) enter(m, kMDeploy);
      break;
    case kMDeploy:
      c.arm_goal = 2;
      if (in.arm_at == 2) enter(m, kMDescend);
      break;
    case kMDescend:
      c.arm_goal = 3;
      if (in.arm_at == 3) enter(m, kMClose);
      break;
    case kMClose:
      c.arm_goal = 3;
      c.close = true;
      if (in.settled && in.gap < 0.04 + m.obj_width) enter(m, kMVerify);
      break;
    case kMVerify:
      c.arm_goal = 3;
      c.close = true;
      if (holding(in.gap, m.obj_width, m.tol)) {
        enter(m, kMLift);
      } else {
        ++m.retries;
        if (m.retries > m.max_retries) {
          m.fault = 1;
          enter(m, kMFault);
        } else {
          enter(m, kMDeploy);
        }
      }
      break;
    case kMLift:
      c.arm_goal = 2;
      c.close = true;
      if (in.arm_at == 2) enter(m, m.skip_stow ? kMDrivePlace : kMStow);
      break;
    case kMStow:
      c.arm_goal = 1;
      c.close = true;
      if (in.arm_at == 1) enter(m, kMDrivePlace);
      break;
    case kMDrivePlace:
      c.drive_to = 2;
      c.arm_goal = m.skip_stow ? 0 : 1;
      c.close = true;
      if (in.at_station == 2 && in.base_stopped && in.arm_at == 1) enter(m, kMDeploy2);
      break;
    case kMDeploy2:
      c.arm_goal = 2;
      c.close = true;
      if (in.arm_at == 2) enter(m, kMLower);
      break;
    case kMLower:
      c.arm_goal = 3;
      c.close = true;
      if (in.arm_at == 3) enter(m, kMRelease);
      break;
    case kMRelease:
      c.arm_goal = 3;
      c.close = false;
      if (in.settled && in.gap > m.obj_width + m.tol) enter(m, kMRetreat);
      break;
    case kMRetreat:
      c.arm_goal = 2;
      if (in.arm_at == 2) enter(m, kMStow2);
      break;
    case kMStow2:
      c.arm_goal = 1;
      if (in.arm_at == 1) enter(m, kMDone);
      break;
    case kMDone:
      break;
    case kMFault:
      c.close = m.closed;
      break;
  }
  m.closed = c.close;
  if (m.use_guard) {
    bool blocked = false;
    c = guard(c, in, &blocked);
  }
  return c;
}


}  // namespace mobile
}  // namespace rc

#endif
