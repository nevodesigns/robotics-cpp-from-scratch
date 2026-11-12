#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

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
    // TODO: a Stamp with the mono given, wall = offset_ + mono and synced = synced_
    (void)mono;
    (void)offset_;
    (void)synced_;
    return Stamp();
  }

  // The real time is known: at `mono` seconds after boot it was `true_wall`. From now on the offset is true_wall - mono and the clock is synced.
  void sync(double mono, double true_wall) {
    // TODO: offset_ = true_wall - mono and synced_ = true
    (void)mono;
    (void)true_wall;
    (void)offset_;
    (void)synced_;
  }

  bool synced() const { return synced_; }

  // What the wall time was at `mono` seconds after boot, now that it is known: offset plus mono, for a moment before the sync as well as after it. NaN if the clock has not been set.
  double wall_of(double mono) const {
    // TODO: offset_ + mono once synced, NaN before (std::nan with an empty string)
    (void)mono;
    (void)offset_;
    (void)synced_;
    return 0.0;
  }

 private:
  double offset_ = 0.0;
  bool synced_ = false;
};

// By how much the wall clock jumped between two readings: how much wall time passed minus how much real time passed (the change in mono, which cannot jump).
inline double jump(double prev_mono, double prev_wall, double mono, double wall) {
  // TODO: (wall - prev_wall) - (mono - prev_mono)
  (void)prev_mono;
  (void)prev_wall;
  (void)mono;
  (void)wall;
  return 0.0;
}

enum class ClockStep { none, forward, backward };

// A jump of more than `threshold` seconds either way is a step; a smaller one is the clock being slewed or just noise.
inline ClockStep classify(double jump_seconds, double threshold) {
  // TODO: forward above the threshold, backward below minus the threshold, otherwise none
  (void)jump_seconds;
  (void)threshold;
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
  // TODO: a stable sort by boot and then by mono
  (void)records;
  return records;
}

enum class Validity { valid, expired, not_yet, unknown };

// Is a certificate (or a licence, or a schedule) valid now, between not_before and not_after (both inclusive) in wall time? If the clock has not been set the answer is `unknown`: the wall time is a guess and must not be used to say yes or no.
inline Validity check_validity(double not_before, double not_after, const BootClock& clock, double mono) {
  // TODO: unknown if the clock is not synced; else now = clock.stamp(mono).wall: not_yet below not_before, expired above not_after, valid otherwise
  (void)not_before;
  (void)not_after;
  (void)clock;
  (void)mono;
  return Validity::unknown;
}

#endif  // LESSON_SOLUTION_HPP
