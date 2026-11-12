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


#ifdef RC_HAVE_GZ
#include <rc/gz/gz_backend.hpp>
#endif

namespace {

const double kPi = 3.14159265358979323846;

const char* kBurger =
    "robot burger\nkind diff_drive\n"
    "wheel_radius 0.033 source test\ntrack_width 0.160 source test\nlength 0.138 source test\nwidth 0.178 source test\n"
    "height 0.192 source test\nmass 1.0 source test\nwheel_mass 0.03 source typical\nclearance 0.02 source typical\n"
    "wheel_width 0.018 source typical\nmax_speed 0.22 source test\nmax_turn_rate 2.84 source test\n";

const char* kYard = "scenario yard\nduration 30\nground gravel\nstart 1 2 90\n";

rc::field::FieldScenario scenario() { return rc::field::parse_scenario(kYard).scenario; }
rc::field::RobotModel robot() { return rc::field::parse_robot(kBurger).robot; }

std::vector<BackendPose> line(double dx, int n) {
  std::vector<BackendPose> t;
  for (int i = 0; i < n; ++i) {
    BackendPose p;
    p.t = 0.1 * i;
    p.x = dx * i;
    t.push_back(p);
  }
  return t;
}

}  // namespace

RC_TEST("the gap between two traces is the distance, sample against sample") {
  const std::vector<BackendPose> a = line(0.1, 11);
  std::vector<BackendPose> b = a;
  const TraceGap same = compare_traces(a, b);
  RC_CHECK_NEAR(same.max_position, 0.0, 1e-15);
  RC_CHECK(same.samples == 11);
  for (std::size_t i = 0; i < b.size(); ++i) b[i].y += 0.1;
  b.back().theta = kPi / 2.0;
  const TraceGap shifted = compare_traces(a, b);
  RC_CHECK_NEAR(shifted.max_position, 0.1, 1e-12);
  RC_CHECK_NEAR(shifted.rms_position, 0.1, 1e-12);
  RC_CHECK_NEAR(shifted.final_position, 0.1, 1e-12);
  RC_CHECK_NEAR(shifted.final_heading_deg, 90.0, 1e-9);
  std::vector<BackendPose> shorter(b.begin(), b.begin() + 5);
  RC_CHECK(compare_traces(a, shorter).samples == 5);
  RC_CHECK(compare_traces(a, std::vector<BackendPose>()).samples == 0);
}

RC_TEST("the time constant of a first order response is read back from its trace") {
  for (int k = 0; k < 3; ++k) {
    const double tau = 0.05 + 0.15 * k;
    std::vector<BackendPose> t;
    double x = 0.0;
    for (int i = 0; i <= 400; ++i) {
      BackendPose p;
      p.t = 0.01 * i;
      p.x = x;
      t.push_back(p);
      x += 0.2 * (1.0 - std::exp(-0.01 * i / tau)) * 0.01;
    }
    const double read = response_time_constant(t);
    std::cout << "  set " << tau << " read " << read << "\n";
    RC_CHECK_NEAR(read, tau, 0.012);
  }
  RC_CHECK(response_time_constant(line(0.1, 3)) == 0.0);
  RC_CHECK(response_time_constant(std::vector<BackendPose>(20)) == 0.0);
}

RC_TEST("two built in models with different lags differ, the lag is read from the slower one, and set in the faster one they agree") {
  BuiltinBackend fast(0.05);
  BuiltinBackend slow(0.30);
  const std::vector<GapCase> cases = standard_cases();
  const std::vector<TraceGap> before = run_cases(fast, slow, scenario(), robot(), cases, 0.1);
  std::cout << gap_table(cases, before);
  rec("syn_before", worst_gap(before), 3);

  // The step response of the slow one, sampled finely, as one would sample the other simulator.
  RC_CHECK(slow.start(scenario(), robot()));
  const double tau = response_time_constant(run_script(slow, std::vector<ScriptStep>(1, ScriptStep{0.2, 0.0, 3.0}), 0.01));
  rec("syn_tau", tau, 3);
  RC_CHECK_NEAR(tau, 0.30, 0.02);

  BuiltinBackend calibrated(tau);
  const std::vector<TraceGap> after = run_cases(calibrated, slow, scenario(), robot(), cases, 0.1);
  rec("syn_after", worst_gap(after), 4);
  RC_CHECK(worst_gap(before) > 0.03);
  RC_CHECK(worst_gap(after) < 0.2 * worst_gap(before));
  RC_CHECK(gap_table(cases, before).find("straight: largest") == 0);
}

#ifdef RC_HAVE_GZ
RC_TEST("the built in model and Gazebo Sim on the five cases, before and after the built in model is given Gazebo's time constant") {
  rc::gz::GzBackend gazebo;
  const std::vector<GapCase> cases = standard_cases();
  BuiltinBackend uncalibrated(0.05);
  const std::vector<TraceGap> before = run_cases(uncalibrated, gazebo, scenario(), robot(), cases, 0.1);
  std::cout << "  before calibration:\n" << gap_table(cases, before);
  rec("before_max", worst_gap(before), 3);
  rec("straight_before", before[0].max_position, 3);
  rec("turn_before", before[1].max_position, 3);
  rec("stop_before", before[3].max_position, 3);
  rec("combined_before", before[4].max_position, 3);
  rec("combined_deg_before", before[4].final_heading_deg, 1);

  RC_CHECK(gazebo.start(scenario(), robot()));
  const double tau = response_time_constant(run_script(gazebo, std::vector<ScriptStep>(1, ScriptStep{0.2, 0.0, 3.0}), 0.01));
  rec("tau_gz", tau, 3);
  RC_CHECK(tau > 0.0);

  BuiltinBackend calibrated(tau);
  const std::vector<TraceGap> after = run_cases(calibrated, gazebo, scenario(), robot(), cases, 0.1);
  std::cout << "  after calibration:\n" << gap_table(cases, after);
  rec("after_max", worst_gap(after), 3);
  rec("straight_after", after[0].max_position, 3);
  rec("turn_after", after[1].max_position, 3);
  rec("stop_after", after[3].max_position, 3);
  rec("combined_after", after[4].max_position, 3);
  rec("combined_deg_after", after[4].final_heading_deg, 1);
  // The simple cases improve; the compound one does not have to, and that is what the lesson is about.
  RC_CHECK(after[0].max_position <= before[0].max_position + 0.01);
  RC_CHECK(worst_gap(after) < 0.5);
}
#endif
