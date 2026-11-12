// rc/locate/dead_reckon.hpp
//
// How lost can it be, from lesson 25-06.
//
// DeadReckoning carries a pose (x, y, heading) and its 3 by 3 covariance forward by odometry steps (a distance ds and a turn dtheta). The distance is wrong by a share of itself (scale_error), the turn by a share of itself (turn_error) and the heading by a random walk that grows with the distance (heading_walk, radians per root metre). The covariance is carried by P = F P F^T + G Q G^T, with F the Jacobian of the step with respect to the pose and G with respect to the odometry. radius95 is the radius of the circle that holds the position with 95 percent probability (the largest standard deviation of the position times the square root of 5.991, the 95 percent point of a chi-square with two degrees of freedom). reset_to sets the pose and its position variance from a fix.
//
// For a heading walk of 0.005 rad per root metre and a scale error of 1 percent the radius was 0.22 m after 10 m, 7.07 m after 100 m (31.6 times as much for ten times the distance, where a straight proportion would be 10); a Monte Carlo of 400 noisy runs had 95 percent of the robots within 5.10 m after 100 m where the bound said 7.07 m; a fix of 5 cm brought it back to 0.122 m; and the robot could go 27 m before it might be 1 m out and 80 m before it might be 5 m out.

#ifndef RC_RC_LOCATE_DEAD_RECKON_HPP
#define RC_RC_LOCATE_DEAD_RECKON_HPP

#include <cmath>

namespace rc {
namespace locate {

struct OdometryNoise {
  double scale_error = 0.01;     // the distance is wrong by this share of itself
  double turn_error = 0.03;      // the turn is wrong by this share of itself
  double heading_walk = 0.005;   // radians per root metre: the heading wanders as the robot goes
};

class DeadReckoning {
 public:
  explicit DeadReckoning(const OdometryNoise& n) : n_(n) {
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) P_[i][j] = 0.0;
  }

  double x() const { return x_; }
  double y() const { return y_; }
  double theta() const { return th_; }

  // Sets the pose from a fix good to `sigma` metres in each coordinate, and the heading to `heading_sigma`.
  void reset_to(double x, double y, double theta, double sigma, double heading_sigma) {
    x_ = x;
    y_ = y;
    th_ = theta;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) P_[i][j] = 0.0;
    P_[0][0] = P_[1][1] = sigma * sigma;
    P_[2][2] = heading_sigma * heading_sigma;
  }

  // One odometry step: a distance ds along the heading and a turn dtheta.
  void step(double ds, double dtheta) {
    const double mid = th_ + 0.5 * dtheta;
    const double c = std::cos(mid), s = std::sin(mid);
    const double var_ds = n_.scale_error * n_.scale_error * ds * ds;
    const double var_dth = n_.turn_error * n_.turn_error * dtheta * dtheta + n_.heading_walk * n_.heading_walk * std::fabs(ds);
    const double F[3][3] = {{1, 0, -ds * s}, {0, 1, ds * c}, {0, 0, 1}};
    const double G[3][2] = {{c, -0.5 * ds * s}, {s, 0.5 * ds * c}, {0, 1}};
    double FP[3][3], next[3][3];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) {
        FP[i][j] = 0.0;
        for (int k = 0; k < 3; ++k) FP[i][j] += F[i][k] * P_[k][j];
      }
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) {
        next[i][j] = 0.0;
        for (int k = 0; k < 3; ++k) next[i][j] += FP[i][k] * F[j][k];
        next[i][j] += G[i][0] * var_ds * G[j][0] + G[i][1] * var_dth * G[j][1];
      }
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) P_[i][j] = next[i][j];
    x_ += ds * c;
    y_ += ds * s;
    th_ += dtheta;
  }

  // The radius of the circle that holds the position with 95 percent probability.
  double radius95() const {
    const double a = P_[0][0], b = P_[0][1], d = P_[1][1];
    const double lambda = 0.5 * (a + d) + std::sqrt(0.25 * (a - d) * (a - d) + b * b);
    return std::sqrt(5.991 * lambda);
  }

  double heading_sigma() const { return std::sqrt(P_[2][2]); }

  // Whether the robot may now be further out than `limit` metres, and so should get a fix.
  bool needs_fix(double limit) const { return radius95() > limit; }

 private:
  OdometryNoise n_;
  double x_ = 0.0, y_ = 0.0, th_ = 0.0;
  double P_[3][3];
};


}  // namespace locate
}  // namespace rc

#endif
