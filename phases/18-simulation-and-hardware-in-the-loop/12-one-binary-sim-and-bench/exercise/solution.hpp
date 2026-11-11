#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <rc/sim/bench_link.hpp>
#include <rc/sim/judge.hpp>
#include <rc/sim/plant.hpp>
#include <rc/sim/scenario.hpp>
#include <rc/sim/sensor_model.hpp>
#include <rc/sim/streams.hpp>

using rc::sim::CommandFrame;
using rc::sim::ControllerSide;
using rc::sim::MeasureFrame;
using rc::sim::PlantEmulator;
using rc::sim::SensorModel;
using rc::sim::SensorOptions;
using rc::sim::SimMotor;
using rc::sim::Trace;

enum class BackendKind { sim, bench };

// Everything that is chosen when the program starts. The controller does not see any of it.
struct RigConfig {
  BackendKind kind = BackendKind::sim;
  int link_delay = 1;          // bench: ticks each way
  double loss = 0.0;           // bench: the chance a frame is lost, each way
  double noise = 0.0;          // the position sensor's noise, in the units of the position
  int sensor_delay = 0;        // sim: how many ticks old the position is when the controller sees it
  int watchdog = 5;            // bench: the plant side's watchdog, in ticks
  unsigned seed = 1;
};

struct ConfigError {
  int line = 0;
  std::string message;
};

struct ConfigResult {
  bool ok = false;
  std::vector<ConfigError> errors;
};

inline std::string trimmed(const std::string& s) {
  std::size_t a = 0;
  std::size_t b = s.size();
  while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
  while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
  return s.substr(a, b - a);
}

// Read a configuration from text, one `key = value` to a line, with # starting a comment:
//   backend = sim | bench     required
//   link_delay = <whole number, 0 or more>      loss = <0 to 1>      noise = <0 or more>
//   sensor_delay = <whole number, 0 or more>    watchdog = <whole number, 0 or more>    seed = <whole number, 1 or more>
// Every bad line is reported with its number. Unknown keys are errors, and a missing backend is an error too. With any error the result is not ok and `out` is unchanged.
inline ConfigResult parse_config(const std::string& text, RigConfig& out) {
  // TODO: Read the text line by line (split at newlines), counting from 1. Cut a line at the first #, trim spaces (trimmed() does it), skip it if empty. Each line is key = value (split at the first =); no = is an error "expected: key = value".
  // Keys: backend (sim or bench, anything else an error) sets kind and notes that a backend was given; link_delay, sensor_delay and watchdog are whole numbers of 0 or more; loss is a number from 0 to 1; noise is a number of 0 or more; seed is a whole number of 1 or more. A number is the whole value (use strtod and check it consumed it all).
  // An unknown key is an error "unknown setting: KEY". Collect every error with its line number (ConfigError{line, message}) and go on. At the end, no backend line and no other error is the error "backend is required".
  // If there is any error return with ok false and leave `out` as it was; otherwise copy the settings into `out` and return ok.
  (void)text;
  (void)out;
  return ConfigResult();
}

// What the controller talks to. It gives a command for one tick and gets back the position as measured, and the truth is there for the test to look at.
class Backend {
 public:
  virtual ~Backend() {}
  virtual const char* name() const = 0;
  virtual double step(double command, double dt) = 0;     // the measured position after the tick
  virtual double truth() const = 0;                       // where the plant really is
};

// The plant and the sensor, directly: no link.
class SimBackend : public Backend {
 public:
  explicit SimBackend(const RigConfig& c) : motor_(2.0, 0.1, 12.0), sensor_(options(c)) {}
  const char* name() const override { return "sim"; }
  double step(double command, double dt) override {
    motor_.apply(command, dt);
    const rc::sim::Reading r = sensor_.read(motor_.position());
    if (r.valid) last_ = r.value;
    return last_;
  }
  double truth() const override { return motor_.position(); }

 private:
  static SensorOptions options(const RigConfig& c) {
    SensorOptions o;
    o.noise = c.noise;
    o.delay = c.sensor_delay;
    o.seed = c.seed;
    return o;
  }
  SimMotor motor_;
  SensorModel sensor_;
  double last_ = 0.0;
};

