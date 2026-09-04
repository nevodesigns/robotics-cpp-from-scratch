#include <rc/test/rc_test.hpp>

#include <rc/math/vector.hpp>
#include <rc/sim/diff_drive.hpp>

#include <cmath>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

using rc::math::Vec2;
using rc::sim::Pose;

constexpr double kWheelBase = 0.5;
constexpr double kDt = 0.02;   // 50 Hz
constexpr int kMaxSteps = 4000;

class Noise {
 public:
  explicit Noise(std::uint64_t seed) : state_(seed * 6364136223846793005ULL + 1ULL) {}
  double gaussian() {
    double total = 0.0;
    for (int i = 0; i < 12; ++i) {
      state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
      total += static_cast<double>((state_ >> 11) & 0x1FFFFFFFFFFFFFULL) /
               static_cast<double>(0x20000000000000ULL);
    }
    return total - 6.0;
  }

 private:
  std::uint64_t state_;
};

// Ten metres east, then ten metres north. One right angle, which is the corner
// every corner-cutting question is really about.
std::vector<Vec2> corner_path() {
  std::vector<Vec2> path;
  for (double x = 0.0; x <= 10.0; x += 0.05) path.push_back({x, 0.0});
  for (double y = 0.05; y <= 10.0; y += 0.05) path.push_back({10.0, y});
  return path;
}

// Out along one leg and back along another 0.6 m away, then off. A corridor
// swept in both directions, and the shape that decides whether a target search
// is allowed to look backwards.
std::vector<Vec2> returning_path() {
  std::vector<Vec2> path;
  for (double x = 0.0; x <= 8.0; x += 0.05) path.push_back({x, 0.0});
  for (double y = 0.05; y <= 0.6; y += 0.05) path.push_back({8.0, y});
  for (double x = 7.95; x >= 0.0; x -= 0.05) path.push_back({x, 0.6});
  for (double y = 0.65; y <= 4.0; y += 0.05) path.push_back({0.0, y});
  return path;
}

double path_length(const std::vector<Vec2>& path) {
  double total = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i)
    total += std::hypot(path[i].x - path[i - 1].x, path[i].y - path[i - 1].y);
  return total;
}

struct Drive {
  double rms = 0.0;        // cross track error
  double worst = 0.0;
  double churn = 0.0;      // how much the steering command moves, per step
  double travelled = 0.0;
  bool arrived = false;
};

// Drive the path, steering by a pose that is `late` steps old and `sigma`
// metres wrong. That is the whole point of the lesson: the follower never sees
// the truth, it sees an estimate, and the lookahead decides how much it trusts
// it.
Drive drive_path(const std::vector<Vec2>& path, double lookahead, double speed,
                 double sigma, int late) {
  PurePursuit follower(path, lookahead);
  Noise noise(17);
  Pose pose{0.0, 0.0, 0.0};
  std::deque<Pose> history;

  Drive result;
  double squared = 0.0, last_curvature = 0.0;
  int counted = 0;

  for (int i = 0; i < kMaxSteps; ++i) {
    history.push_back(pose);
    if (static_cast<int>(history.size()) > late + 1) history.pop_front();

    Pose believed = history.front();
    if (sigma > 0.0) {
      believed.x += sigma * noise.gaussian();
      believed.y += sigma * noise.gaussian();
    }

    const Steering steering = follower.follow(believed);
    const WheelSpeeds wheels = wheels_for(speed, steering.curvature, kWheelBase);

    const Pose before = pose;
    pose = rc::sim::step(pose, wheels.left, wheels.right, kWheelBase, kDt);
    result.travelled += std::hypot(pose.x - before.x, pose.y - before.y);

    const double error = follower.cross_track_error(pose);
    squared += error * error;
    if (error > result.worst) result.worst = error;
    result.churn += std::fabs(steering.curvature - last_curvature);
    last_curvature = steering.curvature;
    ++counted;

    if (follower.arrived(pose, 0.2)) {
      result.arrived = true;
      break;
    }
  }

  result.rms = std::sqrt(squared / counted);
  result.churn /= counted;
  return result;
}

}  // namespace

RC_TEST("the arc from here through a point ahead") {
  const std::vector<Vec2> straight = {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}};
  PurePursuit follower(straight, 1.0);

  // On the line and pointing along it: no turn at all.
  const Steering ahead = follower.follow(Pose{0.0, 0.0, 0.0});
  RC_CHECK(ahead.have_target);
  RC_CHECK_NEAR(ahead.curvature, 0.0, 1e-12);

  // Half a metre to the right of it. The target is 1 m ahead and 0.5 m to the
  // left, so the arc has curvature 2 * 0.5 / 1.25.
  follower.reset();
  const Steering offset = follower.follow(Pose{1.0, -0.5, 0.0});
  RC_CHECK(offset.curvature > 0.0);   // turn left, back onto the path
  RC_CHECK_NEAR(offset.curvature, 2.0 * 0.5 / 1.25, 1e-9);

  // And a mirror image turns the other way by the same amount.
  follower.reset();
  const Steering mirrored = follower.follow(Pose{1.0, 0.5, 0.0});
  RC_CHECK_NEAR(mirrored.curvature, -offset.curvature, 1e-9);
}

