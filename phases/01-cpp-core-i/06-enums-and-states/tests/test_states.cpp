#include <rc/test/rc_test.hpp>

#include <string>
#include <vector>

#include "solution.hpp"

namespace {
const std::vector<DriveState> kAllStates = {DriveState::Idle, DriveState::Driving,
                                            DriveState::Stopping, DriveState::Faulted};
const std::vector<Event> kAllEvents = {Event::Start, Event::Stop, Event::Arrived,
                                       Event::Fault, Event::Reset};
}  // namespace

RC_TEST("every state has its own name") {
  RC_CHECK_EQ(std::string(name(DriveState::Idle)), std::string("idle"));
  RC_CHECK_EQ(std::string(name(DriveState::Driving)), std::string("driving"));
  RC_CHECK_EQ(std::string(name(DriveState::Stopping)), std::string("stopping"));
  RC_CHECK_EQ(std::string(name(DriveState::Faulted)), std::string("faulted"));
}

RC_TEST("no state is left unnamed") {
  for (const DriveState state : kAllStates) {
    RC_CHECK(std::string(name(state)) != std::string("unknown"));
  }
}

RC_TEST("no event is left unnamed") {
  for (const Event event : kAllEvents) {
    RC_CHECK(std::string(name(event)) != std::string("unknown"));
  }
}

RC_TEST("a robot starts driving when told to start") {
  RC_CHECK(next(DriveState::Idle, Event::Start) == DriveState::Driving);
}

RC_TEST("a driving robot stops through Stopping, not straight to Idle") {
  const DriveState stopping = next(DriveState::Driving, Event::Stop);
  RC_CHECK(stopping == DriveState::Stopping);
  RC_CHECK(next(stopping, Event::Arrived) == DriveState::Idle);
}

RC_TEST("a fault is accepted from every state") {
  for (const DriveState state : kAllStates) {
    RC_CHECK(next(state, Event::Fault) == DriveState::Faulted);
  }
}

RC_TEST("a faulted robot ignores everything except a reset") {
  // The safety rule. Recovery is a decision somebody makes, never something a
  // stale command can cause.
  for (const Event event : kAllEvents) {
    if (event == Event::Reset || event == Event::Fault) continue;
    RC_CHECK(next(DriveState::Faulted, event) == DriveState::Faulted);
  }
}

RC_TEST("a reset clears a fault") {
  RC_CHECK(next(DriveState::Faulted, Event::Reset) == DriveState::Idle);
}

RC_TEST("a reset does nothing to a healthy robot") {
  RC_CHECK(next(DriveState::Idle, Event::Reset) == DriveState::Idle);
  RC_CHECK(next(DriveState::Driving, Event::Reset) == DriveState::Driving);
}

RC_TEST("an event that means nothing here leaves the state alone") {
  RC_CHECK(next(DriveState::Idle, Event::Arrived) == DriveState::Idle);
  RC_CHECK(next(DriveState::Driving, Event::Start) == DriveState::Driving);
  RC_CHECK(next(DriveState::Stopping, Event::Start) == DriveState::Stopping);
}

RC_TEST("no event from any state produces a state with no name") {
  // A transition table that returns something outside the enum would show up
  // here rather than much later in a log nobody reads.
  for (const DriveState state : kAllStates) {
    for (const Event event : kAllEvents) {
      RC_CHECK(std::string(name(next(state, event))) != std::string("unknown"));
    }
  }
}

RC_TEST("moving means driving or stopping") {
  RC_CHECK(!is_moving(DriveState::Idle));
  RC_CHECK(is_moving(DriveState::Driving));
  RC_CHECK(is_moving(DriveState::Stopping));
  RC_CHECK(!is_moving(DriveState::Faulted));
}

RC_TEST("a faulted robot accepts no commands") {
  RC_CHECK(accepts_commands(DriveState::Idle));
  RC_CHECK(accepts_commands(DriveState::Driving));
  RC_CHECK(accepts_commands(DriveState::Stopping));
  RC_CHECK(!accepts_commands(DriveState::Faulted));
}

RC_TEST("a full mission runs and comes back to idle") {
  DriveState state = DriveState::Idle;
  for (const Event event : {Event::Start, Event::Stop, Event::Arrived}) {
    state = next(state, event);
  }
  RC_CHECK(state == DriveState::Idle);
  RC_CHECK(!is_moving(state));
}

RC_TEST("a fault mid mission needs a reset before driving again") {
  DriveState state = next(DriveState::Idle, Event::Start);
  state = next(state, Event::Fault);
  RC_CHECK(state == DriveState::Faulted);

  state = next(state, Event::Start);
  RC_CHECK(state == DriveState::Faulted);   // still refusing

  state = next(state, Event::Reset);
  state = next(state, Event::Start);
  RC_CHECK(state == DriveState::Driving);
}
