#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>

enum Mode { kFull = 0, kReduced = 1, kCreep = 2, kSafeStop = 3 };

struct VehicleHealth {
  bool scanner_ok = true;
  bool gnss_ok = true;
  bool gyro_ok = true;
  bool wheels_ok = true;
  bool link_ok = true;
  double sigma_m = 0.05;  // how sure the position is
};

struct ModePolicy {
  double cap_full = 2.0;      // m/s
  double cap_reduced = 0.8;
  double cap_creep = 0.25;
  double sigma_reduced = 0.3; // m of sigma at which the vehicle must be at least reduced
  double sigma_creep = 0.8;
  double sigma_stop = 1.5;
  double dwell_s = 10.0;      // s the requirement must be better before the vehicle goes up a step
};

inline double mode_speed_cap(Mode m, const ModePolicy& p) {
  switch (m) {
    case kFull: return p.cap_full;
    case kReduced: return p.cap_reduced;
    case kCreep: return p.cap_creep;
    default: return 0.0;
  }
}

// The mode the health asks for.
inline Mode required_mode(const VehicleHealth& h, const ModePolicy& p) {
  Mode m = kFull;
  (void)h;  // TODO: the scanner lost is a safe stop; the gyro lost a creep; the GNSS, wheels or link lost a reduced mode; two of those four (the gyro counts) a creep; and the sigma past its three steps makes it worse (a sigma that is not a number is a safe stop)
  (void)p;
  return m;
}

// Goes to a worse mode at once and back one step at a time after a dwell in which the requirement was better.
class ModeManager {
 public:
  explicit ModeManager(const ModePolicy& p) : p_(p) {}

  Mode update(double t, Mode required) {
    if (required > mode_) {
      mode_ = required;
      better_since_ = -1.0;
      ++changes_;
    } else if (required < mode_) {
      if (better_since_ < 0.0) better_since_ = t;
      if (t - better_since_ >= p_.dwell_s) {
        mode_ = static_cast<Mode>(std::max(static_cast<int>(required), static_cast<int>(mode_) - 1));
        better_since_ = t;
        ++changes_;
      }
    } else {
      better_since_ = -1.0;
    }
    return mode_;
  }

  Mode mode() const { return mode_; }
  int changes() const { return changes_; }

 private:
  ModePolicy p_;
  Mode mode_ = kFull;
  double better_since_ = -1.0;
  int changes_ = 0;
};

// The distance that dead reckoning can go before sigma, which grows as sqrt(sigma0^2 + k^2 d), reaches a limit.
inline double dead_reckoning_distance(double sigma0, double k, double sigma_limit) {
  if (sigma_limit <= sigma0 || k <= 0.0) return 0.0;
  return (sigma_limit * sigma_limit - sigma0 * sigma0) / (k * k);
}


#endif
