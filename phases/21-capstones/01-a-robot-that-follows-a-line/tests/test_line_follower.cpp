#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

LineConfig config(double kp, double speed = 0.4, int delay = 0, int debounce = 3) {
  LineConfig c;
  c.kp = kp;
  c.speed = speed;
  c.delay_frames = delay;
  c.lost_debounce = debounce;
  return c;
}

LineResult run(const LineConfig& c, const LineFault& f = LineFault(), double seconds = 20.0) { return follow_line(LineTrack(), c, f, seconds); }

}  // namespace

RC_TEST("the camera sees the line to the left or the right by as much as the robot is off it, and sees nothing when it is dead or the line is out of view") {
  LineTrack straight;
  straight.amplitude = 0.0;
  std::vector<std::uint8_t> buf;
  rc::vision::Gray8 img;
  std::cout << "  a straight line, the robot at these distances to its left: measured offset of the line\n";
  const double ys[] = {-0.10, 0.0, 0.05, 0.10};
  for (double y : ys) {
    rc::sim::Pose p;
    p.y = y;
    render_line(straight, p, LineFault(), false, buf, img);
    const LineMeasure m = measure_line(img);
    std::cout << "  robot at " << std::showpos << std::fixed << std::setprecision(2) << y << std::noshowpos << " m: line " << std::showpos << m.offset << std::noshowpos << " m (" << m.pixels << " pixels)\n";
    RC_CHECK(m.found);
    RC_CHECK_NEAR(m.offset, -y, 0.01);
  }
  rc::sim::Pose p;
  render_line(straight, p, LineFault(), true, buf, img);
  RC_CHECK(!measure_line(img).found);
  p.y = 0.5;
  render_line(straight, p, LineFault(), false, buf, img);
  RC_CHECK(!measure_line(img).found);
  std::cout << "\n";
}

RC_TEST("losing the line is ok, then suspect, then faulted, by how many frames it has been gone, and the wheels turn the robot the way the turn rate says") {
  using rc::safety::CheckState;
  RC_CHECK(line_sensor_state(0, 3) == CheckState::ok);
  RC_CHECK(line_sensor_state(1, 3) == CheckState::suspect);
  RC_CHECK(line_sensor_state(2, 3) == CheckState::suspect);
  RC_CHECK(line_sensor_state(3, 3) == CheckState::faulted);
  RC_CHECK(line_sensor_state(40, 3) == CheckState::faulted);
  RC_CHECK(line_sensor_state(1, 1) == CheckState::faulted);
  RC_CHECK(line_sensor_state(1, 0) == CheckState::faulted);
  RC_CHECK(line_sensor_state(-5, 3) == CheckState::ok);
  const LineWheels straight = line_wheels(0.4, 0.0, 0.2);
  RC_CHECK_NEAR(straight.left, 0.4, 1e-12);
  RC_CHECK_NEAR(straight.right, 0.4, 1e-12);
  const LineWheels left = line_wheels(0.4, 2.0, 0.2);
  RC_CHECK_NEAR(left.left, 0.2, 1e-12);
  RC_CHECK_NEAR(left.right, 0.6, 1e-12);
  RC_CHECK(left.right > left.left);
  RC_CHECK_NEAR((left.left + left.right) / 2.0, 0.4, 1e-12);
  RC_CHECK_NEAR(line_wheels(0.0, -1.0, 0.2).left, 0.1, 1e-12);
}

RC_TEST("the gain decides how close it holds the line, and the whole of the course is driven under the supervisor without a stop") {
  std::cout << "  8 m of course at 0.4 m/s, started 5 cm off the line (kd 0.2):\n  kp    rms error    largest    reached\n";
  const double kps[] = {1.5, 3.0, 5.0, 8.0};
  double rms[4];
  for (int i = 0; i < 4; ++i) {
    const LineResult r = run(config(kps[i]));
    rms[i] = r.rms;
    std::cout << "  " << std::fixed << std::setprecision(1) << kps[i] << "   " << std::setprecision(4) << r.rms << "      " << r.max_error << "     " << std::setprecision(2) << r.final_x << " m" << (r.stopped ? "  STOPPED" : "") << "\n";
    RC_CHECK(!r.stopped);
    RC_CHECK(r.final_x > 7.3);
  }
  std::cout << "\n";
  RC_CHECK(rms[0] > 0.08);
  RC_CHECK(rms[1] < rms[0] / 2.0);
  RC_CHECK(rms[2] < 0.02);
  RC_CHECK(rms[2] < rms[1]);
  RC_CHECK(rms[3] > rms[2]);                          // past a point more gain does not help
}

RC_TEST("twice the speed makes the same controller about three times as far off the line") {
  const LineResult slow = run(config(5.0, 0.4), LineFault(), 20.0);
  const LineResult fast = run(config(5.0, 0.8), LineFault(), 10.0);
  std::cout << "  0.4 m/s: rms " << std::fixed << std::setprecision(4) << slow.rms << ", largest " << slow.max_error << "; 0.8 m/s: rms " << fast.rms << ", largest " << fast.max_error << " (" << std::setprecision(1) << fast.rms / slow.rms << " times)\n\n";
  RC_CHECK(!fast.stopped);
  RC_CHECK(fast.rms > 2.5 * slow.rms);
  RC_CHECK(fast.rms < 0.06);
}

