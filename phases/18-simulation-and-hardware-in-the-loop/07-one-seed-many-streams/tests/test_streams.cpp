#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

int bits_differing(std::uint64_t a, std::uint64_t b) {
  std::uint64_t x = a ^ b;
  int n = 0;
  while (x) {
    n += static_cast<int>(x & 1ULL);
    x >>= 1;
  }
  return n;
}

double correlation(const std::vector<double>& a, const std::vector<double>& b) {
  const std::size_t n = a.size() < b.size() ? a.size() : b.size();
  double ma = 0.0, mb = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    ma += a[i];
    mb += b[i];
  }
  ma /= n;
  mb /= n;
  double sab = 0.0, saa = 0.0, sbb = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    sab += (a[i] - ma) * (b[i] - mb);
    saa += (a[i] - ma) * (a[i] - ma);
    sbb += (b[i] - mb) * (b[i] - mb);
  }
  return sab / std::sqrt(saa * sbb);
}

std::vector<double> draw(Stream& s, int n) {
  std::vector<double> v;
  for (int i = 0; i < n; ++i) v.push_back(s.uniform());
  return v;
}

}  // namespace

RC_TEST("the mixer turns neighbouring numbers into unrelated ones, flipping about half of the 64 bits when one bit of the input changes") {
  double total = 0.0;
  const int n = 1000;
  for (int i = 0; i < n; ++i) total += bits_differing(mix64(static_cast<std::uint64_t>(i)), mix64(static_cast<std::uint64_t>(i) ^ 1ULL));
  const double mean = total / n;
  std::cout << "  mixing a number and the same number with one bit changed: on average " << std::fixed << std::setprecision(1) << mean << " of 64 output bits differ\n\n";
  RC_CHECK(mean > 28.0 && mean < 36.0);
  RC_CHECK(mix64(0) != 0);
  RC_CHECK(mix64(1) != mix64(2));
  RC_CHECK(mix64(12345) == mix64(12345));
}

RC_TEST("a seed derived for a name is the same every time, and names that differ by a single letter get seeds that share nothing") {
  RC_CHECK(derive_seed(42, "motor") == derive_seed(42, "motor"));
  RC_CHECK(derive_seed(42, "motor") != derive_seed(43, "motor"));
  RC_CHECK(derive_seed(42, "motor") != derive_seed(42, "motors"));
  double total = 0.0;
  const int n = 200;
  for (int i = 0; i < n; ++i) {
    const std::string a = "motor" + std::to_string(i);
    const std::string b = "motor" + std::to_string(i + 1);
    total += bits_differing(derive_seed(7, a), derive_seed(7, b));
  }
  const double mean = total / n;
  std::cout << "  seeds for motor0, motor1, motor2, ... : consecutive names differ in " << std::setprecision(1) << mean << " of 64 bits on average\n\n";
  RC_CHECK(mean > 28.0 && mean < 36.0);
  RC_CHECK(derive_seed(0, "") != derive_seed(0, "a"));
}

RC_TEST("a stream gives numbers that are spread evenly from 0 up to but not including 1, and normal ones with mean 0 and spread 1") {
  Stream s(derive_seed(1, "check"));
  const int n = 100000;
  double sum = 0.0;
  double sq = 0.0;
  double lowest = 1.0;
  double highest = 0.0;
  for (int i = 0; i < n; ++i) {
    const double u = s.uniform();
    sum += u;
    sq += u * u;
    if (u < lowest) lowest = u;
    if (u > highest) highest = u;
  }
  const double mean = sum / n;
  const double variance = sq / n - mean * mean;
  double gsum = 0.0;
  double gsq = 0.0;
  for (int i = 0; i < n; ++i) {
    const double g = s.gaussian();
    gsum += g;
    gsq += g * g;
  }
  const double gmean = gsum / n;
  const double gsd = std::sqrt(gsq / n - gmean * gmean);
  std::cout << "  100000 uniform numbers: mean " << std::setprecision(4) << mean << ", variance " << variance << " (1/12 is " << 1.0 / 12.0 << "), range " << lowest << " to " << highest
            << "; normal: mean " << gmean << ", spread " << gsd << "\n\n";
  RC_CHECK_NEAR(mean, 0.5, 0.005);
  RC_CHECK_NEAR(variance, 1.0 / 12.0, 0.002);
  RC_CHECK(lowest >= 0.0 && highest < 1.0);
  RC_CHECK_NEAR(gmean, 0.0, 0.015);
  RC_CHECK_NEAR(gsd, 1.0, 0.015);
  Stream zero(0);
  RC_CHECK(zero.next() != zero.next());                    // a seed of zero is not stuck at zero
}

