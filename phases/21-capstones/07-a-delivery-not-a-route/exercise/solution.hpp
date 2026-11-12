#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/safety/supervisor.hpp>

// A delivery along one road: from home (position 0) to the drop (position `distance`), unload, and back. The robot decides what to do from its state, and the state changes on events; a robot whose state has no answer for an event is a robot that does something nobody chose.
enum class MissionState { idle, loading, to_drop, unloading, to_home, done, aborted, help };

inline const char* mission_state_name(MissionState s) {
  // TODO: the words: idle, loading, to drop, unloading, to home, done, aborted, needs help (for help)
  (void)s;
  return "?";
}

// The energy to cover a distance at a rate in watt hours a metre (a negative distance counts as its size).
inline double energy_for(double distance, double wh_per_m) {
  // TODO: the size of the distance times the rate (a negative rate counts as 0)
  (void)distance;
  (void)wh_per_m;
  return 0.0;
}

// May the mission start? The battery must cover the whole round trip (there and back, 2 times the distance to the drop) and still have the reserve left.
inline bool can_start(double battery_wh, double distance_to_drop, double wh_per_m, double reserve_wh) {
  // TODO: the battery less the energy for twice the distance to the drop is at least the reserve
  (void)battery_wh;
  (void)distance_to_drop;
  (void)wh_per_m;
  (void)reserve_wh;
  return true;
}

// Must the robot turn back now? When what is left after getting home from where it is would be less than the reserve. `position` is the distance from home along the road; wh_per_m is what it expects to use (add a margin to it for what it does not expect).
inline bool must_turn_back(double battery_wh, double position, double wh_per_m, double reserve_wh) {
  // TODO: the battery less the energy to get home from `position` is below the reserve
  (void)battery_wh;
  (void)position;
  (void)wh_per_m;
  (void)reserve_wh;
  return false;
}

struct MissionConfig {
  double distance = 100.0;           // metres to the drop
  double load_time = 2.0;
  double unload_time = 2.0;
  double blocked_timeout = 10.0;     // seconds waiting for the way to clear before turning back
  double wh_per_m_expected = 0.1;    // what the robot expects to use
  double rate_margin = 1.0;          // and plans with this many times that, for what it does not expect
  double reserve_wh = 5.0;
  double speed = 0.5;
  bool check_on_the_way = true;      // turn back if the battery falls short of getting home (otherwise only the check at the start)
};

struct MissionInputs {
  double battery_wh = 0.0;
  bool blocked = false;              // something is in the way of the direction it is going
  bool stopped = false;              // the supervisor has stopped the robot
};

// What the robot should do this tick: how fast (negative is towards home), and whether the mission state changed.
struct MissionStep {
  double velocity = 0.0;
};

class Mission {
 public:
  explicit Mission(const MissionConfig& cfg) : cfg_(cfg) {}

  MissionState state() const { return state_; }
  double position() const { return position_; }
  bool delivered() const { return delivered_; }
  bool turned_back() const { return turned_back_; }
  double held_time() const { return held_time_; }
  double waited() const { return waited_total_; }

  // Press go: from idle, to loading if there is enough battery for the round trip at the planning rate (the expected rate times the margin), otherwise to aborted. Anything else is ignored.
  void start(double battery_wh) {
    if (state_ != MissionState::idle) return;
    state_ = can_start(battery_wh, cfg_.distance, cfg_.wh_per_m_expected * cfg_.rate_margin, cfg_.reserve_wh) ? MissionState::loading : MissionState::aborted;
  }

  // One tick of dt seconds. Stopped by the supervisor: nothing moves and nothing times out (the state, timers and position are kept; held_time counts the seconds), and the velocity is 0.
  //  loading:   the timer runs; after load_time, to_drop.
  //  to_drop:   if blocked, wait (velocity 0, the time waited counts) and after blocked_timeout turn back (to_home, turned_back set); otherwise, if check_on_the_way and must_turn_back, turn back; otherwise move at the speed towards the drop, and on arriving (position at the drop) go to unloading and set delivered once unloaded.
  //  unloading: the timer runs; after unload_time, delivered is set and to_home follows.
  //  to_home:   if blocked, wait, and after blocked_timeout, help; otherwise move towards home at the speed (velocity negative), and on arriving, done if nothing went wrong and aborted if the mission turned back.
  //  idle, done, aborted, help: nothing moves.
  // The position never goes below 0 or above the distance.
  MissionStep step(const MissionInputs& in, double dt) {
    MissionStep out;
    if (in.stopped && state_ != MissionState::idle && state_ != MissionState::done && state_ != MissionState::aborted && state_ != MissionState::help) {
      held_time_ += dt;
      return out;
    }
    switch (state_) {
      case MissionState::loading:
        timer_ += dt;
        if (timer_ >= cfg_.load_time) {
          state_ = MissionState::to_drop;
          timer_ = 0.0;
          blocked_for_ = 0.0;
        }
        break;
      case MissionState::to_drop:
        if (in.blocked) {
          blocked_for_ += dt;
          waited_total_ += dt;
          if (blocked_for_ > cfg_.blocked_timeout) {
            state_ = MissionState::to_home;
            turned_back_ = true;
            blocked_for_ = 0.0;
          }
        } else if (cfg_.check_on_the_way && must_turn_back(in.battery_wh, position_, cfg_.wh_per_m_expected * cfg_.rate_margin, cfg_.reserve_wh)) {
          state_ = MissionState::to_home;
          turned_back_ = true;
          blocked_for_ = 0.0;
        } else {
          blocked_for_ = 0.0;
          out.velocity = cfg_.speed;
          position_ = std::min(cfg_.distance, position_ + cfg_.speed * dt);
          if (position_ >= cfg_.distance) {
            state_ = MissionState::unloading;
            timer_ = 0.0;
          }
        }
        break;
      case MissionState::unloading:
        timer_ += dt;
        if (timer_ >= cfg_.unload_time) {
          delivered_ = true;
          state_ = MissionState::to_home;
          timer_ = 0.0;
          blocked_for_ = 0.0;
        }
        break;
      case MissionState::to_home:
        if (in.blocked) {
          blocked_for_ += dt;
          waited_total_ += dt;
          if (blocked_for_ > cfg_.blocked_timeout) state_ = MissionState::help;
        } else {
          blocked_for_ = 0.0;
          out.velocity = -cfg_.speed;
          position_ = std::max(0.0, position_ - cfg_.speed * dt);
          if (position_ <= 0.0) state_ = turned_back_ ? MissionState::aborted : MissionState::done;
        }
        break;
      case MissionState::idle:
      case MissionState::done:
      case MissionState::aborted:
      case MissionState::help:
        break;
    }
    return out;
  }