RC_TEST("a curvature is not a wheel speed until it meets a speed") {
  // Straight ahead: both wheels the same.
  const WheelSpeeds straight = wheels_for(1.0, 0.0, kWheelBase);
  RC_CHECK_NEAR(straight.left, 1.0, 1e-12);
  RC_CHECK_NEAR(straight.right, 1.0, 1e-12);

  // A 2 m radius left turn at 1 m/s is 0.5 rad/s, so the wheels differ by the
  // turn rate times the wheel base.
  const WheelSpeeds turning = wheels_for(1.0, 0.5, kWheelBase);
  RC_CHECK_NEAR(turning.right - turning.left, 0.5 * kWheelBase, 1e-12);
  RC_CHECK_NEAR((turning.left + turning.right) / 2.0, 1.0, 1e-12);

  // The same curvature at twice the speed is twice the turn rate, which is why
  // a follower tuned at walking pace surprises everybody at running pace.
  const WheelSpeeds faster = wheels_for(2.0, 0.5, kWheelBase);
  RC_CHECK_NEAR(faster.right - faster.left, 2.0 * (turning.right - turning.left), 1e-12);
}

RC_TEST("cross track error is measured to the path, not to a waypoint") {
  // Waypoints a whole metre apart, and the robot half a metre along, one
  // centimetre off. The nearest waypoint is 0.5 m away. The path is 0.01 m
  // away, and the path is the answer.
  const std::vector<Vec2> coarse = {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}};
  PurePursuit follower(coarse, 1.0);
  RC_CHECK_NEAR(follower.cross_track_error(Pose{0.5, 0.01, 0.0}), 0.01, 1e-12);

  // Past the end, it measures to the end.
  RC_CHECK_NEAR(follower.cross_track_error(Pose{3.0, 0.0, 0.0}), 1.0, 1e-12);

  // And on it, zero.
  RC_CHECK_NEAR(follower.cross_track_error(Pose{1.0, 0.0, 0.0}), 0.0, 1e-12);
}

RC_TEST("with a perfect estimate, shorter is simply better") {
  std::cout << "\n    a right angle corner at 1 m/s, steering by the truth\n\n";
  std::cout << "    " << std::right << std::setw(12) << "lookahead" << std::setw(12)
            << "rms" << std::setw(12) << "worst" << std::setw(20) << "worst / lookahead"
            << "\n";

  const std::vector<Vec2> path = corner_path();
  std::vector<double> worst;
  for (const double lookahead : {0.10, 0.20, 0.40, 0.80, 1.50, 3.00, 5.00}) {
    const Drive drive = drive_path(path, lookahead, 1.0, 0.0, 0);
    worst.push_back(drive.worst);
    std::cout << "    " << std::right << std::fixed << std::setprecision(2)
              << std::setw(12) << lookahead << std::setprecision(4) << std::setw(12)
              << drive.rms << std::setw(12) << drive.worst << std::setprecision(3)
              << std::setw(20) << drive.worst / lookahead << "\n";
    RC_CHECK(drive.arrived);
  }

  std::cout << "\n    nothing here argues for a long lookahead, and that is\n";
  std::cout << "    because nothing here is wrong about where the robot is\n";

  // Monotonic: every longer lookahead cuts the corner harder than the one
  // before it. There is no U in this table at all.
  for (std::size_t i = 1; i < worst.size(); ++i) RC_CHECK(worst[i] > worst[i - 1]);

  // And the cut is proportional to the lookahead: a right angle is cut by about
  // 27 percent of it, once the lookahead is large enough to matter.
  RC_CHECK_NEAR(worst.back() / 5.00, 0.27, 0.02);
  RC_CHECK_NEAR(worst[4] / 1.50, 0.27, 0.02);
}

RC_TEST("the lookahead is a gain on the estimate's error") {
  const std::vector<Vec2> path = corner_path();

  std::cout << "\n    the same corner, steering by what the robot believes\n\n";
  std::cout << "    " << std::right << std::setw(12) << "lookahead" << std::setw(22)
            << "2 cm of noise" << std::setw(24) << "100 ms of latency" << "\n";
  std::cout << "    " << std::right << std::setw(12) << "" << std::setw(11) << "rms"
            << std::setw(11) << "churn" << std::setw(13) << "rms" << std::setw(11)
            << "churn" << "\n";

  Drive noisy_short{}, noisy_long{}, late_short{}, late_long{};
  for (const double lookahead : {0.10, 0.20, 0.40, 0.80, 1.50}) {
    const Drive noisy = drive_path(path, lookahead, 1.0, 0.02, 0);
    const Drive late = drive_path(path, lookahead, 1.0, 0.0, 5);
    if (lookahead == 0.10) { noisy_short = noisy; late_short = late; }
    if (lookahead == 0.80) { noisy_long = noisy; late_long = late; }

    std::cout << "    " << std::right << std::fixed << std::setprecision(2)
              << std::setw(12) << lookahead << std::setprecision(4) << std::setw(11)
              << noisy.rms << std::setw(11) << noisy.churn << std::setw(13);
    if (late.arrived) std::cout << late.rms;
    else std::cout << "lost";
    std::cout << std::setw(11) << late.churn << "\n";
  }

  std::cout << "\n    noise is paid for in the steering, latency in the path\n";

  // Noise does not move the tracking much. It is paid for entirely in the
  // actuator: a hundred times the steering movement for the same corner.
  RC_CHECK(noisy_short.arrived);
  RC_CHECK(noisy_short.churn > noisy_long.churn * 20.0);
  RC_CHECK(noisy_short.rms < noisy_long.rms);

  // Latency is different in kind. At a lookahead of 0.10 m, with a pose 0.10 m
  // behind, the target point is inside the error and the robot loses the path
  // altogether rather than tracking it badly.
  RC_CHECK(!late_short.arrived);
  RC_CHECK(late_short.worst > 1.0);
  RC_CHECK(late_long.arrived);
}

