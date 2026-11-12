#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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
    (void)c;  // TODO: P' = F P F^T + G diag(var_ds, var_dth) G^T, with F = [[1,0,-ds sin],[0,1,ds cos],[0,0,1]] and G = [[cos, -ds sin / 2],[sin, ds cos / 2],[0,1]] at the middle heading
    (void)s;
    (void)var_ds;
    (void)var_dth;
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


#endif
