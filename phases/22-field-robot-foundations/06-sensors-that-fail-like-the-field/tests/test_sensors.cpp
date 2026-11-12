#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}


namespace {

const double kPi = 3.14159265358979323846;

World2 room(double half) {
  World2 w;
  w.walls.push_back(Segment{-half, -half, half, -half});
  w.walls.push_back(Segment{half, -half, half, half});
  w.walls.push_back(Segment{half, half, -half, half});
  w.walls.push_back(Segment{-half, half, -half, -half});
  return w;
}

double stddev_of(const std::vector<double>& v) {
  double mean = 0.0;
  for (double x : v) mean += x;
  mean /= static_cast<double>(v.size());
  double s = 0.0;
  for (double x : v) s += (x - mean) * (x - mean);
  return std::sqrt(s / static_cast<double>(v.size()));
}

}  // namespace

RC_TEST("a ray finds the nearest wall or post, and nothing when there is none in range") {
  World2 w;
  w.walls.push_back(Segment{5.0, -10.0, 5.0, 10.0});
  w.posts.push_back(Circle{3.0, 0.0, 0.5});
  RC_CHECK_NEAR(cast_ray(w, 0.0, 0.0, 0.0, 12.0), 2.5, 1e-9);
  RC_CHECK_NEAR(cast_ray(w, 0.0, 2.0, 0.0, 12.0), 5.0, 1e-9);
  const double oblique = cast_ray(w, 0.0, 2.0, kPi / 6.0, 12.0);
  rec("oblique", oblique, 4);
  RC_CHECK_NEAR(oblique, 5.0 / std::cos(kPi / 6.0), 1e-9);
  RC_CHECK(cast_ray(w, 0.0, 0.0, kPi, 12.0) == no_return());
  RC_CHECK(cast_ray(w, 0.0, 0.0, kPi / 2.0, 12.0) == no_return());
  RC_CHECK(cast_ray(w, 0.0, 2.0, 0.0, 4.0) == no_return());
  RC_CHECK(cast_ray(w, 0.0, 20.0, 0.0, 12.0) == no_return());
}

RC_TEST("the lidar noise and the dropouts measured from many scans are the ones set") {
  const World2 w = room(5.0);
  LidarSpec clean;
  clean.sigma = 0.0;
  clean.dropout = 0.0;
  rc::core::Rng quiet(1);
  const std::vector<double> truth = scan(clean, w, 0.0, 0.0, 0.0, quiet);
  RC_CHECK(truth.size() == 360);
  RC_CHECK_NEAR(truth[180], 5.0, 0.05);

  LidarSpec noisy;
  rc::core::Rng rng(7);
  std::vector<double> errors;
  int lost = 0;
  int total = 0;
  for (int k = 0; k < 40; ++k) {
    const std::vector<double> r = scan(noisy, w, 0.0, 0.0, 0.0, rng);
    for (std::size_t i = 0; i < r.size(); ++i) {
      ++total;
      if (r[i] == no_return()) {
        ++lost;
      } else {
        errors.push_back(r[i] - truth[i]);
      }
    }
  }
  const double sigma = stddev_of(errors);
  const double pct = 100.0 * lost / total;
  rec("lidar_sigma", sigma, 4);
  rec("dropout_pct", pct, 2);
  RC_CHECK_NEAR(sigma, 0.02, 0.001);
  RC_CHECK_NEAR(pct, 1.0, 0.25);
}

RC_TEST("a gyro with a bias of 0.01 rad/s turns the heading by about 34 degrees a minute while the robot sits still") {
  Imu imu(ImuSpec(), 3);
  double heading = 0.0;
  for (int i = 0; i < 6000; ++i) heading += imu.measure(0.0, 0.0, 0.0, 0.01).gyro_z * 0.01;
  const double deg = heading * 180.0 / kPi;
  rec("gyro_drift", deg, 1);
  RC_CHECK(deg > 30.0 && deg < 40.0);
}

RC_TEST("a GNSS receiver is a few metres off alone and a couple of centimetres when fixed, and sometimes jumps") {
  Gnss g(GnssSpec(), 11);
  std::vector<double> single;
  std::vector<double> fixed;
  int jumps = 0;
  const int n = 6000;
  for (int i = 0; i < n; ++i) {
    const GnssFix a = g.measure(10.0, 20.0, Fix::single);
    const GnssFix b = g.measure(10.0, 20.0, Fix::fixed);
    if (std::hypot(b.x - 10.0, b.y - 20.0) > 1.0) {
      ++jumps;
    } else {
      fixed.push_back(b.x - 10.0);
    }
    if (std::hypot(a.x - 10.0, a.y - 20.0) < 6.0) single.push_back(a.x - 10.0);
  }
  rec("gnss_single", stddev_of(single), 2);
  rec("gnss_fixed", stddev_of(fixed), 3);
  rec("outlier_pct", 100.0 * jumps / n, 2);
  RC_CHECK_NEAR(stddev_of(single), 1.5, 0.15);
  RC_CHECK_NEAR(stddev_of(fixed), 0.02, 0.002);
  RC_CHECK(jumps > 20 && jumps < 120);
  RC_CHECK(!g.measure(0.0, 0.0, Fix::none).valid);
}

RC_TEST("an encoder is a tick short at most, and a wrong wheel or a slipping one reads long") {
  Encoder good{EncoderSpec()};
  EncoderSpec off;
  off.scale_error = 0.02;
  Encoder wrong(off);
  EncoderSpec slippy;
  slippy.slip = 0.05;
  Encoder slipping(slippy);
  for (int i = 0; i < 10000; ++i) {
    good.move(0.001);
    wrong.move(0.001);
    slipping.move(0.001);
  }
  rec("enc_quant", (10.0 - good.distance()) * 1000.0, 2);
  rec("enc_scale", wrong.distance() - 10.0, 2);
  rec("enc_slip", slipping.distance() - 10.0, 2);
  RC_CHECK(std::fabs(good.distance() - 10.0) <= 0.0011);
  RC_CHECK_NEAR(wrong.distance() - 10.0, 0.2, 0.002);
  RC_CHECK_NEAR(slipping.distance() - 10.0, 0.5, 0.002);
}