 private:
  MissionConfig cfg_;
  MissionState state_ = MissionState::idle;
  double position_ = 0.0;
  double timer_ = 0.0;
  double blocked_for_ = 0.0;
  double waited_total_ = 0.0;
  double held_time_ = 0.0;
  bool delivered_ = false;
  bool turned_back_ = false;
};

// A run of a mission against a world: the battery is used at `wh_per_m_actual` for each metre moved (more than the robot expects when there is a headwind, say), a stretch of road can be blocked for a time in each direction, and the link can be lost for a time, which stops the robot by the supervisor
// until it comes back and a reset is pressed 1 s later.
struct MissionWorld {
  double wh_per_m_actual = 0.1;
  double battery_start = 40.0;
  double blocked_from = -1.0;        // from this time to blocked_to, the way in the direction of travel is blocked (negative: never)
  double blocked_to = -1.0;
  double blocked_back_from = -1.0;   // the same, but only for the journey home
  double link_lost_from = -1.0;
  double link_lost_to = -1.0;
};

struct MissionResult {
  MissionState final_state = MissionState::idle;
  bool delivered = false;
  bool turned_back = false;
  double battery_left = 0.0;
  double position_end = 0.0;
  double time = 0.0;
  double held_time = 0.0;
  double waited = 0.0;
  bool stranded = false;             // the battery ran out away from home
};

// Run for at most `seconds` of 0.1 s ticks. Each tick: the battery and the world's events, the supervisor (heartbeat lost while the link is lost, a reset 1 s after it returns), the mission step; the battery falls by the energy of the distance moved. The run ends when the state is done, aborted or help, or the battery is empty
// (stranded if the robot is away from home).
inline MissionResult run_mission(const MissionConfig& cfg, const MissionWorld& world, double seconds) {
  MissionResult r;
  const double dt = 0.1;
  Mission mission(cfg);
  rc::safety::SupervisorConfig sc;
  rc::safety::Supervisor sup(sc);
  rc::safety::Inputs healthy;
  healthy.estop_closed = true;
  healthy.heartbeat_fresh = true;
  healthy.fence = rc::safety::FenceState::inside;
  healthy.tilt = rc::safety::TiltState::ok;
  healthy.sensors = rc::safety::CheckState::ok;
  healthy.power.state = rc::safety::PowerState::ok;
  healthy.power.factor = 1.0;
  healthy.range_cap = sc.top_speed + 1.0;
  sup.reset(healthy);
  double battery = world.battery_start;
  mission.start(battery);
  double link_back_at = -1.0;
  const int steps = static_cast<int>(seconds / dt);
  for (int i = 0; i < steps; ++i) {
    const double t = i * dt;
    const bool link_lost = world.link_lost_from >= 0.0 && t >= world.link_lost_from && (world.link_lost_to < 0.0 || t < world.link_lost_to);
    if (!link_lost && world.link_lost_from >= 0.0 && t >= world.link_lost_from && link_back_at < 0.0) link_back_at = t;
    rc::safety::Inputs in = healthy;
    in.heartbeat_fresh = !link_lost;
    rc::safety::Decision d = sup.step(in);
    if (d.mode == rc::safety::Mode::stopped && !link_lost && link_back_at >= 0.0 && t - link_back_at >= 1.0 && sup.reset(healthy)) d = sup.step(in);
    const bool going_home = mission.state() == MissionState::to_home;
    const bool blocked = (world.blocked_from >= 0.0 && t >= world.blocked_from && (world.blocked_to < 0.0 || t < world.blocked_to) && !going_home) ||
                         (world.blocked_back_from >= 0.0 && t >= world.blocked_back_from && going_home);
    MissionInputs mi;
    mi.battery_wh = battery;
    mi.blocked = blocked;
    mi.stopped = d.mode == rc::safety::Mode::stopped;
    const MissionStep step = mission.step(mi, dt);
    battery -= energy_for(step.velocity * dt, world.wh_per_m_actual);
    r.time = t + dt;
    const MissionState s = mission.state();
    if (s == MissionState::done || s == MissionState::aborted || s == MissionState::help) break;
    if (battery <= 0.0) {
      battery = 0.0;
      r.stranded = mission.position() > 0.0;
      break;
    }
  }
  r.final_state = mission.state();
  r.delivered = mission.delivered();
  r.turned_back = mission.turned_back();
  r.battery_left = battery;
  r.position_end = mission.position();
  r.held_time = mission.held_time();
  r.waited = mission.waited();
  return r;
}

#endif  // LESSON_SOLUTION_HPP
