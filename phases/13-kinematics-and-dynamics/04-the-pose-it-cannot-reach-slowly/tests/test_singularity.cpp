#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kFirst = 0.5;
constexpr double kSecond = 0.4;
constexpr double kFullReach = kFirst + kSecond;

// The joint angles that put the tool at (reach, 0) with the elbow on the
// positive side. Straight out of lesson 13-02.
struct Pose {
  double q1 = 0.0;
  double q2 = 0.0;
};

Pose pose_at(double reach) {
  double cosine = (reach * reach - kFirst * kFirst - kSecond * kSecond) /
                  (2.0 * kFirst * kSecond);
  if (cosine > 1.0) cosine = 1.0;
  if (cosine < -1.0) cosine = -1.0;

  Pose pose;
  pose.q2 = std::acos(cosine);
  pose.q1 = -std::atan2(kSecond * std::sin(pose.q2),
                        kFirst + kSecond * std::cos(pose.q2));
  return pose;
}

double tool_speed(const PlanarJacobian& jacobian, const JointRates& rates) {
  return std::hypot(jacobian.a * rates.first + jacobian.b * rates.second,
                    jacobian.c * rates.first + jacobian.d * rates.second);
}

}  // namespace

RC_TEST("the Jacobian of this arm has a closed form, and it agrees with measuring") {
  const Pose pose = pose_at(0.7);
  const PlanarJacobian jacobian = planar_jacobian(kFirst, kSecond, pose.q1, pose.q2);

  // Measure it the way lesson 13-03 had to, one joint at a time, and compare.
  const auto tool = [](double q1, double q2) {
    return std::pair<double, double>{
        kFirst * std::cos(q1) + kSecond * std::cos(q1 + q2),
        kFirst * std::sin(q1) + kSecond * std::sin(q1 + q2)};
  };

  const double step = 1e-6;
  const auto above1 = tool(pose.q1 + step, pose.q2);
  const auto below1 = tool(pose.q1 - step, pose.q2);
  const auto above2 = tool(pose.q1, pose.q2 + step);
  const auto below2 = tool(pose.q1, pose.q2 - step);

  RC_CHECK_NEAR(jacobian.a, (above1.first - below1.first) / (2 * step), 1e-6);
  RC_CHECK_NEAR(jacobian.c, (above1.second - below1.second) / (2 * step), 1e-6);
  RC_CHECK_NEAR(jacobian.b, (above2.first - below2.first) / (2 * step), 1e-6);
  RC_CHECK_NEAR(jacobian.d, (above2.second - below2.second) / (2 * step), 1e-6);

  // And the determinant works out to l1 l2 sin(q2), which depends on the elbow
  // and on nothing else.
  RC_CHECK_NEAR(jacobian.determinant(), kFirst * kSecond * std::sin(pose.q2), 1e-12);
  RC_CHECK_NEAR(manipulability(kFirst, kSecond, pose.q2),
                std::fabs(jacobian.determinant()), 1e-12);

  // Largest with the elbow square, and it does not depend on where the arm is
  // pointing.
  RC_CHECK_NEAR(manipulability(kFirst, kSecond, 3.14159265358979 / 2.0),
                kFirst * kSecond, 1e-9);
  for (const double q1 : {0.0, 1.0, -2.0, 3.0}) {
    const PlanarJacobian anywhere = planar_jacobian(kFirst, kSecond, q1, 0.7);
    RC_CHECK_NEAR(std::fabs(anywhere.determinant()),
                  manipulability(kFirst, kSecond, 0.7), 1e-12);
  }
}

RC_TEST("the joint rates needed to move the tool at a steady speed") {
  std::cout << "\n    a 0.5 and 0.4 metre arm, tool moving outward at 0.1 m/s\n";
  std::cout << "    full extension is " << kFullReach << " m\n\n";
  std::cout << "    " << std::right << std::setw(10) << "reach" << std::setw(11)
            << "elbow" << std::setw(14) << "det J" << std::setw(14) << "q1 rate"
            << std::setw(14) << "q2 rate" << "\n";

  double worst_rate = 0.0;
  for (const double reach : {0.5, 0.7, 0.85, 0.88, 0.895, 0.899, 0.8999}) {
    const Pose pose = pose_at(reach);
    const PlanarJacobian jacobian = planar_jacobian(kFirst, kSecond, pose.q1, pose.q2);
    const JointRates rates = joint_rates(jacobian, 0.1, 0.0, 0.0);
    RC_REQUIRE(rates.ok);

    worst_rate = std::fabs(rates.second);
    std::cout << "    " << std::right << std::fixed << std::setprecision(4)
              << std::setw(10) << reach << std::setw(11) << pose.q2
              << std::setprecision(6) << std::setw(14) << jacobian.determinant()
              << std::setprecision(2) << std::setw(14) << rates.first << std::setw(14)
              << rates.second << "\n";

    // Whatever the rates, they do produce the movement that was asked for.
    RC_CHECK_NEAR(tool_speed(jacobian, rates), 0.1, 1e-9);
  }

  std::cout << "\n    a tenth of a millimetre from full extension, moving the\n";
  std::cout << "    tool a hundred millimetres a second needs the elbow at\n";
  std::cout << "    fifteen radians a second, about a hundred and forty\n";
  std::cout << "    revolutions a minute, and it is unbounded from there\n";

  RC_CHECK(worst_rate > 14.0);

  // Ten times closer to the limit is ten times the rate, which is what
  // unbounded means in practice.
  const Pose nearer = pose_at(0.89999);
  const PlanarJacobian near_jacobian =
      planar_jacobian(kFirst, kSecond, nearer.q1, nearer.q2);
  const JointRates near_rates = joint_rates(near_jacobian, 0.1, 0.0, 0.0);
  RC_REQUIRE(near_rates.ok);
  RC_CHECK(std::fabs(near_rates.second) > worst_rate * 2.5);

  // At full extension it is not large, it is impossible: the tool cannot move
  // outward at any joint speed at all.
  const PlanarJacobian straight = planar_jacobian(kFirst, kSecond, 0.0, 0.0);
  RC_CHECK_NEAR(straight.determinant(), 0.0, 1e-15);
  RC_CHECK(!joint_rates(straight, 0.1, 0.0, 1e-9).ok);

  // Sideways is still available there, which is what "loses a direction" means.
  const JointRates sideways = damped_joint_rates(straight, 0.0, 0.1, 0.001);
  RC_CHECK(sideways.ok);
  RC_CHECK(tool_speed(straight, sideways) > 0.09);
}

