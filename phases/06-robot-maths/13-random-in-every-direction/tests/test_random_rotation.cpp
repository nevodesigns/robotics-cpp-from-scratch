#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const int kSamples = 100000;
const double kPi = 3.14159265358979323846;

}  // namespace

RC_TEST("the angle of a rotation, and the angle between two directions") {
  using rc::math::from_axis_angle;
  RC_CHECK_NEAR(rotation_angle(Quat{}), 0.0, 1e-12);
  RC_CHECK_NEAR(rotation_angle(from_axis_angle(Vec3{0, 0, 1}, kPi / 2.0)), kPi / 2.0, 1e-12);
  RC_CHECK_NEAR(rotation_angle(from_axis_angle(Vec3{1, 0, 0}, kPi)), kPi, 1e-9);
  // The same rotation with every sign flipped has the same angle.
  const Quat q = from_axis_angle(Vec3{0, 1, 0}, 1.0);
  const Quat flipped{-q.w, -q.x, -q.y, -q.z};
  RC_CHECK_NEAR(rotation_angle(flipped), rotation_angle(q), 1e-12);
  // Rounding that leaves |w| a hair over one must not give not a number.
  RC_CHECK(!std::isnan(rotation_angle(Quat{1.0 + 1e-15, 0.0, 0.0, 0.0})));

  RC_CHECK_NEAR(angle_between_degrees(Vec3{1, 0, 0}, Vec3{0, 1, 0}), 90.0, 1e-9);
  RC_CHECK_NEAR(angle_between_degrees(Vec3{0, 0, 1}, Vec3{0, 0, -1}), 180.0, 1e-9);
}

RC_TEST("the directions are unit vectors, and so are the rotations") {
  rc::core::Rng rng(3);
  double worst_direction = 0.0;
  double worst_quat = 0.0;
  for (int i = 0; i < 1000; ++i) {
    const Vec3 d = random_direction(rng);
    worst_direction = std::fmax(worst_direction, std::fabs(std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z) - 1.0));
    const Quat q = random_rotation(rng);
    worst_quat = std::fmax(worst_quat, std::fabs(rc::math::norm(q) - 1.0));
  }
  RC_CHECK_NEAR(worst_direction, 0.0, 1e-12);
  RC_CHECK_NEAR(worst_quat, 0.0, 1e-12);

  // And they are different each time, not one fixed answer.
  rc::core::Rng again(3);
  const Vec3 first = random_direction(again);
  const Vec3 second = random_direction(again);
  RC_CHECK(first.z != second.z || first.x != second.x);
  const Quat qa = random_rotation(again);
  const Quat qb = random_rotation(again);
  RC_CHECK(qa.w != qb.w || qa.x != qb.x);
}

RC_TEST("random latitude and longitude crowd the poles, and choosing the height evenly does not") {
  rc::core::Rng a(1);
  rc::core::Rng b(1);
  int naive_near_pole = 0;
  int even_near_pole = 0;
  for (int i = 0; i < kSamples; ++i) {
    const Vec3 n = random_direction_naive(a);
    const Vec3 e = random_direction(b);
    if (angle_between_degrees(n, Vec3{0, 0, 1}) <= 10.0) ++naive_near_pole;
    if (angle_between_degrees(e, Vec3{0, 0, 1}) <= 10.0) ++even_near_pole;
  }
  const double expected = (1.0 - std::cos(10.0 * kPi / 180.0)) / 2.0;      // the share of the sphere within 10 degrees of a pole
  const double naive_share = static_cast<double>(naive_near_pole) / kSamples;
  const double even_share = static_cast<double>(even_near_pole) / kSamples;
  std::cout << "  within 10 degrees of the north pole, out of " << kSamples << " directions: " << std::fixed << std::setprecision(4)
            << "even choice " << even_share << ", latitude and longitude " << naive_share << ", the true share of the sphere " << expected
            << " (" << std::setprecision(1) << naive_share / expected << " times too many)\n";
  RC_CHECK_NEAR(even_share, expected, 0.001);
  RC_CHECK(naive_share > 5.0 * expected);
}

