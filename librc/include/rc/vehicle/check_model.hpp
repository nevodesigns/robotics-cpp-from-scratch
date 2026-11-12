// rc/vehicle/check_model.hpp
//
// Checking a model against the simulator, from lesson 23-09.
//
// ModelBackend is a differential drive with a longitudinal slip, a turn efficiency and a lag of the wheel speeds, behind the same Backend interface as the built in model and Gazebo, so that the standard cases of lesson 22-15 run on it. turn_tests measures how much of a commanded turn a backend makes from rest, after a drive and after a drive and a stop, and context_dependent says whether the three differ by more than a tenth: a model with one turn efficiency cannot be right for a simulator that turns differently in different circumstances, however it is fitted. fit_efficiency finds the efficiency that minimises the worst gap to a set of reference traces, and gives the curve it searched.
//
// A model fitted to another model of efficiency 0.8 found 0.80; a backend whose turn is short after a drive had turn ratios of 0.97, 0.68 and 0.68 and could not be fitted below a worst gap of 0.013 m, against 0.0000 m for one that turned the same every time; and Gazebo with the weight of the robot 1 mm behind its axle made 1.01, 0.74 and 0.85 of the turns asked, where with the weight behind it, it made 0.99, 1.01 and 1.02.

#ifndef RC_RC_VEHICLE_CHECK_MODEL_HPP
#define RC_RC_VEHICLE_CHECK_MODEL_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/field/gap.hpp>
#include <string>
#include <vector>

namespace rc {
namespace vehicle {

// A differential drive with its losses, behind the rc::field::Backend interface: commands are turned into wheel speeds by the ideal geometry of the robot, the wheel speeds follow them with a lag, and the body moves by the ground speed less the slip and turns by the efficiency of the wheels' difference.
class ModelBackend : public rc::field::Backend {
 public:
  ModelBackend(double slip, double efficiency, double tau) : slip_(slip), eff_(efficiency), tau_(tau) {}

  std::string name() const override { return "model"; }

  bool start(const rc::field::FieldScenario& scenario, const rc::field::RobotModel& robot) override {
    if (!scenario.has_start) return false;
    r_ = robot.wheel_radius.value;
    b_ = robot.track_width.value;
    x_ = scenario.start_x;
    y_ = scenario.start_y;
    theta_ = scenario.start_heading_deg * 3.14159265358979323846 / 180.0;
    wl_ = wr_ = cl_ = cr_ = 0.0;
    t_ = 0.0;
    travelled_ = 0.0;
    return true;
  }

  void command(double v, double w) override {
    cl_ = (v - 0.5 * w * b_) / r_;
    cr_ = (v + 0.5 * w * b_) / r_;
  }

  void advance(double seconds) override {
    const long n = std::lround(seconds / 0.001);
    const double alpha = 1.0 - std::exp(-0.001 / tau_);
    for (long i = 0; i < n; ++i) {
      wl_ += alpha * (cl_ - wl_);
      wr_ += alpha * (cr_ - wr_);
      const double v = 0.5 * r_ * (wl_ + wr_) * (1.0 - slip_);
      const double w = efficiency() * r_ * (wr_ - wl_) / b_;
      const double mid = theta_ + 0.5 * w * 0.001;
      x_ += v * std::cos(mid) * 0.001;
      y_ += v * std::sin(mid) * 0.001;
      theta_ += w * 0.001;
      travelled_ += std::fabs(v) * 0.001;
      t_ += 0.001;
    }
  }

  rc::field::BackendPose pose() const override {
    rc::field::BackendPose p;
    p.t = t_;
    p.x = x_;
    p.y = y_;
    p.theta = theta_;
    return p;
  }

  void set_efficiency(double e) { eff_ = e; }

 protected:
  // The efficiency at this moment; a subclass may make it depend on what the vehicle has done.
  virtual double efficiency() const { return eff_; }
  double travelled() const { return travelled_; }

 private:
  double slip_;
  double eff_;
  double tau_;
  double r_ = 0.033;
  double b_ = 0.16;
  double x_ = 0.0, y_ = 0.0, theta_ = 0.0;
  double wl_ = 0.0, wr_ = 0.0, cl_ = 0.0, cr_ = 0.0;
  double t_ = 0.0;
  double travelled_ = 0.0;
};

// How much of a commanded turn a backend makes: the heading it turned through (summed sample by sample so that a turn of more than half a circle is counted) over the heading asked for.
inline double turn_ratio(rc::field::Backend& b, const rc::field::FieldScenario& scenario, const rc::field::RobotModel& robot, const std::vector<rc::field::ScriptStep>& lead) {
  if (!b.start(scenario, robot)) return 0.0;
  if (!lead.empty()) rc::field::run_script(b, lead, 0.1);
  const double w = 1.0;
  const double seconds = 1.6;
  std::vector<rc::field::ScriptStep> turn(1, rc::field::ScriptStep{0.0, w, seconds});
  const std::vector<rc::field::BackendPose> trace = rc::field::run_script(b, turn, 0.1);
  double turned = 0.0;
  for (std::size_t i = 1; i < trace.size(); ++i) turned += rc::field::angle_between(trace[i].theta, trace[i - 1].theta);
  return turned / (w * seconds);
}

struct TurnTests {
  double from_rest = 0.0;
  double after_drive = 0.0;
  double after_stop = 0.0;
  double spread() const { return std::max(from_rest, std::max(after_drive, after_stop)) - std::min(from_rest, std::min(after_drive, after_stop)); }
};

// The turn from rest, after two seconds of driving at 0.2 m/s, and after the same and a second at rest.
inline TurnTests turn_tests(rc::field::Backend& b, const rc::field::FieldScenario& scenario, const rc::field::RobotModel& robot) {
  TurnTests t;
  t.from_rest = turn_ratio(b, scenario, robot, std::vector<rc::field::ScriptStep>());
  std::vector<rc::field::ScriptStep> drive(1, rc::field::ScriptStep{0.2, 0.0, 2.0});
  t.after_drive = turn_ratio(b, scenario, robot, drive);
  drive.push_back(rc::field::ScriptStep{0.0, 0.0, 1.0});
  t.after_stop = turn_ratio(b, scenario, robot, drive);
  return t;
}

// A turn that depends on what came before it is a different thing from a turn efficiency, and no value of one parameter fits it.
inline bool context_dependent(const TurnTests& t) { return t.spread() > 0.1; }

struct Fit {
  double efficiency = 0.0;
  double worst_gap = 0.0;
  std::vector<double> curve;  // the worst gap at each efficiency tried, from lo to hi
};

// Tries efficiencies from lo to hi (points of them) in a model with the given slip and lag, and keeps the one whose worst gap to the reference traces is smallest.
inline Fit fit_efficiency(const std::vector<std::vector<rc::field::BackendPose>>& reference, const rc::field::FieldScenario& scenario, const rc::field::RobotModel& robot, const std::vector<rc::field::GapCase>& cases, double slip, double tau, double lo, double hi, int points) {
  Fit f;
  f.worst_gap = 1.0e9;
  for (int k = 0; k < points; ++k) {
    const double e = lo + (hi - lo) * static_cast<double>(k) / static_cast<double>(points - 1);
    ModelBackend model(slip, e, tau);
    const double worst = rc::field::worst_gap(rc::field::gaps_of(rc::field::run_all(model, scenario, robot, cases, 0.1), reference));
    f.curve.push_back(worst);
    if (worst < f.worst_gap) {
      f.worst_gap = worst;
      f.efficiency = e;
    }
  }
  return f;
}


}  // namespace vehicle
}  // namespace rc

#endif
