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
inline std::vector<double> solve(Matrix a, std::vector<double> b, bool& ok) {
  // TODO: a must be square and b the same length, and not empty: otherwise ok is false.
  //
  // For each column k in turn:
  //   1. PIVOT. Find the row at or below k whose entry in column k is the largest in absolute
  //      value, and swap it with row k (in a and in b). Without this a small pivot makes a huge
  //      multiple, which drowns the rest of the row in rounding error.
  //   2. If that largest entry is no more than 1e-14 times the largest entry in the whole
  //      matrix (largest_entry), the matrix is singular to working precision: ok is false and
  //      the answer is zeros.
  //   3. For each row below k: factor = entry / pivot, subtract factor times row k from it
  //      (from column k on), and the same from b.
  //
  // Then back substitute: from the last row up, x[i] = (b[i] - the sum of a[i][j] x[j] for j
  // above i) / a[i][i].
  (void)a;
  (void)b;
  ok = false;
  return std::vector<double>();
}

// The largest column sum of absolute values: the 1-norm. Given.
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
inline double condition_1norm(const Matrix& a) {
  // TODO: the inverse, one column at a time: solve A x = e_c, where e_c is the c-th column of
  // the identity (zeros with a single one), for each c, and the answers are the columns of the
  // inverse. If any solve fails the matrix is singular and its condition number is infinity.
  // A matrix that is not square, or empty, is infinity too.
  //
  // The result is one_norm(a) * one_norm(inverse). A condition number of 10^k means about k of
  // the digits in an answer are lost, however good the solver is.
  (void)a;
  return 0.0;
}

// The x that makes A x closest to b, by forming the normal equations and solving them.
inline std::vector<double> least_squares_normal(const Matrix& a, const std::vector<double>& b, bool& ok) {
  // TODO: solve (A^T A) x = A^T b: transpose, multiply, and solve. It is the textbook method, and
  // it squares the condition number.
  (void)a;
  (void)b;
  ok = false;
  return std::vector<double>();
}

// The same answer by QR.
inline std::vector<double> least_squares_qr(Matrix a, std::vector<double> b, bool& ok) {
  // TODO: a must have at least as many rows as columns, b one entry per row, and not be empty.
  //
  // For each column k: reflect so that column k below the diagonal becomes zero (Householder).
  //   norm  = length of column k from row k down. If it is no more than 1e-14 times the largest
  //           entry of a, the columns are dependent: ok is false.
  //   alpha = -norm if a[k][k] > 0, else +norm (the sign avoids cancellation)
  //   v     = column k from row k down, with v[k] -= alpha (zero above k)
  //   for each column j from k on, and for b: subtract (2 * (v . column) / (v . v)) * v
  // After all columns, a is upper triangular. Back substitute for x as in solve. No product of A
  // with its transpose is ever formed, so the condition number is A's own and not its square.
  (void)a;
  (void)b;
  ok = false;
  return std::vector<double>();
}

#endif  // LESSON_SOLUTION_HPP
