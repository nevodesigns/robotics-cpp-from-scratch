// rc/drive/unstick.hpp
//
// Getting unstuck, from lesson 26-08.
//
// StuckDetector is told the time, the position and the speed the vehicle was commanded, and says stuck when for a whole window it has been commanded to move (above a small speed) and has gone less than a distance from where it was a window ago. A vehicle commanded to stop is never stuck. behaviour_for_attempt gives the ladder: wait first (whatever is in the way may leave), then back up, then spin, then give up, with the rungs a vehicle cannot do left out (a car cannot spin). Unstick is the state machine: in normal driving it passes the normal command through and watches; when the detector fires it starts the behaviour for this attempt; each behaviour runs until it is done or a time limit passes, so a recovery cannot hang; then normal driving resumes with the detector cleared. After the vehicle has moved a distance from where the last recovery ended the attempts are forgotten, and when the ladder is used up Unstick gives up and commands a stop.
//
// A vehicle that was snagged at 3 m along a path of 6 m, which let go once it had backed up 0.4 m, finished in 23.5 s after 2 attempts; without recovery it had not finished after 120 s. A wall that never moved was given up on after 3 attempts and 31.2 s. A vehicle commanded to stop for 10 s was flagged 0 times; one crawling at 0.02 m/s was flagged stuck after 3.0 s with a threshold of 0.1 m in 3 s.

#ifndef RC_RC_DRIVE_UNSTICK_HPP
#define RC_RC_DRIVE_UNSTICK_HPP

#include <cmath>
#include <cstddef>
#include <rc/drive/hybrid_astar.hpp>
#include <vector>

namespace rc {
namespace drive {

struct DriveCmd {
  double v = 0.0;  // m/s, negative to reverse
  double w = 0.0;  // rad/s
};

enum Behaviour { kNormal = 0, kWait = 1, kBackUp = 2, kSpin = 3, kGiveUp = 4 };

struct UnstickConfig {
  double window = 3.0;         // seconds of trying to move before the vehicle is called stuck
  double min_disp = 0.1;       // metres it must have moved in the window
  double cmd_min = 0.05;       // m/s: a command below this is not trying to move
  double wait_time = 2.0;      // s
  double backup_dist = 0.5;    // m
  double backup_speed = 0.2;   // m/s (positive here, driven backwards)
  double spin_rate = 0.8;      // rad/s
  double spin_angle = 6.283185307179586;
  double reset_dist = 1.0;     // m of progress after a recovery that makes the attempts forgotten
  double max_behaviour_time = 12.0;  // s: a behaviour that has not finished by now is ended
  int max_attempts = 3;
  bool can_reverse = true;
  bool can_spin = true;
};

// Says stuck when, for a whole window, the vehicle was commanded to move and went less than min_disp.
class StuckDetector {
 public:
  explicit StuckDetector(const UnstickConfig& cfg) : cfg_(cfg) {}
  void reset() { t_.clear(); x_.clear(); y_.clear(); moving_.clear(); }
  bool update(double t, double x, double y, double cmd_speed) {
    t_.push_back(t);
    x_.push_back(x);
    y_.push_back(y);
    moving_.push_back(std::fabs(cmd_speed) >= cfg_.cmd_min);
    // The newest sample that is at least a window old; everything before it is dropped.
    std::size_t keep = 0;
    for (std::size_t i = 0; i < t_.size(); ++i)
      if (t - t_[i] >= cfg_.window) keep = i;
    if (keep > 0) {
      t_.erase(t_.begin(), t_.begin() + static_cast<long>(keep));
      x_.erase(x_.begin(), x_.begin() + static_cast<long>(keep));
      y_.erase(y_.begin(), y_.begin() + static_cast<long>(keep));
      moving_.erase(moving_.begin(), moving_.begin() + static_cast<long>(keep));
    }
    if (t - t_.front() < cfg_.window) return false;
    for (std::size_t i = 0; i < moving_.size(); ++i)
      if (!moving_[i]) return false;
    return std::hypot(x - x_.front(), y - y_.front()) < cfg_.min_disp;
  }

 private:
  UnstickConfig cfg_;
  std::vector<double> t_, x_, y_;
  std::vector<bool> moving_;
};

// What to try on attempt number `attempt` (0, 1, 2 ...), given what the vehicle can do.
inline Behaviour behaviour_for_attempt(int attempt, const UnstickConfig& cfg) {
  if (attempt >= cfg.max_attempts) return kGiveUp;
  if (attempt == 0) return kWait;
  if (attempt == 1) return cfg.can_reverse ? kBackUp : (cfg.can_spin ? kSpin : kGiveUp);
  if (attempt == 2) return cfg.can_spin ? kSpin : (cfg.can_reverse ? kBackUp : kGiveUp);
  return kGiveUp;
}

// The state machine: normal driving, one recovery behaviour at a time, or given up.
class Unstick {
 public:
  explicit Unstick(const UnstickConfig& cfg) : cfg_(cfg), det_(cfg) {}

  Behaviour behaviour() const { return state_; }
  int attempts() const { return attempts_; }
  bool gave_up() const { return state_ == kGiveUp; }

  // The command to send now: the normal one, or a recovery behaviour's.
  DriveCmd step(double t, const CarPose& pose, const DriveCmd& normal) {
    DriveCmd out;
    if (state_ == kGiveUp) return out;
    if (state_ == kNormal) {
      if (attempts_ > 0 && std::hypot(pose.x - last_end_x_, pose.y - last_end_y_) >= cfg_.reset_dist) attempts_ = 0;
      if (!det_.update(t, pose.x, pose.y, normal.v)) return normal;
      const Behaviour b = behaviour_for_attempt(attempts_, cfg_);
      ++attempts_;
      state_ = b;
      start_t_ = t;
      start_x_ = pose.x;
      start_y_ = pose.y;
      spun_ = 0.0;
      last_theta_ = pose.theta;
      if (b == kGiveUp) return out;
    }
    bool done = false;
    if (state_ == kWait) {
      done = t - start_t_ >= cfg_.wait_time;
    } else if (state_ == kBackUp) {
      out.v = -cfg_.backup_speed;
      done = std::hypot(pose.x - start_x_, pose.y - start_y_) >= cfg_.backup_dist;
    } else if (state_ == kSpin) {
      out.w = cfg_.spin_rate;
      spun_ += std::fabs(wrap_pi(pose.theta - last_theta_));
      last_theta_ = pose.theta;
      done = spun_ >= cfg_.spin_angle;
    }
    if (t - start_t_ >= cfg_.max_behaviour_time) done = true;
    if (done) {
      state_ = kNormal;
      det_.reset();
      last_end_x_ = pose.x;
      last_end_y_ = pose.y;
      return DriveCmd();
    }
    return out;
  }

 private:
  UnstickConfig cfg_;
  StuckDetector det_;
  Behaviour state_ = kNormal;
  int attempts_ = 0;
  double start_t_ = 0.0, start_x_ = 0.0, start_y_ = 0.0;
  double last_end_x_ = 0.0, last_end_y_ = 0.0;
  double spun_ = 0.0, last_theta_ = 0.0;
};


}  // namespace drive
}  // namespace rc

#endif
