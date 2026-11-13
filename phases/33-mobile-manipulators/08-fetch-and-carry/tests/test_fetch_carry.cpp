#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

// A world: the base takes 15 ticks to drive between stations, the arm 5 ticks to go to a place; the hand takes a tick to settle.
struct World {
  int at_station = 0;
  int drive_target = 0;
  int drive_count = 0;
  int arm_goal = 1;
  int arm_count = 0;
  int arm_at = 1;
  bool closed = false;
  bool settled = true;
  double gap = 0.085;
  int misses = 0;
  int drop_after = -1;
  int carried = 0;
  double energy = 100.0;
  int unsafe = 0;   // ticks with the base moving and the arm not tucked, or the arm going out while the base moves

  MissionIn sense() const {
    MissionIn in;
    in.at_station = drive_count == 0 ? at_station : 0;
    in.base_stopped = drive_count == 0;
    in.arm_at = arm_count == 0 ? arm_at : 0;
    in.gap = gap;
    in.settled = settled;
    in.energy_wh = energy;
    return in;
  }
  void apply(const MissionCmd& c, MState s) {
    if (c.drive_to != 0 && c.drive_to != drive_target) {
      drive_target = c.drive_to;
      drive_count = 15;
      at_station = 0;
    } else if (drive_count > 0) {
      --drive_count;
      energy -= 0.15;
      if (drive_count == 0) at_station = drive_target;
    }
    if (c.arm_goal != 0 && c.arm_goal != arm_goal) {
      arm_goal = c.arm_goal;
      arm_count = 5;
    } else if (arm_count > 0) {
      --arm_count;
      if (arm_count == 0) arm_at = arm_goal;
    }
    if (drive_count > 0 && (arm_at != 1 || arm_goal != 1)) ++unsafe;
    if (c.close && !closed) {
      closed = true;
      settled = false;
      if (at_station == 1 && arm_at == 3 && misses > 0) {
        --misses;
        gap = 0.0;
      } else if (at_station == 1 && arm_at == 3) {
        gap = 0.065;
      }
    } else if (!c.close && closed) {
      closed = false;
      settled = false;
      gap = 0.085;
    } else {
      settled = true;
    }
    if (s == kMDrivePlace && closed && gap > 0.0 && gap < 0.08) {
      ++carried;
      if (drop_after >= 0 && carried >= drop_after) gap = 0.0;
    }
  }
};

int run(Mission& m, World& w, int max_ticks) {
  for (int t = 0; t < max_ticks; ++t) {
    const MissionCmd c = mission_step(m, w.sense());
    w.apply(c, m.s);
    if (m.s == kMDone || m.s == kMFault) return t + 1;
  }
  return max_ticks;
}

}  // namespace

RC_TEST("the interlock holds the base while the arm is out and the arm while the base moves") {
  MissionIn arm_out;
  arm_out.arm_at = 2;
  MissionCmd drive;
  drive.drive_to = 2;
  bool blocked = false;
  MissionCmd g = guard(drive, arm_out, &blocked);
  RC_CHECK(blocked && g.drive_to == 0);
  MissionIn tucked;
  g = guard(drive, tucked, &blocked);
  RC_CHECK(!blocked && g.drive_to == 2);
  MissionIn moving;
  moving.base_stopped = false;
  MissionCmd out;
  out.arm_goal = 2;
  g = guard(out, moving, &blocked);
  RC_CHECK(blocked && g.arm_goal == 0);
  MissionCmd stow;
  stow.arm_goal = 1;
  g = guard(stow, moving, &blocked);
  RC_CHECK(!blocked && g.arm_goal == 1);
}

RC_TEST("a mission that goes to plan ends in done with no retries and nothing unsafe") {
  Mission m;
  World w;
  const int ticks = run(m, w, 2000);
  rec("ticks_ok", ticks, 0);
  RC_CHECK(m.s == kMDone);
  RC_CHECK(m.retries == 0 && m.fault == 0);
  RC_CHECK(w.unsafe == 0);
  RC_CHECK(w.at_station == 2);
}

RC_TEST("a missed grasp is tried again and a hand that always misses stops with the reason") {
  Mission m;
  World w;
  w.misses = 1;
  const int ticks = run(m, w, 2000);
  rec("ticks_miss", ticks, 0);
  RC_CHECK(m.s == kMDone && m.retries == 1);
  Mission m3;
  World w3;
  w3.misses = 100;
  run(m3, w3, 4000);
  rec("f_miss", m3.fault, 0);
  rec("retries3", m3.retries, 0);
  RC_CHECK(m3.s == kMFault && m3.fault == 1 && m3.retries == m3.max_retries + 1);
}

RC_TEST("a dropped part, a low battery and a faulty plan are each stopped, and the interlock is what stops the last") {
  Mission md;
  World wd;
  wd.drop_after = 3;
  run(md, wd, 2000);
  rec("f_drop", md.fault, 0);
  RC_CHECK(md.s == kMFault && md.fault == 2);
  Mission mb;
  World wb;
  wb.energy = 4.5;
  run(mb, wb, 2000);
  rec("wh_low", 4.5, 1);
  rec("wh_need", mb.energy_needed, 1);
  rec("f_batt", mb.fault, 0);
  RC_CHECK(mb.s == kMFault && mb.fault == 4);
  Mission ms;
  ms.skip_stow = true;
  World ws;
  run(ms, ws, 2000);
  rec("f_skip", ms.fault, 0);
  rec("unsafe_guard", ws.unsafe, 0);
  RC_CHECK(ms.s == kMFault && ms.fault == 3);
  RC_CHECK(ws.unsafe == 0);
  Mission mn;
  mn.skip_stow = true;
  mn.use_guard = false;
  World wn;
  run(mn, wn, 2000);
  rec("unsafe_noguard", wn.unsafe, 0);
  RC_CHECK(wn.unsafe > 0);
}