RC_TEST("a delayed camera makes a fast robot wander, and one delayed enough loses the line, which the supervisor turns into a stop") {
  std::cout << "  0.8 m/s, kp 5: camera delay -> rms error\n";
  const int delays[] = {0, 6, 10, 15, 20};
  double rms[5];
  bool stopped[5];
  unsigned cause = 0;
  double stop_x = 0.0;
  for (int i = 0; i < 5; ++i) {
    const LineResult r = run(config(5.0, 0.8, delays[i]), LineFault(), 10.0);
    rms[i] = r.rms;
    stopped[i] = r.stopped;
    if (r.stopped) {
      cause = r.causes;
      stop_x = r.final_x;
    }
    std::cout << "  " << std::setw(3) << delays[i] * 20 << " ms  " << std::fixed << std::setprecision(4) << r.rms << "   largest " << r.max_error << (r.stopped ? "   LINE LOST, STOPPED at x " + std::to_string(r.final_x).substr(0, 4) + " m" : "") << "\n";
  }
  std::cout << "\n";
  RC_CHECK(!stopped[0] && !stopped[1] && !stopped[2] && !stopped[3]);
  RC_CHECK(stopped[4]);
  RC_CHECK(cause == rc::safety::cause_sensors);
  RC_CHECK(stop_x < 3.0);
  RC_CHECK(rms[3] > 1.4 * rms[0]);
  RC_CHECK(rms[2] >= rms[1] - 1e-9);
}

RC_TEST("a gap in the line shorter than the picture is driven across, a longer one stops the robot unless the debounce is longer than the gap takes") {
  std::cout << "  a gap of 0.2 m: ";
  LineFault small;
  small.gap_from = 3.0;
  small.gap_to = 3.2;
  const LineResult s = run(config(5.0, 0.4, 0, 3), small);
  std::cout << (s.stopped ? "stopped" : "driven across") << " (debounce 3)\n  a gap of 0.6 m:\n";
  LineFault big;
  big.gap_from = 3.0;
  big.gap_to = 3.6;
  const int debounces[] = {3, 10, 25, 50};
  bool stops[4];
  double at[4];
  for (int i = 0; i < 4; ++i) {
    const LineResult r = run(config(5.0, 0.4, 0, debounces[i]), big);
    stops[i] = r.stopped;
    at[i] = r.final_x;
    std::cout << "  debounce " << std::setw(2) << debounces[i] << " (" << debounces[i] * 20 << " ms): " << (r.stopped ? "stopped at x " + std::to_string(r.final_x).substr(0, 4) + " m" : "driven across, reached " + std::to_string(r.final_x).substr(0, 4) + " m") << "\n";
  }
  std::cout << "\n";
  RC_CHECK(!s.stopped && s.final_x > 7.3);
  RC_CHECK(stops[0] && stops[1] && stops[2]);
  RC_CHECK(!stops[3]);
  RC_CHECK(at[0] < 3.2);
  RC_CHECK(at[3] > 7.3);
}

RC_TEST("a camera that goes black stops the robot within three frames, and how far it goes first depends on the speed, and the black box shows how it came about") {
  LineFault dead;
  dead.camera_dead_at = 10.0;
  const LineResult slow = run(config(5.0, 0.4), dead, 30.0);
  const LineResult fast = run(config(5.0, 0.8), dead, 30.0);
  std::cout << "  camera dead at 10 s: 0.4 m/s stopped at " << std::fixed << std::setprecision(2) << slow.stopped_at << " s and went " << std::setprecision(3) << slow.stop_distance << " m further; 0.8 m/s stopped at " << std::setprecision(2) << fast.stopped_at
            << " s and went " << std::setprecision(3) << fast.stop_distance << " m further (" << std::setprecision(1) << fast.stop_distance / slow.stop_distance << " times)\n";
  RC_CHECK(slow.stopped && fast.stopped);
  RC_CHECK(slow.causes == rc::safety::cause_sensors && fast.causes == rc::safety::cause_sensors);
  RC_CHECK(slow.stopped_at > 10.0 && slow.stopped_at < 10.1);
  RC_CHECK(fast.stop_distance > 3.0 * slow.stop_distance);
  RC_CHECK(slow.stop_distance < 0.05);
  std::cout << "  the measured offsets in the black box just before the stop:";
  int missing = 0;
  for (double o : slow.offsets_before_stop) {
    if (std::isnan(o)) {
      std::cout << " none";
      ++missing;
    } else {
      std::cout << " " << std::showpos << std::setprecision(3) << o << std::noshowpos;
    }
  }
  std::cout << "\n\n";
  RC_CHECK(slow.offsets_before_stop.size() == 6);
  RC_CHECK(missing == 3);
}
