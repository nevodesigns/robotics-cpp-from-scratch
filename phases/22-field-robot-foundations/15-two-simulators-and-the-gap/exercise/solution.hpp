#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <rc/field/backend.hpp>
#include <sstream>
#include <string>
#include <vector>

using namespace rc::field;

// How far apart two traces are, sample against sample.
struct TraceGap {
  double max_position = 0.0;       // the largest distance between the two poses at the same sample, metres
  double rms_position = 0.0;
  double final_position = 0.0;     // the distance at the last sample
  double final_heading_deg = 0.0;  // the difference of the headings at the last sample, degrees, always positive
  int samples = 0;
};

// The traces are taken to be sampled at the same times; if one is longer, only the samples both have are compared.
inline TraceGap compare_traces(const std::vector<BackendPose>& a, const std::vector<BackendPose>& b) {
  TraceGap g;
  (void)a;  // TODO: for each sample both have, the distance between the two poses; the largest, the root mean square, and the distance and the heading difference at the last one
  (void)b;
  return g;
}

// The speed in each interval of a trace, from the distance between its poses over the time between them.
inline std::vector<double> speeds_of(const std::vector<BackendPose>& t) {
  std::vector<double> v;
  for (std::size_t i = 1; i < t.size(); ++i) {
    const double dt = t[i].t - t[i - 1].t;
    v.push_back(dt > 0.0 ? std::hypot(t[i].x - t[i - 1].x, t[i].y - t[i - 1].y) / dt : 0.0);
  }
  return v;
}

// From the trace of a robot started from rest at a steady command: the time at which its speed first reached 63.2 percent of its final speed (the mean of the last fifth of the trace), by interpolating between the two samples either side. Zero if there is nothing to read.
inline double response_time_constant(const std::vector<BackendPose>& t) {
  const std::vector<double> v = speeds_of(t);
  if (v.size() < 5) return 0.0;
  (void)t;  // TODO: find the final speed (the mean of the last fifth), take 63.2 percent of it, and find the time at which the speed first reaches that, between the two samples either side; each speed belongs to the middle of its interval
  return 0.0;
}

struct GapCase {
  std::string name;
  std::vector<ScriptStep> script;
};

// Five things a robot is asked to do, each from rest: go straight, turn in place, drive an arc, stop, and the three in a row (drive, turn, drive).
inline std::vector<GapCase> standard_cases() {
  std::vector<GapCase> cases;
  GapCase straight;
  straight.name = "straight";
  straight.script.push_back(ScriptStep{0.2, 0.0, 5.0});
  GapCase turn;
  turn.name = "turn";
  turn.script.push_back(ScriptStep{0.0, 1.0, 1.5});
  GapCase arc;
  arc.name = "arc";
  arc.script.push_back(ScriptStep{0.2, 0.5, 6.0});
  GapCase stop;
  stop.name = "stop";
  stop.script.push_back(ScriptStep{0.2, 0.0, 3.0});
  stop.script.push_back(ScriptStep{0.0, 0.0, 2.0});
  GapCase combined;
  combined.name = "drive, turn, drive";
  combined.script.push_back(ScriptStep{0.2, 0.0, 2.0});
  combined.script.push_back(ScriptStep{0.0, 1.0, 1.6});
  combined.script.push_back(ScriptStep{0.2, 0.0, 1.0});
  cases.push_back(straight);
  cases.push_back(turn);
  cases.push_back(arc);
  cases.push_back(stop);
  cases.push_back(combined);
  return cases;
}

// Runs every case on both backends, each started afresh on the scenario, and gives the gap of each.
inline std::vector<TraceGap> run_cases(Backend& a, Backend& b, const FieldScenario& scenario, const RobotModel& robot, const std::vector<GapCase>& cases, double sample_dt) {
  std::vector<TraceGap> gaps;
  for (std::size_t i = 0; i < cases.size(); ++i) {
    if (!a.start(scenario, robot) || !b.start(scenario, robot)) {
      gaps.push_back(TraceGap());
      continue;
    }
    gaps.push_back(compare_traces(run_script(a, cases[i].script, sample_dt), run_script(b, cases[i].script, sample_dt)));
  }
  return gaps;
}

inline double worst_gap(const std::vector<TraceGap>& gaps) {
  double w = 0.0;
  for (std::size_t i = 0; i < gaps.size(); ++i) w = std::max(w, gaps[i].max_position);
  return w;
}

inline std::string gap_table(const std::vector<GapCase>& cases, const std::vector<TraceGap>& gaps) {
  std::ostringstream o;
  o << std::fixed << std::setprecision(3);
  for (std::size_t i = 0; i < cases.size() && i < gaps.size(); ++i)
    o << cases[i].name << ": largest " << gaps[i].max_position << " m, rms " << gaps[i].rms_position << " m, at the end " << gaps[i].final_position << " m and " << std::setprecision(1) << gaps[i].final_heading_deg << " degrees\n" << std::setprecision(3);
  return o.str();
}


#endif
