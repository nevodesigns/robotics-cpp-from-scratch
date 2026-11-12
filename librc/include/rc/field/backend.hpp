// rc/field/backend.hpp
//
// The simulator backend interface, from lesson 22-14.
//
// A Backend is a simulator seen from a program: start it on a scenario and a robot, give it a command (a speed and a turn rate), let it advance by a time, and ask where the robot is. BuiltinBackend is the built in model: the fake drive of lesson 22-01 with the limits of the robot file and a time constant. run_script is the program: it works through a list of (speed, turn rate, seconds) steps against any Backend and returns the trace of poses, sampled every so often. The Gazebo Sim backend, GzBackend in rc/gz/gz_backend.hpp, is the same interface over gz-transport, built where Gazebo is installed.
//
// The script of a straight drive of 5 s at 0.2 m/s ended on the built in backend at y = 2.990 from a start at y = 2, and a turn in place of a quarter of a turn ended at 88.8 degrees; the commands of 5 m/s were held to 0.22 m/s by the limits of the file, and on Gazebo Sim the same script ended 0.239 m and 23.4 degrees from the built in model.

#ifndef RC_RC_FIELD_BACKEND_HPP
#define RC_RC_FIELD_BACKEND_HPP

#include <cmath>
#include <cstddef>
#include <rc/field/field_scenario.hpp>
#include <rc/field/frames.hpp>
#include <rc/field/robot_file.hpp>
#include <rc/hal/interfaces.hpp>
#include <string>
#include <vector>

namespace rc {
namespace field {

// Where the robot is in the world, and when.
struct BackendPose {
  double t = 0.0;  // seconds of simulated time since the start
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

// A simulator as a program sees it. The robot and the world come from a scenario and a robot file; the program never sees which simulator it has.
class Backend {
 public:
  virtual ~Backend() = default;
  virtual std::string name() const = 0;
  virtual bool start(const rc::field::FieldScenario& scenario, const rc::field::RobotModel& robot) = 0;
  virtual void command(double v, double w) = 0;  // held until the next command
  virtual void advance(double seconds) = 0;
  virtual BackendPose pose() const = 0;
};

// The built in model: the fake drive with the robot's limits, started at the scenario's start pose. There is no terrain and no walls in it yet; the lessons that follow add them.
class BuiltinBackend : public Backend {
 public:
  explicit BuiltinBackend(double tau = 0.05) : tau_(tau) {}

  std::string name() const override { return "builtin"; }

  bool start(const rc::field::FieldScenario& scenario, const rc::field::RobotModel& robot) override {
    if (!scenario.has_start) return false;
    rc::hal::DriveLimits l = rc::field::drive_limits_of(robot);
    l.tau = tau_;
    drive_ = rc::hal::FakeDrive(l);
    start_.x = scenario.start_x;
    start_.y = scenario.start_y;
    start_.theta = scenario.start_heading_deg * 3.14159265358979323846 / 180.0;
    time_ = 0.0;
    v_ = 0.0;
    w_ = 0.0;
    return true;
  }

  void command(double v, double w) override {
    v_ = v;
    w_ = w;
  }

  void advance(double seconds) override {
    const long steps = std::lround(seconds / 0.001);
    for (long i = 0; i < steps; ++i) {
      rc::hal::DriveCommand c;
      c.v = v_;
      c.w = w_;
      drive_.command(c);
      drive_.step(0.001);
      time_ += 0.001;
    }
  }

  BackendPose pose() const override {
    const rc::hal::DriveState s = drive_.state();
    rc::field::Tf2 local;
    local.x = s.x;
    local.y = s.y;
    local.theta = s.theta;
    const rc::field::Tf2 world = rc::field::compose(start_, local);
    BackendPose p;
    p.t = time_;
    p.x = world.x;
    p.y = world.y;
    p.theta = world.theta;
    return p;
  }

 private:
  double tau_;
  rc::hal::FakeDrive drive_;
  rc::field::Tf2 start_;
  double time_ = 0.0;
  double v_ = 0.0;
  double w_ = 0.0;
};

// One step of a script: a speed, a turn rate, and for how long.
struct ScriptStep {
  double v = 0.0;
  double w = 0.0;
  double seconds = 0.0;
};

// The program: work through a script against any backend, and record the pose every sample_dt seconds (and at the start).
inline std::vector<BackendPose> run_script(Backend& backend, const std::vector<ScriptStep>& script, double sample_dt) {
  std::vector<BackendPose> trace;
  trace.push_back(backend.pose());
  for (std::size_t i = 0; i < script.size(); ++i) {
    backend.command(script[i].v, script[i].w);
    const long chunks = std::lround(script[i].seconds / sample_dt);
    for (long k = 0; k < chunks; ++k) {
      backend.advance(sample_dt);
      trace.push_back(backend.pose());
    }
  }
  return trace;
}

inline double angle_between(double a, double b) { return std::atan2(std::sin(a - b), std::cos(a - b)); }


}  // namespace field
}  // namespace rc

#endif
