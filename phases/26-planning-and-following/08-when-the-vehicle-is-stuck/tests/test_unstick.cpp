#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

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
    {"t_snag", 23.4500000000002},
    {"att_snag", 2.0},
    {"t_limit", 120.0},
    {"att_wall", 3.0},
    {"t_giveup", 31.20000000000031},
    {"false_stop", 0.0},
    {"t_crawl", 3.049999999999997},
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

struct Outcome {
  bool finished = false;
  bool gave_up = false;
  double time = 0.0;
  int attempts = 0;
};

// A unicycle that drives east along y = 0 towards x = 6 m. At x = 3 m something stops it going forward:
// scenario 0, a snag that lets go once the vehicle has backed up to x <= 2.6; scenario 1, a wall that never moves.
Outcome drive(int scenario, bool recover, double limit) {
  UnstickConfig cfg;
  Unstick u(cfg);
  CarPose p;
  bool released = false;
  bool snagged = false;
  int most = 0;
  Outcome o;
  const double dt = 0.05;
  for (double t = 0.0; t < limit; t += dt) {
    DriveCmd normal;
    normal.v = 0.5;
    normal.w = 1.5 * wrap_pi(std::atan2(0.0 - p.y, 1.0) - p.theta);
    DriveCmd c = recover ? u.step(t, p, normal) : normal;
    most = std::max(most, u.attempts());
    if (recover && u.gave_up()) {
      o.gave_up = true;
      o.time = t;
      o.attempts = most;
      return o;
    }
    CarPose q = p;
    q.theta = wrap_pi(p.theta + c.w * dt);
    q.x += c.v * dt * std::cos(p.theta + 0.5 * c.w * dt);
    q.y += c.v * dt * std::sin(p.theta + 0.5 * c.w * dt);
    if (q.x > 2.9) snagged = true;
    if (scenario == 0 && snagged && q.x <= 2.6) released = true;
    const bool blocked = q.x > 3.0 && (scenario == 1 || !released);
    if (blocked) {
      q.x = p.x;
      q.y = p.y;
    }
    p = q;
    if (p.x >= 6.0) {
      o.finished = true;
      o.time = t;
      o.attempts = most;
      return o;
    }
  }
  o.time = limit;
  o.attempts = most;
  return o;
}

}  // namespace

RC_TEST("a vehicle caught on a snag backs up out of it and finishes, where one without recovery never does") {
  const Outcome with = drive(0, true, 120.0);
  const Outcome without = drive(0, false, 120.0);
  rec("t_snag", with.time, 1);
  rec("att_snag", with.attempts, 0);
  rec("t_limit", 120.0, 0);
  RC_CHECK(with.finished);
  RC_CHECK(!without.finished);
  RC_CHECK(with.attempts == 2);
}

RC_TEST("a wall that never moves is given up on, in bounded time, not tried for ever") {
  const Outcome o = drive(1, true, 300.0);
  rec("att_wall", o.attempts - 1, 0);
  rec("t_giveup", o.time, 1);
  RC_CHECK(o.gave_up);
  RC_CHECK(!o.finished);
  RC_CHECK(o.attempts == 4);
  RC_CHECK(o.time < 120.0);
}

RC_TEST("a vehicle commanded to stop is never called stuck") {
  UnstickConfig cfg;
  StuckDetector d(cfg);
  int flagged = 0;
  for (double t = 0.0; t < 10.0; t += 0.05)
    if (d.update(t, 1.0, 2.0, 0.0)) ++flagged;
  rec("false_stop", flagged, 0);
  RC_CHECK(flagged == 0);
}

RC_TEST("a vehicle crawling slower than the threshold is called stuck: the threshold must sit under the slowest crawl") {
  UnstickConfig cfg;
  StuckDetector d(cfg);
  double first = -1.0;
  for (double t = 0.0; t < 10.0 && first < 0.0; t += 0.05)
    if (d.update(t, 0.02 * t, 0.0, 0.02 + 0.05)) first = t;  // commanded above cmd_min, moving at 0.02 m/s
  rec("t_crawl", first, 1);
  RC_CHECK(first > 0.0);
  UnstickConfig lower = cfg;
  lower.min_disp = 0.03;
  StuckDetector d2(lower);
  bool flagged = false;
  for (double t = 0.0; t < 10.0; t += 0.05)
    if (d2.update(t, 0.02 * t, 0.0, 0.07)) flagged = true;
  RC_CHECK(!flagged);
}

RC_TEST("the ladder differs by vehicle: a car cannot spin") {
  UnstickConfig diff, car, wheel;
  car.can_spin = false;
  wheel.can_reverse = false;
  wheel.can_spin = false;
  RC_CHECK(behaviour_for_attempt(0, diff) == kWait);
  RC_CHECK(behaviour_for_attempt(1, diff) == kBackUp);
  RC_CHECK(behaviour_for_attempt(2, diff) == kSpin);
  RC_CHECK(behaviour_for_attempt(3, diff) == kGiveUp);
  RC_CHECK(behaviour_for_attempt(1, car) == kBackUp);
  RC_CHECK(behaviour_for_attempt(2, car) == kBackUp);
  RC_CHECK(behaviour_for_attempt(1, wheel) == kGiveUp);
}

RC_TEST("a recovery that cannot finish is ended by its time limit") {
  UnstickConfig cfg;
  cfg.backup_dist = 5.0;       // a back-up that never gets that far
  cfg.max_behaviour_time = 4.0;
  Unstick u(cfg);
  CarPose p;
  DriveCmd normal;
  normal.v = 0.5;
  int ended = 0;
  Behaviour last = kNormal;
  for (double t = 0.0; t < 60.0; t += 0.05) {
    u.step(t, p, normal);  // the vehicle never moves
    if (last != kNormal && u.behaviour() == kNormal) ++ended;
    last = u.behaviour();
    if (u.gave_up()) break;
  }
  RC_CHECK(ended >= 2);
  RC_CHECK(u.gave_up());
}
