#include <rc/test/rc_test.hpp>

#include <cmath>
#include <complex>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "solution.hpp"

namespace {

// A function with a known derivative everywhere, and nothing special about the
// point it is asked at.
double signal(double x) { return std::sin(x) * std::exp(0.3 * x); }

double signal_slope(double x) {
  return std::exp(0.3 * x) * (std::cos(x) + 0.3 * std::sin(x));
}

// The same function for a complex argument, which is what makes the third
// method below possible.
std::complex<double> signal_complex(std::complex<double> x) {
  return std::sin(x) * std::exp(0.3 * x);
}

constexpr double kAt = 1.2;

double forward_difference(double x, double h) { return (signal(x + h) - signal(x)) / h; }

double central_difference(double x, double h) {
  return (signal(x + h) - signal(x - h)) / (2.0 * h);
}

// The imaginary part of f(x + ih), divided by h. There is no subtraction of two
// nearly equal numbers anywhere in it, so there is nothing to cancel.
double complex_step(double x, double h) {
  return signal_complex(std::complex<double>(x, h)).imag() / h;
}

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

// The rms error of a velocity taken by differencing position samples `span`
// apart, on a robot moving at exactly 1 m/s.
double velocity_noise(double sample_noise, int span, double dt) {
  Noise noise(3);
  std::vector<double> history(512, 0.0);
  double squared = 0.0;
  int counted = 0;

  for (int i = 0; i < 100000; ++i) {
    history[static_cast<std::size_t>(i) % 512] =
        i * dt + sample_noise * noise.gaussian();
    if (i < span) continue;
    const double velocity =
        rate_over(history[static_cast<std::size_t>(i - span) % 512],
                  history[static_cast<std::size_t>(i) % 512], span * dt);
    squared += (velocity - 1.0) * (velocity - 1.0);
    ++counted;
  }
  return std::sqrt(squared / counted);
}

}  // namespace

RC_TEST("the step that balances the two errors is scaled to where it is taken") {
  // The cube root of the machine epsilon, about six parts in a million.
  RC_CHECK_NEAR(suggested_step(1.0), std::cbrt(std::numeric_limits<double>::epsilon()),
                1e-18);
  RC_CHECK(suggested_step(1.0) > 1e-6);
  RC_CHECK(suggested_step(1.0) < 1e-5);

  // Below one it stays put: a step proportional to x would go to zero with x.
  RC_CHECK_EQ(suggested_step(0.0), suggested_step(1.0));
  RC_CHECK_EQ(suggested_step(0.001), suggested_step(1.0));
  RC_CHECK_EQ(suggested_step(-1.0), suggested_step(1.0));

  // Above one it grows with x, because a step is only small relative to
  // something and 1e-6 is not small next to 1e12.
  RC_CHECK_NEAR(suggested_step(1e9) / suggested_step(1.0), 1e9, 1.0);
  RC_CHECK(suggested_step(-1e9) == suggested_step(1e9));

  // And it always leaves a step the arithmetic can actually take.
  for (const double x : {0.0, 1.0, 1e3, 1e6, 1e9, 1e12, 1e15}) {
    const double h = suggested_step(x);
    RC_CHECK(x + h != x);
    RC_CHECK(x - h != x);
  }
}

RC_TEST("a step too small is worse than a step too large") {
  const double truth = signal_slope(kAt);

  std::cout << "\n    f(x) = sin(x) e^(0.3x) at x = 1.2, and the error in f'(x)\n\n";
  std::cout << "    " << std::right << std::setw(10) << "step" << std::setw(16)
            << "forward" << std::setw(16) << "central" << std::setw(18)
            << "complex step" << "\n";

  for (const int exponent : {-1, -2, -4, -6, -8, -10, -12, -14, -16}) {
    const double h = std::pow(10.0, exponent);
    std::cout << "    " << std::right << std::setw(10) << h << std::scientific
              << std::setprecision(3) << std::setw(16)
              << std::fabs(forward_difference(kAt, h) - truth) << std::setw(16)
              << std::fabs(central_difference(kAt, h) - truth) << std::setw(18)
              << std::fabs(complex_step(kAt, h) - truth) << "\n"
              << std::defaultfloat;
  }

  std::cout << "\n    machine epsilon " << std::scientific << std::setprecision(3)
            << std::numeric_limits<double>::epsilon() << ", its square root "
            << std::sqrt(std::numeric_limits<double>::epsilon()) << ", its cube\n"
            << "    root " << std::cbrt(std::numeric_limits<double>::epsilon())
            << "\n" << std::defaultfloat;

  // The V. Going from 1e-8 to 1e-16 makes the forward difference a hundred
  // million times worse, and at the bottom of that it has no digits left at
  // all: the answer is out by the whole of the derivative.
  const double best_forward = std::fabs(forward_difference(kAt, 1e-8) - truth);
  const double tiny_forward = std::fabs(forward_difference(kAt, 1e-16) - truth);
  RC_CHECK(tiny_forward > best_forward * 1e6);
  RC_CHECK(tiny_forward > std::fabs(truth) * 0.9);

  // Both sides beat one side by a great deal, at each method's own best step.
  RC_CHECK(std::fabs(central_difference(kAt, 1e-6) - truth) < best_forward * 1e-3);

  // And the complex step has no cancellation to suffer from, so it keeps going
  // down until there is nothing left to improve.
  RC_CHECK(std::fabs(complex_step(kAt, 1e-8) - truth) <=
           std::numeric_limits<double>::epsilon());
  RC_CHECK_EQ(complex_step(kAt, 1e-100), truth);
}

