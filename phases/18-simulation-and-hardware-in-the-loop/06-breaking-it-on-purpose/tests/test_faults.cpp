#include <rc/test/rc_test.hpp>
#include <rc/sim/plant.hpp>
#include <rc/sim/sensor_model.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// A position loop on the motor of lesson 18-02, reading its position through a sensor with noise, with a fault switched on at `fault_at`.
// A shadow of the motor, driven by the same commands, says what the sensor should read; the monitors watch the difference.
struct Run {
  double residual_alarm_at = -1.0;
  double stuck_alarm_at = -1.0;
  int residual_alarms = 0;        // readings at which the residual monitor was raised
  double worst_position_error = 0.0;
};

Run run(FaultKind kind, double magnitude, double fault_at, double noise, double threshold, int persist, double second_target_at, double seconds,
        std::uint32_t seed) {
  rc::sim::SimMotor motor(2.0, 0.1, 12.0);
  rc::sim::SimMotor shadow(2.0, 0.1, 12.0);
  rc::sim::SensorOptions so;
  so.noise = noise;
  so.seed = seed;
  rc::sim::SensorModel sensor(so);
  FaultInjector fault;
  fault.set(kind, fault_at, magnitude);
  ResidualMonitor residual(threshold, persist);
  StuckMonitor stuck(25, 1e-9);

  Run r;
  double target = 1.0;
  double measured = 0.0;
  const double dt = 0.01;
  const int steps = static_cast<int>(seconds / dt + 0.5);
  for (int i = 1; i <= steps; ++i) {
    const double t = i * dt;
    if (second_target_at >= 0.0 && t >= second_target_at) target = -0.5;
    const double command = 8.0 * (target - measured);
    motor.apply(command, dt);
    shadow.apply(command, dt);
    measured = fault.distort(sensor.read(motor.position()).value, t);
    const bool moving = std::fabs(shadow.velocity()) > 0.01;
    const bool res_alarm = residual.update(measured - shadow.position());
    const bool stuck_alarm = stuck.update(measured, moving);
    if (res_alarm) {
      ++r.residual_alarms;
      if (r.residual_alarm_at < 0.0) r.residual_alarm_at = t;
    }
    if (stuck_alarm && r.stuck_alarm_at < 0.0) r.stuck_alarm_at = t;
    const double err = std::fabs(motor.position() - target);
    if (t > 1.0 && err > r.worst_position_error) r.worst_position_error = err;
  }
  return r;
}

}  // namespace

RC_TEST("each fault does its one thing from the moment it begins, and leaves the readings before alone") {
  FaultInjector stuck;
  stuck.set(FaultKind::stuck, 1.0, 0.0);
  FaultInjector offset;
  offset.set(FaultKind::offset, 1.0, 0.5);
  FaultInjector drift;
  drift.set(FaultKind::drift, 1.0, 0.1);
  FaultInjector spike;
  spike.set(FaultKind::spike, 1.0, 4.0);
  FaultInjector none;

  double s_before = stuck.distort(2.0, 0.5);
  double s_first = stuck.distort(3.0, 1.0);
  double s_later = stuck.distort(9.0, 2.0);
  std::cout << "  a stuck sensor reading 2.0, then 3.0, then 9.0 from the fault at 1.0 s: before " << s_before << ", " << s_first << ", " << s_later << "\n";
  RC_CHECK(s_before == 2.0);
  RC_CHECK(s_first == 2.0);                              // frozen at the last good reading
  RC_CHECK(s_later == 2.0);
  RC_CHECK(offset.distort(2.0, 0.5) == 2.0);
  RC_CHECK_NEAR(offset.distort(2.0, 1.5), 2.5, 1e-12);
  RC_CHECK_NEAR(drift.distort(2.0, 0.9), 2.0, 1e-12);
  RC_CHECK_NEAR(drift.distort(2.0, 3.0), 2.2, 1e-12);                      // 0.1 per second for 2 s
  RC_CHECK_NEAR(drift.distort(2.0, 11.0), 3.0, 1e-12);
  int spikes = 0;
  for (int i = 0; i < 100; ++i)
    if (spike.distort(1.0, 2.0 + i * 0.01) != 1.0) ++spikes;
  std::cout << "  a spike of 4 on every 25th reading: " << spikes << " spikes in 100 readings\n\n";
  RC_CHECK(spikes == 4);
  RC_CHECK(none.distort(7.0, 100.0) == 7.0);
}

RC_TEST("one reading over the line is not an alarm, a run of them is, and it clears when the readings come back") {
  ResidualMonitor m(0.1, 3);
  RC_CHECK(!m.update(0.05));
  RC_CHECK(!m.update(0.5));                              // one
  RC_CHECK(!m.update(0.05));                             // back inside: the run starts again
  RC_CHECK(!m.update(0.5));
  RC_CHECK(!m.update(-0.5));                             // two, either side of zero
  RC_CHECK(m.update(0.5));                               // three: the alarm
  RC_CHECK(m.alarm());
  RC_CHECK(m.update(0.9));                               // it stays up
  RC_CHECK(!m.update(0.0));                              // and drops as soon as one comes back
  m.update(1.0);
  m.update(1.0);
  m.update(1.0);
  m.reset();
  RC_CHECK(!m.alarm());
  ResidualMonitor eager(0.1, 0);                         // a persistence below one is one
  RC_CHECK(eager.update(0.5));
}

