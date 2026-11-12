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


#ifdef RC_HAVE_GZ
#include <rc/gz/gz_backend.hpp>
#endif

namespace {

const char* kBurger =
    "robot burger\nkind diff_drive\n"
    "wheel_radius 0.033 source test\ntrack_width 0.160 source test\nlength 0.138 source test\nwidth 0.178 source test\n"
    "height 0.192 source test\nmass 1.0 source test\nwheel_mass 0.03 source typical\nclearance 0.02 source typical\n"
    "wheel_width 0.018 source typical\nmax_speed 0.22 source test\nmax_turn_rate 2.84 source test\n";

const char* kYard = "scenario yard\nduration 30\nground gravel\nstart 1 2 90\n";

FieldScenario scenario() { return parse_scenario(kYard).scenario; }
RobotModel robot() { return parse_robot(kBurger).robot; }

// A model whose turn is 30 percent short once it has moved more than a few centimetres: the fault we found in Gazebo, put into a model so that it can be found without Gazebo.
class ContextBackend : public ModelBackend {
 public:
  ContextBackend() : ModelBackend(0.0, 1.0, 0.05) {}

 protected:
  double efficiency() const override { return travelled() > 0.05 ? 0.7 : 1.0; }
};

}  // namespace

RC_TEST("a model fitted to another model of efficiency 0.8 finds 0.8, and the curve of the worst gap has its minimum there") {
  ModelBackend reference(0.0, 0.8, 0.05);
  const std::vector<GapCase> cases = standard_cases();
  const std::vector<std::vector<BackendPose>> traces = run_all(reference, scenario(), robot(), cases, 0.1);
  const Fit f = fit_efficiency(traces, scenario(), robot(), cases, 0.0, 0.05, 0.5, 1.1, 61);
  rec("fit_eff", f.efficiency, 2);
  rec("clean_fit", f.worst_gap, 4);
  RC_CHECK_NEAR(f.efficiency, 0.8, 0.011);
  RC_CHECK(f.worst_gap < 0.01);
  RC_CHECK(f.curve.size() == 61);
  RC_CHECK(f.curve.front() > 10.0 * f.worst_gap + 0.05);
}

RC_TEST("a model whose turn depends on what came before it is found by the experiments, and no efficiency fits it") {
  ModelBackend clean(0.0, 0.9, 0.05);
  const TurnTests c = turn_tests(clean, scenario(), robot());
  RC_CHECK(!context_dependent(c));
  RC_CHECK_NEAR(c.from_rest, 0.9 * (1.0 - 0.05 / 1.6), 0.02);  // the lag costs the time constant of the turn

  ContextBackend defect;
  const TurnTests t = turn_tests(defect, scenario(), robot());
  rec("ctx_rest", t.from_rest, 2);
  rec("ctx_drive", t.after_drive, 2);
  rec("ctx_stop", t.after_stop, 2);
  std::cout << "  turn from rest " << t.from_rest << ", after a drive " << t.after_drive << ", after a drive and a stop " << t.after_stop << "\n";
  RC_CHECK(context_dependent(t));
  RC_CHECK_NEAR(t.from_rest, 1.0 * (1.0 - 0.05 / 1.6), 0.02);
  RC_CHECK_NEAR(t.after_drive, 0.7 * (1.0 - 0.05 / 1.6), 0.03);
  RC_CHECK_NEAR(t.after_stop, 0.7 * (1.0 - 0.05 / 1.6), 0.03);

  const std::vector<GapCase> cases = standard_cases();
  const std::vector<std::vector<BackendPose>> traces = run_all(defect, scenario(), robot(), cases, 0.1);
  const Fit f = fit_efficiency(traces, scenario(), robot(), cases, 0.0, 0.05, 0.5, 1.1, 61);
  rec("ctx_fit", f.worst_gap, 3);
  RC_CHECK(f.worst_gap > 0.005);
}

RC_TEST("the turn ratio of an ideal backend is one, whatever came before") {
  ModelBackend ideal(0.0, 1.0, 0.05);
  const TurnTests t = turn_tests(ideal, scenario(), robot());
  RC_CHECK_NEAR(t.from_rest, 1.0, 0.04);
  RC_CHECK_NEAR(t.after_drive, 1.0, 0.04);
  RC_CHECK(t.spread() < 0.02);
}

#ifdef RC_HAVE_GZ
RC_TEST("Gazebo with the weight of the robot a millimetre behind its axle turns short after a drive, and with the weight well behind it does not") {
  rc::gz::GzBackend gazebo;
  const TurnTests ok = turn_tests(gazebo, scenario(), robot());
  const RobotParse on_axle = parse_robot(std::string(kBurger) + "com_back 0.001 source test: the weight 1 mm behind the axle\n");
  RC_CHECK(on_axle.ok());
  const TurnTests broken = turn_tests(gazebo, scenario(), on_axle.robot);
  rec("ok_rest", ok.from_rest, 2);
  rec("ok_drive", ok.after_drive, 2);
  rec("ok_stop", ok.after_stop, 2);
  rec("broken_rest", broken.from_rest, 2);
  rec("broken_drive", broken.after_drive, 2);
  rec("broken_stop", broken.after_stop, 2);
  std::cout << "  weight behind the axle: " << ok.from_rest << " " << ok.after_drive << " " << ok.after_stop << "; weight over it: " << broken.from_rest << " " << broken.after_drive << " " << broken.after_stop << "\n";
  RC_CHECK(!context_dependent(ok));
  RC_CHECK(context_dependent(broken));
  RC_CHECK(broken.after_drive < 0.85);
  RC_CHECK_NEAR(broken.from_rest, 1.0, 0.08);

  // How the shortfall depends on where the weight is: the further behind the axle, the smaller it is.
  const double margins[] = {0.003, 0.006, 0.010};
  const char* keys[] = {"sweep_3", "sweep_6", "sweep_10"};
  double last = broken.after_drive;
  for (int i = 0; i < 3; ++i) {
    const RobotParse r = parse_robot(std::string(kBurger) + "com_back " + std::to_string(margins[i]) + " source test: a sweep of where the weight is\n");
    std::vector<ScriptStep> drive(1, ScriptStep{0.2, 0.0, 2.0});
    const double ratio = turn_ratio(gazebo, scenario(), r.robot, drive);
    rec(keys[i], ratio, 2);
    std::cout << "  weight " << margins[i] * 1000.0 << " mm behind the axle: " << ratio << " of the turn after a drive\n";
    RC_CHECK(ratio > last - 0.12);
    last = ratio;
  }
  RC_CHECK(last > 0.85);

  // With the weight right, a single efficiency fits the five cases against Gazebo.
  const std::vector<GapCase> cases = standard_cases();
  const std::vector<std::vector<BackendPose>> traces = run_all(gazebo, scenario(), robot(), cases, 0.1);
  const Fit f = fit_efficiency(traces, scenario(), robot(), cases, 0.0, 0.062, 0.8, 1.2, 21);
  rec("gz_fit_eff", f.efficiency, 2);
  rec("gz_fit_gap", f.worst_gap, 3);
  RC_CHECK(f.worst_gap < 0.05);
  RC_CHECK_NEAR(f.efficiency, 1.0, 0.1);
}
#endif