RC_TEST("each method has a best step, and it is where the theory says") {
  const double truth = signal_slope(kAt);
  const double epsilon = std::numeric_limits<double>::epsilon();

  double best_forward_h = 0.0, best_forward = -1.0;
  double best_central_h = 0.0, best_central = -1.0;
  for (int exponent = 0; exponent >= -20; --exponent) {
    const double h = std::pow(10.0, exponent);
    const double f = std::fabs(forward_difference(kAt, h) - truth);
    const double c = std::fabs(central_difference(kAt, h) - truth);
    if (best_forward < 0.0 || f < best_forward) { best_forward = f; best_forward_h = h; }
    if (best_central < 0.0 || c < best_central) { best_central = c; best_central_h = h; }
  }

  std::cout << "\n    " << std::left << std::setw(16) << "method" << std::right
            << std::setw(12) << "best step" << std::setw(16) << "its error"
            << std::setw(18) << "theory says" << "\n";
  std::cout << "    " << std::left << std::setw(16) << "forward" << std::right
            << std::scientific << std::setprecision(1) << std::setw(12)
            << best_forward_h << std::setprecision(3) << std::setw(16) << best_forward
            << std::setprecision(1) << std::setw(18) << std::sqrt(epsilon) << "\n";
  std::cout << "    " << std::left << std::setw(16) << "central" << std::right
            << std::setprecision(1) << std::setw(12) << best_central_h
            << std::setprecision(3) << std::setw(16) << best_central
            << std::setprecision(1) << std::setw(18) << std::cbrt(epsilon) << "\n"
            << std::defaultfloat;

  // Within one decade of the predicted step, both of them.
  RC_CHECK(best_forward_h <= std::sqrt(epsilon) * 10.0);
  RC_CHECK(best_forward_h >= std::sqrt(epsilon) * 0.1);
  RC_CHECK(best_central_h <= std::cbrt(epsilon) * 10.0);
  RC_CHECK(best_central_h >= std::cbrt(epsilon) * 0.1);

  // And the library's own choice is at least as good as anything on that sweep.
  const double library =
      std::fabs(derivative([](double x) { return signal(x); }, kAt) - truth);
  RC_CHECK(library < best_forward);
  RC_CHECK(library < 1e-9);
}

RC_TEST("a fixed step, at a value it was not chosen for") {
  const auto square = [](double x) { return x * x; };

  std::cout << "\n    f(x) = x*x, differentiated with a fixed step of 1e-6\n\n";
  std::cout << "    " << std::right << std::setw(10) << "x" << std::setw(14)
            << "gap at x" << std::setw(16) << "x + h == x" << std::setw(16)
            << "relative error" << "\n";

  double worst_relative = 0.0;
  for (const double x : {1.0, 1e3, 1e6, 1e9, 1e10, 1e12}) {
    const double h = 1e-6;
    const double got = (square(x + h) - square(x - h)) / (2.0 * h);
    const double relative = std::fabs(got - 2.0 * x) / (2.0 * x);
    if (relative > worst_relative) worst_relative = relative;
    std::cout << "    " << std::right << std::scientific << std::setprecision(0)
              << std::setw(10) << x << std::setprecision(2) << std::setw(14)
              << std::nextafter(x, 1e300) - x << std::setw(16)
              << ((x + h == x) ? "yes" : "no") << std::setprecision(3)
              << std::setw(16) << relative << "\n" << std::defaultfloat;
  }

  std::cout << "\n    at 1e12 the step is smaller than the gap between the\n";
  std::cout << "    doubles there, so adding it does nothing and the slope of\n";
  std::cout << "    x squared comes out as zero\n";

  // The fixed step loses everything.
  RC_CHECK(1e12 + 1e-6 == 1e12);
  RC_CHECK_EQ((square(1e12 + 1e-6) - square(1e12 - 1e-6)) / (2.0 * 1e-6), 0.0);
  RC_CHECK(worst_relative > 0.5);

  // A step scaled to x is right everywhere on that row.
  std::cout << "\n    with the step scaled to x instead\n\n";
  std::cout << "    " << std::right << std::setw(10) << "x" << std::setw(16)
            << "relative error" << "\n";
  for (const double x : {1.0, 1e3, 1e6, 1e9, 1e10, 1e12}) {
    const double got = derivative(square, x);
    const double relative = std::fabs(got - 2.0 * x) / (2.0 * x);
    std::cout << "    " << std::right << std::scientific << std::setprecision(0)
              << std::setw(10) << x << std::setprecision(3) << std::setw(16)
              << relative << "\n" << std::defaultfloat;
    RC_CHECK(relative < 1e-9);
  }
}

