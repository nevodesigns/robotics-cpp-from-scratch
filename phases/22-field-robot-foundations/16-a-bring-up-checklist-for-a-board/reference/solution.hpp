#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

// The lowest level of a drive board: no kinematics, no speeds in metres, only what the hardware itself has.
class BoardProbe {
 public:
  virtual ~BoardProbe() = default;
  virtual bool ping() = 0;
  virtual void drive_wheels(double left, double right) = 0;  // -1 to 1 of full power
  virtual void step(double dt) = 0;                          // lets time pass (a real board has its own clock)
  virtual long encoder_left() const = 0;
  virtual long encoder_right() const = 0;
  virtual double battery_volts() const = 0;
  virtual bool estop_pressed() const = 0;
  virtual void accel(double& ax, double& ay, double& az) const = 0;
  virtual double gyro_z() const = 0;
};

// How far the robot really went, as somebody with a tape measure would say: mark where it is, and later say how far it is from the mark.
class DistanceMeter {
 public:
  virtual ~DistanceMeter() = default;
  virtual void mark() = 0;
  virtual double since_mark() = 0;
};

// What can be wrong with a new board. The defaults are a board with nothing wrong.
struct BoardFaults {
  bool swap_wheels = false;     // the left command reaches the right motor
  bool invert_left = false;     // the left motor or encoder runs the wrong way
  double tick_scale = 1.0;      // the board's real ticks per metre against the documented
  double watchdog = 0.5;        // seconds; 0 is no watchdog
  bool stuck_right = false;     // the right encoder never counts
  double battery = 12.0;        // volts
  double az_sign = 1.0;         // -1: mounted upside down
  double gyro_bias = 0.0;       // rad/s
  double deadband = 0.08;       // the command below which the motors do not turn
  bool estop = false;           // the stop button is pressed
};

// A model of a board: motors that lag by a tenth of a second, a documented 1200 ticks a metre and 0.5 m/s at full power.
class FakeBoard : public BoardProbe, public DistanceMeter {
 public:
  explicit FakeBoard(const BoardFaults& faults = BoardFaults()) : f_(faults) {}

  bool ping() override { return true; }

  void drive_wheels(double left, double right) override {
    cmd_l_ = left;
    cmd_r_ = right;
    since_ = 0.0;
  }

  void step(double dt) override {
    since_ += dt;
    double l = cmd_l_;
    double r = cmd_r_;
    if (f_.watchdog > 0.0 && since_ > f_.watchdog) l = r = 0.0;
    if (f_.swap_wheels) std::swap(l, r);
    if (f_.invert_left) l = -l;
    const double target_l = std::fabs(l) < f_.deadband ? 0.0 : l * 0.5;
    const double target_r = std::fabs(r) < f_.deadband ? 0.0 : r * 0.5;
    const double alpha = 1.0 - std::exp(-dt / 0.1);
    v_l_ += alpha * (target_l - v_l_);
    v_r_ += alpha * (target_r - v_r_);
    const double per_metre = 1200.0 * f_.tick_scale;
    acc_l_ += v_l_ * dt * per_metre;
    acc_r_ += v_r_ * dt * per_metre;
    travelled_ += 0.5 * (v_l_ + v_r_) * dt;
  }

  long encoder_left() const override { return static_cast<long>(std::floor(acc_l_)); }
  long encoder_right() const override { return f_.stuck_right ? 0 : static_cast<long>(std::floor(acc_r_)); }
  double battery_volts() const override { return f_.battery; }
  bool estop_pressed() const override { return f_.estop; }
  void accel(double& ax, double& ay, double& az) const override {
    ax = 0.02;
    ay = -0.01;
    az = 9.81 * f_.az_sign;
  }
  double gyro_z() const override { return f_.gyro_bias; }

  // What a person with a tape measure would read: the distance the middle of the robot has moved since the mark.
  void mark() override { mark_ = travelled_; }
  double since_mark() override { return travelled_ - mark_; }

