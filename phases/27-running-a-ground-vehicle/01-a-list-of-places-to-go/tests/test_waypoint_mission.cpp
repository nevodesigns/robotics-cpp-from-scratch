#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

Waypoint wp(double x, double y, double tol, double dwell, double timeout) {
  Waypoint w;
  w.x = x;
  w.y = y;
  w.tolerance = tol;
  w.dwell = dwell;
  w.timeout = timeout;
  return w;
}

struct Drive {
  double x = 0.0, y = 0.0, dist = 0.0, t = 0.0;
  double dwelled = 0.0;
};

// A point vehicle that goes straight to the target at the speed it is given.
Drive run(WaypointMission& m, Drive d, double limit, const std::vector<std::pair<double, double> >* blocked = nullptr) {
  const double dt = 0.05;
  for (; d.t < limit; d.t += dt) {
    const MissionStatus s = m.step(d.t, d.x, d.y);
    if (s.done || s.aborted) break;
    if (s.dwelling) d.dwelled += dt;
    const double dx = s.target_x - d.x, dy = s.target_y - d.y;
    const double dist = std::hypot(dx, dy);
    if (blocked != nullptr && s.leg < static_cast<int>(blocked->size()) && (*blocked)[static_cast<std::size_t>(s.leg)].first > 0.0) continue;  // cannot move
    if (dist > 1e-9 && s.speed > 0.0) {
      const double step = std::min(dist, s.speed * dt);
      d.x += dx / dist * step;
      d.y += dy / dist * step;
      d.dist += step;
    }
  }
  return d;
}

}  // namespace

RC_TEST("a list of waypoints is reached in order, standing at the one that asks to") {
  std::vector<Waypoint> legs;
  legs.push_back(wp(5.0, 0.0, 0.3, 0.0, 30.0));
  legs.push_back(wp(5.0, 5.0, 0.3, 4.0, 30.0));
  legs.push_back(wp(0.0, 5.0, 0.3, 0.0, 30.0));
  legs.push_back(wp(0.0, 0.0, 0.3, 0.0, 30.0));
  WaypointMission m(legs, kSkipOnTimeout);
  const Drive d = run(m, Drive(), 120.0);
  rec("dwell_used", d.dwelled, 1);
  RC_CHECK(m.done());
  for (std::size_t i = 0; i < 4; ++i) RC_CHECK(m.results()[i] == kReached);
  RC_CHECK(d.dwelled > 3.9 && d.dwelled < 4.3);
  RC_CHECK(d.dist > 18.0 && d.dist < 20.5);
}

RC_TEST("a leg that cannot be reached times out, and the policy skips it or ends the mission") {
  std::vector<Waypoint> legs;
  legs.push_back(wp(5.0, 0.0, 0.3, 0.0, 10.0));
  legs.push_back(wp(5.0, 5.0, 0.3, 0.0, 30.0));
  std::vector<std::pair<double, double> > blocked;
  blocked.push_back(std::make_pair(1.0, 0.0));  // the vehicle cannot move on leg 0
  blocked.push_back(std::make_pair(0.0, 0.0));
  WaypointMission skip(legs, kSkipOnTimeout), abort(legs, kAbortOnTimeout);
  const Drive a = run(skip, Drive(), 120.0, &blocked);
  const Drive b = run(abort, Drive(), 120.0, &blocked);
  rec("t_timeout", b.t, 1);
  RC_CHECK(skip.results()[0] == kTimedOut);
  RC_CHECK(skip.results()[1] == kPending || skip.results()[1] == kReached);
  RC_CHECK(b.t > 9.9 && b.t < 10.3);
  RC_CHECK(abort.aborted());
  RC_CHECK(!abort.done());
  RC_CHECK(a.t > b.t);
}

RC_TEST("restoring saved progress goes on from the leg, where starting again drives the first legs twice") {
  std::vector<Waypoint> legs;
  legs.push_back(wp(10.0, 0.0, 0.3, 0.0, 30.0));
  legs.push_back(wp(10.0, 10.0, 0.3, 0.0, 30.0));
  legs.push_back(wp(0.0, 10.0, 0.3, 0.0, 30.0));
  legs.push_back(wp(0.0, 0.0, 0.3, 0.0, 30.0));
  WaypointMission m(legs, kSkipOnTimeout);
  // Drive until the third leg has begun, then save, as if the vehicle rebooted.
  Drive d;
  for (; d.t < 120.0; d.t += 0.05) {
    const MissionStatus s = m.step(d.t, d.x, d.y);
    if (s.leg == 2) break;
    const double dx = s.target_x - d.x, dy = s.target_y - d.y, dist = std::hypot(dx, dy);
    d.x += dx / dist * std::min(dist, 0.05);
    d.y += dy / dist * std::min(dist, 0.05);
  }
  const std::string saved = m.save();
  WaypointMission resumed(legs, kSkipOnTimeout), fresh(legs, kSkipOnTimeout);
  RC_CHECK(resumed.restore(saved));
  Drive from_there;
  from_there.x = d.x;
  from_there.y = d.y;
  const Drive a = run(resumed, from_there, 200.0);
  const Drive b = run(fresh, from_there, 200.0);
  rec("dist_resume", a.dist, 1);
  rec("dist_restart", b.dist, 1);
  RC_CHECK(resumed.done() && fresh.done());
  RC_CHECK(resumed.leg() == 4);
  RC_CHECK(b.dist > a.dist + 15.0);
}

RC_TEST("progress that does not fit the list is refused") {
  std::vector<Waypoint> legs;
  legs.push_back(wp(1.0, 0.0, 0.3, 0.0, 30.0));
  legs.push_back(wp(2.0, 0.0, 0.3, 0.0, 30.0));
  WaypointMission m(legs, kSkipOnTimeout);
  RC_CHECK(!m.restore("leg=1;r=1,0,0"));
  RC_CHECK(!m.restore("junk"));
  RC_CHECK(!m.restore("leg=7;r=1,1"));
  RC_CHECK(!m.restore("leg=1;r=1,9"));
  RC_CHECK(m.leg() == 0);
  RC_CHECK(m.restore("leg=1;r=1,0"));
  RC_CHECK(m.leg() == 1);
}

RC_TEST("a tolerance finer than the turning circle can be reached keeps a car circling the point") {
  // A car with a smallest radius of 0.742 m, facing east, driven by pure pursuit towards a waypoint 0.8 m to its left: inside its smallest circle, whose nearest pass is 0.68 m from the point.
  const double tols[2] = {0.10, 0.70};
  double got[2] = {0.0, 0.0};
  for (int k = 0; k < 2; ++k) {
    std::vector<Waypoint> legs;
    legs.push_back(wp(0.0, 0.8, tols[k], 0.0, 40.0));
    WaypointMission m(legs, kAbortOnTimeout);
    CarPose pose;
    FollowConfig cfg;
    cfg.lookahead = 1.0;
    double t = 0.0;
    for (; t < 60.0; t += cfg.dt) {
      const MissionStatus s = m.step(t, pose.x, pose.y);
      if (s.done || s.aborted) break;
      std::vector<CarPose> path(2);
      path[0] = pose;
      path[1].x = s.target_x;
      path[1].y = s.target_y;
      const double steer = pure_pursuit_steer(pose, path, 0, cfg);
      pose = drive_arc(pose, steer, 1.0 * cfg.dt, cfg.wheelbase);
    }
    got[k] = m.done() ? t : 1e9;
  }
  rec("t_tight", 40.0, 0);
  rec("t_loose", got[1], 1);
  RC_CHECK(got[0] > 1e8);
  RC_CHECK(got[1] < 20.0);
}
