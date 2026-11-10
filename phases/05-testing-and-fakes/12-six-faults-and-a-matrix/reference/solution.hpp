#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <vector>

// A fault to put into the loop on purpose. Everything the loop does is a first-order plant driven by a
// PI controller; each field breaks one thing.
struct Fault {
  const char* name = "no fault";
  double kp = 2.0;
  double ki = 3.0;
  double tau = 0.5;
  double sensor_offset = 0.0;     // the sensor reads this much too high
  double actuator_limit = 2.5;    // what the actuator can really deliver
  int delay_steps = 0;            // the controller sees the sensor this many steps late
  int dropout_from = -1;          // the sensor reads zero from this step ...
  int dropout_to = -1;            // ... up to this one
  bool limiter = true;            // the controller holds its own output to the actuator's limit
  double declared_limit = 2.5;    // the limit the specification says the controller output must respect
};

// Everything a check might look at.
struct Run {
  std::vector<double> y;           // where the plant really was
  std::vector<double> seen;        // what the controller was told
  std::vector<double> u;           // what the controller commanded
  double dt = 0.001;
  double target = 1.0;
  double declared_limit = 2.5;
};

inline Run simulate_fault(const Fault& f, double seconds = 10.0) {
  Run r;
  r.declared_limit = f.declared_limit;
  double y = 0.0;
  double integral = 0.0;
  std::deque<double> pipe(static_cast<std::size_t>(f.delay_steps), 0.0);
  const int steps = static_cast<int>(seconds / r.dt);
  for (int i = 0; i < steps; ++i) {
    double measured = y + f.sensor_offset;
    if (i >= f.dropout_from && i < f.dropout_to) measured = 0.0;
    pipe.push_back(measured);
    const double seen = pipe.front();
    pipe.pop_front();

    const double error = r.target - seen;
    integral += error * r.dt;
    double u = f.kp * error + f.ki * integral;
    if (f.limiter) u = std::max(-f.actuator_limit, std::min(f.actuator_limit, u));
    // The actuator cannot deliver more than it has, whatever is asked of it.
    const double delivered = std::max(-f.actuator_limit, std::min(f.actuator_limit, u));

    y += r.dt * (delivered - y) / f.tau;
    r.y.push_back(y);
    r.seen.push_back(seen);
    r.u.push_back(u);
  }
  return r;
}

// A check says whether a run is fine: true if it passes, false if it has found a fault.
using Check = std::function<bool(const Run&)>;

// caught[f][c] is true when check c fails on faulty run f, that is, when it finds that fault.
inline std::vector<std::vector<bool> > detection_matrix(const std::vector<Check>& checks, const std::vector<Run>& faulty_runs) {
  std::vector<std::vector<bool> > caught(faulty_runs.size(), std::vector<bool>(checks.size(), false));
  for (std::size_t f = 0; f < faulty_runs.size(); ++f) {
    for (std::size_t c = 0; c < checks.size(); ++c) caught[f][c] = !checks[c](faulty_runs[f]);
  }
  return caught;
}

// The checks that fail on a run with no fault in it. There should be none.
inline int false_alarms(const std::vector<Check>& checks, const Run& clean) {
  int count = 0;
  for (const Check& check : checks) {
    if (!check(clean)) ++count;
  }
  return count;
}

// The faults that no check catches.
inline std::vector<std::size_t> faults_missed(const std::vector<std::vector<bool> >& caught) {
  std::vector<std::size_t> missed;
  for (std::size_t f = 0; f < caught.size(); ++f) {
    if (std::find(caught[f].begin(), caught[f].end(), true) == caught[f].end()) missed.push_back(f);
  }
  return missed;
}

// The faults that exactly one check catches. Take that check away, or change it, and the fault goes unseen.
inline std::vector<std::size_t> faults_with_one_witness(const std::vector<std::vector<bool> >& caught) {
  std::vector<std::size_t> alone;
  for (std::size_t f = 0; f < caught.size(); ++f) {
    int witnesses = 0;
    for (std::size_t c = 0; c < caught[f].size(); ++c) {
      if (caught[f][c]) ++witnesses;
    }
    if (witnesses == 1) alone.push_back(f);
  }
  return alone;
}

// How many faults would no longer be caught by anything if check `removed` were taken out.
inline int unseen_without(const std::vector<std::vector<bool> >& caught, std::size_t removed) {
  int unseen = 0;
  for (std::size_t f = 0; f < caught.size(); ++f) {
    bool still = false;
    for (std::size_t c = 0; c < caught[f].size(); ++c) {
      if (c != removed && caught[f][c]) still = true;
    }
    if (!still) ++unseen;
  }
  return unseen;
}

#endif  // LESSON_SOLUTION_HPP
