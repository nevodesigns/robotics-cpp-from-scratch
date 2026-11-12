// rc/field/gap.hpp
//
// The gap between two simulators, from lesson 22-15.
//
// compare_traces takes two traces of poses sampled at the same times and gives the largest and the rms distance between them, the distance and the heading difference at the end. speeds_of gives the speed in each sample; response_time_constant reads, from the trace of a robot started from rest at a steady command, the time it took to reach 63.2 percent of its final speed. standard_cases are four scripts (straight, turn, arc, stop), run_cases runs them on two backends, starting each afresh, and returns the gaps, and gap_table writes them in rows.
//
// The built in model with a time constant of 0.05 s and Gazebo Sim differed by 0.036 m at the worst over the five cases; Gazebo's response to a step of speed was read as a time constant of 0.062 s; with the built in model's set to that the worst gap was 0.036 m. Without Gazebo the same loop was run between two built in models, one with a time constant of 0.3 s: the gap was 0.050 m, the time constant read back was 0.299 s and the gap after calibrating was 0.0001 m.

#ifndef RC_RC_FIELD_GAP_HPP
#define RC_RC_FIELD_GAP_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <rc/field/backend.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace rc {
namespace field {

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
  const std::size_t n = a.size() < b.size() ? a.size() : b.size();
  double sum = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double d = std::hypot(a[i].x - b[i].x, a[i].y - b[i].y);
    g.max_position = std::max(g.max_position, d);
    sum += d * d;
    if (i + 1 == n) {
      g.final_position = d;
      g.final_heading_deg = std::fabs(angle_between(a[i].theta, b[i].theta)) * 180.0 / 3.14159265358979323846;
    }
  }
  g.samples = static_cast<int>(n);
  if (n > 0) g.rms_position = std::sqrt(sum / static_cast<double>(n));
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
  const std::size_t tail = v.size() / 5 > 0 ? v.size() / 5 : 1;
  double final_speed = 0.0;
  for (std::size_t i = v.size() - tail; i < v.size(); ++i) final_speed += v[i];
  final_speed /= static_cast<double>(tail);
  if (final_speed <= 0.0) return 0.0;
  const double target = 0.632 * final_speed;
  // Each speed is the average over its interval, so it belongs to the middle of it.
  double prev_t = 0.0;
  double prev_v = 0.0;
  for (std::size_t i = 0; i < v.size(); ++i) {
    const double mid = 0.5 * (t[i].t + t[i + 1].t);
    if (v[i] >= target) {
      const double span = v[i] - prev_v;
      return span > 0.0 ? prev_t + (target - prev_v) / span * (mid - prev_t) : mid;
    }
    prev_t = mid;
    prev_v = v[i];
  }
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

// Runs every case on a backend, started afresh on the scenario for each, and gives the traces. A case that could not be started gives an empty trace.
inline std::vector<std::vector<BackendPose>> run_all(Backend& backend, const FieldScenario& scenario, const RobotModel& robot, const std::vector<GapCase>& cases, double sample_dt) {
  std::vector<std::vector<BackendPose>> traces;
  for (std::size_t i = 0; i < cases.size(); ++i) {
    if (backend.start(scenario, robot)) traces.push_back(run_script(backend, cases[i].script, sample_dt));
    else traces.push_back(std::vector<BackendPose>());
  }
  return traces;
}

inline std::vector<TraceGap> gaps_of(const std::vector<std::vector<BackendPose>>& a, const std::vector<std::vector<BackendPose>>& b) {
  std::vector<TraceGap> gaps;
  for (std::size_t i = 0; i < a.size() && i < b.size(); ++i) gaps.push_back(compare_traces(a[i], b[i]));
  return gaps;
}

// Runs every case on both backends and gives the gap of each.
inline std::vector<TraceGap> run_cases(Backend& a, Backend& b, const FieldScenario& scenario, const RobotModel& robot, const std::vector<GapCase>& cases, double sample_dt) {
  return gaps_of(run_all(a, scenario, robot, cases, sample_dt), run_all(b, scenario, robot, cases, sample_dt));
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


}  // namespace field
}  // namespace rc

#endif
