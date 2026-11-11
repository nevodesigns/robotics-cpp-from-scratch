// rc/sim/rig.hpp
//
// One program for simulation and bench, from lesson 18-12, graduated: the phase's capstone.
//
// A configuration read from text chooses the backend at start up (backend = sim or bench, with link delay, loss, noise, sensor delay, watchdog and seed), reporting every bad line and changing nothing on error. A simulation backend (plant and sensor) and a bench backend (the plant behind a lossy delayed link with a watchdog) answer to the same interface,
// and a scenario is run through either with a controller that never learns which.
//
// On a clean bench (no delay, no loss) the run was identical to the simulation's (rms difference 0.0000) and both passed the same criteria. Against a bench with a link delay of 2 ticks each way, 5 percent loss and noise 0.003 (two runs of it differing by 0.0053), a simulation with no sensor delay differed by 0.1901 rms and one with a sensor delay of 4 ticks, the link's
// round trip, by 0.0772. The same configuration twice gave identical runs; another seed gave a different one.

#ifndef RC_SIM_RIG_HPP
#define RC_SIM_RIG_HPP

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

namespace rc {
namespace sim {

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
  ConfigResult result;
  RigConfig parsed;
  bool have_backend = false;
  int line_number = 0;
  std::size_t start = 0;
  while (start < text.size()) {
    std::size_t stop = text.find('\n', start);
    if (stop == std::string::npos) stop = text.size();
    std::string line = text.substr(start, stop - start);
    start = stop + 1;
    ++line_number;
    const std::size_t hash = line.find('#');
    if (hash != std::string::npos) line = line.substr(0, hash);
    line = trimmed(line);
    if (line.empty()) continue;
    const auto fail = [&](const std::string& why) { result.errors.push_back(ConfigError{line_number, why}); };
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      fail("expected: key = value");
      continue;
    }
    const std::string key = trimmed(line.substr(0, eq));
    const std::string value = trimmed(line.substr(eq + 1));
    char* end = nullptr;
    const double number = std::strtod(value.c_str(), &end);
    const bool is_number = !value.empty() && end == value.c_str() + value.size();
    const bool is_whole = is_number && number == std::floor(number);
    if (key == "backend") {
      if (value == "sim") parsed.kind = BackendKind::sim;
      else if (value == "bench") parsed.kind = BackendKind::bench;
      else {
        fail("backend is sim or bench, not " + value);
        continue;
      }
      have_backend = true;
    } else if (key == "link_delay" || key == "sensor_delay" || key == "watchdog") {
      if (!is_whole || number < 0.0) {
        fail(key + " is a whole number, 0 or more");
        continue;
      }
      if (key == "link_delay") parsed.link_delay = static_cast<int>(number);
      else if (key == "sensor_delay") parsed.sensor_delay = static_cast<int>(number);
      else parsed.watchdog = static_cast<int>(number);
    } else if (key == "loss") {
      if (!is_number || number < 0.0 || number > 1.0) {
        fail("loss is a number from 0 to 1");
        continue;
      }
      parsed.loss = number;
    } else if (key == "noise") {
      if (!is_number || number < 0.0) {
        fail("noise is a number, 0 or more");
        continue;
      }
      parsed.noise = number;
    } else if (key == "seed") {
      if (!is_whole || number < 1.0) {
        fail("seed is a whole number, 1 or more");
        continue;
      }
      parsed.seed = static_cast<unsigned>(number);
    } else {
      fail("unknown setting: " + key);
    }
  }
  if (!have_backend && result.errors.empty()) result.errors.push_back(ConfigError{line_number, "backend is required"});
  if (!result.errors.empty()) return result;
  out = parsed;
  result.ok = true;
  return result;
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
    // The command goes down, unless the link loses it, and arrives after the link's delay.
    if (streams_.get("down").uniform() >= config_.loss) down_.push_back(Pending<CommandFrame>{tick_ + config_.link_delay, CommandFrame{++seq_, command}});
    deliver_down();
    // The plant steps, and its position is read by the sensor and sent back the same way.
    MeasureFrame m = emulator_.step(dt);
    truth_ = m.position;
    const rc::sim::Reading r = sensor_.read(m.position);
    if (r.valid) m.position = r.value;
    if (streams_.get("up").uniform() >= config_.loss) up_.push_back(Pending<MeasureFrame>{tick_ + config_.link_delay, m});
    deliver_up();
    controller_.end_tick();
    ++tick_;
    return controller_.position();
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
  if (c.kind == BackendKind::bench) return std::unique_ptr<Backend>(new BenchBackend(c));
  return std::unique_ptr<Backend>(new SimBackend(c));
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

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_RIG_HPP