 private:
  BoardFaults f_;
  double cmd_l_ = 0.0, cmd_r_ = 0.0;
  double since_ = 0.0;
  double v_l_ = 0.0, v_r_ = 0.0;
  double acc_l_ = 0.0, acc_r_ = 0.0;
  double travelled_ = 0.0;
  double mark_ = 0.0;
};

struct BringUpExpect {
  double ticks_per_m = 1200.0;
  double scale_tolerance = 0.03;
  double battery_min = 10.5;
  double battery_max = 12.6;
  double watchdog = 0.5;
  double max_deadband = 0.15;
};

struct BringUpResult {
  std::string id;
  bool ok = false;
  double measured = 0.0;
  std::string advice;  // empty when the check passed
};

// Drives the wheels for a time, resending the command every 0.1 s so that a watchdog stays quiet.
inline void drive_for(BoardProbe& b, double left, double right, double seconds) {
  const int steps = static_cast<int>(seconds / 0.01 + 0.5);
  for (int i = 0; i < steps; ++i) {
    if (i % 10 == 0) b.drive_wheels(left, right);
    b.step(0.01);
  }
}

inline void settle(BoardProbe& b) {
  b.drive_wheels(0.0, 0.0);
  for (int i = 0; i < 150; ++i) b.step(0.01);
}

inline BringUpResult make_result(const std::string& id, bool ok, double measured, const std::string& advice) {
  BringUpResult r;
  r.id = id;
  r.ok = ok;
  r.measured = measured;
  if (!ok) r.advice = advice;
  return r;
}

// Both wheels asked to go forward: both encoders must count up.
inline BringUpResult check_direction(BoardProbe& b) {
  settle(b);
  const long l0 = b.encoder_left();
  const long r0 = b.encoder_right();
  drive_for(b, 0.3, 0.3, 1.0);
  const long dl = b.encoder_left() - l0;
  const long dr = b.encoder_right() - r0;
  std::string advice;
  if (dl < 0 && dr < 0) advice = "both encoders count down when driving forward: swap the encoder A and B wires, or negate both in the driver";
  else if (dl < 0) advice = "the left encoder counts down when driving forward: swap its A and B wires, or negate it in the driver";
  else if (dr < 0) advice = "the right encoder counts down when driving forward: swap its A and B wires, or negate it in the driver";
  else if (dl == 0 && dr == 0) advice = "neither encoder counts: check the encoder power and the pins";
  else if (dl == 0) advice = "the left encoder does not count: check its power, its pull ups and the pin";
  else if (dr == 0) advice = "the right encoder does not count: check its power, its pull ups and the pin";
  return make_result("direction", advice.empty(), static_cast<double>(dl < dr ? dl : dr), advice);
}

// Only the left wheel asked: the left encoder must count and the right must not (a swap shows here, a reversal shows above).
inline BringUpResult check_sides(BoardProbe& b) {
  settle(b);
  const long l0 = b.encoder_left();
  const long r0 = b.encoder_right();
  drive_for(b, 0.3, 0.0, 1.0);
  const long dl = b.encoder_left() - l0;
  const long dr = b.encoder_right() - r0;
  std::string advice;
  if (std::labs(dr) > std::labs(dl) / 2 + 5) advice = "the left command moved the right wheel: the motor outputs or the encoder inputs are swapped";
  else if (dl <= 0) advice = "the left command did not make the left encoder count up";
  return make_result("sides", advice.empty(), static_cast<double>(dl), advice);
}

// Drives a known stretch and reads the ticks a metre against the documented. The distance is what a person measured on the floor.
inline BringUpResult check_scale(BoardProbe& b, double measured_metres, const BringUpExpect& e, long ticks_driven) {
  (void)b;
  const double per_metre = measured_metres > 0.0 ? static_cast<double>(ticks_driven) / measured_metres : 0.0;
  const bool ok = std::fabs(per_metre - e.ticks_per_m) <= e.scale_tolerance * e.ticks_per_m;
  return make_result("scale", ok, per_metre, "the ticks a metre are not the documented ones: check the wheel diameter, the gear ratio and the counts a turn, and the resolution of the decoder (x1, x2, x4)");
}