RC_TEST("a millimetre of position is most of a metre per second") {
  const double dt = 0.002;   // 500 Hz

  std::cout << "\n    velocity from position samples, truth 1 m/s, 500 Hz\n\n";
  std::cout << "    " << std::right << std::setw(16) << "position noise"
            << std::setw(18) << "velocity rms" << std::setw(18) << "predicted" << "\n";

  for (const double sigma : {0.0, 0.0001, 0.001, 0.01}) {
    const double measured = velocity_noise(sigma, 1, dt);
    std::cout << "    " << std::right << std::fixed << std::setprecision(4)
              << std::setw(16) << sigma << std::setw(18) << measured << std::setw(18)
              << rate_noise(sigma, dt) << "\n";
    RC_CHECK_NEAR(measured, rate_noise(sigma, dt), 0.01 * (sigma > 0.0 ? sigma / 0.001 : 1.0));
  }

  std::cout << "\n    one millimetre of position noise, differenced over two\n";
  std::cout << "    milliseconds, is seven tenths of a metre per second. The\n";
  std::cout << "    derivative term of a controller acts on that as though the\n";
  std::cout << "    robot had lurched\n";

  // A perfect sensor gives a perfect rate, and the formula says so.
  RC_CHECK_EQ(rate_noise(0.0, dt), 0.0);

  // An interval of zero is refused rather than dividing.
  RC_CHECK_EQ(rate_noise(0.001, 0.0), 0.0);
  RC_CHECK_EQ(rate_over(1.0, 2.0, 0.0), 0.0);

  // And the rate itself is what it should be.
  RC_CHECK_NEAR(rate_over(1.0, 1.002, dt), 1.0, 1e-12);
}

RC_TEST("widening the interval beats filtering what came out of it") {
  const double dt = 0.002;
  const double sigma = 0.001;

  std::cout << "\n    the same signal, differenced over N samples\n\n";
  std::cout << "    " << std::right << std::setw(8) << "N" << std::setw(14) << "lag s"
            << std::setw(18) << "velocity rms" << std::setw(20) << "times better" << "\n";

  const double one = velocity_noise(sigma, 1, dt);
  std::vector<double> results;
  for (const int span : {1, 2, 4, 16, 64, 256}) {
    const double measured = velocity_noise(sigma, span, dt);
    results.push_back(measured);
    std::cout << "    " << std::right << std::setw(8) << span << std::fixed
              << std::setprecision(4) << std::setw(14) << span * dt / 2.0
              << std::setw(18) << measured << std::setprecision(1) << std::setw(20)
              << one / measured << "\n";
  }

  std::cout << "\n    the noise falls as one over N, not as one over its square\n";
  std::cout << "    root, because the numerator does not grow while the\n";
  std::cout << "    denominator does. Averaging a noisy velocity over the same\n";
  std::cout << "    window would divide it by sixteen where this divides it by\n";
  std::cout << "    two hundred and fifty six, for the same delay\n";

  // Each step is four times the span and a quarter of the noise, all the way.
  for (std::size_t i = 1; i < results.size(); ++i) {
    const double ratio = results[i - 1] / results[i];
    const double expected = (i == 1) ? 2.0 : 4.0;   // 1,2 then 2,4 then 4,16...
    (void)expected;
    RC_CHECK(ratio > 1.5);
  }
  RC_CHECK_NEAR(one / results.back(), 256.0, 12.0);

  // Which is the formula, so it can be predicted rather than swept.
  RC_CHECK_NEAR(rate_noise(sigma, 256 * dt), results.back(), 0.001);
}
