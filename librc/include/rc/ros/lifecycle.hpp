// rc/ros/lifecycle.hpp
//
// A managed node's life cycle, from lesson 17-18, graduated.
//
// States unconfigured, inactive, active and finalized, moved between by configure, activate, deactivate, cleanup and shutdown, each running the node's own callback. A transition from the wrong state is refused and its
// callback is not run. A callback that fails leaves the node where it was; one that errors sends it to error processing, whose own recovery decides between unconfigured (start over) and finalized. A node sends only while active:
// anything else is dropped and counted.
//
// The way up and down went inactive, active, inactive, unconfigured, finalized. Six requests out of order were all refused and only the two legal callbacks ran. A failed configure was retried to inactive, a failed deactivate
// left the node active, a failed shutdown left it active. An error with a recovery that works went to unconfigured and was brought up again; one whose recovery failed went to finalized. Five messages over the life of the node:
// 2 sent, 3 dropped.

#ifndef RC_ROS_LIFECYCLE_HPP
#define RC_ROS_LIFECYCLE_HPP

#include <functional>
#include <string>

namespace rc {
namespace ros {

// Where a managed node is.
enum class LifeState { unconfigured, inactive, active, finalized, error_processing };

// What a node can be asked to do.
enum class Transition { configure, cleanup, activate, deactivate, shutdown };

// What one of the node's own callbacks says about how it went.
enum class CallbackResult { success, failure, error };

struct TransitionResult {
  bool legal = false;          // was this transition allowed from where the node was
  LifeState state = LifeState::unconfigured;   // where the node is afterwards
  CallbackResult callback = CallbackResult::success;
};

inline const char* name_of(LifeState s) {
  switch (s) {
    case LifeState::unconfigured: return "unconfigured";
    case LifeState::inactive: return "inactive";
    case LifeState::active: return "active";
    case LifeState::finalized: return "finalized";
    case LifeState::error_processing: return "error_processing";
  }
  return "?";
}

// A node whose start up and shut down are steps taken one at a time, each of which can fail.
//   configure:  unconfigured -> inactive      (the node gets what it needs: opens the device, reads its parameters)
//   activate:   inactive -> active            (it starts doing its work)
//   deactivate: active -> inactive            (it stops, but keeps what it has)
//   cleanup:    inactive -> unconfigured      (it gives it all back)
//   shutdown:   unconfigured, inactive or active -> finalized
// A transition asked for from any other state is not legal, and its callback is not run.
// Each transition runs the node's callback for it, and what the callback says decides where the node ends up:
//   success: the state the transition leads to
//   failure: the state it was in, unchanged (a failed shutdown too)
//   error:   error processing, whose own callback decides: success goes back to unconfigured, anything else is finalized
class LifecycleNode {
 public:
  using Callback = std::function<CallbackResult()>;

  void on(Transition t, Callback cb) { callbacks_[static_cast<int>(t)] = std::move(cb); }
  void on_error(Callback cb) { error_callback_ = std::move(cb); }

  LifeState state() const { return state_; }

  TransitionResult trigger(Transition t) {
    TransitionResult r;
    LifeState target = LifeState::unconfigured;
    if (!allowed(t, target)) {
      r.legal = false;
      r.state = state_;
      return r;
    }
    r.legal = true;
    const LifeState before = state_;
    const Callback& cb = callbacks_[static_cast<int>(t)];
    const CallbackResult result = cb ? cb() : CallbackResult::success;
    r.callback = result;
    if (result == CallbackResult::success) {
      state_ = target;
    } else if (result == CallbackResult::failure) {
      state_ = before;
    } else {
      state_ = LifeState::error_processing;
      const CallbackResult recovery = error_callback_ ? error_callback_() : CallbackResult::failure;
      state_ = recovery == CallbackResult::success ? LifeState::unconfigured : LifeState::finalized;
    }
    r.state = state_;
    return r;
  }

  // Send something, but only while active. Anything sent in any other state is dropped and counted.
  bool publish(const std::string& payload, const std::function<void(const std::string&)>& send) {
    if (state_ != LifeState::active) {
      ++dropped_;
      return false;
    }
    send(payload);
    ++sent_;
    return true;
  }

  int dropped() const { return dropped_; }
  int sent() const { return sent_; }

 private:
  bool allowed(Transition t, LifeState& target) const {
    switch (t) {
      case Transition::configure:
        target = LifeState::inactive;
        return state_ == LifeState::unconfigured;
      case Transition::cleanup:
        target = LifeState::unconfigured;
        return state_ == LifeState::inactive;
      case Transition::activate:
        target = LifeState::active;
        return state_ == LifeState::inactive;
      case Transition::deactivate:
        target = LifeState::inactive;
        return state_ == LifeState::active;
      case Transition::shutdown:
        target = LifeState::finalized;
        return state_ == LifeState::unconfigured || state_ == LifeState::inactive || state_ == LifeState::active;
    }
    return false;
  }

  LifeState state_ = LifeState::unconfigured;
  Callback callbacks_[5];
  Callback error_callback_;
  int dropped_ = 0;
  int sent_ = 0;
};

}  // namespace ros
}  // namespace rc

#endif  // RC_ROS_LIFECYCLE_HPP