// The plant behind a link, as on a bench: commands go down, measurements come back, each way delayed and possibly lost, and the plant side has its watchdog.
class BenchBackend : public Backend {
 public:
  explicit BenchBackend(const RigConfig& c)
      : emulator_(2.0, 0.1, 12.0, c.watchdog), sensor_(options(c)), config_(c), streams_(c.seed) {}
  const char* name() const override { return "bench"; }

  double step(double command, double dt) override {
    // TODO: one tick of the bench.
    // 1. The command goes down: draw streams_.get("down").uniform(); if it is at least config_.loss the command is not lost: push Pending<CommandFrame>{tick_ + config_.link_delay, CommandFrame{++seq_, command}} on down_. Then deliver_down().
    // 2. The plant steps: MeasureFrame m = emulator_.step(dt); truth_ = m.position; read it through the sensor: if sensor_.read(m.position) is valid, m.position = its value.
    // 3. The measurement goes up the same way (streams_.get("up"), loss, delay) onto up_; then deliver_up(); then controller_.end_tick(); then ++tick_.
    // Return controller_.position(), the position the controller side holds.
    (void)command;
    (void)dt;
    (void)emulator_;
    (void)sensor_;
    (void)config_;
    (void)streams_;
    (void)down_;
    (void)up_;
    (void)tick_;
    (void)seq_;
    (void)truth_;
    return 0.0;
  }

  double truth() const override { return truth_; }

 private:
  template <class T>
  struct Pending {
    int arrives;
    T frame;
  };
  static SensorOptions options(const RigConfig& c) {
    SensorOptions o;
    o.noise = c.noise;
    o.seed = c.seed + 1000;
    return o;
  }
  void deliver_down() {
    std::vector<Pending<CommandFrame>> rest;
    for (const Pending<CommandFrame>& p : down_) {
      if (p.arrives <= tick_) emulator_.on_command(p.frame);
      else rest.push_back(p);
    }
    down_ = rest;
  }
  void deliver_up() {
    std::vector<Pending<MeasureFrame>> rest;
    for (const Pending<MeasureFrame>& p : up_) {
      if (p.arrives <= tick_) controller_.accept(p.frame);
      else rest.push_back(p);
    }
    up_ = rest;
  }

  PlantEmulator emulator_;
  ControllerSide controller_;
  SensorModel sensor_;
  RigConfig config_;
  rc::sim::Streams streams_;
  std::vector<Pending<CommandFrame>> down_;
  std::vector<Pending<MeasureFrame>> up_;
  int tick_ = 0;
  int seq_ = 0;
  double truth_ = 0.0;
};

// Make the backend the configuration asks for.
inline std::unique_ptr<Backend> make_backend(const RigConfig& c) {
  // TODO: a BenchBackend when c.kind is BackendKind::bench, otherwise a SimBackend, as a std::unique_ptr<Backend> (std::unique_ptr<Backend>(new BenchBackend(c)))
  (void)c;
  return std::unique_ptr<Backend>();
}

// Run a scenario with a proportional controller of gain `kp` against any backend, at 100 Hz, and record the target and where the plant really was.
// The controller never learns which backend it has. The change times are returned for judging.
inline Trace run_scenario(Backend& backend, const rc::sim::Scenario& scenario, double kp, std::vector<double>& change_times) {
  Trace trace;
  trace.dt = 0.01;
  change_times.clear();
  double target = 0.0;
  double measured = 0.0;
  double t0 = -1.0;
  const int steps = static_cast<int>(scenario.duration() / trace.dt + 0.5);
  for (int i = 0; i < steps; ++i) {
    const double t1 = i * trace.dt;
    for (const rc::sim::ScenarioEvent& e : rc::sim::events_between(scenario, t0, t1))
      if (e.kind == rc::sim::ScenarioEvent::Kind::target) {
        target = e.value;
        change_times.push_back(e.time);
      }
    t0 = t1;
    measured = backend.step(kp * (target - measured), trace.dt);
    trace.target.push_back(target);
    trace.position.push_back(backend.truth());
  }
  return trace;
}

#endif  // LESSON_SOLUTION_HPP