RC_TEST("the smallest workable lookahead is speed times latency") {
  std::cout << "\n    100 ms of latency, no noise: rms, or lost\n\n";
  std::cout << "    " << std::right << std::setw(8) << "speed" << std::setw(16)
            << "speed x 0.1 s" << std::setw(11) << "0.05" << std::setw(11) << "0.10"
            << std::setw(11) << "0.20" << std::setw(11) << "0.40" << "\n";

  const std::vector<Vec2> path = corner_path();
  for (const double speed : {0.5, 1.0, 2.0, 4.0}) {
    std::cout << "    " << std::right << std::fixed << std::setprecision(1)
              << std::setw(8) << speed << std::setprecision(2) << std::setw(16)
              << speed * 0.1;
    for (const double lookahead : {0.05, 0.10, 0.20, 0.40}) {
      const Drive drive = drive_path(path, lookahead, speed, 0.0, 5);
      std::cout << std::setw(11);
      if (drive.arrived) std::cout << std::setprecision(4) << drive.rms;
      else std::cout << "lost";
      // Every lookahead at or below the distance travelled during the delay
      // fails, and the first one above it works. At every speed.
      if (lookahead <= speed * 0.1) RC_CHECK(!drive.arrived);
    }
    std::cout << "\n";
  }

  std::cout << "\n    the lookahead has to be longer than the distance the\n";
  std::cout << "    robot covers while the estimate is catching up\n";

  // Which is the rule worth taking away: it scales with speed, so a follower
  // tuned standing still or at walking pace has a fixed number in it that is
  // wrong as soon as anybody speeds the robot up.
  RC_CHECK(drive_path(path, 0.20, 1.0, 0.0, 5).arrived);
  RC_CHECK(!drive_path(path, 0.20, 2.0, 0.0, 5).arrived);
}

RC_TEST("a target search that is allowed to look backwards") {
  const std::vector<Vec2> path = returning_path();
  std::cout << "\n    a path out along one leg and back along another 0.6 m\n";
  std::cout << "    away, which is " << std::fixed << std::setprecision(2)
            << path_length(path) << " m long\n\n";

  // The follower as written: forward only.
  const Drive forward = drive_path(path, 0.5, 1.0, 0.0, 0);
  std::cout << "    " << std::left << std::setw(26) << "searching forward only"
            << std::right << (forward.arrived ? "arrived" : "lost   ") << ", "
            << std::setprecision(2) << forward.travelled << " m driven\n";

  // The same geometry with the search starting from zero every time, which is
  // what the loop says if you write it straight from the description.
  {
    PurePursuit follower(path, 0.5);
    Pose pose{0.0, 0.0, 0.0};
    double travelled = 0.0;
    bool arrived = false;
    for (int i = 0; i < kMaxSteps; ++i) {
      follower.reset();   // forget where the last target was: search from zero
      const Steering steering = follower.follow(pose);
      const WheelSpeeds wheels = wheels_for(1.0, steering.curvature, kWheelBase);
      const Pose before = pose;
      pose = rc::sim::step(pose, wheels.left, wheels.right, kWheelBase, kDt);
      travelled += std::hypot(pose.x - before.x, pose.y - before.y);
      if (follower.arrived(pose, 0.2)) { arrived = true; break; }
    }
    std::cout << "    " << std::left << std::setw(26) << "searching from the start"
              << std::right << (arrived ? "arrived" : "lost   ") << ", " << travelled
              << " m driven, ending at (" << pose.x << ", " << pose.y << ")\n";

    // It never turns. A point near the start of the path is more than a
    // lookahead away and comes first in the array, so the robot aims at
    // something behind it, which has no sideways offset, which is no steering.
    RC_CHECK(!arrived);
    RC_CHECK(travelled > path_length(path) * 3.0);
    RC_CHECK(std::fabs(pose.y) < 0.01);
  }

  std::cout << "\n    a straight test path cannot tell these two apart\n";
  RC_CHECK(forward.arrived);
  RC_CHECK(forward.travelled < path_length(path) * 1.05);
}
