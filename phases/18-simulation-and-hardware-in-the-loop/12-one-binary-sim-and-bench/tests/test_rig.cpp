#include <rc/test/rc_test.hpp>
#include <rc/sim/compare.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

const char* kScript = "at 0 target 1\nat 2.5 target -0.5\nat 5 target 1\nat 8 end\n";

rc::sim::Scenario scenario() {
  rc::sim::Scenario s;
  rc::sim::parse_scenario(kScript, s);
  return s;
}

RigConfig config_from(const std::string& text) {
  RigConfig c;
  const ConfigResult r = parse_config(text, c);
  (void)r;
  return c;
}

Trace run_with(const RigConfig& c, std::vector<double>& changes) {
  std::unique_ptr<Backend> b = make_backend(c);
  if (!b) {
    changes.clear();
    return Trace();                                                    // nothing to run: the test's own checks say so
  }
  return run_scenario(*b, scenario(), 8.0, changes);
}

}  // namespace

RC_TEST("a configuration is read into settings, with defaults for what is not said, and the backend is the one thing that must be") {
  RigConfig c;
  const ConfigResult r = parse_config("# a bench run\nbackend = bench\nlink_delay = 2\nloss = 0.1   # one in ten\nnoise = 0.005\nwatchdog = 8\nseed = 7\n", c);
  RC_CHECK(r.ok);
  RC_CHECK(c.kind == BackendKind::bench);
  RC_CHECK(c.link_delay == 2);
  RC_CHECK_NEAR(c.loss, 0.1, 1e-12);
  RC_CHECK_NEAR(c.noise, 0.005, 1e-12);
  RC_CHECK(c.watchdog == 8);
  RC_CHECK(c.seed == 7);
  RigConfig d;
  RC_CHECK(parse_config("backend=sim", d).ok);                           // no spaces needed, no newline at the end
  RC_CHECK(d.kind == BackendKind::sim);
  RC_CHECK(d.link_delay == 1 && d.loss == 0.0 && d.watchdog == 5 && d.seed == 1);
  RC_CHECK(d.sensor_delay == 0);
  std::cout << "  'backend = bench' with a delay, a loss, noise and a watchdog was read; 'backend=sim' alone gave the defaults: link delay " << d.link_delay << ", watchdog " << d.watchdog
            << ", seed " << d.seed << "\n\n";
}

RC_TEST("every bad line is reported with its number, a missing backend is an error, and a configuration with errors changes nothing") {
  RigConfig c;
  c.seed = 99;
  const ConfigResult r = parse_config("backend = hardware\nlink_delay = 1.5\nloss = 2\nnoise = -1\nseed = 0\nspeed = 3\njust some words\nwatchdog = -2\n", c);
  std::cout << "  bad lines found:";
  for (const ConfigError& e : r.errors) std::cout << " " << e.line;
  std::cout << "\n  first: line " << (r.errors.empty() ? 0 : r.errors[0].line) << ": " << (r.errors.empty() ? "" : r.errors[0].message) << "\n\n";
  RC_CHECK(!r.ok);
  RC_CHECK(r.errors.size() == 8);
  if (r.errors.size() != 8) return;
  for (int i = 0; i < 8; ++i) RC_CHECK(r.errors[static_cast<std::size_t>(i)].line == i + 1);
  RC_CHECK(r.errors[0].message == "backend is sim or bench, not hardware");
  RC_CHECK(r.errors[5].message == "unknown setting: speed");
  RC_CHECK(c.seed == 99);                                                 // left alone
  const ConfigResult none = parse_config("loss = 0.1\n", c);
  RC_CHECK(!none.ok);
  RC_CHECK(none.errors.size() == 1 && none.errors[0].message == "backend is required");
  RC_CHECK(!parse_config("", c).ok);
}

RC_TEST("the configuration chooses the backend, and both answer to the same four questions") {
  const std::unique_ptr<Backend> sim = make_backend(config_from("backend = sim"));
  const std::unique_ptr<Backend> bench = make_backend(config_from("backend = bench"));
  RC_CHECK(sim != nullptr && bench != nullptr);
  if (sim == nullptr || bench == nullptr) return;
  std::cout << "  backends made: " << sim->name() << " and " << bench->name() << "; before any step both have the plant at " << sim->truth() << " and " << bench->truth() << "\n\n";
  RC_CHECK(std::string(sim->name()) == "sim");
  RC_CHECK(std::string(bench->name()) == "bench");
  RC_CHECK(sim->truth() == 0.0 && bench->truth() == 0.0);
  for (int i = 0; i < 100; ++i) {
    sim->step(6.0, 0.01);
    bench->step(6.0, 0.01);
  }
  RC_CHECK(sim->truth() > 1.0);
  RC_CHECK(bench->truth() > 1.0);
}

