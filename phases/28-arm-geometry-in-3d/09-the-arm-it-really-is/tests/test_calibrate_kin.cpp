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
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

std::vector<std::vector<double> > poses(int n, unsigned long long seed) {
  Lcg3 rng(seed);
  std::vector<std::vector<double> > out;
  for (int k = 0; k < n; ++k) {
    std::vector<double> q(6);
    for (int i = 0; i < 6; ++i) q[static_cast<std::size_t>(i)] = -1.5 + 3.0 * rng.uniform();
    out.push_back(q);
  }
  return out;
}

// What manufacture does to a table: up to 0.3 degrees in the angles and 1 mm in the lengths.
std::vector<double> tolerance_truth(unsigned long long seed) {
  Lcg3 rng(seed);
  std::vector<double> c(24);
  for (std::size_t i = 0; i < 6; ++i) {
    c[4 * i + 0] = 0.005 * (2.0 * rng.uniform() - 1.0);
    c[4 * i + 1] = 0.001 * (2.0 * rng.uniform() - 1.0);
    c[4 * i + 2] = 0.001 * (2.0 * rng.uniform() - 1.0);
    c[4 * i + 3] = 0.005 * (2.0 * rng.uniform() - 1.0);
  }
  return c;
}

double mm(double m) { return 1000.0 * m; }

}  // namespace

RC_TEST("calibrating from enough measured poses takes the error from millimetres to the size of the noise, on poses it has not seen too") {
  const ArmTable nominal = ur5();
  const std::vector<double> truth = tolerance_truth(3);
  const std::vector<Measure> train = simulate_measurements(nominal, truth, poses(60, 11), 5e-5, 21);
  const std::vector<Measure> valid = simulate_measurements(nominal, truth, poses(30, 12), 5e-5, 22);
  const CalibResult r = calibrate(nominal, train, 1e-9, 12);
  const double before = rms_error(nominal, valid);
  const double after_train = rms_error(perturbed(nominal, r.correction), train);
  const double after_val = rms_error(perturbed(nominal, r.correction), valid);
  rec("before", mm(before), 2);
  rec("n_train", 60, 0);
  rec("n_val", 30, 0);
  rec("train", mm(after_train), 3);
  rec("val", mm(after_val), 3);
  RC_CHECK(before > 0.001);
  RC_CHECK(after_val < before / 10.0);
  RC_CHECK(after_train < 2.0e-4);
  RC_CHECK(after_val < 3.0e-4);
}

RC_TEST("with fewer measurements than corrections the fit is exact on its own poses and poor on others, and it gets better with every pose added") {
  const ArmTable nominal = ur5();
  const std::vector<double> truth = tolerance_truth(3);
  const std::vector<Measure> valid = simulate_measurements(nominal, truth, poses(30, 12), 5e-5, 22);
  const int counts[5] = {4, 8, 12, 24, 60};
  double val[5], train[5];
  for (int k = 0; k < 5; ++k) {
    const std::vector<Measure> ms = simulate_measurements(nominal, truth, poses(counts[k], 31), 5e-5, 41);
    const CalibResult r = calibrate(nominal, ms, 1e-9, 25);
    train[k] = rms_error(perturbed(nominal, r.correction), ms);
    val[k] = rms_error(perturbed(nominal, r.correction), valid);
  }
  rec("n_few", counts[0], 0);
  rec("few_train", mm(train[0]), 8);
  rec("few_val", mm(val[0]), 2);
  rec("val8", mm(val[1]), 3);
  rec("val24", mm(val[3]), 3);
  rec("val60", mm(val[4]), 3);
  RC_CHECK(train[0] < 1e-9);
  RC_CHECK(val[0] > 5.0 * val[4]);
  for (int k = 1; k < 5; ++k) RC_CHECK(val[k] < val[k - 1]);
}

RC_TEST("more noise in the measurements is more error on new poses") {
  const ArmTable nominal = ur5();
  const std::vector<double> truth = tolerance_truth(3);
  const std::vector<Measure> valid = simulate_measurements(nominal, truth, poses(30, 12), 0.0, 22);
  const std::vector<Measure> quiet = simulate_measurements(nominal, truth, poses(60, 11), 5e-5, 21);
  const std::vector<Measure> noisy = simulate_measurements(nominal, truth, poses(60, 11), 1e-3, 21);
  const double vq = rms_error(perturbed(nominal, calibrate(nominal, quiet, 1e-9, 12).correction), valid);
  const double vn = rms_error(perturbed(nominal, calibrate(nominal, noisy, 1e-9, 12).correction), valid);
  rec("noisy_val", mm(vn), 3);
  RC_CHECK(vn > vq);
  RC_CHECK(vn < rms_error(nominal, valid));
}

RC_TEST("two corrections that give the same tool place for every pose cannot be told apart by measuring it") {
  const ArmTable nominal = ur5();
  // Joints 2 and 3 have parallel axes: shifting along one of them or along the other moves the elbow the same way.
  std::vector<double> a(24, 0.0), b(24, 0.0);
  a[4 * 1 + 1] = 0.001;
  b[4 * 2 + 1] = 0.001;
  const ArmTable ta = perturbed(nominal, a), tb = perturbed(nominal, b);
  double worst = 0.0;
  const std::vector<std::vector<double> > ps = poses(50, 5);
  for (std::size_t k = 0; k < ps.size(); ++k) {
    const Pose pa = forward(ta, ps[k], Pose(), Pose()), pb = forward(tb, ps[k], Pose(), Pose());
    for (int i = 0; i < 3; ++i) worst = std::fmax(worst, std::fabs(pa.p[i] - pb.p[i]));
  }
  rec("dup_err", worst, 12);
  RC_CHECK(worst < 1e-12);
}

RC_TEST("a table with no error is left where it is") {
  const ArmTable nominal = ur5();
  const std::vector<Measure> ms = simulate_measurements(nominal, std::vector<double>(24, 0.0), poses(40, 7), 0.0, 1);
  const CalibResult r = calibrate(nominal, ms, 1e-9, 5);
  RC_CHECK(r.rms_before < 1e-12);
  RC_CHECK(r.rms_after < 1e-9);
  double n = 0.0;
  for (std::size_t i = 0; i < r.correction.size(); ++i) n = std::fmax(n, std::fabs(r.correction[i]));
  RC_CHECK(n < 1e-6);
}