RC_TEST("two answers that become one") {
  std::cout << "\n    the elbow up and elbow down solutions, as the arm extends\n\n";
  std::cout << "    " << std::right << std::setw(12) << "reach" << std::setw(14)
            << "elbow up" << std::setw(14) << "elbow down" << std::setw(14)
            << "apart" << "\n";

  double last_gap = 0.0;
  for (const double reach : {0.5, 0.8, 0.88, 0.895, 0.8999, 0.89999}) {
    const Pose pose = pose_at(reach);
    last_gap = 2.0 * pose.q2;
    std::cout << "    " << std::right << std::fixed << std::setprecision(5)
              << std::setw(12) << reach << std::setprecision(6) << std::setw(14)
              << pose.q2 << std::setw(14) << -pose.q2 << std::setw(14) << last_gap
              << "\n";
  }

  std::cout << "\n    two completely different postures, four radians apart with\n";
  std::cout << "    the arm half out and two hundredths of a radian apart near\n";
  std::cout << "    the limit. Which one a solver picks there is decided by a\n";
  std::cout << "    rounding error\n";

  RC_CHECK(last_gap < 0.02);
  RC_CHECK_NEAR(2.0 * pose_at(0.5).q2, 3.9646, 0.001);

  // Both answers are still exactly right, which is what makes it dangerous: a
  // solver has nothing to complain about.
  for (const double sign : {1.0, -1.0}) {
    const Pose pose = pose_at(0.895);
    const double q2 = sign * pose.q2;
    const double q1 = -sign * std::fabs(pose.q1);
    const double x = kFirst * std::cos(q1) + kSecond * std::cos(q1 + q2);
    const double y = kFirst * std::sin(q1) + kSecond * std::sin(q1 + q2);
    RC_CHECK_NEAR(x, 0.895, 1e-9);
    RC_CHECK_NEAR(y, 0.0, 1e-9);
  }
}

RC_TEST("damping buys a bounded rate with the speed you asked for") {
  const Pose pose = pose_at(0.8999);
  const PlanarJacobian jacobian = planar_jacobian(kFirst, kSecond, pose.q1, pose.q2);

  std::cout << "\n    a tenth of a millimetre from full extension, asked for\n";
  std::cout << "    0.1 m/s outward\n\n";
  std::cout << "    " << std::right << std::setw(12) << "lambda" << std::setw(14)
            << "q1 rate" << std::setw(14) << "q2 rate" << std::setw(18)
            << "speed given" << "\n";

  std::vector<double> rates, speeds;
  for (const double lambda : {0.0, 0.001, 0.01, 0.05, 0.2}) {
    const JointRates got = damped_joint_rates(jacobian, 0.1, 0.0, lambda);
    RC_REQUIRE(got.ok);
    rates.push_back(std::fabs(got.second));
    speeds.push_back(tool_speed(jacobian, got));

    std::cout << "    " << std::right << std::fixed << std::setprecision(4)
              << std::setw(12) << lambda << std::setprecision(2) << std::setw(14)
              << got.first << std::setw(14) << got.second << std::setprecision(5)
              << std::setw(18) << speeds.back() << "\n";
  }

  std::cout << "\n    damping does not get the arm through the singularity. It\n";
  std::cout << "    makes the arm refuse to go there quickly, which is why a\n";
  std::cout << "    real machine slows to a crawl near full extension rather\n";
  std::cout << "    than throwing itself at the stop\n";

  // With no damping it is exact and unbounded.
  RC_CHECK_NEAR(speeds.front(), 0.1, 1e-9);
  RC_CHECK(rates.front() > 14.0);

  // Every step of damping costs rate and costs speed, together.
  for (std::size_t i = 1; i < rates.size(); ++i) {
    RC_CHECK(rates[i] < rates[i - 1]);
    RC_CHECK(speeds[i] < speeds[i - 1]);
  }

  // A hundredth of damping is a quarter of the joint rate and a quarter of the
  // speed, which is the exchange rate lambda sets.
  RC_CHECK(rates[2] < rates[0] * 0.3);
  RC_CHECK(speeds[2] < 0.03);

  // And away from the singularity damping costs almost nothing, so it can be
  // left switched on.
  const Pose comfortable = pose_at(0.7);
  const PlanarJacobian easy =
      planar_jacobian(kFirst, kSecond, comfortable.q1, comfortable.q2);
  const JointRates damped = damped_joint_rates(easy, 0.1, 0.0, 0.01);
  RC_REQUIRE(damped.ok);
  RC_CHECK(tool_speed(easy, damped) > 0.099);
}