RC_TEST("a sensor that stops changing is noticed only when it was expected to change") {
  StuckMonitor m(5, 1e-9);
  bool alarm = false;
  for (int i = 0; i < 20; ++i) alarm = m.update(1.0, false);        // a robot at rest: nothing to see
  RC_CHECK(!alarm);
  int at = -1;
  for (int i = 0; i < 20; ++i)
    if (m.update(1.0, true) && at < 0) at = i;
  std::cout << "  an unchanging reading with no motion expected: no alarm in 20; with motion expected the alarm came at the " << at + 1 << "th reading\n\n";
  RC_CHECK(at == 4);                                                // the fifth in a row with motion expected
  RC_CHECK(!m.update(1.5, true));                                   // it changed
  StuckMonitor jittery(5, 1e-9);
  bool any = false;
  for (int i = 0; i < 50; ++i) any = any || jittery.update(1.0 + 1e-3 * (i % 2), true);
  RC_CHECK(!any);                                                   // noise keeps it from looking stuck
}

RC_TEST("a healthy run raises no alarm, and each fault is caught after a delay that depends on what it is") {
  const Run healthy = run(FaultKind::none, 0.0, 2.0, 0.002, 0.02, 3, 5.0, 10.0, 1);
  std::cout << "  a position loop, noise 0.002, residual threshold 0.02 for 3 readings; the fault begins at 2.0 s, a second target at 5.0 s\n"
            << "  fault                        residual alarm   after the fault (s)   stuck alarm\n"
            << "  none                         " << std::setw(14) << (healthy.residual_alarm_at < 0.0 ? "none" : "ALARM") << "\n";
  RC_CHECK(healthy.residual_alarm_at < 0.0);
  RC_CHECK(healthy.stuck_alarm_at < 0.0);

  const Run offset = run(FaultKind::offset, 0.3, 2.0, 0.002, 0.02, 3, 5.0, 10.0, 1);
  const Run drift = run(FaultKind::drift, 0.05, 2.0, 0.002, 0.02, 3, 5.0, 10.0, 1);
  const Run spike = run(FaultKind::spike, 0.4, 2.0, 0.002, 0.02, 3, 5.0, 10.0, 1);
  const Run stuck = run(FaultKind::stuck, 0.0, 2.0, 0.002, 0.02, 3, 5.0, 10.0, 1);
  const struct { const char* name; const Run* r; } rows[] = {{"offset of 0.3", &offset}, {"drift of 0.05 a second", &drift}, {"spike of 0.4, one in 25", &spike}, {"stuck", &stuck}};
  for (const auto& row : rows) {
    std::cout << "  " << std::left << std::setw(28) << row.name << std::right << std::setw(14) << (row.r->residual_alarm_at < 0.0 ? "never" : "yes") << "   " << std::setw(19) << std::fixed
              << std::setprecision(2) << (row.r->residual_alarm_at < 0.0 ? -1.0 : row.r->residual_alarm_at - 2.0) << "   " << std::setw(11)
              << (row.r->stuck_alarm_at < 0.0 ? "never" : "at the move") << "\n";
  }
  std::cout << "\n";
  RC_CHECK(offset.residual_alarm_at >= 2.0 && offset.residual_alarm_at - 2.0 < 0.1);                  // at once
  RC_CHECK(drift.residual_alarm_at - 2.0 > 0.2 && drift.residual_alarm_at - 2.0 < 0.8);               // about threshold over rate: 0.4 s
  RC_CHECK(spike.residual_alarm_at < 0.0);                                                           // one reading in 25 never lasts three
  RC_CHECK(stuck.residual_alarm_at - 2.0 > 2.0);                                                     // seconds late: a frozen reading at the target looks right
  RC_CHECK(stuck.stuck_alarm_at > 5.0 && stuck.stuck_alarm_at < 5.5);                                // the stuck monitor knows only when the robot was asked to move
}

RC_TEST("the line is a trade: set low it raises alarms on a healthy sensor, and set high it lets a drifting one go a long way") {
  std::cout << "  200 healthy runs of 10 s with noise 0.002: runs with a false alarm\n  threshold   persist 1   persist 3\n";
  int false_tight = 0;
  int false_loose = 0;
  for (double threshold : {0.004, 0.006, 0.01, 0.02}) {
    int one = 0;
    int three = 0;
    for (int seed = 1; seed <= 200; ++seed) {
      if (run(FaultKind::none, 0.0, 99.0, 0.002, threshold, 1, -1.0, 10.0, seed).residual_alarms > 0) ++one;
      if (run(FaultKind::none, 0.0, 99.0, 0.002, threshold, 3, -1.0, 10.0, seed).residual_alarms > 0) ++three;
    }
    std::cout << "  " << std::setw(9) << std::fixed << std::setprecision(3) << threshold << "   " << std::setw(9) << one << "   " << std::setw(9) << three << "\n";
    if (threshold == 0.004) false_tight = one;
    if (threshold == 0.02) false_loose = one;
  }
  std::cout << "\n  a drift of 0.05 a second from 2.0 s: how late the alarm comes, by threshold\n  threshold   alarm after the fault (s)\n";
  double late_low = 0.0;
  double late_high = 0.0;
  for (double threshold : {0.02, 0.05, 0.1, 0.2}) {
    const Run r = run(FaultKind::drift, 0.05, 2.0, 0.002, threshold, 3, -1.0, 10.0, 1);
    const double delay = r.residual_alarm_at < 0.0 ? -1.0 : r.residual_alarm_at - 2.0;
    std::cout << "  " << std::setw(9) << std::fixed << std::setprecision(2) << threshold << "   " << std::setw(18) << std::setprecision(2) << delay << "\n";
    if (threshold == 0.02) late_low = delay;
    if (threshold == 0.2) late_high = delay;
  }
  std::cout << "\n";
  RC_CHECK(false_tight > 100);
  RC_CHECK(false_loose == 0);
  RC_CHECK(late_low > 0.0);
  RC_CHECK(late_high > 4.0 * late_low);
}
