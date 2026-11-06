#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

// A matrix of doubles, stored by rows.
//
// Small, and written out because the point of this lesson is what is inside a solver. A
// library does the same thing faster, and a learner who has written it knows what its
// errors mean.
class Matrix {
 public:
  Matrix(std::size_t rows, std::size_t cols, double fill = 0.0)
      : rows_(rows), cols_(cols), data_(rows * cols, fill) {}

  std::size_t rows() const { return rows_; }
  std::size_t cols() const { return cols_; }
  double& at(std::size_t r, std::size_t c) { return data_[r * cols_ + c]; }
  double at(std::size_t r, std::size_t c) const { return data_[r * cols_ + c]; }

  Matrix transpose() const {
    Matrix t(cols_, rows_);
    for (std::size_t r = 0; r < rows_; ++r)
      for (std::size_t c = 0; c < cols_; ++c) t.at(c, r) = at(r, c);
    return t;
  }

 private:
  std::size_t rows_;
  std::size_t cols_;
  std::vector<double> data_;
};

inline Matrix multiply(const Matrix& a, const Matrix& b) {
  Matrix out(a.rows(), b.cols());
  for (std::size_t i = 0; i < a.rows(); ++i)
    for (std::size_t k = 0; k < a.cols(); ++k)
      for (std::size_t j = 0; j < b.cols(); ++j) out.at(i, j) += a.at(i, k) * b.at(k, j);
  return out;
}

inline std::vector<double> multiply(const Matrix& a, const std::vector<double>& x) {
  std::vector<double> out(a.rows(), 0.0);
  for (std::size_t i = 0; i < a.rows(); ++i)
    for (std::size_t j = 0; j < a.cols(); ++j) out[i] += a.at(i, j) * x[j];
  return out;
}

// The biggest entry in absolute value, which is the scale a pivot is compared against.
inline double largest_entry(const Matrix& a) {
  double big = 0.0;
  for (std::size_t r = 0; r < a.rows(); ++r)
    for (std::size_t c = 0; c < a.cols(); ++c) big = std::max(big, std::fabs(a.at(r, c)));
  return big;
}

// Solves A x = b by Gaussian elimination with partial pivoting.
//
// Elimination subtracts a multiple of the pivot row from every row below it, and the multiple
// is the entry below over the pivot. A small pivot makes a huge multiple, which drowns the
// rest of the row in rounding error. So before each step the row with the largest entry in
// that column is swapped up to be the pivot, and no multiple is ever larger than one.
//
// `ok` is false only for a matrix that is singular to working precision. A matrix that is
// nearly singular is solved, and the answer may be wrong: ok does not say so, and the
// condition number does.
inline std::vector<double> solve(Matrix a, std::vector<double> b, bool& ok) {
  const std::size_t n = a.rows();
  ok = a.cols() == n && b.size() == n && n > 0;
  if (!ok) return std::vector<double>(b.size(), 0.0);

  const double scale = largest_entry(a);
  for (std::size_t k = 0; k < n; ++k) {
    std::size_t pivot = k;
    for (std::size_t i = k + 1; i < n; ++i)
      if (std::fabs(a.at(i, k)) > std::fabs(a.at(pivot, k))) pivot = i;

    if (std::fabs(a.at(pivot, k)) <= 1e-14 * scale) {
      ok = false;
      return std::vector<double>(n, 0.0);
    }

    if (pivot != k) {
      for (std::size_t j = 0; j < n; ++j) std::swap(a.at(k, j), a.at(pivot, j));
      std::swap(b[k], b[pivot]);
    }

    for (std::size_t i = k + 1; i < n; ++i) {
      const double factor = a.at(i, k) / a.at(k, k);
      for (std::size_t j = k; j < n; ++j) a.at(i, j) -= factor * a.at(k, j);
      b[i] -= factor * b[k];
    }
  }

  std::vector<double> x(n, 0.0);
  for (std::size_t i = n; i-- > 0;) {
    double s = b[i];
    for (std::size_t j = i + 1; j < n; ++j) s -= a.at(i, j) * x[j];
    x[i] = s / a.at(i, i);
  }
  return x;
}

