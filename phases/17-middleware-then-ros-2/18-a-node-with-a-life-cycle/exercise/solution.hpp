#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <functional>
#include <string>

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
    // TODO: allowed(t, target) says whether the node is in a state this transition may start from, and what state success leads to. If not allowed: legal = false, the state is unchanged, and the callback is NOT run.
    // Otherwise legal = true, remember the state it was in, run the callback for this transition (a missing one counts as success) and note what it returned.
    // success: the node moves to the target. failure: it stays where it was. error: it goes to error_processing; run the error callback (a missing one counts as failure):
    // success puts the node in unconfigured, anything else in finalized. Report the state it ends in.
    (void)t;
    (void)callbacks_;
    (void)error_callback_;
    return TransitionResult();
  }

  // Send something, but only while active. Anything sent in any other state is dropped and counted.
  bool publish(const std::string& payload, const std::function<void(const std::string&)>& send) {
    // TODO: only an active node sends: call send(payload), count it in sent_ and return true. In any other state count it in dropped_ and return false
    (void)payload;
    (void)send;
    (void)dropped_;
    (void)sent_;
    return false;
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

#endif  // LESSON_SOLUTION_HPP