RC_TEST("two parts of a simulation with the same master seed get streams that are not correlated, and the same part gets the same numbers on every run") {
  Streams run1(2024);
  Streams run2(2024);
  const std::vector<double> a1 = draw(run1.get("encoder"), 10000);
  const std::vector<double> b1 = draw(run1.get("imu"), 10000);
  const std::vector<double> a2 = draw(run2.get("encoder"), 10000);
  const double r = correlation(a1, b1);
  std::cout << "  the encoder's and the imu's streams from one seed: correlation " << std::setprecision(4) << r << "; the encoder's stream in a second run of the same seed: "
            << (a1 == a2 ? "identical" : "different") << "\n\n";
  RC_CHECK(std::fabs(r) < 0.03);
  RC_CHECK(a1 == a2);
  RC_CHECK(a1 != b1);
  Streams other(2025);
  RC_CHECK(draw(other.get("encoder"), 100) != std::vector<double>(a1.begin(), a1.begin() + 100));
  RC_CHECK(run1.count() == 2);
}

RC_TEST("asking for a stream a second time gives the same stream, carrying on from where it was, and the order the streams are first asked for in changes nothing") {
  Streams a(7);
  Streams b(7);
  const double a_motor_1 = a.get("motor").uniform();
  const double a_motor_2 = a.get("motor").uniform();                  // the same stream, the next number
  const double a_imu = a.get("imu").uniform();
  const double b_imu = b.get("imu").uniform();                        // asked for in the other order
  const double b_motor_1 = b.get("motor").uniform();
  const double b_motor_2 = b.get("motor").uniform();
  std::cout << "  streams asked for in two different orders: motor " << std::setprecision(6) << a_motor_1 << " " << a_motor_2 << " and " << b_motor_1 << " " << b_motor_2 << "; imu " << a_imu
            << " and " << b_imu << "\n\n";
  RC_CHECK(a_motor_1 == b_motor_1);
  RC_CHECK(a_motor_2 == b_motor_2);
  RC_CHECK(a_motor_1 != a_motor_2);
  RC_CHECK(a_imu == b_imu);
  RC_CHECK(a.count() == 2);
}

RC_TEST("adding a part to a simulation changes nothing for the others with a stream each, and changes everything for them when they share one") {
  // Three sensors read noise every step for 1000 steps. Then a fourth is added in the middle of the list.
  const auto with_streams = [](bool add_fourth) {
    Streams s(99);
    std::vector<double> imu;
    for (int i = 0; i < 1000; ++i) {
      s.get("encoder").gaussian();
      if (add_fourth) s.get("camera").gaussian();
      imu.push_back(s.get("imu").gaussian());
      s.get("gps").gaussian();
    }
    return imu;
  };
  const auto with_shared = [](bool add_fourth) {
    Stream shared(derive_seed(99, "everything"));
    std::vector<double> imu;
    for (int i = 0; i < 1000; ++i) {
      shared.gaussian();                                              // encoder
      if (add_fourth) shared.gaussian();                              // camera
      imu.push_back(shared.gaussian());
      shared.gaussian();                                              // gps
    }
    return imu;
  };
  const std::vector<double> sep_before = with_streams(false);
  const std::vector<double> sep_after = with_streams(true);
  const std::vector<double> shr_before = with_shared(false);
  const std::vector<double> shr_after = with_shared(true);
  int sep_changed = 0;
  int shr_changed = 0;
  for (int i = 0; i < 1000; ++i) {
    if (sep_before[static_cast<std::size_t>(i)] != sep_after[static_cast<std::size_t>(i)]) ++sep_changed;
    if (shr_before[static_cast<std::size_t>(i)] != shr_after[static_cast<std::size_t>(i)]) ++shr_changed;
  }
  std::cout << "  the imu's noise over 1000 steps, before and after a camera is added to the list: with a stream each " << sep_changed << " of 1000 values changed; sharing one stream "
            << shr_changed << " of 1000\n\n";
  RC_CHECK(sep_changed == 0);
  RC_CHECK(shr_changed == 1000);
}