// The largest column sum of absolute values: the 1-norm, which measures how much a matrix can
// stretch a vector.
inline double one_norm(const Matrix& m) {
  double best = 0.0;
  for (std::size_t c = 0; c < m.cols(); ++c) {
    double sum = 0.0;
    for (std::size_t r = 0; r < m.rows(); ++r) sum += std::fabs(m.at(r, c));
    best = std::max(best, sum);
  }
  return best;
}

// How much a matrix magnifies relative error: its 1-norm times the 1-norm of its inverse.
//
// A condition number of 10^k means that about k of the digits in the answer are lost, however
// good the solver is, because the problem itself cannot tell nearby answers apart. The
// inverse is found by solving for each column of the identity, which is affordable for a small
// matrix and not how a library does it.
inline double condition_1norm(const Matrix& a) {
  const std::size_t n = a.rows();
  if (a.cols() != n || n == 0) return std::numeric_limits<double>::infinity();

  Matrix inverse(n, n);
  for (std::size_t c = 0; c < n; ++c) {
    std::vector<double> unit(n, 0.0);
    unit[c] = 1.0;
    bool ok = false;
    const std::vector<double> column = solve(a, unit, ok);
    if (!ok) return std::numeric_limits<double>::infinity();
    for (std::size_t r = 0; r < n; ++r) inverse.at(r, c) = column[r];
  }
  return one_norm(a) * one_norm(inverse);
}

// The x that makes A x closest to b, by forming the normal equations A^T A x = A^T b and
// solving them.
//
// It is the textbook method and it is the obvious one, and it squares the condition number:
// A^T A is a much worse matrix than A, so the answer loses about twice as many digits as it
// should.
inline std::vector<double> least_squares_normal(const Matrix& a, const std::vector<double>& b, bool& ok) {
  const Matrix at = a.transpose();
  return solve(multiply(at, a), multiply(at, b), ok);
}

// The same answer by QR: rotate A to upper triangular form with reflections, apply the same
// reflections to b, and back substitute. No product A^T A is ever formed, so the condition
// number is A's own and not its square.
inline std::vector<double> least_squares_qr(Matrix a, std::vector<double> b, bool& ok) {
  const std::size_t m = a.rows();
  const std::size_t n = a.cols();
  ok = m >= n && b.size() == m && n > 0;
  if (!ok) return std::vector<double>(n, 0.0);

  const double scale = largest_entry(a);
  for (std::size_t k = 0; k < n; ++k) {
    double norm = 0.0;
    for (std::size_t i = k; i < m; ++i) norm += a.at(i, k) * a.at(i, k);
    norm = std::sqrt(norm);
    if (norm <= 1e-14 * scale) {
      ok = false;
      return std::vector<double>(n, 0.0);
    }

    // A Householder reflection that sends column k to a multiple of the first unit vector. The
    // sign is chosen to avoid cancellation in the first entry.
    const double alpha = a.at(k, k) > 0.0 ? -norm : norm;
    std::vector<double> v(m, 0.0);
    for (std::size_t i = k; i < m; ++i) v[i] = a.at(i, k);
    v[k] -= alpha;
    double vv = 0.0;
    for (std::size_t i = k; i < m; ++i) vv += v[i] * v[i];
    if (vv == 0.0) continue;

    for (std::size_t j = k; j < n; ++j) {
      double dot = 0.0;
      for (std::size_t i = k; i < m; ++i) dot += v[i] * a.at(i, j);
      dot = 2.0 * dot / vv;
      for (std::size_t i = k; i < m; ++i) a.at(i, j) -= dot * v[i];
    }
    double dot = 0.0;
    for (std::size_t i = k; i < m; ++i) dot += v[i] * b[i];
    dot = 2.0 * dot / vv;
    for (std::size_t i = k; i < m; ++i) b[i] -= dot * v[i];
  }

  std::vector<double> x(n, 0.0);
  for (std::size_t i = n; i-- > 0;) {
    double s = b[i];
    for (std::size_t j = i + 1; j < n; ++j) s -= a.at(i, j) * x[j];
    if (std::fabs(a.at(i, i)) <= 1e-14 * scale) {
      ok = false;
      return std::vector<double>(n, 0.0);
    }
    x[i] = s / a.at(i, i);
  }
  return x;
}

#endif  // LESSON_SOLUTION_HPP
