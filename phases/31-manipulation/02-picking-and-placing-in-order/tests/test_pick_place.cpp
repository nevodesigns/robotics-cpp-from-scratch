#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"ticks_ok", 36.0},
    {"miss1", 1.0},
    {"ticks_miss", 48.0},
    {"retries1", 1.0},
    {"f_miss", 1.0},
    {"retries3", 3.0},
    {"f_drop", 2.0},
    {"f_time", 3.0},
    {"t_time", 62.0},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

// A tiny world: the arm takes 3 ticks to arrive at a new target, the hand takes a tick to settle; the part can be missed, dropped or never reached.
struct World {
  int cmd_target = 0;
  int countdown = 0;
  bool closed = false;
  bool has_part = false;
  double width = 0.085;
  int misses = 0;           // closings on the part that find nothing
  int drop_after = -1;      // tick of the transit at which the part falls (-1: never)
  bool stuck = false;       // the arm never arrives
  int in_transit = 0;
  bool settled = false;

  PickIn sense() const {
    PickIn in;
    in.width = width;
    in.settled = settled;
    in.reached = (countdown == 0 && !stuck) ? cmd_target : 0;
    return in;
  }
  void apply(const PickCmd& c, PickState s) {
    if (c.target != cmd_target) {
      cmd_target = c.target;
      countdown = 3;
    } else if (countdown > 0) {
      --countdown;
    }
    if (c.close && !closed) {
      closed = true;
      settled = false;
      if (cmd_target == kAtPick && misses > 0) {
        --misses;
        has_part = false;
        width = 0.0;
      } else if (cmd_target == kAtPick) {
        has_part = true;
        width = 0.04;
      }
    } else if (!c.close && closed) {
      closed = false;
      settled = false;
      has_part = false;
      width = 0.085;
    } else {
      settled = true;
    }
    if (s == kPickTransit && has_part) {
      ++in_transit;
      if (drop_after >= 0 && in_transit >= drop_after) {
        has_part = false;
        width = 0.0;
      }
    }
  }
};

int run(PickMachine& m, World& w, int max_ticks, int* tick_fault = nullptr) {
  int t = 0;
  for (; t < max_ticks; ++t) {
    const PickCmd c = pick_step(m, w.sense());
    w.apply(c, m.s);
    if (m.s == kPickDone) return t + 1;
    if (m.s == kPickFault) {
      if (tick_fault) *tick_fault = t + 1;
      return t + 1;
    }
  }
  return t;
}

}  // namespace

RC_TEST("the grasp check is true for the part's width and false for a closed hand or a wrong part") {
  RC_CHECK(holding(0.040, 0.040, 0.006));
  RC_CHECK(holding(0.044, 0.040, 0.006));
  RC_CHECK(!holding(0.0, 0.040, 0.006));
  RC_CHECK(!holding(0.060, 0.040, 0.006));
}

RC_TEST("a pick and place that goes to plan ends in done with no retries") {
  PickMachine m;
  World w;
  const int ticks = run(m, w, 500);
  rec("ticks_ok", ticks, 0);
  RC_CHECK(m.s == kPickDone);
  RC_CHECK(m.retries == 0);
  RC_CHECK(m.fault == 0);
}

RC_TEST("a missed grasp backs off and tries again, and a hand that always misses stops with the reason") {
  PickMachine m;
  World w;
  w.misses = 1;
  const int ticks = run(m, w, 500);
  rec("miss1", 1, 0);
  rec("ticks_miss", ticks, 0);
  rec("retries1", m.retries, 0);
  RC_CHECK(m.s == kPickDone);
  RC_CHECK(m.retries == 1);
  PickMachine m3;
  World w3;
  w3.misses = 100;
  run(m3, w3, 1000);
  rec("f_miss", m3.fault, 0);
  rec("retries3", m3.retries, 0);
  RC_CHECK(m3.s == kPickFault);
  RC_CHECK(m3.fault == 1);
  RC_CHECK(m3.retries == m3.max_retries + 1);
}

RC_TEST("a part that falls on the way is seen from the gap, and a move that never arrives times out") {
  PickMachine m;
  World w;
  w.drop_after = 2;
  run(m, w, 500);
  rec("f_drop", m.fault, 0);
  RC_CHECK(m.s == kPickFault);
  RC_CHECK(m.fault == 2);
  PickMachine mt;
  World wt;
  wt.stuck = true;
  int tf = 0;
  run(mt, wt, 500, &tf);
  rec("f_time", mt.fault, 0);
  rec("t_time", tf, 0);
  RC_CHECK(mt.s == kPickFault);
  RC_CHECK(mt.fault == 3);
  RC_CHECK(tf <= mt.timeout + 3);
}
