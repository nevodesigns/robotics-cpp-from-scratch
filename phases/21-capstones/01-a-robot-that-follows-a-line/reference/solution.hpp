#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <rc/control/limits.hpp>
#include <rc/control/pid.hpp>
#include <rc/safety/blackbox.hpp>
#include <rc/safety/geofence.hpp>
#include <rc/safety/supervisor.hpp>
#include <rc/sim/diff_drive.hpp>
#include <rc/vision/blobs.hpp>
#include <rc/vision/image.hpp>
#include <string>
#include <vector>

// The course: a line on the floor that runs along x and wanders to either side as a sine, y = amplitude * sin(2 pi x / wavelength), with a width (the half width of the painted line).
struct LineTrack {
  double amplitude = 0.3;
  double wavelength = 4.0;
  double half_width = 0.015;
  double y(double x) const { return amplitude * std::sin(6.283185307179586 * x / wavelength); }
  double slope(double x) const { return amplitude * 6.283185307179586 / wavelength * std::cos(6.283185307179586 * x / wavelength); }
};

// Things that go wrong with the course or the camera: no paint between two values of x, a camera that goes black from a time on.
struct LineFault {
  double gap_from = -1.0;
  double gap_to = -1.0;
  double camera_dead_at = -1.0;
};

// The camera looks straight down from the robot: an image of 64 by 48 pixels of 1 cm each, whose bottom row is 15 cm ahead of the robot and whose top row is 63 cm ahead, centred on the robot's line of travel.
const int kLineW = 64;
const int kLineH = 48;
const double kLinePx = 0.01;
const double kLineNear = 0.15;

// Draw what the camera sees: the pixels within the half width of the line (measured across it) are 220 and the rest 30; a dead camera is all 0. `buf` holds the pixels and `img` points at them.
inline void render_line(const LineTrack& t, const rc::sim::Pose& p, const LineFault& f, bool dead, std::vector<std::uint8_t>& buf, rc::vision::Gray8& img) {
  buf.assign(static_cast<std::size_t>(kLineW * kLineH), dead ? 0 : 30);
  img.data = buf.data();
  img.width = kLineW;
  img.height = kLineH;
  img.stride = kLineW;
  if (dead) return;
  const double c = std::cos(p.theta);
  const double s = std::sin(p.theta);
  for (int v = 0; v < kLineH; ++v) {
    for (int u = 0; u < kLineW; ++u) {
      const double fwd = kLineNear + (kLineH - 1 - v) * kLinePx;
      const double left = (31.5 - u) * kLinePx;
      const double X = p.x + fwd * c - left * s;
      const double Y = p.y + fwd * s + left * c;
      if (f.gap_from >= 0.0 && X >= f.gap_from && X <= f.gap_to) continue;
      const double d = std::fabs(Y - t.y(X)) / std::sqrt(1.0 + t.slope(X) * t.slope(X));
      if (d <= t.half_width) img.set(u, v, 220);
    }
  }
}

struct LineMeasure {
  bool found = false;
  double offset = 0.0;       // how far the line is to the LEFT of the robot's line of travel, in metres, at the middle of the image
  int pixels = 0;
};

// Where the line is: find the bright blobs (threshold 128, eight connected), take the largest, and if it has at least 8 pixels the line is found and its offset is (31.5 - centre_x) times the size of a pixel (31.5 is the middle of 64 pixels counting from 0). No blob that big: not found, and the offset and pixels left as they were made (0).
inline LineMeasure measure_line(const rc::vision::Gray8& img) {
  LineMeasure m;
  const std::vector<rc::vision::Blob> blobs = rc::vision::find_blobs(img, 128, rc::vision::Connectivity::eight);
  const rc::vision::Blob* best = nullptr;
  for (const rc::vision::Blob& b : blobs)
    if (best == nullptr || b.pixels > best->pixels) best = &b;
  if (best == nullptr || best->pixels < 8) return m;
  m.found = true;
  m.pixels = best->pixels;
  m.offset = (31.5 - best->centre_x) * kLinePx;
  return m;
}

// What the supervisor is told about the camera, from how many frames in a row have had no line: none lost is ok, from one up to one less than `debounce` is suspect (the robot slows), and `debounce` or more is faulted (the robot stops). A debounce of 0 or less is read as 1.
inline rc::safety::CheckState line_sensor_state(int lost_frames, int debounce) {
  const int d = debounce < 1 ? 1 : debounce;
  if (lost_frames <= 0) return rc::safety::CheckState::ok;
  if (lost_frames >= d) return rc::safety::CheckState::faulted;
  return rc::safety::CheckState::suspect;
}

struct LineWheels {
  double left = 0.0;
  double right = 0.0;
};

// The wheel speeds for a forward speed and a turn rate (radians a second, left positive) on a robot whose wheels are `wheel_base` apart: the left wheel at speed minus omega * wheel_base / 2 and the right at speed plus it.
inline LineWheels line_wheels(double speed, double omega, double wheel_base) {
  LineWheels w;
  w.left = speed - omega * wheel_base / 2.0;
  w.right = speed + omega * wheel_base / 2.0;
  return w;
}

struct LineConfig {
  double kp = 5.0;
  double ki = 0.0;
  double kd = 0.2;
  double speed = 0.4;
  int delay_frames = 0;             // the frames between a picture being taken and the controller having it
  int lost_debounce = 3;
  double max_turn = 3.0;
};

