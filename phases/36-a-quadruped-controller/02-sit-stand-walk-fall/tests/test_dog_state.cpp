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
    {"v_walk", 0.6},
    {"n_states", 6.0},
    {"t_rise", 0.9000000000000006},
    {"hot", 0.85},
    {"refuse_hot", 1.0},
    {"refuse_batt", 1.0},
    {"f_link", 1.0},
    {"f_hot", 2.0},
    {"f_time", 4.0},
    {"tries", 3.0},
    {"f_tries", 5.0},
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

// A world: the legs rise at 0.2 m/s from a sit at 0.10 m to a stand at 0.28 m and lower at the same rate; a walk slows at 1 m/s2.
struct World {
  double h = 0.10;
  double v = 0.0;
  bool rises_forever_stuck = false;
};

DogIn sense(const World& w, DogIn base) {
  base.stood = w.h >= 0.279;
  base.sat = w.h <= 0.101;
  base.stopped = std::fabs(w.v) < 0.02;
  return base;
}

void advance(World& w, const Dog& d, const DogIn& in, double dt) {
  if (d.s == kDRising && !w.rises_forever_stuck) w.h = std::fmin(0.28, w.h + 0.2 * dt);
  if (d.s == kDLowering) w.h = std::fmax(0.10, w.h - 0.2 * dt);
  const double target = d.s == kDWalk && in.want == kWantWalk ? in.cmd_speed : 0.0;
  if (w.v < target) w.v = std::fmin(target, w.v + 1.0 * dt);
  if (w.v > target) w.v = std::fmax(target, w.v - 1.0 * dt);
}

// Runs for `secs`, recording the order of states entered.
int run(Dog& d, World& w, DogIn in, double secs, DogState* order, int cap, double* t_rise = nullptr) {
  const double dt = 0.01;
  int n = 0;
  DogState prev = d.s;
  double t0 = -1.0;
  for (double t = 0.0; t < secs; t += dt) {
    const DogIn s = sense(w, in);
    dog_step(d, s, dt);
    advance(w, d, in, dt);
    if (d.s != prev) {
      if (n < cap) order[n] = d.s;
      ++n;
      if (d.s == kDRising) t0 = t;
      if (d.s == kDStand && t_rise && t0 >= 0.0) *t_rise = t - t0;
      prev = d.s;
    }
  }
  return n;
}

}  // namespace

RC_TEST("a day of sit, stand, walk, stand and sit goes through the states in order, and a walk is stopped before it is lowered") {
  Dog d;
  World w;
  DogIn in;
  DogState order[16];
  int n = 0;
  double t_rise = 0.0;
  in.want = kWantStand;
  n += run(d, w, in, 3.0, order + n, 16 - n, &t_rise);
  RC_CHECK(d.s == kDStand);
  in.want = kWantWalk;
  in.cmd_speed = 0.6;
  n += run(d, w, in, 2.0, order + n, 16 - n);
  RC_CHECK(d.s == kDWalk);
  rec("v_walk", w.v, 1);
  in.want = kWantSit;
  in.cmd_speed = 0.0;
  // told to sit while walking: it slows first
  Dog probe = d;
  World wp = w;
  DogIn dummy = sense(wp, in);
  dog_step(probe, dummy, 0.01);
  RC_CHECK(probe.s == kDWalk);       // not stopped yet: still walking
  n += run(d, w, in, 4.0, order + n, 16 - n);
  RC_CHECK(d.s == kDSit);
  rec("n_states", n, 0);
  rec("t_rise", t_rise, 1);
  const DogState expect[6] = {kDRising, kDStand, kDWalk, kDStand, kDLowering, kDSit};
  RC_CHECK(n == 6);
  for (int i = 0; i < 6; ++i) RC_CHECK(order[i] == expect[i]);
  RC_CHECK_NEAR(t_rise, 0.18 / 0.2, 0.05);
}