// The smallest command that makes the wheels turn.
inline BringUpResult check_deadband(BoardProbe& b, const BringUpExpect& e) {
  settle(b);
  double found = -1.0;
  for (int k = 1; k <= 60 && found < 0.0; ++k) {
    const double c = 0.01 * k;
    settle(b);
    const long l0 = b.encoder_left();
    const long r0 = b.encoder_right();
    drive_for(b, c, c, 0.3);
    if (b.encoder_left() - l0 > 3 || std::labs(b.encoder_right() - r0) > 3) found = c;
  }
  return make_result("deadband", found >= 0.0 && found <= e.max_deadband, found, "the motors need a command that is too large before they move: raise the minimum PWM in the driver, or check the supply");
}

// A command given once and then silence: the wheels must stop by themselves.
inline BringUpResult check_watchdog(BoardProbe& b, const BringUpExpect& e) {
  settle(b);
  b.drive_wheels(0.3, 0.3);
  const int wait = static_cast<int>((e.watchdog + 1.0) / 0.01);
  for (int i = 0; i < wait; ++i) b.step(0.01);
  const long l0 = b.encoder_left();
  for (int i = 0; i < 50; ++i) b.step(0.01);
  const long moved = b.encoder_left() - l0;
  return make_result("watchdog", std::labs(moved) <= 3, static_cast<double>(moved), "the board keeps driving when the commands stop: turn on its watchdog, or write one in its firmware (and test it with the cable pulled)");
}

// All ten, in the order that costs least first. The scale check drives 0.3 for 3 s and asks the meter how far the robot went.
inline std::vector<BringUpResult> run_bringup(BoardProbe& b, const BringUpExpect& e, DistanceMeter& meter) {
  std::vector<BringUpResult> out;
  out.push_back(make_result("link", b.ping(), b.ping() ? 1.0 : 0.0, "the board does not answer: check the cable, the power and the port"));
  out.push_back(make_result("estop", !b.estop_pressed(), b.estop_pressed() ? 1.0 : 0.0, "the emergency stop is pressed, or reads as pressed: release it, and check its wiring (normally closed)"));
  const double v = b.battery_volts();
  out.push_back(make_result("battery", v >= e.battery_min && v <= e.battery_max, v, "the battery voltage is outside its range: charge it, or check the divider that measures it"));
  double ax = 0.0, ay = 0.0, az = 0.0;
  b.accel(ax, ay, az);
  out.push_back(make_result("imu", std::fabs(az - 9.81) < 0.8 && std::fabs(ax) < 1.0 && std::fabs(ay) < 1.0, az, "the accelerometer does not read gravity on its z axis: the IMU is mounted in another way (or upside down), or its range is set wrongly"));
  double gz = 0.0;
  for (int i = 0; i < 100; ++i) {
    b.step(0.01);
    gz += b.gyro_z() / 100.0;
  }
  out.push_back(make_result("gyro", std::fabs(gz) < 0.05, gz, "the gyro reads a turn while still: calibrate its bias at start-up, or wait for it to warm up"));
  out.push_back(check_direction(b));
  out.push_back(check_sides(b));
  // The scale check: drive, and compare the ticks with the distance somebody measured.
  settle(b);
  meter.mark();
  const long l0 = b.encoder_left();
  const long r0 = b.encoder_right();
  drive_for(b, 0.3, 0.3, 3.0);
  settle(b);
  const long ticks = (b.encoder_left() - l0 + b.encoder_right() - r0) / 2;
  const double metres = meter.since_mark();
  out.push_back(check_scale(b, metres, e, ticks));
  out.push_back(check_deadband(b, e));
  out.push_back(check_watchdog(b, e));
  return out;
}

inline std::vector<std::string> failed_ids(const std::vector<BringUpResult>& results) {
  std::vector<std::string> ids;
  for (std::size_t i = 0; i < results.size(); ++i)
    if (!results[i].ok) ids.push_back(results[i].id);
  return ids;
}


#endif
