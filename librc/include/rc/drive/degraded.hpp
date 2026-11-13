// rc/drive/degraded.hpp
//
// When something stops working, from lesson 27-07.
//
// Mode is one of full, reduced, creep and safe stop, in order of how much the vehicle gives up; mode_speed_cap gives the speed each allows. required_mode takes a VehicleHealth (whether the scanner, the GNSS, the gyro, the wheel sensors and the link are working, and how sure the position is) and gives the mode it asks for: the scanner lost is a safe stop, the gyro lost a creep, the GNSS, the wheel sensors or the link lost a reduced mode, any two of those last four lost a creep, and the sigma of the position moves the mode on again at its three steps. ModeManager goes to a worse mode at once and goes back only one step at a time, and only after the requirement has been better for a dwell. dead_reckoning_distance is how far the vehicle can go before a position sigma that grows with the square root of the distance reaches a limit.
//
// The scanner lost gave 3, a lost gyro 2, a lost GNSS 1, a lost GNSS and link 2 (full is 0, reduced 1, creep 2, safe stop 3). A requirement that flipped between reduced and full every 2 s for 120 s changed the mode 60 times raw and 1 through the manager with a dwell of 10 s. After a scanner fault cleared the vehicle was back to full speed in 30 s, one step per dwell. With the GNSS lost the position sigma reached the limit of 1.5 m after 468 m of dead reckoning.

#ifndef RC_RC_DRIVE_DEGRADED_HPP
#define RC_RC_DRIVE_DEGRADED_HPP

#include <algorithm>
#include <cmath>

namespace rc {
namespace drive {

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
  if (!h.scanner_ok) m = kSafeStop;
  if (!h.gyro_ok) m = std::max(m, kCreep);
  if (!h.gnss_ok || !h.wheels_ok || !h.link_ok) m = std::max(m, kReduced);
  const int lost = (!h.gnss_ok ? 1 : 0) + (!h.wheels_ok ? 1 : 0) + (!h.link_ok ? 1 : 0) + (!h.gyro_ok ? 1 : 0);
  if (lost >= 2) m = std::max(m, kCreep);
  if (!(h.sigma_m <= p.sigma_stop)) m = kSafeStop;
  else if (h.sigma_m > p.sigma_creep) m = std::max(m, kCreep);
  else if (h.sigma_m > p.sigma_reduced) m = std::max(m, kReduced);
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


}  // namespace drive
}  // namespace rc

#endif