RC_TEST("it refuses to rise hot or low, sits down when the battery runs out, and lowers rather than walking on") {
  Dog d;
  World w;
  DogIn in;
  in.want = kWantStand;
  in.heat = 0.85;
  DogState o[4];
  run(d, w, in, 1.0, o, 4);
  rec("hot", 0.85, 2);
  rec("refuse_hot", d.s == kDSit ? 1 : 0, 0);
  RC_CHECK(d.s == kDSit);
  Dog d2;
  World w2;
  DogIn low;
  low.want = kWantStand;
  low.battery_ok = false;
  run(d2, w2, low, 1.0, o, 4);
  rec("refuse_batt", d2.s == kDSit ? 1 : 0, 0);
  RC_CHECK(d2.s == kDSit);
  // standing, the battery goes: it lowers
  Dog d3;
  World w3;
  DogIn ok;
  ok.want = kWantStand;
  run(d3, w3, ok, 3.0, o, 4);
  RC_CHECK(d3.s == kDStand);
  ok.battery_ok = false;
  run(d3, w3, ok, 3.0, o, 4);
  RC_CHECK(d3.s == kDSit);
}

RC_TEST("a lost link, hot knees, a move that never finishes and too many falls are faults that only a reset clears") {
  DogState o[16];
  {
    Dog d;
    World w;
    DogIn in;
    in.want = kWantWalk;
    in.cmd_speed = 0.5;
    run(d, w, in, 4.0, o, 16);
    RC_CHECK(d.s == kDWalk);
    in.link_ok = false;
    run(d, w, in, 0.1, o, 16);
    rec("f_link", d.fault, 0);
    RC_CHECK(d.s == kDFault && d.fault == 1);
    in.link_ok = true;
    run(d, w, in, 1.0, o, 16);
    RC_CHECK(d.s == kDFault);
    in.reset = true;
    in.want = kWantSit;
    run(d, w, in, 0.05, o, 16);
    RC_CHECK(d.s == kDSit && d.fault == 0);
  }
  {
    Dog d;
    World w;
    DogIn in;
    in.want = kWantWalk;
    in.cmd_speed = 0.5;
    run(d, w, in, 4.0, o, 16);
    in.heat = 1.05;
    run(d, w, in, 0.1, o, 16);
    rec("f_hot", d.fault, 0);
    RC_CHECK(d.s == kDFault && d.fault == 2);
  }
  {
    Dog d;
    World w;
    w.rises_forever_stuck = true;
    DogIn in;
    in.want = kWantStand;
    run(d, w, in, 6.0, o, 16);
    rec("f_time", d.fault, 0);
    RC_CHECK(d.s == kDFault && d.fault == 4);
  }
  {
    Dog d;
    World w;
    DogIn in;
    in.want = kWantStand;
    run(d, w, in, 3.0, o, 16);
    // it falls again and again: each time it is told fallen it tries to rise
    int falls = 0;
    for (int k = 0; k < 6 && d.s != kDFault; ++k) {
      DogIn f = in;
      f.fall = kFallen;
      f.stood = false;
      dog_step(d, f, 0.01);
      ++falls;
      DogIn g = in;
      g.stood = false;
      dog_step(d, g, 0.01);   // fallen -> rising (if tries left)
      d.s = d.s == kDRising ? kDStand : d.s;   // it stood again for a moment before falling
    }
    rec("tries", d.max_tries, 0);
    rec("f_tries", d.fault, 0);
    RC_CHECK(d.fault == 5);
    RC_CHECK(falls >= d.max_tries);
  }
}

RC_TEST("walking needs both the wish and a speed, and getting up needs both the wish and the conditions") {
  DogIn in;
  in.stood = true;
  in.sat = false;
  Dog a;
  a.s = kDStand;
  in.want = kWantWalk;
  in.cmd_speed = 0.0;
  dog_step(a, in, 0.01);
  RC_CHECK(a.s == kDStand);   // asked to walk at no speed: it stands
  in.want = kWantStand;
  in.cmd_speed = 0.5;
  dog_step(a, in, 0.01);
  RC_CHECK(a.s == kDStand);   // a speed without the wish to walk: it stands
  in.want = kWantWalk;
  dog_step(a, in, 0.01);
  RC_CHECK(a.s == kDWalk);
  Dog b;
  b.s = kDFallen;
  DogIn down;
  down.want = kWantSit;
  dog_step(b, down, 0.01);
  RC_CHECK(b.s == kDFallen);  // nobody asked it to get up
  down.want = kWantStand;
  down.battery_ok = false;
  dog_step(b, down, 0.01);
  RC_CHECK(b.s == kDFallen);  // asked, but the battery says no
  down.battery_ok = true;
  dog_step(b, down, 0.01);
  RC_CHECK(b.s == kDRising);
  RC_CHECK(b.tries == 1);
}