RC_TEST("an even direction has a height spread evenly, and its average is the middle of the sphere") {
  rc::core::Rng rng(5);
  const int bands = 10;
  int count[bands] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  double sx = 0, sy = 0, sz = 0;
  for (int i = 0; i < kSamples; ++i) {
    const Vec3 d = random_direction(rng);
    int band = static_cast<int>((d.z + 1.0) / 2.0 * bands);
    if (band >= bands) band = bands - 1;
    ++count[band];
    sx += d.x;
    sy += d.y;
    sz += d.z;
  }
  int lowest = kSamples;
  int highest = 0;
  for (int c : count) {
    lowest = std::min(lowest, c);
    highest = std::max(highest, c);
  }
  std::cout << "  ten bands of height, " << kSamples << " directions: fewest " << lowest << ", most " << highest << " (even is " << kSamples / bands
            << "); average direction (" << std::setprecision(3) << sx / kSamples << ", " << sy / kSamples << ", " << sz / kSamples << ")\n\n";
  RC_CHECK(lowest > 9500);
  RC_CHECK(highest < 10500);
  RC_CHECK_NEAR(sx / kSamples, 0.0, 0.01);
  RC_CHECK_NEAR(sz / kSamples, 0.0, 0.01);
}

RC_TEST("random roll, pitch and yaw give the right average turn and the wrong spread of turns") {
  rc::core::Rng a(7);
  rc::core::Rng b(7);
  double sum_naive = 0.0;
  double sum_even = 0.0;
  int small_naive = 0;
  int small_even = 0;
  for (int i = 0; i < kSamples; ++i) {
    const double an = rotation_angle(random_rotation_naive(a));
    const double ae = rotation_angle(random_rotation(b));
    sum_naive += an;
    sum_even += ae;
    if (an < 30.0 * kPi / 180.0) ++small_naive;
    if (ae < 30.0 * kPi / 180.0) ++small_even;
  }
  // For rotations chosen evenly the angle has the density (1 - cos t) / pi on 0 to pi: a mean of pi/2 + 2/pi, and a share of
  // (t - sin t) / pi below t.
  const double mean_expected = (kPi / 2.0 + 2.0 / kPi) * 180.0 / kPi;
  const double small_expected = (30.0 * kPi / 180.0 - std::sin(30.0 * kPi / 180.0)) / kPi;
  std::cout << "  the angle of the turn over " << kSamples << " rotations: mean " << std::setprecision(1) << std::fixed
            << sum_even / kSamples * 180.0 / kPi << " degrees for the even choice and " << sum_naive / kSamples * 180.0 / kPi
            << " for roll, pitch and yaw (expected " << mean_expected << ")\n"
            << "  the share turning by less than 30 degrees: " << std::setprecision(4) << static_cast<double>(small_even) / kSamples << " and "
            << static_cast<double>(small_naive) / kSamples << " (expected " << small_expected << ")\n\n";
  RC_CHECK_NEAR(sum_even / kSamples * 180.0 / kPi, mean_expected, 0.5);
  // A check on the average alone would pass the bad draw: its mean is within a degree of the right one.
  RC_CHECK_NEAR(sum_naive / kSamples * 180.0 / kPi, mean_expected, 1.0);
  RC_CHECK_NEAR(static_cast<double>(small_even) / kSamples, small_expected, 0.002);
  // And it makes small turns about 40 percent too rarely.
  RC_CHECK(static_cast<double>(small_naive) / kSamples < 0.7 * small_expected);
}

RC_TEST("turning a fixed direction by an even rotation gives an even direction") {
  rc::core::Rng rng(11);
  int near_pole = 0;
  for (int i = 0; i < kSamples; ++i) {
    const Vec3 d = rc::math::rotate(random_rotation(rng), Vec3{0, 0, 1});
    if (angle_between_degrees(d, Vec3{0, 0, 1}) <= 10.0) ++near_pole;
  }
  const double expected = (1.0 - std::cos(10.0 * kPi / 180.0)) / 2.0;
  std::cout << "  the north pole turned by an even rotation, within 10 degrees of itself: " << std::setprecision(4)
            << static_cast<double>(near_pole) / kSamples << " (expected " << expected << ")\n\n";
  RC_CHECK_NEAR(static_cast<double>(near_pole) / kSamples, expected, 0.001);
}
