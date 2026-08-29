#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

enum class DriveState {
  Idle,
  Driving,
  Stopping,
  Faulted,
};

enum class Event {
  Start,
  Stop,
  Arrived,
  Fault,
  Reset,
};

// TODO: a word for each state.
//
// Handle every case and do not add a default label. Without one, the compiler
// warns when a value is added and this switch has not been updated. Put the
// fallback after the switch instead, where it catches a value cast in from
// outside without disabling that warning.
inline const char* name(DriveState state) {
  (void)state;
  return "unknown";
}

inline const char* name(Event event) {
  // TODO
  (void)event;
  return "unknown";
}

// The transition table.
//
//   Fault   from any state    -> Faulted
//   Reset   from Faulted      -> Idle, and does nothing elsewhere
//   Start   from Idle         -> Driving
//   Stop    from Driving      -> Stopping
//   Arrived from Stopping     -> Idle
//   anything else             -> unchanged
//
// A faulted machine must ignore everything except a reset, so it cannot drive
// away because a stale Start arrived from somewhere.
inline DriveState next(DriveState current, Event event) {
  // TODO
  (void)event;
  return current;
}

inline bool is_moving(DriveState state) {
  // TODO: true for Driving and Stopping only.
  (void)state;
  return false;
}

inline bool accepts_commands(DriveState state) {
  // TODO: false when faulted.
  (void)state;
  return true;
}

#endif  // LESSON_SOLUTION_HPP
