#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/fk.hpp>
#include <vector>

using namespace rc::arm;

// A correction is four numbers per joint: 4 * i + 0 offset of the angle, +1 d, +2 a, +3 alpha.
inline ArmTable perturbed(const ArmTable& t, const std::vector<double>& c) {
  ArmTable out = t;
  for (std::size_t i = 0; i < out.rows.size(); ++i) {
    out.rows[i].theta_offset += c[4 * i + 0];
    out.rows[i].d += c[4 * i + 1];
    out.rows[i].a += c[4 * i + 2];
    out.rows[i].alpha += c[4 * i + 3];
  }
  return out;
}

struct Measure {
  std::vector<double> q;
  double p[3];
};

struct Lcg3 {
  unsigned long long s;
  explicit Lcg3(unsigned long long seed) : s(seed * 2862933555777941757ULL + 3037000493ULL) {}
  double uniform() {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<double>((s >> 33) & 0xFFFFFFULL) / 16777216.0;
  }
  double normal() {
    double a = 0.0;
    for (int i = 0; i < 12; ++i) a += uniform();
    return a - 6.0;
  }
};

// The tool's place for each set of joint angles, from the arm as it is (the nominal table and a correction), with noise of sigma metres.
inline std::vector<Measure> simulate_measurements(const ArmTable& nominal, const std::vector<double>& truth, const std::vector<std::vector<double> >& poses, double sigma, unsigned long long seed) {
  Lcg3 rng(seed);
  const ArmTable real = perturbed(nominal, truth);
  std::vector<Measure> out;
  for (std::size_t k = 0; k < poses.size(); ++k) {
    Measure m;
    m.q = poses[k];
    const Pose p = forward(real, poses[k], Pose(), Pose());
    for (int i = 0; i < 3; ++i) m.p[i] = p.p[i] + sigma * rng.normal();
    out.push_back(m);
  }
  return out;
}

// The root mean square distance between what the table predicts and what was measured, in metres.
inline double rms_error(const ArmTable& t, const std::vector<Measure>& ms) {
  double sum = 0.0;
  for (std::size_t k = 0; k < ms.size(); ++k) {
    const Pose p = forward(t, ms[k].q, Pose(), Pose());
    for (int i = 0; i < 3; ++i) sum += (p.p[i] - ms[k].p[i]) * (p.p[i] - ms[k].p[i]);
  }
  return ms.empty() ? 0.0 : std::sqrt(sum / static_cast<double>(ms.size()));
}

// Solves A x = b by elimination with partial pivoting; a pivot that is too small leaves that unknown at zero.
inline std::vector<double> solve_dense(std::vector<std::vector<double> > a, std::vector<double> b) {
  const std::size_t n = b.size();
  std::vector<double> x(n, 0.0);
  for (std::size_t c = 0; c < n; ++c) {
    std::size_t p = c;
    for (std::size_t r = c + 1; r < n; ++r)
      if (std::fabs(a[r][c]) > std::fabs(a[p][c])) p = r;
    if (std::fabs(a[p][c]) < 1e-300) continue;
    std::swap(a[c], a[p]);
    std::swap(b[c], b[p]);
    for (std::size_t r = c + 1; r < n; ++r) {
      const double f = a[r][c] / a[c][c];
      for (std::size_t k = c; k < n; ++k) a[r][k] -= f * a[c][k];
      b[r] -= f * b[c];
    }
  }
  for (std::size_t r = n; r-- > 0;) {
    if (std::fabs(a[r][r]) < 1e-300) continue;
    double s = b[r];
    for (std::size_t k = r + 1; k < n; ++k) s -= a[r][k] * x[k];
    x[r] = s / a[r][r];
  }
  return x;
}

struct CalibResult {
  std::vector<double> correction;
  double rms_before = 0.0;
  double rms_after = 0.0;
  int iterations = 0;
};

// Fits the correction to the measurements by Levenberg-Marquardt: lambda is the damping to start with.
inline CalibResult calibrate(const ArmTable& nominal, const std::vector<Measure>& ms, double lambda, int iterations) {
  CalibResult out;
  const std::size_t np = 4 * nominal.rows.size(), nr = 3 * ms.size();
  out.correction.assign(np, 0.0);
  out.rms_before = rms_error(nominal, ms);
  const double h = 1e-6;
  double lam = lambda;
  for (int it = 0; it < iterations; ++it) {
    const ArmTable now = perturbed(nominal, out.correction);
    std::vector<double> r(nr);
    for (std::size_t k = 0; k < ms.size(); ++k) {
      const Pose p = forward(now, ms[k].q, Pose(), Pose());
      for (int i = 0; i < 3; ++i) r[3 * k + i] = ms[k].p[i] - p.p[i];
    }
    std::vector<std::vector<double> > jac(nr, std::vector<double>(np, 0.0));
    for (std::size_t c = 0; c < np; ++c) {
      std::vector<double> ca = out.correction, cb = out.correction;
      ca[c] += h;
      cb[c] -= h;
      const ArmTable ta = perturbed(nominal, ca), tb = perturbed(nominal, cb);
      for (std::size_t k = 0; k < ms.size(); ++k) {
        const Pose pa = forward(ta, ms[k].q, Pose(), Pose()), pb = forward(tb, ms[k].q, Pose(), Pose());
        for (int i = 0; i < 3; ++i) jac[3 * k + i][c] = (pa.p[i] - pb.p[i]) / (2.0 * h);
      }
    }
    std::vector<std::vector<double> > a(np, std::vector<double>(np, 0.0));
    std::vector<double> g(np, 0.0);
    for (std::size_t i = 0; i < np; ++i) {
      for (std::size_t j = 0; j < np; ++j)
        for (std::size_t k = 0; k < nr; ++k) a[i][j] += jac[k][i] * jac[k][j];
      a[i][i] += lam;
      for (std::size_t k = 0; k < nr; ++k) g[i] += jac[k][i] * r[k];
    }
    std::vector<double> step(np, 0.0);
    step = solve_dense(a, g);
    // Keep the step only if it lowers the error; if it does not, damp harder (that is what makes it Levenberg and Marquardt's and not a plain Gauss-Newton).
    std::vector<double> candidate = out.correction;
    for (std::size_t i = 0; i < np; ++i) candidate[i] += step[i];
    const double cost_now = rms_error(now, ms), cost_new = rms_error(perturbed(nominal, candidate), ms);
    if (cost_new < cost_now) {
      out.correction = candidate;
      lam = std::fmax(lam / 3.0, 1e-12);
    } else {
      lam *= 10.0;
    }
    out.iterations = it + 1;
  }
  out.rms_after = rms_error(perturbed(nominal, out.correction), ms);
  return out;
}


#endif
