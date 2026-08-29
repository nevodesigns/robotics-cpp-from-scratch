#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// A type that can hold exactly the states a robot has, and nothing else.
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

// No default label. Handling every case means the compiler warns when a value is
// added and some switch has not been updated, which is exactly when you want to
// be told. The fallback sits after the switch, where it catches a value cast in
// from outside without disabling that check.
inline const char* name(DriveState state) {
  switch (state) {
    case DriveState::Idle:     return "idle";
    case DriveState::Driving:  return "driving";
    case DriveState::Stopping: return "stopping";
    case DriveState::Faulted:  return "faulted";
  }
  return "unknown";
}

inline const char* name(Event event) {
  switch (event) {
    case Event::Start:   return "start";
    case Event::Stop:    return "stop";
    case Event::Arrived: return "arrived";
    case Event::Fault:   return "fault";
    case Event::Reset:   return "reset";
  }
  return "unknown";
}

inline DriveState next(DriveState current, Event event) {
  // A fault is accepted from anywhere. Checking it before anything else means no
  // other rule can accidentally swallow it, which matters more than the two
  // lines it saves.
  if (event == Event::Fault) return DriveState::Faulted;

  // Recovery is deliberate. A faulted machine ignores everything except a reset,
  // so it cannot drive away because a stale Start arrived from somewhere.
  if (current == DriveState::Faulted) {
    return event == Event::Reset ? DriveState::Idle : DriveState::Faulted;
  }

  if (current == DriveState::Idle && event == Event::Start) return DriveState::Driving;
  if (current == DriveState::Driving && event == Event::Stop) return DriveState::Stopping;
  if (current == DriveState::Stopping && event == Event::Arrived) return DriveState::Idle;

  // An event that means nothing in this state leaves it alone. That is a
  // decision rather than an oversight: an unexpected event is not a reason to
  // move a machine.
  return current;
}

inline bool is_moving(DriveState state) {
  return state == DriveState::Driving || state == DriveState::Stopping;
}

inline bool accepts_commands(DriveState state) { return state != DriveState::Faulted; }

#endif  // LESSON_SOLUTION_HPP