struct LineResult {
  double rms = 0.0;                 // the root mean square of the true distance from the line, over the run
  double max_error = 0.0;
  double distance = 0.0;
  double final_x = 0.0;
  bool stopped = false;
  double stopped_at = 0.0;
  double stop_distance = 0.0;       // travelled between the supervisor's stop and standing still
  unsigned causes = 0;
  int frames = 0;
  std::vector<double> offsets_before_stop;   // the measured offsets in the black box just before the stop, NaN where the line was not found
};

// Run the robot along the line for `seconds` at 50 frames a second, started 5 cm off the line, under a supervisor whose top speed is the robot's speed and whose slow speed is half of it, a fence 0.7 m either side of the x axis and a debounce on losing the line.
// Each frame: draw the picture, measure it, pass the measurement through a delay of `delay_frames` frames (for these frames at the start there is nothing yet, which is not counted as a lost line), count frames without the line, tell the supervisor, and if it has not stopped steer with the PID on
// minus the measured offset (so a line to the left turns left) and drive. When the supervisor stops the robot, the speed goes down at 2 m/s^2 and the run ends when it is still, and the black box has the event.
inline LineResult follow_line(const LineTrack& track, const LineConfig& cfg, const LineFault& fault, double seconds) {
  LineResult r;
  const double dt = 0.02;
  const double wheel_base = 0.2;
  rc::sim::Pose pose;
  pose.y = 0.05;
  rc::control::Pid pid(cfg.kp, cfg.ki, cfg.kd, -cfg.max_turn, cfg.max_turn);
  rc::safety::SupervisorConfig sc;
  sc.top_speed = cfg.speed;
  sc.slow_speed = 0.5 * cfg.speed;
  rc::safety::Supervisor sup(sc);
  const std::vector<rc::safety::Point> area = {{-0.5, -0.7}, {10.0, -0.7}, {10.0, 0.7}, {-0.5, 0.7}};
  rc::safety::Inputs healthy;
  healthy.estop_closed = true;
  healthy.heartbeat_fresh = true;
  healthy.fence = rc::safety::FenceState::inside;
  healthy.tilt = rc::safety::TiltState::ok;
  healthy.sensors = rc::safety::CheckState::ok;
  healthy.power.state = rc::safety::PowerState::ok;
  healthy.power.factor = 1.0;
  healthy.range_cap = cfg.speed + 1.0;
  sup.reset(healthy);
  std::deque<LineMeasure> pipe(static_cast<std::size_t>(cfg.delay_frames), LineMeasure());
  std::vector<std::uint8_t> buf;
  rc::vision::Gray8 img;
  rc::safety::BlackBox box(128);
  int lost = 0;
  double v = cfg.speed;
  double squares = 0.0;
  bool braking = false;
  double distance_at_stop = 0.0;
  const int steps = static_cast<int>(seconds / dt);
  for (int i = 0; i < steps; ++i) {
    const double t = i * dt;
    const bool dead = fault.camera_dead_at >= 0.0 && t >= fault.camera_dead_at;
    render_line(track, pose, fault, dead, buf, img);
    pipe.push_back(measure_line(img));
    LineMeasure used = pipe.back();
    if (cfg.delay_frames > 0) {
      used = pipe.front();
      pipe.pop_front();
    }
    if (i < cfg.delay_frames) {
      used.found = true;
      used.offset = 0.0;
    }
    lost = used.found ? 0 : lost + 1;
    rc::safety::Inputs in = healthy;
    in.fence = rc::safety::fence_state(area, {pose.x, pose.y}, 0.1, 0.1);
    in.sensors = line_sensor_state(lost, cfg.lost_debounce);
    const rc::safety::Decision d = sup.step(in);
    const double slope = track.slope(pose.x);
    const double e_true = (pose.y - track.y(pose.x)) / std::sqrt(1.0 + slope * slope);
    squares += e_true * e_true;
    r.max_error = std::max(r.max_error, std::fabs(e_true));
    ++r.frames;
    box.record(t, 1, used.found ? used.offset : std::nan(""));
    double target = std::min(cfg.speed, d.speed_cap);
    if (d.mode == rc::safety::Mode::stopped) {
      if (!braking) {
        braking = true;
        r.stopped = true;
        r.stopped_at = t;
        r.causes = d.stop_causes;
        distance_at_stop = r.distance;
        box.record(t, 9, static_cast<double>(d.stop_causes));
      }
      target = 0.0;
    }
    v = rc::control::rate_limit(v, target, 2.0 * dt);
    double omega = 0.0;
    if (used.found && !braking) omega = pid.update(0.0, -used.offset, dt);
    const LineWheels w = line_wheels(v, omega, wheel_base);
    pose = rc::sim::step(pose, w.left, w.right, wheel_base, dt);
    r.distance += v * dt;
    if (braking && v <= 1e-9) break;
  }
  r.rms = std::sqrt(squares / static_cast<double>(r.frames > 0 ? r.frames : 1));
  r.final_x = pose.x;
  r.stop_distance = r.stopped ? r.distance - distance_at_stop : 0.0;
  if (r.stopped) {
    const rc::safety::Dump dump = box.dump();
    for (const rc::safety::Event& e : rc::safety::window_before(dump, 9, 6)) r.offsets_before_stop.push_back(e.value);
  }
  return r;
}

#endif  // LESSON_SOLUTION_HPP
