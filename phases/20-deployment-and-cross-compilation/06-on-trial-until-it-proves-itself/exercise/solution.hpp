#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// The state the boot loader keeps across resets: which slot is active, which to go back to, whether the active one is still on trial and how many tries it has left.
struct BootState {
  int active = 0;
  int previous = -1;
  bool trial = false;
  int tries_left = 0;
  bool rolled_back = false;
};

// Put a new release on trial: the slot that was active becomes `previous`, the new slot becomes active, trial is true with this many tries, and a rollback recorded earlier is forgotten.
inline void start_trial(BootState& s, int new_slot, int tries) {
  // TODO: previous = the active slot; active = new_slot; trial = true; tries_left = tries; rolled_back = false
  (void)s;
  (void)new_slot;
  (void)tries;
}

// The boot loader's decision at each reset: which slot to run. A release that is not on trial just runs. One that is on trial with no tries left is given up on: the previous slot becomes active, the trial ends and rolled_back is set, and the previous slot runs.
// One with tries left runs, and the try is used up BEFORE it runs, so a release that never gets as far as saying anything still costs a try.
inline int on_boot(BootState& s) {
  // TODO: on trial with no tries left: active = previous (if there is one), trial = false, rolled_back = true, return the active slot. On trial with tries: use one up, then return the active slot. Not on trial: return the active slot
  (void)s;
  return 0;
}

// The release says it is healthy: the trial is over and this release stays. False if there was no trial.
inline bool confirm(BootState& s) {
  // TODO: false if not on trial; otherwise the trial ends and true
  (void)s;
  return false;
}

// How a release behaves once booted, in seconds after it starts: when it crashes (negative: never) and when its health checks first pass (negative: never).
struct Behaviour {
  double crash_at = -1.0;
  double healthy_at = -1.0;
};

struct RunResult {
  int boots = 0;
  int final_slot = 0;
  bool up = false;            // it ended in a boot that stays up
  bool rolled_back = false;
  bool on_trial = false;      // still unconfirmed at the end
};

// Run the robot through resets until it stays up or `max_boots` boots have been used. behaviours[slot] says how the release in that slot behaves. For each boot, in order:
//  - on_boot chooses the slot;
//  - the release is HEALTHY IN TIME if healthy_at is not negative and not after `deadline`; if it is not, a supervisor resets the robot at the deadline (or sooner if it crashes first);
//  - a release on trial is confirmed at healthy_at + soak (the checks must have passed, and have gone on passing for the soak) if that comes before the boot ends, or at the start of the boot if confirm_at_start is true (which ignores the health checks altogether);
//  - the boot ends at crash_at, or at the deadline if it was not healthy in time, or never. A boot that never ends is up, and the run stops there.
inline RunResult simulate(BootState s, const Behaviour behaviours[2], double soak, double deadline, bool confirm_at_start, int max_boots) {
  // TODO: follow the comment above: loop until a boot never ends (up) or max_boots. healthy_in_time, end (crash_at, or the deadline when not healthy in time and not crashed sooner), confirmation at healthy_at + soak or at 0 when confirm_at_start, only on trial and only if it comes before the end. Fill boots, final_slot (the active slot at the end), up, rolled_back, on_trial
  (void)s;
  (void)behaviours;
  (void)soak;
  (void)deadline;
  (void)confirm_at_start;
  (void)max_boots;
  return RunResult();
}

#endif  // LESSON_SOLUTION_HPP
