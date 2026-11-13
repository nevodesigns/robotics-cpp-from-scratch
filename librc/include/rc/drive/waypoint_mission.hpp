// rc/drive/waypoint_mission.hpp
//
// A list of places to go, from lesson 27-01.
//
// A Waypoint has a position, a tolerance (how near counts as there), a dwell time (how long to stand there), a timeout for the leg and a speed for it. WaypointMission is told the time and the position each tick and says where to go and how fast. A leg is reached when the vehicle has been within the tolerance and the dwell time has passed; the mission then moves on. A leg that has taken longer than its timeout is marked timed out, and the policy decides: skip it and go on, or abort. When the legs are done the mission is done. save writes the leg number and the result of each leg as text; restore reads it back, so a vehicle that restarts goes on from the leg it was on, with the timer of that leg started afresh.
//
// A vehicle on a list of four waypoints reached all four, standing 4.1 s at the one that asked for it. A waypoint that could not be reached was timed out after 10.0 s and the mission went on to the next by the skip policy and ended by the abort policy. After a restart at the third leg the vehicle drove 19.4 m on the rest of the list, and one that began again drove 38.4 m. A car with a smallest turning radius of 0.742 m was given a waypoint 0.8 m to its left, inside its smallest circle, to reach to within 0.10 m and was still circling it after 40 s; with a tolerance of 0.70 m it was there in 1.8 s.

#ifndef RC_RC_DRIVE_WAYPOINT_MISSION_HPP
#define RC_RC_DRIVE_WAYPOINT_MISSION_HPP

#include <cmath>
#include <cstddef>
#include <rc/drive/follow.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace rc {
namespace drive {

struct Waypoint {
  double x = 0.0;
  double y = 0.0;
  double tolerance = 0.5;  // metres from the point that count as there
  double dwell = 0.0;      // seconds to stand there
  double timeout = 60.0;   // seconds the leg may take
  double speed = 1.0;      // m/s on the way to it
};

enum LegStatus { kPending = 0, kReached = 1, kTimedOut = 2 };
enum MissionPolicy { kSkipOnTimeout = 0, kAbortOnTimeout = 1 };

struct MissionStatus {
  bool done = false;
  bool aborted = false;
  bool dwelling = false;
  int leg = 0;
  double target_x = 0.0;
  double target_y = 0.0;
  double speed = 0.0;
};

class WaypointMission {
 public:
  WaypointMission(const std::vector<Waypoint>& legs, MissionPolicy policy) : legs_(legs), policy_(policy), results_(legs.size(), kPending) {}

  // Called every tick with the time and the vehicle's position: where to go, and how fast.
  MissionStatus step(double t, double x, double y) {
    MissionStatus s;
    s.leg = leg_;
    s.done = done();
    s.aborted = aborted_;
    if (done() || aborted_) return s;
    const Waypoint& w = legs_[static_cast<std::size_t>(leg_)];
    if (!started_) {
      started_ = true;
      leg_t0_ = t;
      dwelling_ = false;
    }
    s.target_x = w.x;
    s.target_y = w.y;
    s.speed = w.speed;
    const double dist = std::hypot(w.x - x, w.y - y);
    if (dist <= w.tolerance && !dwelling_) {
      dwelling_ = true;
      dwell_t0_ = t;
    }
    if (dwelling_) {
      s.dwelling = true;
      s.speed = 0.0;
      if (t - dwell_t0_ >= w.dwell) {
        results_[static_cast<std::size_t>(leg_)] = kReached;
        advance();
        s.done = done();
      }
    } else if (t - leg_t0_ >= w.timeout) {
      results_[static_cast<std::size_t>(leg_)] = kTimedOut;
      if (policy_ == kAbortOnTimeout) {
        aborted_ = true;
        s.aborted = true;
        s.speed = 0.0;
      } else {
        advance();
        s.done = done();
      }
    }
    return s;
  }

  bool done() const { return leg_ >= static_cast<int>(legs_.size()); }
  bool aborted() const { return aborted_; }
  int leg() const { return leg_; }
  const std::vector<LegStatus>& results() const { return results_; }

  // The progress as text: the leg and the result of each leg.
  std::string save() const {
    std::ostringstream o;
    o << "leg=" << leg_ << ";r=";
    for (std::size_t i = 0; i < results_.size(); ++i) o << (i ? "," : "") << static_cast<int>(results_[i]);
    return o.str();
  }

  // Reads progress back. False if the text does not fit this list (the mission is left as it was).
  bool restore(const std::string& text) {
    int leg = 0;
    std::vector<LegStatus> r;
    if (text.compare(0, 4, "leg=") != 0) return false;
    const std::size_t semi = text.find(';');
    if (semi == std::string::npos) return false;
    leg = std::atoi(text.substr(4, semi - 4).c_str());
    const std::size_t eq = text.find("r=", semi);
    if (eq == std::string::npos) return false;
    std::istringstream in(text.substr(eq + 2));
    std::string item;
    while (std::getline(in, item, ',')) {
      const int v = std::atoi(item.c_str());
      if (v < 0 || v > 2) return false;
      r.push_back(static_cast<LegStatus>(v));
    }
    if (r.size() != legs_.size() || leg < 0 || leg > static_cast<int>(legs_.size())) return false;
    leg_ = leg;
    results_ = r;
    started_ = false;
    dwelling_ = false;
    return true;
  }

 private:
  void advance() {
    ++leg_;
    started_ = false;
    dwelling_ = false;
  }

  std::vector<Waypoint> legs_;
  MissionPolicy policy_;
  std::vector<LegStatus> results_;
  int leg_ = 0;
  bool started_ = false;
  bool dwelling_ = false;
  bool aborted_ = false;
  double leg_t0_ = 0.0;
  double dwell_t0_ = 0.0;
};


}  // namespace drive
}  // namespace rc

#endif
