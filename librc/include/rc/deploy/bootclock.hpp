// rc/deploy/bootclock.hpp
//
// The clock of a robot at boot, from lesson 20-10, graduated.
//
// BootClock starts at an initial wall time (1970 on a robot with no battery backed clock), stamps moments with the time since boot, the wall time and whether it has been set, and is set once with sync, after which wall_of gives the real wall time of any moment since boot, early ones included, and NaN before. jump is the wall time that passed minus the time since boot that passed, classify says forward, backward or none against a threshold, sort_records orders records by boot and then time since boot, and check_validity says unknown, not_yet, valid or expired, and refuses to judge before the clock is set.
//
// Setting the clock at 40 s made the wall clock jump 1794699960 s (56.9 years) while 0.2 s passed; a timeout in wall time expired for nothing and one in time since boot did not, and with the clock put back an hour the wall time timeout could never fire. Sorted by wall time, 5 of 6 records of two boots were in the wrong place.

#ifndef RC_DEPLOY_BOOTCLOCK_HPP
#define RC_DEPLOY_BOOTCLOCK_HPP

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace rc {
namespace deploy {

// A moment as the robot saw it: how long since it booted (which only ever goes forward), what the wall clock said, and whether the wall clock had been set from a real source by then.
struct Stamp {
  double mono = 0.0;
  double wall = 0.0;
  bool synced = false;
};

// The clock of a robot with no battery backed clock: it starts at `initial_wall` (the last time saved, or 1970) and counts up from there, and is set once, later, when the network gives it the real time.
class BootClock {
 public:
  explicit BootClock(double initial_wall) : offset_(initial_wall) {}

  // The stamp of a moment `mono` seconds after boot: wall is the offset plus mono, and synced says whether the clock had been set.
  Stamp stamp(double mono) const {
    Stamp s;
    s.mono = mono;
    s.wall = offset_ + mono;
    s.synced = synced_;
    return s;
  }

  // The real time is known: at `mono` seconds after boot it was `true_wall`. From now on the offset is true_wall - mono and the clock is synced.
  void sync(double mono, double true_wall) {
    offset_ = true_wall - mono;
    synced_ = true;
  }

  bool synced() const { return synced_; }

  // What the wall time was at `mono` seconds after boot, now that it is known: offset plus mono, for a moment before the sync as well as after it. NaN if the clock has not been set.
  double wall_of(double mono) const { return synced_ ? offset_ + mono : std::nan(""); }

 private:
  double offset_ = 0.0;
  bool synced_ = false;
};

// By how much the wall clock jumped between two readings: how much wall time passed minus how much real time passed (the change in mono, which cannot jump).
inline double jump(double prev_mono, double prev_wall, double mono, double wall) { return (wall - prev_wall) - (mono - prev_mono); }

enum class ClockStep { none, forward, backward };

// A jump of more than `threshold` seconds either way is a step; a smaller one is the clock being slewed or just noise.
inline ClockStep classify(double jump_seconds, double threshold) {
  if (jump_seconds > threshold) return ClockStep::forward;
  if (jump_seconds < -threshold) return ClockStep::backward;
  return ClockStep::none;
}

struct Record {
  int boot = 0;               // which boot wrote it: 1, 2, 3 ...
  double mono = 0.0;
  double wall = 0.0;
  std::string text;
};

// The records in the order they happened: by boot, then by time since boot. Records with the same boot and time keep the order they came in.
inline std::vector<Record> sort_records(std::vector<Record> records) {
  std::stable_sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
    if (a.boot != b.boot) return a.boot < b.boot;
    return a.mono < b.mono;
  });
  return records;
}

enum class Validity { valid, expired, not_yet, unknown };

// Is a certificate (or a licence, or a schedule) valid now, between not_before and not_after (both inclusive) in wall time? If the clock has not been set the answer is `unknown`: the wall time is a guess and must not be used to say yes or no.
inline Validity check_validity(double not_before, double not_after, const BootClock& clock, double mono) {
  if (!clock.synced()) return Validity::unknown;
  const double now = clock.stamp(mono).wall;
  if (now < not_before) return Validity::not_yet;
  if (now > not_after) return Validity::expired;
  return Validity::valid;
}

}  // namespace deploy
}  // namespace rc

#endif  // RC_DEPLOY_BOOTCLOCK_HPP