RC_TEST("the same controller and scenario on a clean bench and on the simulation give nearly the same run, and both pass the same criteria") {
  std::vector<double> c1;
  std::vector<double> c2;
  const Trace a = run_with(config_from("backend = sim"), c1);
  const Trace b = run_with(config_from("backend = bench\nlink_delay = 0\nloss = 0"), c2);
  const rc::sim::Metrics m = rc::sim::compare(a.position, b.position, 10);
  const rc::sim::Criteria criteria{0.3, 1.0, 0.02};
  const std::vector<std::string> va = rc::sim::verdict(rc::sim::analyse_steps(a, c1, 0.02), criteria);
  const std::vector<std::string> vb = rc::sim::verdict(rc::sim::analyse_steps(b, c2, 0.02), criteria);
  std::cout << "  the scenario of three steps, a proportional controller of gain 8, 800 ticks\n  simulation against a bench with no delay and no loss: rms difference " << std::fixed << std::setprecision(4) << m.rmse
            << ", largest " << m.max_abs << ", best shift " << m.lag << " ticks; criteria failures: simulation " << va.size() << ", bench " << vb.size() << "\n\n";
  RC_CHECK(a.position.size() == 800);
  RC_CHECK(b.position.size() == 800);
  RC_CHECK(c1.size() == 3);
  RC_CHECK(m.rmse < 0.05);
  RC_CHECK(va.empty());
  RC_CHECK(vb.empty());
}

RC_TEST("a bench with a delay and loss differs from a simulation that does not know about them, and putting the delay in the simulation closes most of the gap") {
  const RigConfig bench_config = config_from("backend = bench\nlink_delay = 2\nloss = 0.05\nnoise = 0.003\nseed = 5");
  const RigConfig bench_repeat = config_from("backend = bench\nlink_delay = 2\nloss = 0.05\nnoise = 0.003\nseed = 6");
  std::vector<double> changes;
  const Trace bench = run_with(bench_config, changes);
  const Trace repeat = run_with(bench_repeat, changes);
  const double floor = rc::sim::noise_floor(bench.position, repeat.position);
  std::cout << "  the bench: link delay 2 ticks each way, 5 percent loss, noise 0.003; two runs of it differ by " << std::fixed << std::setprecision(4) << floor << " rms\n  simulation's sensor delay (ticks)   rms difference from the bench\n";
  double at_zero = 0.0;
  double best = 1e9;
  int best_delay = -1;
  for (int delay = 0; delay <= 8; delay += 1) {
    RigConfig c = config_from("backend = sim\nnoise = 0.003\nseed = 5");
    c.sensor_delay = delay;
    const Trace sim = run_with(c, changes);
    const rc::sim::Metrics m = rc::sim::compare(sim.position, bench.position, 0);
    std::cout << "  " << std::setw(33) << delay << "   " << std::setw(12) << m.rmse << "\n";
    if (delay == 0) at_zero = m.rmse;
    if (m.rmse < best) {
      best = m.rmse;
      best_delay = delay;
    }
  }
  std::cout << "\n  the best match is a sensor delay of " << best_delay << " ticks, an rms difference of " << best << " against " << at_zero << " with none\n\n";
  RC_CHECK(at_zero > 0.03);
  RC_CHECK(best < 0.6 * at_zero);
  RC_CHECK(best_delay >= 3 && best_delay <= 6);                           // about the round trip of the link, 4 ticks, plus the plant's own step
}

RC_TEST("the same configuration gives the same run to the last bit, and a different seed gives a different one") {
  std::vector<double> ch;
  const RigConfig c = config_from("backend = bench\nlink_delay = 1\nloss = 0.1\nnoise = 0.01\nseed = 3");
  const Trace a = run_with(c, ch);
  const Trace b = run_with(c, ch);
  RigConfig other = c;
  other.seed = 4;
  const Trace d = run_with(other, ch);
  std::cout << "  the same bench configuration twice: " << (a.position == b.position ? "identical" : "different") << "; with another seed: " << (a.position == d.position ? "identical" : "different") << "\n\n";
  RC_CHECK(a.position == b.position);
  RC_CHECK(a.position != d.position);
  RC_CHECK(a.position.size() == 800);
  RigConfig sim = config_from("backend = sim\nnoise = 0.01\nseed = 3");
  RC_CHECK(run_with(sim, ch).position == run_with(sim, ch).position);
}
